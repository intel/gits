// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Recorder-side buffer-map tracking: captures implicit writes at unmap, explicit writes at flush
// and coherent or implicit-flush persistent writes before commands that may read them as
// MappedDataMetaCommands.
//
// Lives under recorder/ rather than common/services/, mirroring Vulkan and DirectX: capturing a
// mapped range needs the recorder's own real-driver dispatch table and CaptureManager, neither of
// which the recorder+player-shared services may depend on. The player needs no counterpart -
// MappedDataMetaRunner performs its whole map/write/unmap against the real driver in one call and
// never tracks an in-progress mapping.

#include "glTypes.h"
#include "watchedShadow.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits::opengl {

class MapTrackingService {
public:
  static MapTrackingService& Get();

  void NoteMap(bool range, uint32_t access, bool succeeded);

  using BufferParameterCallback = std::function<bool(GLenum, GLint64&)>;
  using BufferReadCallback = std::function<void(GLintptr, GLsizeiptr, void*)>;
  static bool NeedsInitialShadowContents(GLbitfield access);
  std::vector<uint8_t> PrepareInitialBytes(GLintptr offset,
                                           GLsizeiptr length,
                                           GLbitfield access,
                                           bool getterAvailable,
                                           const BufferParameterCallback& parameter,
                                           const BufferReadCallback& read);

  // Registers a successful glMapBufferRange and returns the application-visible pointer.
  void* OnMapBufferRange(GLenum target,
                         GLintptr offset,
                         GLsizeiptr length,
                         GLbitfield access,
                         void* mappedPointer,
                         std::span<const uint8_t> initialBytes = {});

  // Performs the application-visible glUnmapBuffer and returns its result unchanged, snapshotting
  // the tracked range before or after that call as the mapping's access flags require. Emits a
  // MappedDataMetaCommand only for a write-capable mapping whose unmap succeeded.
  GLboolean UnmapBufferAndCapture(GLenum target);

  // --- Extensions beyond the original glMapBufferRange/glUnmapBuffer pair -------------------
  //
  // Additional map/unmap entry points use the same range and snapshot bookkeeping.

  // Registers a just-succeeded glMapBuffer/glMapBufferARB (target-addressed, implicit
  // whole-buffer range - neither function takes an explicit offset/length or a GL_MAP_*_BIT
  // bitfield) so the matching glUnmapBuffer/glUnmapBufferARB can capture whatever the
  // application writes. `access` is gl.xml's BufferAccessARB enumerant
  // (GL_READ_ONLY/GL_WRITE_ONLY/GL_READ_WRITE), not glMapBufferRange's GL_MAP_*_BIT bitfield -
  // translated internally (see MapAccessEnumToBits) into the same MappedRange
  // representation OnMapBufferRange above uses. Does nothing when the mapped target has no
  // resolvable buffer bound.
  void OnMapBuffer(GLenum target, GLenum access, void* mappedPointer);

  // Core and EXT DSA maps provide the buffer name directly. `querySize` uses the appropriate
  // family's GL_BUFFER_SIZE getter; range maps already provide offset, length, and access.
  using NamedBufferSizeFn = GLint64 (*)(GLuint);
  void OnMapNamedBuffer(GLuint buffer,
                        GLenum access,
                        void* mappedPointer,
                        NamedBufferSizeFn querySize);

  // Registers a successful core or EXT named-buffer range map.
  void* OnMapNamedBufferRange(GLuint buffer,
                              GLintptr offset,
                              GLsizeiptr length,
                              GLbitfield access,
                              void* mappedPointer,
                              const BufferParameterCallback& parameter,
                              std::span<const uint8_t> initialBytes = {});

  void OnBufferData(GLuint buffer, const std::function<bool(GLint&)>& queryMapped);
  // After a glBufferSubData-family call, copies its bytes into the application's shadow of a
  // persistent mapping of `buffer`, so publishing a page the application writes later does not
  // restore the bytes the update replaced. The pages it writes are published again, as a
  // concurrent application write to them would otherwise be lost.
  void OnSubData(
      uint64_t shareGroup, GLuint buffer, uint64_t offset, uint64_t size, const void* data);
  // Whether a mapping may need PublishInvalidationBeforeSubData(). Reads no locked state, so that
  // uploads check it cheaply.
  bool MayDeferInvalidation() const;
  // Before a glBufferSubData-family call reserves its key: the driver invalidated the buffer when
  // the application mapped it, but replay invalidates it at the first update of the mapping. If
  // the driver accepts the upload during a mapping with GL_MAP_INVALIDATE_BUFFER_BIT that has no
  // update yet, records an update that carries the invalidation, so the upload replays after it.
  void PublishInvalidationBeforeSubData(
      uint64_t shareGroup, GLuint buffer, uint64_t offset, uint64_t size, const void* data);
  // Before glDeleteBuffers, which implicitly unmaps the buffers it deletes: publishes the writes
  // to their persistent mappings, as their retained objects may still be used, and the driver
  // mappings are gone once the call returns. Other mappings of deleted buffers are unsupported.
  void PublishBeforeDelete(GLsizei count, const GLuint* buffers);
  // After glDeleteBuffers, forgets the mappings of the deleted buffers.
  void OnDelete(GLsizei count, const GLuint* buffers);
  // `command` and `target` (0 when the buffer is named directly) only describe the rejected call.
  void AssertNoWritablePersistentMap(const char* command,
                                     GLenum target,
                                     uint64_t shareGroup,
                                     GLuint buffer);
  // Before GPU work may write `buffer` through `target`: a writable persistent mapping of it whose
  // writes are published before use gets the GPU results at RefreshGpuWrittenMaps(), and from
  // then on publishes only the bytes the application changed. Such mappings without
  // GL_MAP_READ_BIT, and other writable persistent mappings, are unsupported.
  void OnGpuResultWrite(const char* command, GLenum target, uint64_t shareGroup, GLuint buffer);
  // After a host sync that makes completed GPU writes visible to the application: publishes the
  // pending writes to GPU-written mappings of the current share group, then makes their shadow
  // pages get the driver contents on their next access, or at once, except for the bytes the
  // application changed, for pages written meanwhile.
  void RefreshGpuWrittenMaps();

  // Asks the real driver which buffer is bound to `target`. Returns 0, warning once per
  // target, for an untracked target or one with nothing bound.
  GLuint ResolveBoundBuffer(GLenum target);
  // Reserves the ordering key of the flush's update itself.
  void FlushMappedRange(GLuint buffer,
                        GLintptr offset,
                        GLsizeiptr length,
                        const std::function<void()>& realFlush);
  void* TranslateShadowPointerQuery(GLuint buffer, void* pointer);

  // Called before a command that may read or write buffer or texture memory, before its key is
  // reserved.
  // Writes through writable coherent and implicit-flush persistent maps of the current share group
  // reach the driver and the stream, and so do host writes to memory that the recorder of another
  // API shares through stream::RecordingSession. Does nothing between glBegin and glEnd on this
  // thread.
  void PublishMappedWrites();
  void SetInPrimitivePair(bool inPair);

  // Uses its own real driver entry point; glUnmapBuffer and glUnmapBufferARB may resolve
  // independently.
  GLboolean UnmapBufferARBAndCapture(GLenum target);

  // Named-buffer unmap uses its own core or EXT dispatch family. Readback falls back to a
  // whole-buffer map only when the range entry point is unavailable, then to subdata if needed.
  using NamedUnmapFn = GLboolean(APIENTRY*)(GLuint);
  using NamedReadOnlyRemapFn = void* (*)(GLuint, GLintptr, GLsizeiptr);
  using NamedReadSubDataFn = bool (*)(GLuint, GLintptr, GLsizeiptr, void*);
  using NamedStorageFlagsFn = bool (*)(GLuint, GLint&);
  GLboolean UnmapNamedBufferAndCapture(const char* callerName,
                                       GLuint buffer,
                                       NamedUnmapFn unmap,
                                       NamedReadOnlyRemapFn readOnlyRemap,
                                       NamedReadSubDataFn readSubData,
                                       NamedStorageFlagsFn queryStorageFlags);

private:
  MapTrackingService();

  struct MapStatistics {
    uint64_t Attempts{0};
    uint64_t Successful{0};
    uint64_t Updates{0};
    uint64_t UpdatedBytes{0};
  };
  void NoteUpdate(bool range, uint32_t access, GLsizeiptr length);
  void LogStatistics();

  std::mutex m_StatisticsMutex;
  std::map<std::pair<bool, uint32_t>, MapStatistics> m_Statistics;
  // Buffer names are unique within a share group, not process-globally.
  struct Key {
    uint64_t ShareGroup;
    GLuint Buffer;
    bool operator==(const Key& other) const {
      return ShareGroup == other.ShareGroup && Buffer == other.Buffer;
    }
  };
  struct KeyHash {
    size_t operator()(const Key& key) const {
      return std::hash<uint64_t>()(key.ShareGroup) ^ (std::hash<GLuint>()(key.Buffer) << 1);
    }
  };

  // This offset is relative to the mapped pointer; MappedBufferUpdate uses a buffer-relative
  // offset. MakeMappedUpdate converts the former to the latter.
  struct MapRelativeRange {
    GLintptr Offset;
    GLsizeiptr Length;
  };

  struct CaptureRangePolicy {
    enum class Mode {
      FullRange,
      DeclaredFlush,
      DirtyPages
    };
    Mode Type{Mode::FullRange};
    std::optional<MapRelativeRange> SelectFlushRange(GLintptr offset, GLsizeiptr length) const;
    std::optional<MapRelativeRange> SelectUnmapRange(GLsizeiptr length) const;
  };

  struct ByteSource {
    enum class Kind {
      DriverPointer,
      WatchedShadow
    };
    Kind Type{Kind::DriverPointer};
    void* DriverPointer{nullptr};
    void* ApplicationPointer{nullptr};
    std::unique_ptr<WatchedShadow> Shadow;
    std::vector<uint8_t> Baseline;

    void PrepareOwnedShadow(GLintptr offset,
                            GLsizeiptr length,
                            size_t alignment,
                            std::span<const uint8_t> initialBytes,
                            bool initialize,
                            WatchedShadow::Tracking tracking);
    void* TranslatePointer(void* pointer) const;
    std::vector<uint8_t> Snapshot(size_t offset, size_t length) const;
    void WriteBack(size_t offset, std::span<const uint8_t> bytes) const;
  };

  struct MappedRange {
    ByteSource Source;
    GLintptr Offset{0};
    GLsizeiptr Length{0};
    GLbitfield Access{0};
    bool IsRange{true}; // Whether it is a range mapping or a whole-buffer mapping.
    // GL_BUFFER_SIZE and GL_BUFFER_STORAGE_FLAGS of a persistently mapped buffer, which decide
    // whether the driver accepts a glBufferSubData-family update during the mapping.
    GLint64 BufferSize{0};
    GLbitfield StorageFlags{0};
    CaptureRangePolicy RangePolicy;
    Key Map{0, 0};
    // Whether an update of this mapping was taken. Only the first update of a mapping carries
    // its invalidation bits, so a later update does not discard the bytes of earlier ones.
    bool Updated{false};
    // Whether GPU work may write this shadow-backed mapping. Source.Baseline then holds the
    // shadow contents as of the last page refresh or publication, and only bytes differing from
    // it are published, so that publishing does not overwrite GPU results. The shadow refreshes
    // pages into it, so it is not reallocated.
    bool GpuWritten{false};

    // Whether the driver accepts a glBufferSubData-family update of [offset, offset + size) of
    // the buffer during this mapping.
    bool AcceptsSubData(uint64_t offset, uint64_t size) const;
    // Whether the first update carries GL_MAP_INVALIDATE_BUFFER_BIT, which would discard the
    // uploads the driver accepts before it.
    bool MayDeferInvalidation() const;
  };

  struct MappedBufferUpdate {
    Key Map;
    GLintptr Offset{0};
    GLsizeiptr Length{0};
    bool IsRange{true};
    GLbitfield Access{0};
    std::vector<uint8_t> Bytes;
    bool First{true}; // Whether it is the first update of its mapping.
  };

  MappedBufferUpdate MakeMappedUpdate(MappedRange& range,
                                      MapRelativeRange interval,
                                      std::vector<uint8_t> bytes);
  MappedBufferUpdate PrepareFlushMappedUpdate(MappedRange& range, MapRelativeRange interval);
  using KeyedUpdates = std::vector<std::pair<uint64_t, std::optional<MappedBufferUpdate>>>;
  // Copies the written pages in `dirty` of a mapping published before use to the driver mapping,
  // and into Source.Baseline when it has one, and appends their updates, each with its reserved
  // key. Of a GPU-written mapping, only the changed bytes are copied, as separate updates.
  // Callers hold m_MappedBuffersMutex, or own `range`, and record the updates after releasing it.
  void AppendPublishedUpdates(MappedRange& range,
                              const std::vector<WatchedShadow::ByteRange>& dirty,
                              KeyedUpdates& updates);
  // AppendPublishedUpdates() for the pages written since the last call.
  void TakePublishedUpdates(MappedRange& range, KeyedUpdates& updates);
  // Makes a readable mapping published before use GPU-written: appends the updates of its
  // pending writes, and its unwritten shadow pages get the driver contents on their next access.
  // Callers hold m_MappedBuffersMutex or own `range`, and record the updates after releasing it.
  void StartGpuWriteTracking(MappedRange& range, KeyedUpdates& updates);
  // `command` is null when no command is rejected, and `target` 0 when the buffer is named.
  static std::string DescribeMapping(const char* command,
                                     GLenum target,
                                     GLuint buffer,
                                     const MappedRange& range);
  // After the real GL call, record the captured update at the reserved key or skip that key.
  void RecordOrSkipMappedUpdate(uint64_t key, const std::optional<MappedBufferUpdate>& update);

  void* RegisterRange(GLuint buffer,
                      GLintptr offset,
                      GLsizeiptr length,
                      GLbitfield access,
                      void* driverPointer,
                      const BufferParameterCallback& parameter,
                      std::span<const uint8_t> initialBytes);

  // Overwrites any previous entry for the same (share group, buffer) without warning: GL
  // forbids mapping an already-mapped buffer, so the driver itself already enforces that at
  // most one mapping per buffer is live.
  void StoreMappedRange(uint64_t shareGroup, GLuint buffer, MappedRange range);

  // "Take", not "peek": a tracked mapping is valid for at most one matching unmap. Returns
  // std::nullopt when the buffer has no tracked mapping in that share group.
  std::optional<MappedRange> TakeMappedRange(uint64_t shareGroup, GLuint buffer);

  // Translates glMapBuffer/glMapBufferARB/glMapNamedBuffer(EXT)'s own `access` parameter - one of
  // GL_READ_ONLY/GL_WRITE_ONLY/GL_READ_WRITE (gl.xml's BufferAccessARB group - NOT a bitfield,
  // unlike glMapBufferRange's GL_MAP_*_BIT `access`) - into the GL_MAP_READ_BIT/GL_MAP_WRITE_BIT
  // bitfield convention MappedRange::Access already uses. Invalid values throw.
  static GLbitfield MapAccessEnumToBits(GLenum access);
  static GLenum MapAccessBitsToEnum(GLbitfield access);

  // Shared tail of target and named whole-buffer maps: they map the entire buffer implicitly (no
  // explicit length), so each separately queries it (via its own target/DSA-appropriate
  // GL_BUFFER_SIZE getter) before reaching this common StoreMappedRange call. Unlike
  // OnMapBufferRange/OnMapNamedBufferRange, this never checks GL_MAP_COHERENT_BIT: `access`
  // here is the BufferAccessARB enum group, not MapBufferAccessMask, so the whole-buffer map
  // family has no coherent-mapping concept of its own at all.
  void StoreWholeBufferMap(
      uint64_t shareGroup, GLuint buffer, GLenum access, void* mappedPointer, GLsizeiptr length);

  // Shared snapshot and readback policy for every unmap entry point.
  //
  // `callerName` is used only for this function's own LOG_WARNING messages, so a warning always
  // names the specific entry point (e.g. "glUnmapBufferARB") that actually observed the
  // problem. `realUnmap` performs the one real, app-visible unmap call - whichever specific
  // driver entry point matches the caller - and returns its GLboolean result, exactly as the
  // app's own call would. `realReadOnlyRemap` performs the write-only fallback's temporary
  // read-only re-map of [offset, offset+length) and returns the mapped pointer, or nullptr on
  // failure; `realInternalUnmap` performs the second, purely-internal unmap that follows a
  // successful remap. Its result is checked, but never propagated to the app. `realReadSubData`
  // reads the same range if a read remap is forbidden or fails; `queryStorageFlags` checks
  // whether buffer storage permits a read remap.
  GLboolean SnapshotBufferAndUnmap(
      const char* callerName,
      GLuint buffer,
      uint64_t shareGroup,
      const std::function<GLboolean()>& realUnmap,
      const std::function<void*(GLintptr, GLsizeiptr)>& realReadOnlyRemap,
      const std::function<GLboolean()>& realInternalUnmap,
      const std::function<bool(GLintptr, GLsizeiptr, void*)>& realReadSubData,
      const std::function<bool(GLint&)>& queryStorageFlags);

  // Callers hold m_MappedBuffersMutex.
  void ForgetMappedRangeCounts(const MappedRange& range);
  // Callers hold m_MappedBuffersMutex.
  void EraseMappedRange(std::unordered_map<Key, MappedRange, KeyHash>::iterator it);

  std::mutex m_MappedBuffersMutex;
  std::unordered_map<Key, MappedRange, KeyHash> m_MappedBuffers;
  // Entries of m_MappedBuffers whose writes PublishMappedWrites() publishes. Modified under
  // m_MappedBuffersMutex; read without it so that commands check it cheaply.
  std::atomic<size_t> m_PublishedBeforeUseMaps{0};
  // Those of them with write-watch shadows, likewise.
  std::atomic<size_t> m_WriteWatchPublishedMaps{0};
  // Entries of m_MappedBuffers with GpuWritten, likewise.
  std::atomic<size_t> m_GpuWrittenMaps{0};
  // Entries of m_MappedBuffers for which MappedRange::MayDeferInvalidation() holds, likewise.
  std::atomic<size_t> m_DeferredInvalidationMaps{0};
  // TakePublishedUpdates() calls between taking written pages and reserving the keys of their
  // updates, once the pages are no longer counted by WatchedShadow::PendingWrittenPages().
  std::atomic<size_t> m_PublicationsInFlight{0};

  // Warn-once state for unrecognized targets, missing bindings, and unsupported coherent maps.
  // Map/unmap is a low-frequency path, so a plain mutex is sufficient.
  std::mutex m_WarnedTargetsMutex;
  std::unordered_set<GLenum> m_WarnedUnrecognizedTargets;
  std::unordered_set<GLenum> m_WarnedNoBufferBound;
};

} // namespace gits::opengl
