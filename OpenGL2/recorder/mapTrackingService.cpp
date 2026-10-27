// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"

#include "bufferShadowService.h"
#include "captureManager.h"
#include "clientMemoryCapture.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "messageBus.h"
#include "openglDriversAuto.h"

#include <cstring>
#include <functional>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

// Hand-declared instead of including <windows.h> - see recorderWrappersAuto.cpp.mako's
// identical GetCurrentThreadId precedent (that header comment explains why this backend
// avoids <windows.h> in any translation unit that also needs glTypes.h's own declarations).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace gits::opengl2 {

namespace {

// Builds and records a MappedDataMetaCommand from the snapshot.
// `bytes.size()` reaches ArrayArgument<uint8_t>'s own size_t-based constructor with no narrowing
// cast: a mapped range can legitimately approach or exceed 4 GiB on this x64-only backend, and
// narrowing here would reintroduce the exact truncation bug MappedDataMetaCommand's already
// 64-bit coding path was fixed to avoid (see mappedDataMetaTypes.h).
void RecordMappedDataMetaCommand(uint64_t key,
                                 uint64_t shareGroup,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr length,
                                 const std::vector<uint8_t>& bytes) {
  BufferShadowService::Get().OnMappedBytes(shareGroup, buffer, static_cast<uint64_t>(offset),
                                           bytes);
  MappedDataMetaCommand command(static_cast<uint32_t>(GetCurrentThreadId()));
  command.m_Buffer.Value = buffer;
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  command.m_Offset.Value = static_cast<uint64_t>(offset);
  command.m_Length.Value = static_cast<uint64_t>(length);
  command.m_Data = ArrayArgument<uint8_t>(bytes.data(), bytes.size());
  command.m_Key = key;
  CaptureManager::Get().RecordCommand(command, new MappedDataMetaSerializer(command));
}

// Read-only remap of [offset, offset + length) of the buffer bound to `target`, for capture only.
// If the range entry point is unavailable, map the whole buffer through glMapBuffer or its ARB
// variant. A supported range map that fails must instead use subdata readback; another map would
// also fail when immutable storage disallows reading. The matching unmap is the caller's.
void* MapRangeForCaptureRead(GLenum target, GLintptr offset, GLsizeiptr length) {
  auto mapRange = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  if (mapRange != nullptr) {
    void* pointer = mapRange(target, offset, length, GL_MAP_READ_BIT);
    if (pointer != nullptr || g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire) !=
                                  &ResolveAndCall_glMapBufferRange) {
      return pointer;
    }
  }
  auto map = g_DispatchTable.glMapBuffer.load(std::memory_order_acquire);
  void* whole = map != nullptr ? map(target, GL_READ_ONLY) : nullptr;
  if (whole == nullptr &&
      g_DispatchTable.glMapBuffer.load(std::memory_order_acquire) == &ResolveAndCall_glMapBuffer) {
    auto mapARB = g_DispatchTable.glMapBufferARB.load(std::memory_order_acquire);
    whole = mapARB != nullptr ? mapARB(target, GL_READ_ONLY) : nullptr;
  }
  return whole != nullptr ? static_cast<uint8_t*>(whole) + offset : nullptr;
}

// Void dispatch thunks return without writing data if their real getter cannot be resolved.
bool ReadBufferSubData(GLenum target, GLintptr offset, GLsizeiptr length, void* data) {
  g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire)(target, offset, length, data);
  return g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire) !=
         &ResolveAndCall_glGetBufferSubData;
}

} // namespace

MapTrackingService& MapTrackingService::Get() {
  // Meyer's singleton - same rationale as ContextHandleMapService::Get()/
  // ObjectNameMapService::Get(): no MessageBus/OrderingRecorder ownership here, so none of
  // CaptureManager's static-destruction-order hazards apply.
  static MapTrackingService instance;
  return instance;
}

// Functions for buffer mapping statistics tracking.

MapTrackingService::MapTrackingService() {
  MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                              [this](Topic, const MessagePtr&) { LogStatistics(); });
}

void MapTrackingService::NoteMap(bool range, uint32_t access, bool succeeded) {
  std::lock_guard<std::mutex> lock(m_StatisticsMutex);
  auto& stats = m_Statistics[{range, access}];
  ++stats.Attempts;
  stats.Successful += succeeded;
}

void MapTrackingService::NoteUpdate(bool range, uint32_t access, GLsizeiptr length) {
  if (!range) {
    access = MapAccessBitsToEnum(access);
  }
  std::lock_guard<std::mutex> lock(m_StatisticsMutex);
  auto& stats = m_Statistics[{range, access}];
  ++stats.Updates;
  stats.UpdatedBytes += static_cast<uint64_t>(length);
}

void MapTrackingService::LogStatistics() {
  std::lock_guard<std::mutex> lock(m_StatisticsMutex);
  if (m_Statistics.empty()) {
    return;
  }
  LOG_INFO << "Tracked buffer mapping summary (application requests, grouped by exact access):";
  for (const auto& [signature, stats] : m_Statistics) {
    const auto [range, access] = signature;
    std::ostringstream description;
    if (range) {
      description << "range mask "
                  << GetBitmaskString(GLenumApi::GL, access, GLenumGroup::MapBufferAccessMask);
    } else {
      description << "whole-buffer access "
                  << GetGLenumString(GLenumApi::GL, access, GLenumGroup::BufferAccessARB);
    }
    LOG_INFO << "  " << description.str() << ": " << stats.Attempts << " attempts, "
             << stats.Successful << " succeeded, " << stats.Updates << " captured updates ("
             << stats.UpdatedBytes << " bytes)";
  }
}

// End of functions for buffer mapping statistics tracking.

bool MapTrackingService::NeedsInitialShadowContents(GLbitfield access) {
  constexpr GLbitfield required = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
  constexpr GLbitfield excluded =
      GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  return (access & required) == required && (access & excluded) == 0;
}

std::vector<uint8_t> MapTrackingService::PrepareInitialBytes(
    GLintptr offset,
    GLsizeiptr length,
    GLbitfield access,
    bool getterAvailable,
    const BufferParameterCallback& parameter,
    const BufferReadCallback& read) {
  if (!getterAvailable || !NeedsInitialShadowContents(access) || offset < 0 || length <= 0) {
    return {};
  }
  GLint64 size = 0;
  GLint64 mapped = 0;
  if (!parameter(GL_BUFFER_SIZE, size) || !parameter(GL_BUFFER_MAPPED, mapped)) {
    return {};
  }
  if (mapped != 0 || size < 0 || offset > size || length > size - offset ||
      static_cast<uint64_t>(length) > std::vector<uint8_t>().max_size()) {
    return {};
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(length));
  // Readback may synchronize even when the subsequent map requests GL_MAP_UNSYNCHRONIZED_BIT.
  read(offset, length, bytes.data());
  return bytes;
}

void* MapTrackingService::OnMapBufferRange(GLenum target,
                                           GLintptr offset,
                                           GLsizeiptr length,
                                           GLbitfield access,
                                           void* mappedPointer,
                                           std::span<const uint8_t> initialBytes) {
  const GLuint buffer = ResolveBoundBuffer(target);
  GITS_ASSERT(buffer != 0, "Successful buffer range map has no resolvable buffer");
  return RegisterRange(buffer, offset, length, access, mappedPointer, initialBytes);
}

GLboolean MapTrackingService::UnmapBufferAndCapture(GLenum target) {
  auto realUnmapBuffer = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
  const GLuint buffer = ResolveBoundBuffer(target);
  return SnapshotBufferAndUnmap(
      "glUnmapBuffer", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapBuffer, target] { return realUnmapBuffer(target); },
      /*realReadOnlyRemap=*/
      [target](GLintptr offset, GLsizeiptr length) {
        return MapRangeForCaptureRead(target, offset, length);
      },
      /*realInternalUnmap=*/[realUnmapBuffer, target] { return realUnmapBuffer(target); },
      /*realReadSubData=*/
      [target](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadBufferSubData(target, offset, length, data);
      });
}

// --- Extensions beyond the original glMapBufferRange/glUnmapBuffer pair ---------------------
//
// Map and unmap variants share the same tracked-range and snapshot rules.

GLbitfield MapTrackingService::MapAccessEnumToBits(GLenum access) {
  switch (access) {
  case GL_READ_ONLY:
    return GL_MAP_READ_BIT;
  case GL_WRITE_ONLY:
    return GL_MAP_WRITE_BIT;
  case GL_READ_WRITE:
    return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
  default:
    throw std::invalid_argument("Unsupported whole-buffer map access enum");
  }
}

GLenum MapTrackingService::MapAccessBitsToEnum(GLbitfield access) {
  switch (access) {
  case GL_MAP_READ_BIT:
    return GL_READ_ONLY;
  case GL_MAP_WRITE_BIT:
    return GL_WRITE_ONLY;
  case GL_MAP_READ_BIT | GL_MAP_WRITE_BIT:
    return GL_READ_WRITE;
  default:
    throw std::invalid_argument("Unsupported whole-buffer map access bits");
  }
}

void MapTrackingService::StoreWholeBufferMap(
    uint64_t shareGroup, GLuint buffer, GLenum access, void* mappedPointer, GLsizeiptr length) {
  StoreMappedRange(shareGroup, buffer,
                   MappedRange{mappedPointer,
                               mappedPointer,
                               {},
                               /*offset=*/0,
                               length,
                               MapAccessEnumToBits(access),
                               false});
}

void MapTrackingService::OnMapBuffer(GLenum target, GLenum access, void* mappedPointer) {
  if (mappedPointer == nullptr) {
    return;
  }
  const GLuint buffer = ResolveBoundBuffer(target);
  if (buffer == 0) {
    return;
  }
  // Whole-buffer map calls provide no length; query the buffer instead of inferring its size.
  GLint64 size = 0;
  if (!GetBoundBufferSize(target, size) || size <= 0) {
    LOG_WARNING << "MapTrackingService: GL_BUFFER_SIZE of buffer " << buffer
                << " could not be queried - buffer-map tracking skipped for this map";
    return;
  }
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void MapTrackingService::OnMapNamedBuffer(GLuint buffer,
                                          GLenum access,
                                          void* mappedPointer,
                                          NamedBufferSizeFn querySize) {
  if (mappedPointer == nullptr || buffer == 0) {
    return;
  }
  const GLint64 size = querySize(buffer);
  if (size <= 0) {
    LOG_WARNING << "MapTrackingService: GL_BUFFER_SIZE of buffer " << buffer
                << " could not be queried - buffer-map tracking skipped for this map";
    return;
  }
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void* MapTrackingService::OnMapNamedBufferRange(GLuint buffer,
                                                GLintptr offset,
                                                GLsizeiptr length,
                                                GLbitfield access,
                                                void* mappedPointer,
                                                std::span<const uint8_t> initialBytes) {
  GITS_ASSERT(buffer != 0, "Successful named buffer range map has no buffer name");
  return RegisterRange(buffer, offset, length, access, mappedPointer, initialBytes);
}

void* MapTrackingService::RegisterRange(GLuint buffer,
                                        GLintptr offset,
                                        GLsizeiptr length,
                                        GLbitfield access,
                                        void* driverPointer,
                                        std::span<const uint8_t> initialBytes) {
  GITS_ASSERT(driverPointer != nullptr && offset >= 0 && length > 0,
              "Successful buffer range map has invalid tracking parameters");
  if (access & (GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT)) {
    const std::string message =
        (access & GL_MAP_COHERENT_BIT
             ? "Coherent persistent buffer maps are not supported by mapped-data capture"
             : "Non-coherent persistent buffer maps are not supported by mapped-data capture") +
        std::string(" (access: ") +
        GetBitmaskString(GLenumApi::GL, access, GLenumGroup::MapBufferAccessMask) + ')';
    GITS_ASSERT(false, message);
  }
  MappedRange range{driverPointer, driverPointer, {}, offset, length, access, true};
  if (access & GL_MAP_FLUSH_EXPLICIT_BIT) {
    GITS_ASSERT((access & GL_MAP_WRITE_BIT) != 0,
                "Explicit-flush buffer map has no writable byte source");
    if ((access & GL_MAP_READ_BIT) == 0) {
      GITS_ASSERT(!NeedsInitialShadowContents(access) ||
                      initialBytes.size() == static_cast<size_t>(length),
                  "Non-invalidating write-only explicit-flush map has no initial bytes");
      const auto [major, minor] = CurrentContextGLVersion();
      GITS_ASSERT(major > 4 || (major == 4 && minor >= 2) ||
                      CurrentContextHasExtension(major, "GL_ARB_map_buffer_alignment"),
                  "GL_MIN_MAP_BUFFER_ALIGNMENT is unavailable for shadow mapping");
      GLint alignment = 0;
      g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_MIN_MAP_BUFFER_ALIGNMENT,
                                                                    &alignment);
      GITS_ASSERT(alignment > 0, "Invalid buffer map alignment reported by driver");
      GITS_ASSERT(static_cast<size_t>(alignment) <= range.Shadow.max_size() &&
                      static_cast<size_t>(length) <=
                          range.Shadow.max_size() - static_cast<size_t>(alignment),
                  "Cannot allocate aligned shadow for buffer map");
      try {
        range.Shadow.resize(static_cast<size_t>(length) + static_cast<size_t>(alignment));
      } catch (const std::bad_alloc&) {
        GITS_ASSERT(false, "Cannot allocate shadow for buffer map");
      }
      const auto base = reinterpret_cast<uintptr_t>(range.Shadow.data());
      const auto residue = static_cast<size_t>(offset) % static_cast<size_t>(alignment);
      const auto padding =
          (residue + static_cast<size_t>(alignment) - base % static_cast<size_t>(alignment)) %
          static_cast<size_t>(alignment);
      range.ApplicationPointer = range.Shadow.data() + padding;
      if (NeedsInitialShadowContents(access)) {
        std::memcpy(range.ApplicationPointer, initialBytes.data(), static_cast<size_t>(length));
      }
    }
  }
  void* applicationPointer = range.ApplicationPointer;
  StoreMappedRange(CurrentShareGroup(), buffer, std::move(range));
  return applicationPointer;
}

void* MapTrackingService::TranslateShadowPointerQuery(GLuint buffer, void* pointer) {
  if (pointer == nullptr) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  const auto it = m_MappedBuffers.find(Key{CurrentShareGroup(), buffer});
  if (it == m_MappedBuffers.end() || it->second.Shadow.empty()) {
    return pointer;
  }
  GITS_ASSERT(pointer == it->second.DriverPointer,
              "Buffer map pointer query does not match the tracked driver mapping");
  return it->second.ApplicationPointer;
}

void MapTrackingService::FlushMappedRange(GLuint buffer,
                                          GLintptr offset,
                                          GLsizeiptr length,
                                          uint64_t key,
                                          const std::function<void()>& realFlush) {
  if (offset < 0 || length < 0) {
    realFlush();
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::vector<uint8_t> bytes;
  GLintptr absoluteOffset = 0;
  bool rangeMap = true;
  GLbitfield access = 0;
  {
    std::unique_lock<std::mutex> lock(m_MappedBuffersMutex);
    const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
    if (it == m_MappedBuffers.end() || (it->second.Access & GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
      lock.unlock();
      realFlush();
      CaptureManager::Get().SkipCommandKey(key);
      return;
    }
    const MappedRange& range = it->second;
    if (offset > range.Length || length > range.Length - offset ||
        range.Offset > std::numeric_limits<GLintptr>::max() - offset) {
      lock.unlock();
      realFlush();
      CaptureManager::Get().SkipCommandKey(key);
      return;
    }
    absoluteOffset = range.Offset + offset;
    rangeMap = range.IsRange;
    access = range.Access;
    const auto start = static_cast<size_t>(offset);
    const auto count = static_cast<size_t>(length);
    bytes.resize(count);
    if (count != 0) {
      std::memcpy(bytes.data(), static_cast<const uint8_t*>(range.ApplicationPointer) + start,
                  count);
      if (!range.Shadow.empty()) {
        std::memcpy(static_cast<uint8_t*>(range.DriverPointer) + start, bytes.data(), count);
      }
    }
  }
  realFlush();
  if (length > 0) {
    RecordMappedDataMetaCommand(key, shareGroup, buffer, absoluteOffset, length, bytes);
    NoteUpdate(rangeMap, access, length);
  } else {
    CaptureManager::Get().SkipCommandKey(key);
  }
}

GLboolean MapTrackingService::SnapshotBufferAndUnmap(
    const char* callerName,
    GLuint buffer,
    uint64_t shareGroup,
    const std::function<GLboolean()>& realUnmap,
    const std::function<void*(GLintptr, GLsizeiptr)>& realReadOnlyRemap,
    const std::function<GLboolean()>& realInternalUnmap,
    const std::function<bool(GLintptr, GLsizeiptr, void*)>& realReadSubData) {
  std::optional<MappedRange> range =
      buffer != 0 ? TakeMappedRange(shareGroup, buffer) : std::nullopt;
  // Untracked or read-only maps need no snapshot and no ordering key.
  if (!range || (range->Access & GL_MAP_WRITE_BIT) == 0) {
    return realUnmap();
  }
  if (range->Access & GL_MAP_FLUSH_EXPLICIT_BIT) {
    // Capture only flushed ranges. Writes after the final flush and before unmap leave the
    // corresponding buffer contents undefined, so unmap must not snapshot those bytes.
    return realUnmap();
  }
  // Reserve before the real unmap, then record or release on every path; otherwise another
  // thread can claim a later key and stall the ordering recorder on this one.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  std::vector<uint8_t> bytes(static_cast<size_t>(range->Length));
  if (range->Access & GL_MAP_READ_BIT) {
    // Readable mappings can be copied before the application-visible unmap invalidates them.
    std::memcpy(bytes.data(), range->ApplicationPointer, bytes.size());
    const GLboolean result = realUnmap();
    if (result == GL_TRUE) {
      RecordMappedDataMetaCommand(key, shareGroup, buffer, range->Offset, range->Length, bytes);
      NoteUpdate(range->IsRange, range->Access, range->Length);
    } else {
      LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
    return result;
  }
  // Write-only pointers cannot be read safely. Unmap first, then read the resulting buffer.
  const GLboolean result = realUnmap();
  if (result != GL_TRUE) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                << " returned GL_FALSE (data store contents corrupted per spec) - no "
                   "MappedDataMetaCommand recorded for this unmap";
    CaptureManager::Get().SkipCommandKey(key);
    return result;
  }
  // TODO: Track immutable storage permissions to skip a read remap forbidden by its flags;
  // the failed probe may leave an application-visible GL error before subdata readback.
  void* readPointer = realReadOnlyRemap(range->Offset, range->Length);
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    if (realInternalUnmap() != GL_TRUE) {
      LOG_WARNING << "MapTrackingService (" << callerName << "): internal unmap of buffer "
                  << buffer << " returned GL_FALSE - no MappedDataMetaCommand recorded";
      CaptureManager::Get().SkipCommandKey(key);
      return result;
    }
  } else if (!realReadSubData(range->Offset, range->Length, bytes.data())) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): no readback function for buffer "
                << buffer << " - no MappedDataMetaCommand recorded for this unmap";
    CaptureManager::Get().SkipCommandKey(key);
    return result;
  }
  RecordMappedDataMetaCommand(key, shareGroup, buffer, range->Offset, range->Length, bytes);
  NoteUpdate(range->IsRange, range->Access, range->Length);
  return result;
}

GLboolean MapTrackingService::UnmapBufferARBAndCapture(GLenum target) {
  auto realUnmapBufferARB = g_DispatchTable.glUnmapBufferARB.load(std::memory_order_acquire);
  const GLuint buffer = ResolveBoundBuffer(target);
  return SnapshotBufferAndUnmap(
      "glUnmapBufferARB", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapBufferARB, target] { return realUnmapBufferARB(target); },
      /*realReadOnlyRemap=*/
      [target](GLintptr offset, GLsizeiptr length) {
        return MapRangeForCaptureRead(target, offset, length);
      },
      /*realInternalUnmap=*/[realUnmapBufferARB, target] { return realUnmapBufferARB(target); },
      /*realReadSubData=*/
      [target](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadBufferSubData(target, offset, length, data);
      });
}

GLboolean MapTrackingService::UnmapNamedBufferAndCapture(const char* callerName,
                                                         GLuint buffer,
                                                         NamedUnmapFn unmap,
                                                         NamedReadOnlyRemapFn readOnlyRemap,
                                                         NamedReadSubDataFn readSubData) {
  return SnapshotBufferAndUnmap(
      callerName, buffer, CurrentShareGroup(),
      /*realUnmap=*/[unmap, buffer] { return unmap(buffer); },
      /*realReadOnlyRemap=*/
      [buffer, readOnlyRemap](GLintptr offset, GLsizeiptr length) {
        return readOnlyRemap(buffer, offset, length);
      },
      /*realInternalUnmap=*/[unmap, buffer] { return unmap(buffer); },
      /*realReadSubData=*/
      [buffer, readSubData](GLintptr offset, GLsizeiptr length, void* data) {
        return readSubData(buffer, offset, length, data);
      });
}

GLuint MapTrackingService::ResolveBoundBuffer(GLenum target) {
  const GLenum pname = BufferBindingPname(target);
  if (pname == 0) {
    std::lock_guard<std::mutex> lock(m_WarnedTargetsMutex);
    if (m_WarnedUnrecognizedTargets.insert(target).second) {
      LOG_WARNING << "MapTrackingService: target "
                  << GetGLenumString(GLenumApi::GL, target, GLenumGroup::BufferTargetARB)
                  << " is not a tracked buffer target - buffer-map tracking skipped";
    }
    return 0;
  }
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(pname, &value);
  const GLuint buffer = static_cast<GLuint>(value);
  if (buffer == 0) {
    std::lock_guard<std::mutex> lock(m_WarnedTargetsMutex);
    if (m_WarnedNoBufferBound.insert(target).second) {
      LOG_WARNING << "MapTrackingService: target "
                  << GetGLenumString(GLenumApi::GL, target, GLenumGroup::BufferTargetARB)
                  << " has no buffer bound - buffer-map tracking skipped for this target";
    }
  }
  return buffer;
}

void MapTrackingService::OnDelete(GLsizei count, const GLuint* buffers) {
  if (buffers == nullptr) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  for (GLsizei index = 0; index < count; ++index) {
    m_MappedBuffers.erase(Key{shareGroup, buffers[index]});
  }
}

void MapTrackingService::StoreMappedRange(uint64_t shareGroup, GLuint buffer, MappedRange range) {
  if (range.Access & GL_MAP_WRITE_BIT) {
    BufferShadowService::Get().OnMapWrite(shareGroup, buffer, range.Access);
  }
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  m_MappedBuffers[key] = std::move(range);
}

std::optional<MapTrackingService::MappedRange> MapTrackingService::TakeMappedRange(
    uint64_t shareGroup, GLuint buffer) {
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  auto it = m_MappedBuffers.find(key);
  if (it == m_MappedBuffers.end()) {
    return std::nullopt;
  }
  MappedRange range = std::move(it->second);
  m_MappedBuffers.erase(it);
  return range;
}

} // namespace gits::opengl2
