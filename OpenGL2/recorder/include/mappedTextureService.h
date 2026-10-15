// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Recorder-side tracking of GL_INTEL_map_texture mappings. A glMapTexture2DINTEL pointer is the
// texture's own memory and may stay mapped while the GPU samples it, so its contents are
// captured (as a MappedTextureDataMetaCommand) whenever the application could have published
// writes: before glSyncTextureINTEL, before glUnmapTexture2DINTEL, and before each swap while the
// texture stays mapped. A level is left out when its contents hash the same as the last ones
// replay already has. The application's own map/sync/unmap calls are not recorded.

#include "glTypes.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits::opengl2 {

class MappedTextureService {
public:
  static MappedTextureService& Get();

  // Tracks a just-succeeded glMapTexture2DINTEL. Read-only mappings are ignored.
  void OnMap(GLuint texture, GLint level, GLbitfield access, void* pointer, GLint stride);

  // Records the changed mappings' contents as one command. Called before the real
  // glSyncTextureINTEL (every mapped level of `texture`) and glUnmapTexture2DINTEL (`level` only),
  // while the pointers are still valid; the unmap also stops tracking it. The command's key is
  // reserved before any mapping is read, and only when there is a tracked mapping.
  void CaptureOnSync(GLuint texture);
  void CaptureOnUnmap(GLuint texture, GLint level);

  // Records every changed mapping of the current share group under `key`, which the swap's
  // wrapper reserved ahead of its own (see MappedTextureLayer), or skips the key.
  void CaptureOnSwap(uint64_t key);

  // Stops tracking deleted textures, whose mappings the driver releases with them.
  void OnDelete(GLsizei n, const GLuint* textures);

private:
  MappedTextureService() = default;

  struct Key {
    uint64_t ShareGroup;
    GLuint Texture;
    GLint Level;
    bool operator==(const Key& other) const {
      return ShareGroup == other.ShareGroup && Texture == other.Texture && Level == other.Level;
    }
  };
  struct KeyHash {
    size_t operator()(const Key& key) const {
      return std::hash<uint64_t>()(key.ShareGroup) ^ (std::hash<GLuint>()(key.Texture) << 1) ^
             (std::hash<GLint>()(key.Level) << 2);
    }
  };
  struct Mapping {
    const uint8_t* Pointer{nullptr};
    GLint Stride{0};
    GLint Rows{0};
    uint64_t Hash{0}; // Of the contents replay already has.
  };

  using Entry = std::pair<const Key, Mapping>;

  // Records the changed ones of `entries` under `key`, or skips it. Callers hold m_Mutex.
  void CaptureChanged(const std::vector<Entry*>& entries, uint64_t key);

  std::mutex m_Mutex;
  std::unordered_map<Key, Mapping, KeyHash> m_Mappings;
};

} // namespace gits::opengl2
