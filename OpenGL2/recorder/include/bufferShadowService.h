// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits::opengl2 {

// The pname reporting which buffer is bound to `target`, or 0 for a target that is not a
// buffer binding point.
GLenum BufferBindingPname(GLenum target);

// A buffer addressed by the binding of `Target` on the current context, or by name.
struct BufferRef {
  GLenum Target{0};
  GLuint Name{0};

  static BufferRef ByTarget(GLenum target) {
    return {target, 0};
  }
  static BufferRef ByName(GLuint name) {
    return {0, name};
  }
};

// Recorder-side copies of element buffer contents, so a draw that reads client-memory vertex
// arrays through indices in a buffer can find its vertex range without reading the buffer back
// from the driver. Reading back a buffer the GPU still uses synchronizes with the application's
// in-flight work, and the recorder should not touch the application's buffers.
// This is buffer-content state tracking, not storage returned to the application by a map.
// MapTrackingService owns mapped pointers (including any application-visible shadow), mapping
// lifetime, and publication; it notifies this service when mapped writes affect its copy.
//
// A buffer gets a copy when data is uploaded to GL_ELEMENT_ARRAY_BUFFER, or when a draw first
// reads it (seeded through a staging copy). Contexts without GL 3.1 / ARB_copy_buffer cannot
// stage, so there every complete upload gets a copy, whatever its target or whether it is made
// by name. Once copied, every upload and tracked mapped write
// on any target updates it. Buffers the GPU or external memory can write (copy and clear
// destinations, pixel pack and query buffers, transform feedback, shader storage, atomic
// counters, buffer textures, external storage) are never trusted and always read through the
// staging copy. The hooked commands are listed in codegen/buffer_content_hooks.py.
class BufferShadowService {
public:
  static BufferShadowService& Get();

  void OnData(BufferRef ref, uint64_t size, const void* data);
  void OnSubData(BufferRef ref, uint64_t offset, uint64_t size, const void* data);
  void OnGpuWrite(BufferRef ref);
  // Contents undefined until the next complete upload or seeding.
  void OnUndefined(BufferRef ref);
  void OnBind(GLenum target, GLuint buffer);
  void OnBindIndexed(GLenum target, GLuint buffer);
  void OnBindIndexed(GLenum target, GLsizei count, const GLuint* buffers);
  void OnDelete(GLsizei n, const GLuint* buffers);

  // MapTrackingService reports writable map creation and published mapped bytes here.
  // GL_MAP_INVALIDATE_BUFFER_BIT leaves only the mapped range known.
  void OnMapWrite(uint64_t shareGroup, GLuint buffer, GLbitfield access);
  // Called after unmap of a transient writable mapping if buffer contents are same as before the
  // map. Marks the buffer's copy as known to match the actual contents if it matched before the
  // map.
  void OnMapUnchanged(uint64_t shareGroup, GLuint buffer);
  void OnMappedBytes(uint64_t shareGroup,
                     GLuint buffer,
                     uint64_t offset,
                     const std::vector<uint8_t>& bytes);

  enum class ReadSource {
    Copy,
    Staging,
    Failed,
  };
  // Reads [offset, offset + size) of the buffer bound to GL_ELEMENT_ARRAY_BUFFER. Uses the copy
  // when known; otherwise copies the bytes into a recorder-owned staging buffer and reads that,
  // which needs `copyBuffer` (GL 3.1 / ARB_copy_buffer). The whole buffer is staged once to seed
  // its copy, unless it is untrusted or `mapped` (persistently; the application may write it
  // through the mapping at any time). Failed leaves `out` empty.
  ReadSource ReadElementBuffer(
      bool copyBuffer, bool mapped, uint64_t offset, uint64_t size, std::vector<uint8_t>& out);

private:
  BufferShadowService() = default;

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
  struct Shadow {
    std::vector<uint8_t> Bytes;
    bool Known{false};
    bool KnownBeforeMap{false};
  };

  // Resolves `ref` on the current context; Buffer 0 when nothing is bound.
  Key Resolve(BufferRef ref);
  void MarkUntrusted(const Key& key);
  bool ReadThroughStaging(uint64_t shareGroup,
                          uint64_t offset,
                          uint64_t size,
                          std::vector<uint8_t>& out);

  std::mutex m_Mutex;
  std::unordered_map<Key, Shadow, KeyHash> m_Shadows;
  std::unordered_set<Key, KeyHash> m_Untrusted;
  // Recorder-owned staging buffer per (share group, context). A context is current on one
  // thread at a time, so its staging sequence never runs concurrently with another's.
  using StagingKey = std::pair<uint64_t, uintptr_t>;
  std::map<StagingKey, GLuint> m_Staging;
};

} // namespace gits::opengl2
