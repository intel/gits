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
#include "configurator.h"
#include "glBufferQueries.h"
#include "glContextCapabilities.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "messageBus.h"
#include "glDispatchAuto.h"
#include "recordingSession.h"
#include "systemApi.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gits::opengl {

namespace {

thread_local bool t_InPrimitivePair = false;

// For a mapping published before use. A readable one may become GPU-written, whose refresh needs
// page protection.
WatchedShadow::Tracking PublishedShadowTracking(GLbitfield access) {
  const bool pageProtection = Configurator::Get().opengl.recorder.persistentMapWriteTracking ==
                              OpenGLPersistentMapWriteTracking::PAGE_PROTECTION;
  return pageProtection || (access & GL_MAP_READ_BIT) != 0 ? WatchedShadow::Tracking::PageProtection
                                                           : WatchedShadow::Tracking::WriteWatch;
}

void RequireShadowMapAlignment() {
  const auto version = CurrentContextGLVersion();
  GITS_ASSERT(SupportsCoreOrExtension(version, 4, 2, "GL_ARB_map_buffer_alignment"),
              "Shadow mapping requires OpenGL 4.2 or GL_ARB_map_buffer_alignment");
}

// Builds and records a MappedDataMetaCommand from the snapshot.
// `bytes.size()` reaches ArrayArgument<uint8_t>'s own size_t-based constructor with no narrowing
// cast: a mapped range can legitimately approach or exceed 4 GiB on this x64-only backend, and
// narrowing here would reintroduce the exact truncation bug MappedDataMetaCommand's already
// 64-bit coding path was fixed to avoid (see mappedDataMetaTypes.h).
// `spans`, when not empty, are the spans relative to `offset` whose bytes `bytes` holds.
void RecordMappedDataMetaCommand(uint64_t key,
                                 uint64_t shareGroup,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr length,
                                 GLbitfield access,
                                 std::span<const WatchedShadow::ByteRange> spans,
                                 const std::vector<uint8_t>& bytes) {
  std::vector<uint64_t> spanPairs;
  if (spans.empty()) {
    BufferShadowService::Get().OnMappedBytes(shareGroup, buffer, static_cast<uint64_t>(offset),
                                             bytes);
  } else {
    spanPairs.reserve(2 * spans.size());
    size_t source = 0;
    for (const WatchedShadow::ByteRange& span : spans) {
      BufferShadowService::Get().OnMappedBytes(
          shareGroup, buffer, static_cast<uint64_t>(offset) + span.Offset,
          std::span<const uint8_t>(bytes).subspan(source, span.Length));
      source += span.Length;
      spanPairs.push_back(span.Offset);
      spanPairs.push_back(span.Length);
    }
  }
  MappedDataMetaCommand command(CurrentThreadId());
  command.m_Buffer.Value = buffer;
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  command.m_Offset.Value = static_cast<uint64_t>(offset);
  command.m_Length.Value = static_cast<uint64_t>(length);
  command.m_Access.Value = access;
  if (!spanPairs.empty()) {
    command.m_Spans = ArrayArgument<uint64_t>(spanPairs.data(), spanPairs.size());
  }
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
  const auto version = CurrentContextGLVersion();
  if (!ContextSupportsBufferStorage(version)) {
    return true;
  }
  GLint flags = 0;
  return queryStorageFlags(flags) && (flags & GL_MAP_READ_BIT) != 0;
}

// Writable coherent maps, and writable persistent maps without a declared flush, whose writes
// drivers make visible to GPU work before unmap even when the map is not coherent. Their writes
// are published before each command that may use them.
bool PublishesBeforeUse(GLbitfield access) {
  constexpr GLbitfield required = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
  return (access & required) == required &&
         ((access & GL_MAP_COHERENT_BIT) != 0 || (access & GL_MAP_FLUSH_EXPLICIT_BIT) == 0);
}

struct PersistentStorage {
  GLint64 Size{0};
  GLbitfield Flags{0};
};

PersistentStorage PersistentMapSanityCheck(
    GLintptr offset,
    GLsizeiptr length,
    GLbitfield access,
    const MapTrackingService::BufferParameterCallback& parameter) {
  const auto version = CurrentContextGLVersion();
  GITS_ASSERT(ContextSupportsBufferStorage(version),
              "Persistent buffer mapping requires buffer storage support");
  GLint64 size = 0;
  GLint64 storageFlags = 0;
  GLint64 mapped = 0;
  GLint64 mapAccess = 0;
  GITS_ASSERT(parameter(GL_BUFFER_SIZE, size),
              "Cannot query GL_BUFFER_SIZE for persistent buffer mapping");
  GITS_ASSERT(parameter(GL_BUFFER_STORAGE_FLAGS, storageFlags),
              "Cannot query GL_BUFFER_STORAGE_FLAGS for persistent buffer mapping");
  GITS_ASSERT(parameter(GL_BUFFER_MAPPED, mapped),
              "Cannot query GL_BUFFER_MAPPED for persistent buffer mapping");
  GITS_ASSERT(parameter(GL_BUFFER_ACCESS_FLAGS, mapAccess),
              "Cannot query GL_BUFFER_ACCESS_FLAGS for persistent buffer mapping");
  GITS_ASSERT(size >= 0 && offset <= size && length <= size - offset,
              "Persistent buffer mapping range exceeds buffer storage");
  GITS_ASSERT(mapped == GL_TRUE, "Persistent buffer mapping is not reported as mapped");
  GITS_ASSERT(mapAccess == access,
              "Persistent buffer mapping access flags differ from requested access");
  const GLbitfield requiredStorageFlags =
      access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
  GITS_ASSERT((storageFlags & requiredStorageFlags) == requiredStorageFlags,
              "Persistent buffer storage flags do not permit requested mapping access");
  return PersistentStorage{size, static_cast<GLbitfield>(storageFlags)};
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
  constexpr GLbitfield invalidate = GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  if (PublishesBeforeUse(access)) {
    // Published pages are copied whole, so the shadow must hold the buffer contents.
    return WatchedShadow::kTracksWrites && (access & invalidate) == 0;
  }
  constexpr GLbitfield required = GL_MAP_WRITE_BIT;
  constexpr GLbitfield excluded = GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
                                  GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_PERSISTENT_BIT |
                                  GL_MAP_COHERENT_BIT;
  const bool explicitFlush = (access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0;
  const bool unsynchronized = (access & GL_MAP_UNSYNCHRONIZED_BIT) != 0;
  return (access & required) == required && (access & excluded) == 0 &&
         (explicitFlush || (WatchedShadow::kTracksWrites && !unsynchronized));
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
  if ((access & GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
    RequireShadowMapAlignment();
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
  return RegisterRange(
      buffer, offset, length, access, mappedPointer,
      [target](GLenum pname, GLint64& value) {
        return GetBoundBufferParameter(target, pname, value);
      },
      initialBytes);
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
                                                        bool initialize,
                                                        WatchedShadow::Tracking tracking) {
  GITS_ASSERT(!initialize || initialBytes.size() == static_cast<size_t>(length),
              "Cannot initialize buffer-map shadow memory without complete initial bytes");
  GITS_ASSERT(alignment <= std::numeric_limits<size_t>::max() - static_cast<size_t>(length),
              "Cannot allocate aligned shadow for buffer map");
  const auto allocationSize = static_cast<size_t>(length) + alignment;
  try {
    Shadow = std::make_unique<WatchedShadow>(allocationSize, tracking);
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
  GITS_ASSERT(size > 0, "Successful named buffer map has an invalid GL_BUFFER_SIZE");
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void* MapTrackingService::OnMapNamedBufferRange(GLuint buffer,
                                                GLintptr offset,
                                                GLsizeiptr length,
                                                GLbitfield access,
                                                void* mappedPointer,
                                                const BufferParameterCallback& parameter,
                                                std::span<const uint8_t> initialBytes) {
  GITS_ASSERT(buffer != 0, "Successful named buffer range map has no buffer name");
  return RegisterRange(buffer, offset, length, access, mappedPointer, parameter, initialBytes);
}

void* MapTrackingService::RegisterRange(GLuint buffer,
                                        GLintptr offset,
                                        GLsizeiptr length,
                                        GLbitfield access,
                                        void* driverPointer,
                                        const BufferParameterCallback& parameter,
                                        std::span<const uint8_t> initialBytes) {
  GITS_ASSERT(driverPointer != nullptr && offset >= 0 && length > 0,
              "Successful buffer range map has invalid tracking parameters");
  const bool readable = (access & GL_MAP_READ_BIT) != 0;
  const bool writable = (access & GL_MAP_WRITE_BIT) != 0;
  const bool persistent = (access & GL_MAP_PERSISTENT_BIT) != 0;
  const bool coherent = (access & GL_MAP_COHERENT_BIT) != 0;
  const bool explicitFlush = (access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0;
  const bool invalidated =
      (access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)) != 0;
  const bool unsynchronized = (access & GL_MAP_UNSYNCHRONIZED_BIT) != 0;
  const bool publishedBeforeUse = PublishesBeforeUse(access);
  const char* unsupported = nullptr;
  if (coherent && !writable) {
    unsupported = "Read-only coherent persistent buffer maps are unsupported";
  } else if (publishedBeforeUse && !WatchedShadow::kTracksWrites) {
    unsupported = "Writable coherent or implicit-flush persistent buffer maps are unsupported on "
                  "this platform";
  } else if (persistent && !readable && !publishedBeforeUse) {
    unsupported = "Write-only persistent noncoherent explicit-flush buffer maps are unsupported";
  }
  if (unsupported != nullptr) {
    GITS_ASSERT(
        false, std::string(unsupported) + " (access: " +
                   GetBitmaskString(GLenumApi::GL, access, GLenumGroup::MapBufferAccessMask) + ')');
  }
  const PersistentStorage storage =
      persistent ? PersistentMapSanityCheck(offset, length, access, parameter)
                 : PersistentStorage{};
  MappedRange range{ByteSource{ByteSource::Kind::DriverPointer, driverPointer, driverPointer},
                    offset,
                    length,
                    access,
                    true,
                    storage.Size,
                    storage.Flags};
  if (publishedBeforeUse) {
    // The application uses a watched shadow. Its written pages reach the driver mapping and the
    // stream at PublishMappedWrites() and at unmap. Reads from the shadow return the buffer
    // contents: OnSubData() copies GL updates into it, and RefreshGpuWrittenMaps() GPU results.
    GITS_ASSERT(invalidated || initialBytes.size() == static_cast<size_t>(length),
                "Writable coherent or implicit-flush persistent buffer maps require reading the "
                "buffer contents before the map");
    RequireShadowMapAlignment();
    GLint alignment = 0;
    g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_MIN_MAP_BUFFER_ALIGNMENT,
                                                                  &alignment);
    GITS_ASSERT(alignment > 0, "Invalid buffer map alignment reported by driver");
    range.RangePolicy.Type = CaptureRangePolicy::Mode::DirtyPages;
    range.Source.PrepareOwnedShadow(offset, length, static_cast<size_t>(alignment), initialBytes,
                                    !invalidated, PublishedShadowTracking(access));
    void* applicationPointer = range.Source.ApplicationPointer;
    StoreMappedRange(CurrentShareGroup(), buffer, std::move(range));
    return applicationPointer;
  }
  const bool dirtyPageCandidate =
      WatchedShadow::kTracksWrites && !explicitFlush && writable && !readable && !unsynchronized;
  if (dirtyPageCandidate || (explicitFlush && !readable)) {
    RequireShadowMapAlignment();
  }
  // Invalidated maps copy touched pages as-is; other maps need a full baseline for comparison.
  const bool useDirtyPageCapture =
      dirtyPageCandidate && (invalidated || initialBytes.size() == static_cast<size_t>(length));
  if (explicitFlush || useDirtyPageCapture) {
    range.RangePolicy.Type = explicitFlush ? CaptureRangePolicy::Mode::DeclaredFlush
                                           : CaptureRangePolicy::Mode::DirtyPages;
    GITS_ASSERT(writable, "Explicit-flush buffer map has no writable byte source");
    if (!readable) {
      GLint alignment = 0;
      g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_MIN_MAP_BUFFER_ALIGNMENT,
                                                                    &alignment);
      GITS_ASSERT(alignment > 0, "Invalid buffer map alignment reported by driver");
      range.Source.PrepareOwnedShadow(offset, length, static_cast<size_t>(alignment), initialBytes,
                                      NeedsInitialShadowContents(access),
                                      WatchedShadow::Tracking::WriteWatch);
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
    MappedRange& range, MapRelativeRange interval, std::vector<uint8_t> bytes) {
  const bool first = !std::exchange(range.Updated, true);
  return MappedBufferUpdate{range.Map,
                            range.Offset + interval.Offset,
                            interval.Length,
                            range.IsRange,
                            range.Access,
                            std::move(bytes),
                            first};
}

MapTrackingService::MappedBufferUpdate MapTrackingService::PrepareFlushMappedUpdate(
    MappedRange& range, MapRelativeRange interval) {
  const auto start = static_cast<size_t>(interval.Offset);
  const auto count = static_cast<size_t>(interval.Length);
  auto bytes = range.Source.Snapshot(start, count);
  // Shadow-backed bytes must reach the real mapping before the caller flushes it.
  range.Source.WriteBack(start, bytes);
  return MakeMappedUpdate(range, interval, std::move(bytes));
}

void MapTrackingService::AppendPublishedUpdates(MappedRange& range,
                                                const std::vector<WatchedShadow::ByteRange>& dirty,
                                                KeyedUpdates& updates) {
  const auto append = [&](size_t offset, size_t length) {
    auto bytes = range.Source.Snapshot(offset, length);
    range.Source.WriteBack(offset, bytes);
    if (!range.Source.Baseline.empty()) {
      std::memcpy(range.Source.Baseline.data() + offset, bytes.data(), length);
    }
    const MapRelativeRange interval{static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(length)};
    updates.emplace_back(CaptureManager::Get().CreateCommandKey(),
                         MakeMappedUpdate(range, interval, std::move(bytes)));
  };
  for (const WatchedShadow::ByteRange& run : dirty) {
    if (!range.GpuWritten) {
      append(run.Offset, run.Length);
      continue;
    }
    // Unchanged bytes between changed ones may be GPU results newer than the shadow, so the run
    // publishes only its changed spans, in one update rather than one per span.
    const auto snapshot = range.Source.Snapshot(run.Offset, run.Length);
    const auto changed = ChangedSpans(
        snapshot, std::span<const uint8_t>(range.Source.Baseline).subspan(run.Offset, run.Length));
    if (changed.empty()) {
      continue;
    }
    const size_t first = changed.front().Offset;
    const size_t end = changed.back().Offset + changed.back().Length;
    std::vector<uint8_t> bytes;
    std::vector<WatchedShadow::ByteRange> spans;
    spans.reserve(changed.size());
    for (const WatchedShadow::ByteRange& span : changed) {
      const auto spanBytes = std::span<const uint8_t>(snapshot).subspan(span.Offset, span.Length);
      const size_t offset = run.Offset + span.Offset;
      range.Source.WriteBack(offset, spanBytes);
      std::memcpy(range.Source.Baseline.data() + offset, spanBytes.data(), span.Length);
      bytes.insert(bytes.end(), spanBytes.begin(), spanBytes.end());
      spans.push_back({span.Offset - first, span.Length});
    }
    const MapRelativeRange interval{static_cast<GLintptr>(run.Offset + first),
                                    static_cast<GLsizeiptr>(end - first)};
    auto update = MakeMappedUpdate(range, interval, std::move(bytes));
    if (spans.size() > 1) {
      update.Spans = std::move(spans);
    }
    updates.emplace_back(CaptureManager::Get().CreateCommandKey(), std::move(update));
  }
}

void MapTrackingService::TakePublishedUpdates(MappedRange& range, KeyedUpdates& updates) {
  m_PublicationsInFlight.fetch_add(1, std::memory_order_release);
  const auto dirty = range.Source.Shadow->TakeDirtyRanges(range.Source.ApplicationPointer,
                                                          static_cast<size_t>(range.Length));
  AppendPublishedUpdates(range, dirty, updates);
  m_PublicationsInFlight.fetch_sub(1, std::memory_order_release);
}

void MapTrackingService::StartGpuWriteTracking(MappedRange& range, KeyedUpdates& updates) {
  const auto length = static_cast<size_t>(range.Length);
  auto* shadow = static_cast<uint8_t*>(range.Source.ApplicationPointer);
  // Copied before the written pages are taken, so that a write meanwhile is either on a taken
  // page, published whole into the baseline, or on a page written again that differs from it.
  range.Source.Baseline.assign(shadow, shadow + length);
  TakePublishedUpdates(range, updates);
  const bool refreshOnRead = Configurator::Get().opengl.recorder.gpuWrittenMapRefresh ==
                             OpenGLGpuWrittenMapRefresh::ON_READ;
  range.Source.Shadow->SetRefreshSource(
      shadow, length, range.Source.DriverPointer, range.Source.Baseline,
      refreshOnRead ? WatchedShadow::Refresh::OnRead : WatchedShadow::Refresh::OnAccess);
  range.Source.Shadow->MarkUnwrittenStale();
  range.GpuWritten = true;
}

std::string MapTrackingService::DescribeMapping(const char* command,
                                                GLenum target,
                                                GLuint buffer,
                                                const MappedRange& range) {
  std::ostringstream description;
  description << '(';
  if (command != nullptr) {
    description << command;
    if (target != 0) {
      description << '(' << GetGLenumString(GLenumApi::GL, target, GLenumGroup::BufferTargetARB)
                  << ')';
    }
    description << " on ";
  }
  description << "buffer " << buffer << ", mapped range [" << range.Offset << ", "
              << range.Offset + range.Length << ") with "
              << GetBitmaskString(GLenumApi::GL, range.Access, GLenumGroup::MapBufferAccessMask)
              << ')';
  return description.str();
}

void MapTrackingService::RecordOrSkipMappedUpdate(uint64_t key,
                                                  const std::optional<MappedBufferUpdate>& update) {
  if (!update) {
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }
  // A replay map covers only the update's interval, which the update overwrites, so only
  // invalidating the whole buffer would discard bytes of another update. An update with spans
  // overwrites only them, so it invalidates nothing.
  constexpr GLbitfield invalidation = GL_MAP_INVALIDATE_BUFFER_BIT;
  GLbitfield access = update->First ? update->Access : update->Access & ~invalidation;
  if (!update->Spans.empty()) {
    access &= ~(GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  }
  RecordMappedDataMetaCommand(key, update->Map.ShareGroup, update->Map.Buffer, update->Offset,
                              update->Length, access, update->Spans, update->Bytes);
  NoteUpdate(update->IsRange, update->Access,
             update->Spans.empty() ? update->Length
                                   : static_cast<GLsizeiptr>(update->Bytes.size()));
}

void MapTrackingService::FlushMappedRange(GLuint buffer,
                                          GLintptr offset,
                                          GLsizeiptr length,
                                          const std::function<void()>& realFlush) {
  const uint64_t shareGroup = CurrentShareGroup();
  std::optional<MappedBufferUpdate> update;
  uint64_t key = 0;
  {
    std::unique_lock<std::mutex> lock(m_MappedBuffersMutex);
    // The key is reserved under the lock that selects the first update of a mapping, so the
    // update that keeps the invalidation bits also replays first.
    key = CaptureManager::Get().CreateCommandKey();
    const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
    if (it == m_MappedBuffers.end()) {
      lock.unlock();
      realFlush();
      RecordOrSkipMappedUpdate(key, std::nullopt);
      return;
    }
    MappedRange& range = it->second;
    GITS_ASSERT((range.Access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0,
                "Cannot flush a buffer mapping created without GL_MAP_FLUSH_EXPLICIT_BIT: "
                "the application call is invalid and GITS does not support it.");
    if (offset < 0 || length < 0 || offset > range.Length || length > range.Length - offset ||
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
  // TODO: Check whether another thread can legally write to this buffer through a shared GL
  // context while this thread is unmapping it. We remove the mapping from m_MappedBuffers before
  // copying its bytes or calling the driver's unmap function. During that time,
  // AssertNoWritablePersistentMap cannot detect that the buffer is still mapped. If such writes
  // are legal, keep the check working until the driver's unmap function returns.
  std::optional<MappedRange> range =
      buffer != 0 ? TakeMappedRange(shareGroup, buffer) : std::nullopt;
  // Untracked or read-only maps need no snapshot and no ordering key.
  if (!range || (range->Access & GL_MAP_WRITE_BIT) == 0) {
    return realUnmap();
  }
  if (range->GpuWritten) {
    KeyedUpdates updates;
    TakePublishedUpdates(*range, updates);
    const GLboolean result = realUnmap();
    if (result != GL_TRUE) {
      LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
    }
    for (auto& [key, update] : updates) {
      if (result != GL_TRUE) {
        update.reset();
      }
      RecordOrSkipMappedUpdate(key, update);
    }
    return result;
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

void MapTrackingService::OnBufferData(GLuint buffer,
                                      const std::function<bool(GLint&)>& queryMapped) {
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  const auto mapped = m_MappedBuffers.find(Key{CurrentShareGroup(), buffer});
  if (mapped == m_MappedBuffers.end()) {
    return;
  }
  GLint driverMapped = -1;
  GITS_ASSERT(queryMapped(driverMapped), "Cannot query GL_BUFFER_MAPPED after storage replacement");
  GITS_ASSERT(driverMapped == GL_FALSE || driverMapped == GL_TRUE,
              "GL_BUFFER_MAPPED query left its output unchanged or returned an invalid boolean");
  if (driverMapped == GL_FALSE) {
    EraseMappedRange(mapped);
  }
}

bool MapTrackingService::MappedRange::AcceptsSubData(uint64_t offset, uint64_t size) const {
  // The driver rejects the update during a mapping that is not persistent, of storage without
  // GL_DYNAMIC_STORAGE_BIT, or past the end of the buffer, leaving its contents unchanged.
  const auto bufferSize = static_cast<uint64_t>(BufferSize);
  return (Access & GL_MAP_PERSISTENT_BIT) != 0 && (StorageFlags & GL_DYNAMIC_STORAGE_BIT) != 0 &&
         offset <= bufferSize && size <= bufferSize - offset;
}

bool MapTrackingService::MappedRange::MayDeferInvalidation() const {
  return (Access & GL_MAP_INVALIDATE_BUFFER_BIT) != 0 && (Access & GL_MAP_PERSISTENT_BIT) != 0 &&
         (StorageFlags & GL_DYNAMIC_STORAGE_BIT) != 0 &&
         Source.Type == ByteSource::Kind::WatchedShadow;
}

bool MapTrackingService::MayDeferInvalidation() const {
  return m_DeferredInvalidationMaps.load(std::memory_order_acquire) != 0;
}

void MapTrackingService::PublishInvalidationBeforeSubData(
    uint64_t shareGroup, GLuint buffer, uint64_t offset, uint64_t size, const void* data) {
  if (data == nullptr || size == 0) {
    return;
  }
  std::optional<MappedBufferUpdate> update;
  uint64_t key = 0;
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
    if (it == m_MappedBuffers.end()) {
      return;
    }
    MappedRange& range = it->second;
    if (range.Updated || !range.MayDeferInvalidation() || !range.AcceptsSubData(offset, size)) {
      return;
    }
    // The first byte of the shadow carries the invalidation: the invalidated contents are
    // undefined until written, and a page the application writes is published again.
    auto bytes = range.Source.Snapshot(0, 1);
    range.Source.WriteBack(0, bytes);
    update = MakeMappedUpdate(range, MapRelativeRange{0, 1}, std::move(bytes));
    key = CaptureManager::Get().CreateCommandKey();
  }
  RecordOrSkipMappedUpdate(key, update);
}

void MapTrackingService::OnSubData(
    uint64_t shareGroup, GLuint buffer, uint64_t offset, uint64_t size, const void* data) {
  if (data == nullptr || size == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
  if (it == m_MappedBuffers.end()) {
    return;
  }
  MappedRange& range = it->second;
  if (!range.AcceptsSubData(offset, size) || range.Source.Type != ByteSource::Kind::WatchedShadow) {
    return;
  }
  const auto mapBegin = static_cast<uint64_t>(range.Offset);
  const uint64_t mapEnd = mapBegin + static_cast<uint64_t>(range.Length);
  const uint64_t begin = std::max(offset, mapBegin);
  const uint64_t end = std::min(offset + size, mapEnd);
  if (begin >= end) {
    return;
  }
  const auto mapOffset = static_cast<size_t>(begin - mapBegin);
  auto* destination = static_cast<uint8_t*>(range.Source.ApplicationPointer) + mapOffset;
  const auto length = static_cast<size_t>(end - begin);
  const auto* source = static_cast<const uint8_t*>(data) + (begin - offset);
  // The application may pass a pointer into its own mapping.
  if (!range.GpuWritten) {
    std::memmove(destination, source, length);
    return;
  }
  // Writing a stale page refreshes it into the baseline, which must then get the update.
  const std::vector<uint8_t> bytes(source, source + length);
  std::memcpy(destination, bytes.data(), length);
  std::memcpy(range.Source.Baseline.data() + mapOffset, bytes.data(), length);
}

void MapTrackingService::PublishBeforeDelete(GLsizei count, const GLuint* buffers) {
  if (buffers == nullptr) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  KeyedUpdates updates;
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    for (GLsizei index = 0; index < count; ++index) {
      const auto mapped = m_MappedBuffers.find(Key{shareGroup, buffers[index]});
      if (mapped == m_MappedBuffers.end()) {
        continue;
      }
      MappedRange& range = mapped->second;
      if (!PublishesBeforeUse(range.Access)) {
        GITS_ASSERT(false, "Deleting buffer " + std::to_string(buffers[index]) +
                               " while it is mapped with " +
                               GetBitmaskString(GLenumApi::GL, range.Access,
                                                GLenumGroup::MapBufferAccessMask) +
                               " is unsupported: only persistent mappings whose writes are "
                               "published before use keep their writes at deletion");
        continue;
      }
      TakePublishedUpdates(range, updates);
    }
  }
  for (const auto& [key, update] : updates) {
    RecordOrSkipMappedUpdate(key, update);
  }
}

void MapTrackingService::OnDelete(GLsizei count, const GLuint* buffers) {
  if (buffers == nullptr) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  for (GLsizei index = 0; index < count; ++index) {
    const auto mapped = m_MappedBuffers.find(Key{shareGroup, buffers[index]});
    if (mapped != m_MappedBuffers.end()) {
      EraseMappedRange(mapped);
    }
  }
}

void MapTrackingService::StoreMappedRange(uint64_t shareGroup, GLuint buffer, MappedRange range) {
  const GLbitfield access = range.Access;
  const Key key{shareGroup, buffer};
  range.Map = key;
  KeyedUpdates updates;
  // A buffer bound for GPU writes before the map may still be bound.
  constexpr GLbitfield persistentWrite = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
  const bool isPersistentWrite = (access & persistentWrite) == persistentWrite;
  const bool untrusted =
      isPersistentWrite && BufferShadowService::Get().IsUntrusted(shareGroup, buffer);
  if (untrusted) {
    // Unlike for GPU result writes, no later command reports the writes of these.
    if (const char* writer = BufferShadowService::Get().UnhookedGpuWriter(shareGroup, buffer)) {
      GITS_ASSERT(false, "Writable persistent mappings of buffers that image stores, external "
                         "memory, or shaders through a resident address may write are "
                         "unsupported: a mapped snapshot could overwrite replayed GPU results "
                         "with stale bytes " +
                             DescribeMapping(writer, 0, buffer, range));
    }
    if (!PublishesBeforeUse(access)) {
      GITS_ASSERT(false, "Explicit-flush noncoherent writable persistent mappings of buffers the "
                         "GPU may write are unsupported: a flushed snapshot could record GPU "
                         "results as the application's writes " +
                             DescribeMapping(nullptr, 0, buffer, range));
    }
    GITS_ASSERT((access & GL_MAP_READ_BIT) != 0,
                "Writable persistent mappings without GL_MAP_READ_BIT of buffers the GPU may "
                "write are unsupported: publishing the application's writes could overwrite GPU "
                "results the recorder cannot read " +
                    DescribeMapping(nullptr, 0, buffer, range));
    StartGpuWriteTracking(range, updates);
  }
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    const auto previous = m_MappedBuffers.find(key);
    if (previous != m_MappedBuffers.end()) {
      EraseMappedRange(previous);
    }
    if (PublishesBeforeUse(range.Access)) {
      m_PublishedBeforeUseMaps.fetch_add(1, std::memory_order_release);
      if (range.Source.Shadow->GetTracking() == WatchedShadow::Tracking::WriteWatch) {
        m_WriteWatchPublishedMaps.fetch_add(1, std::memory_order_release);
      }
    }
    if (range.GpuWritten) {
      m_GpuWrittenMaps.fetch_add(1, std::memory_order_release);
    }
    if (range.MayDeferInvalidation()) {
      m_DeferredInvalidationMaps.fetch_add(1, std::memory_order_release);
    }
    m_MappedBuffers.emplace(key, std::move(range));
  }
  // GPU writes in another context may have looked for the map before it was registered.
  // BufferShadowService marks their buffers untrusted before looking, so either they found the map
  // or this finds the buffer untrusted.
  if (isPersistentWrite && !untrusted &&
      BufferShadowService::Get().IsUntrusted(shareGroup, buffer)) {
    if (const char* writer = BufferShadowService::Get().UnhookedGpuWriter(shareGroup, buffer)) {
      AssertNoWritablePersistentMap(writer, 0, shareGroup, buffer);
    }
    OnGpuResultWrite(nullptr, 0, shareGroup, buffer);
  }
  // Neither this service nor BufferShadowService calls the other while holding its own mutex.
  if (access & GL_MAP_WRITE_BIT) {
    BufferShadowService::Get().OnMapWrite(shareGroup, buffer, access);
  }
  for (const auto& [updateKey, update] : updates) {
    RecordOrSkipMappedUpdate(updateKey, update);
  }
}

void MapTrackingService::AssertNoWritablePersistentMap(const char* command,
                                                       GLenum target,
                                                       uint64_t shareGroup,
                                                       GLuint buffer) {
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
  constexpr GLbitfield unsupported = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
  if (it == m_MappedBuffers.end() || (it->second.Access & unsupported) != unsupported) {
    return;
  }
  GITS_ASSERT(false,
              "GPU copies, clears, image stores, and external writes during a writable persistent "
              "mapping are unsupported: a mapped snapshot could overwrite replayed GPU results "
              "with stale bytes " +
                  DescribeMapping(command, target, buffer, it->second));
}

void MapTrackingService::OnGpuResultWrite(const char* command,
                                          GLenum target,
                                          uint64_t shareGroup,
                                          GLuint buffer) {
  KeyedUpdates updates;
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    const auto it = m_MappedBuffers.find(Key{shareGroup, buffer});
    if (it == m_MappedBuffers.end() || it->second.GpuWritten) {
      return;
    }
    MappedRange& range = it->second;
    constexpr GLbitfield persistentWrite = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
    if (!PublishesBeforeUse(range.Access)) {
      // Other mappings expose the driver mapping itself, or the GPU must not use them.
      if ((range.Access & persistentWrite) == persistentWrite) {
        GITS_ASSERT(false, "GPU writes during an explicit-flush noncoherent writable persistent "
                           "mapping are unsupported: a flushed snapshot could record GPU results "
                           "as the application's writes " +
                               DescribeMapping(command, target, buffer, range));
      }
      return;
    }
    if ((range.Access & GL_MAP_READ_BIT) == 0) {
      GITS_ASSERT(false,
                  "GPU writes during a writable persistent mapping without GL_MAP_READ_BIT are "
                  "unsupported: publishing the application's writes could overwrite GPU results "
                  "the recorder cannot read " +
                      DescribeMapping(command, target, buffer, range));
      return;
    }
    StartGpuWriteTracking(range, updates);
    m_GpuWrittenMaps.fetch_add(1, std::memory_order_release);
  }
  for (const auto& [key, update] : updates) {
    RecordOrSkipMappedUpdate(key, update);
  }
}

void MapTrackingService::RefreshGpuWrittenMaps() {
  if (m_GpuWrittenMaps.load(std::memory_order_acquire) == 0) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  KeyedUpdates updates;
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    for (auto& [key, range] : m_MappedBuffers) {
      if (key.ShareGroup != shareGroup || !range.GpuWritten) {
        continue;
      }
      TakePublishedUpdates(range, updates);
      range.Source.Shadow->MarkUnwrittenStale();
    }
  }
  for (const auto& [key, update] : updates) {
    RecordOrSkipMappedUpdate(key, update);
  }
}

std::optional<MapTrackingService::MappedRange> MapTrackingService::TakeMappedRange(
    uint64_t shareGroup, GLuint buffer) {
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  auto it = m_MappedBuffers.find(key);
  if (it == m_MappedBuffers.end()) {
    return std::nullopt;
  }
  ForgetMappedRangeCounts(it->second);
  MappedRange range = std::move(it->second);
  m_MappedBuffers.erase(it);
  return range;
}

void MapTrackingService::ForgetMappedRangeCounts(const MappedRange& range) {
  if (PublishesBeforeUse(range.Access)) {
    m_PublishedBeforeUseMaps.fetch_sub(1, std::memory_order_release);
    if (range.Source.Shadow->GetTracking() == WatchedShadow::Tracking::WriteWatch) {
      m_WriteWatchPublishedMaps.fetch_sub(1, std::memory_order_release);
    }
  }
  if (range.GpuWritten) {
    m_GpuWrittenMaps.fetch_sub(1, std::memory_order_release);
  }
  if (range.MayDeferInvalidation()) {
    m_DeferredInvalidationMaps.fetch_sub(1, std::memory_order_release);
  }
}

void MapTrackingService::EraseMappedRange(
    std::unordered_map<Key, MappedRange, KeyHash>::iterator it) {
  ForgetMappedRangeCounts(it->second);
  m_MappedBuffers.erase(it);
}

void MapTrackingService::PublishMappedWrites() {
  if (t_InPrimitivePair) {
    return;
  }
  stream::RecordingSession::Get().PublishHostWrites();
  // Most commands follow no write to a persistent map, which write watch cannot tell unqueried.
  // Taken pages leave the pending count before their publication completes, so publications in
  // flight are checked after it.
  if (m_PublishedBeforeUseMaps.load(std::memory_order_acquire) == 0 ||
      (m_WriteWatchPublishedMaps.load(std::memory_order_acquire) == 0 &&
       WatchedShadow::PendingWrittenPages() == 0 &&
       m_PublicationsInFlight.load(std::memory_order_acquire) == 0)) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  KeyedUpdates updates;
  {
    std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
    for (auto& [key, range] : m_MappedBuffers) {
      if (key.ShareGroup != shareGroup || !PublishesBeforeUse(range.Access) ||
          !range.Source.Shadow->MayHaveWrittenPages()) {
        continue;
      }
      TakePublishedUpdates(range, updates);
    }
  }
  // Recording reaches BufferShadowService, so record after releasing the map mutex.
  for (const auto& [key, update] : updates) {
    RecordOrSkipMappedUpdate(key, update);
  }
}

void MapTrackingService::SetInPrimitivePair(bool inPair) {
  t_InPrimitivePair = inPair;
}

} // namespace gits::opengl
