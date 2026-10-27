// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Recorder-side buffer-map tracking: captures implicit writes at unmap and explicit writes at
// flush as MappedDataMetaCommands. It does not track writes continuously or persistent mappings.
//
// Lives under recorder/ rather than common/services/, mirroring Vulkan and DirectX: capturing a
// mapped range needs the recorder's own real-driver dispatch table and CaptureManager, neither of
// which the recorder+player-shared services may depend on. The player needs no counterpart -
// MappedDataMetaRunner performs its whole map/write/unmap against the real driver in one call and
// never tracks an in-progress mapping.

#include "glTypes.h"
#include "watchedShadow.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits::opengl2 {

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
                              std::span<const uint8_t> initialBytes = {});

  void OnDelete(GLsizei count, const GLuint* buffers);

  // Asks the real driver which buffer is bound to `target`. Returns 0, warning once per
  // target, for an untracked target or one with nothing bound.
  GLuint ResolveBoundBuffer(GLenum target);
  void FlushMappedRange(GLuint buffer,
                        GLintptr offset,
                        GLsizeiptr length,
                        uint64_t key,
                        const std::function<void()>& realFlush);
  void* TranslateShadowPointerQuery(GLuint buffer, void* pointer);

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
                            bool initialize);
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
    CaptureRangePolicy RangePolicy;
    Key Map{0, 0};
  };

  struct MappedBufferUpdate {
    Key Map;
    GLintptr Offset{0};
    GLsizeiptr Length{0};
    bool IsRange{true};
    GLbitfield Access{0};
    std::vector<uint8_t> Bytes;
  };

  MappedBufferUpdate MakeMappedUpdate(const MappedRange& range,
                                      MapRelativeRange interval,
                                      std::vector<uint8_t> bytes);
  MappedBufferUpdate PrepareFlushMappedUpdate(const MappedRange& range, MapRelativeRange interval);
  // After the real GL call, record the captured update at the reserved key or skip that key.
  void RecordOrSkipMappedUpdate(uint64_t key, const std::optional<MappedBufferUpdate>& update);

  void* RegisterRange(GLuint buffer,
                      GLintptr offset,
                      GLsizeiptr length,
                      GLbitfield access,
                      void* driverPointer,
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

  std::mutex m_MappedBuffersMutex;
  std::unordered_map<Key, MappedRange, KeyHash> m_MappedBuffers;

  // Warn-once state for unrecognized targets, missing bindings, and unsupported coherent maps.
  // Map/unmap is a low-frequency path, so a plain mutex is sufficient.
  std::mutex m_WarnedTargetsMutex;
  std::unordered_set<GLenum> m_WarnedUnrecognizedTargets;
  std::unordered_set<GLenum> m_WarnedNoBufferBound;
};

} // namespace gits::opengl2
