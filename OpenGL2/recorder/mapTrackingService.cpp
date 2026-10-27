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

bool CanRemapForRead(const std::function<bool(GLint&)>& queryStorageFlags) {
  const auto [major, minor] = CurrentContextGLVersion();
  if ((major < 4 || (major == 4 && minor < 4)) &&
      !CurrentContextHasExtension(major, "GL_ARB_buffer_storage") &&
      !CurrentContextHasExtension(major, "GL_EXT_buffer_storage")) {
    return true;
  }
  GLint flags = 0;
  return queryStorageFlags(flags) && (flags & GL_MAP_READ_BIT) != 0;
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
  constexpr GLbitfield required = GL_MAP_WRITE_BIT;
  constexpr GLbitfield excluded =
      GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  const bool explicitFlush = (access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0;
  const bool hasExcludedImplicitFlushAccess =
      (access & (GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT)) != 0;
  return (access & required) == required && (access & excluded) == 0 &&
         (explicitFlush || !hasExcludedImplicitFlushAccess);
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
      },
      /*queryStorageFlags=*/
      [target](GLint& flags) {
        return TryResolveRealFunction("glGetBufferParameteriv") &&
               GetBoundBufferParameter(target, GL_BUFFER_STORAGE_FLAGS, flags);
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
  StoreMappedRange(
      shareGroup, buffer,
      MappedRange{ByteSource{ByteSource::Kind::DriverPointer, mappedPointer, mappedPointer},
                  /*offset=*/0, length, MapAccessEnumToBits(access), false});
}

std::optional<MapTrackingService::MapRelativeRange> MapTrackingService::CaptureRangePolicy::
    SelectFlushRange(GLintptr offset, GLsizeiptr length) const {
  GITS_ASSERT(Type != Mode::DirtyPages, "Dirty-page detection requires a byte-source backend");
  if (Type == Mode::DeclaredFlush && length > 0) {
    return MapRelativeRange{offset, length};
  }
  return std::nullopt;
}

std::optional<MapTrackingService::MapRelativeRange> MapTrackingService::CaptureRangePolicy::
    SelectUnmapRange(GLsizeiptr length) const {
  GITS_ASSERT(Type != Mode::DirtyPages, "Dirty-page detection requires a byte-source backend");
  if (Type == Mode::FullRange) {
    return MapRelativeRange{0, length};
  }
  return std::nullopt;
}

void MapTrackingService::ByteSource::PrepareOwnedShadow(GLintptr offset,
                                                        GLsizeiptr length,
                                                        size_t alignment,
                                                        std::span<const uint8_t> initialBytes,
                                                        bool initialize) {
  GITS_ASSERT(!initialize || initialBytes.size() == static_cast<size_t>(length),
              "Cannot initialize buffer-map shadow memory without complete initial bytes");
  GITS_ASSERT(alignment <= std::numeric_limits<size_t>::max() - static_cast<size_t>(length),
              "Cannot allocate aligned shadow for buffer map");
  const auto allocationSize = static_cast<size_t>(length) + alignment;
  try {
    Shadow = std::make_unique<WatchedShadow>(allocationSize);
  } catch (const std::bad_alloc&) {
    GITS_ASSERT(false, "Cannot allocate write watch for buffer map");
  }
  // Keep the application pointer aligned relative to the buffer offset.
  const auto alignedPointer = [offset, alignment](uint8_t* data) {
    const auto base = reinterpret_cast<uintptr_t>(data);
    const auto offsetRemainder = static_cast<size_t>(offset) % alignment;
    const auto baseRemainder = base % alignment;
    const auto padding = (offsetRemainder + alignment - baseRemainder) % alignment;
    return data + padding;
  };
  ApplicationPointer = alignedPointer(static_cast<uint8_t*>(Shadow->Data()));
  if (initialize) {
    std::memcpy(ApplicationPointer, initialBytes.data(), static_cast<size_t>(length));
  }
  Shadow->ResetWriteWatch();
  Type = Kind::WatchedShadow;
}

void* MapTrackingService::ByteSource::TranslatePointer(void* pointer) const {
  if (Type == Kind::DriverPointer) {
    return pointer;
  }
  GITS_ASSERT(pointer == DriverPointer,
              "Buffer map pointer query does not match the tracked driver mapping");
  return ApplicationPointer;
}

std::vector<uint8_t> MapTrackingService::ByteSource::Snapshot(size_t offset, size_t length) const {
  if (length == 0) {
    return {};
  }
  const auto* begin = static_cast<const uint8_t*>(ApplicationPointer) + offset;
  return std::vector<uint8_t>(begin, begin + length);
}

void MapTrackingService::ByteSource::WriteBack(size_t offset,
                                               std::span<const uint8_t> bytes) const {
  if (Type == Kind::WatchedShadow && !bytes.empty()) {
    std::memcpy(static_cast<uint8_t*>(DriverPointer) + offset, bytes.data(), bytes.size());
  }
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
  MappedRange range{ByteSource{ByteSource::Kind::DriverPointer, driverPointer, driverPointer},
                    offset, length, access, true};
  const bool readable = (access & GL_MAP_READ_BIT) != 0;
  const bool writable = (access & GL_MAP_WRITE_BIT) != 0;
  const bool explicitFlush = (access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0;
  const bool invalidated =
      (access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)) != 0;
  const bool unsynchronized = (access & GL_MAP_UNSYNCHRONIZED_BIT) != 0;
  const bool dirtyPageCandidate = !explicitFlush && writable && !readable && !unsynchronized;
  bool shadowAlignmentAvailable = false;
  if (dirtyPageCandidate || (explicitFlush && !readable)) {
    const auto [major, minor] = CurrentContextGLVersion();
    shadowAlignmentAvailable = major > 4 || (major == 4 && minor >= 2) ||
                               CurrentContextHasExtension(major, "GL_ARB_map_buffer_alignment");
  }
  // Invalidated maps copy touched pages as-is; other maps need a full baseline for comparison.
  const bool useDirtyPageCapture =
      dirtyPageCandidate && shadowAlignmentAvailable &&
      (invalidated || initialBytes.size() == static_cast<size_t>(length));
  if (explicitFlush || useDirtyPageCapture) {
    range.RangePolicy.Type = explicitFlush ? CaptureRangePolicy::Mode::DeclaredFlush
                                           : CaptureRangePolicy::Mode::DirtyPages;
    GITS_ASSERT(writable, "Explicit-flush buffer map has no writable byte source");
    if (!readable) {
      GITS_ASSERT(shadowAlignmentAvailable,
                  "GL_MIN_MAP_BUFFER_ALIGNMENT is unavailable for shadow mapping");
      GLint alignment = 0;
      g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_MIN_MAP_BUFFER_ALIGNMENT,
                                                                    &alignment);
      GITS_ASSERT(alignment > 0, "Invalid buffer map alignment reported by driver");
      range.Source.PrepareOwnedShadow(offset, length, static_cast<size_t>(alignment), initialBytes,
                                      NeedsInitialShadowContents(access));
      if (useDirtyPageCapture && !invalidated) {
        range.Source.Baseline.assign(initialBytes.begin(), initialBytes.end());
      }
    }
  }
  void* applicationPointer = range.Source.ApplicationPointer;
  StoreMappedRange(CurrentShareGroup(), buffer, std::move(range));
  return applicationPointer;
}

void* MapTrackingService::TranslateShadowPointerQuery(GLuint buffer, void* pointer) {
  if (pointer == nullptr) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  const auto it = m_MappedBuffers.find(Key{CurrentShareGroup(), buffer});
  if (it == m_MappedBuffers.end()) {
    return pointer;
  }
  return it->second.Source.TranslatePointer(pointer);
}

MapTrackingService::MappedBufferUpdate MapTrackingService::MakeMappedUpdate(
    const MappedRange& range, MapRelativeRange interval, std::vector<uint8_t> bytes) {
  return MappedBufferUpdate{range.Map,       range.Offset + interval.Offset,
                            interval.Length, range.IsRange,
                            range.Access,    std::move(bytes)};
}

MapTrackingService::MappedBufferUpdate MapTrackingService::PrepareFlushMappedUpdate(
    const MappedRange& range, MapRelativeRange interval) {
  const auto start = static_cast<size_t>(interval.Offset);
  const auto count = static_cast<size_t>(interval.Length);
  auto bytes = range.Source.Snapshot(start, count);
  // Shadow-backed bytes must reach the real mapping before the caller flushes it.
  range.Source.WriteBack(start, bytes);
  return MakeMappedUpdate(range, interval, std::move(bytes));
}

void MapTrackingService::RecordOrSkipMappedUpdate(uint64_t key,
                                                  const std::optional<MappedBufferUpdate>& update) {
  if (!update) {
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }
  RecordMappedDataMetaCommand(key, update->Map.ShareGroup, update->Map.Buffer, update->Offset,
                              update->Length, update->Bytes);
  NoteUpdate(update->IsRange, update->Access, update->Length);
}

void MapTrackingService::FlushMappedRange(GLuint buffer,
                                          GLintptr offset,
                                          GLsizeiptr length,
                                          uint64_t key,
                                          const std::function<void()>& realFlush) {
  if (offset < 0 || length < 0) {
    realFlush();
    RecordOrSkipMappedUpdate(key, std::nullopt);
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::optional<MappedBufferUpdate> update;
  {
    std::unique_lock<std::mutex> lock(m_MappedBuffersMutex);
    const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
    if (it == m_MappedBuffers.end()) {
      lock.unlock();
      realFlush();
      RecordOrSkipMappedUpdate(key, std::nullopt);
      return;
    }
    const MappedRange& range = it->second;
    if (offset > range.Length || length > range.Length - offset ||
        range.Offset > std::numeric_limits<GLintptr>::max() - offset) {
      lock.unlock();
      realFlush();
      RecordOrSkipMappedUpdate(key, std::nullopt);
      return;
    }
    const auto interval = range.RangePolicy.SelectFlushRange(offset, length);
    if (interval) {
      update = PrepareFlushMappedUpdate(range, *interval);
    }
  }
  realFlush();
  RecordOrSkipMappedUpdate(key, update);
}

GLboolean MapTrackingService::SnapshotBufferAndUnmap(
    const char* callerName,
    GLuint buffer,
    uint64_t shareGroup,
    const std::function<GLboolean()>& realUnmap,
    const std::function<void*(GLintptr, GLsizeiptr)>& realReadOnlyRemap,
    const std::function<GLboolean()>& realInternalUnmap,
    const std::function<bool(GLintptr, GLsizeiptr, void*)>& realReadSubData,
    const std::function<bool(GLint&)>& queryStorageFlags) {
  std::optional<MappedRange> range =
      buffer != 0 ? TakeMappedRange(shareGroup, buffer) : std::nullopt;
  // Untracked or read-only maps need no snapshot and no ordering key.
  if (!range || (range->Access & GL_MAP_WRITE_BIT) == 0) {
    return realUnmap();
  }
  const bool useDirtyPageCapture = range->RangePolicy.Type == CaptureRangePolicy::Mode::DirtyPages;
  std::optional<MapRelativeRange> interval;
  if (!useDirtyPageCapture) {
    interval = range->RangePolicy.SelectUnmapRange(range->Length);
  }
  if (!useDirtyPageCapture && !interval) {
    // Capture only flushed ranges. Writes after the final flush and before unmap leave the
    // corresponding buffer contents undefined, so unmap must not snapshot those bytes.
    return realUnmap();
  }
  // Reserve before the real unmap, then record or release on every path; otherwise another
  // thread can claim a later key and stall the ordering recorder on this one.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  const auto unmapAndRecord = [&](std::optional<MappedBufferUpdate> update) {
    const GLboolean result = realUnmap();
    if (result != GL_TRUE) {
      LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      update.reset();
    } else if (!update && useDirtyPageCapture && !range->Source.Baseline.empty()) {
      BufferShadowService::Get().OnMapUnchanged(shareGroup, buffer);
    }
    RecordOrSkipMappedUpdate(key, update);
    return result;
  };
  if (useDirtyPageCapture) {
    GITS_ASSERT(range->Source.Shadow != nullptr, "Dirty-page map has no watched shadow");
    const auto dirty = range->Source.Shadow->DirtyRange(range->Source.ApplicationPointer,
                                                        static_cast<size_t>(range->Length),
                                                        range->Source.Baseline);
    if (!dirty) {
      return unmapAndRecord(std::nullopt);
    }
    interval = MapRelativeRange{static_cast<GLintptr>(dirty->Offset),
                                static_cast<GLsizeiptr>(dirty->Length)};
    auto bytes = range->Source.Snapshot(dirty->Offset, dirty->Length);
    range->Source.WriteBack(dirty->Offset, bytes);
    return unmapAndRecord(MakeMappedUpdate(*range, *interval, std::move(bytes)));
  }
  if (range->Access & GL_MAP_READ_BIT) {
    // Readable mappings can be copied before the application-visible unmap invalidates them.
    auto bytes = range->Source.Snapshot(0, static_cast<size_t>(interval->Length));
    return unmapAndRecord(MakeMappedUpdate(*range, *interval, std::move(bytes)));
  }
  // Write-only pointers cannot be read safely. Unmap first, then read the resulting buffer.
  std::vector<uint8_t> bytes(static_cast<size_t>(interval->Length));
  const GLboolean result = realUnmap();
  if (result != GL_TRUE) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                << " returned GL_FALSE (data store contents corrupted per spec) - no "
                   "MappedDataMetaCommand recorded for this unmap";
    RecordOrSkipMappedUpdate(key, std::nullopt);
    return result;
  }
  void* readPointer = CanRemapForRead(queryStorageFlags)
                          ? realReadOnlyRemap(range->Offset, range->Length)
                          : nullptr;
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    if (realInternalUnmap() != GL_TRUE) {
      RecordOrSkipMappedUpdate(key, std::nullopt);
      std::ostringstream message;
      message << "MapTrackingService (" << callerName << "): internal unmap of buffer " << buffer
              << " returned GL_FALSE after the application unmap succeeded; "
                 "buffer contents are undefined and therefore unusable.";
      GITS_ASSERT(false, message.str());
    }
  } else if (!realReadSubData(range->Offset, range->Length, bytes.data())) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): no readback function for buffer "
                << buffer << " - no MappedDataMetaCommand recorded for this unmap";
    RecordOrSkipMappedUpdate(key, std::nullopt);
    return result;
  }
  RecordOrSkipMappedUpdate(key, MakeMappedUpdate(*range, *interval, std::move(bytes)));
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
      },
      /*queryStorageFlags=*/
      [target](GLint& flags) {
        return TryResolveRealFunction("glGetBufferParameteriv") &&
               GetBoundBufferParameter(target, GL_BUFFER_STORAGE_FLAGS, flags);
      });
}

GLboolean MapTrackingService::UnmapNamedBufferAndCapture(const char* callerName,
                                                         GLuint buffer,
                                                         NamedUnmapFn unmap,
                                                         NamedReadOnlyRemapFn readOnlyRemap,
                                                         NamedReadSubDataFn readSubData,
                                                         NamedStorageFlagsFn queryStorageFlags) {
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
      },
      /*queryStorageFlags=*/
      [buffer, queryStorageFlags](GLint& flags) { return queryStorageFlags(buffer, flags); });
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
  range.Map = key;
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
