// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayContextCaps.h"

#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "openglDriversAuto.h"

#include <cstring>

namespace gits::opengl2 {

namespace {

struct CachedCaps {
  bool Valid{false};
  HGLRC Context{nullptr};
  ReplayContextCaps Caps;
};

thread_local CachedCaps t_Cached;

class ExtensionChecker {
public:
  explicit ExtensionChecker(std::pair<int, int> version) : m_Version(version) {
    auto& table = player::g_DispatchTable;
    if (GlVersionAtLeast(m_Version, 3, 0)) {
      // GL_EXTENSIONS is not a valid glGetString name on core profiles.
      table.glGetIntegerv.load(std::memory_order_acquire)(GL_NUM_EXTENSIONS, &m_Count);
    } else {
      m_Legacy = reinterpret_cast<const char*>(
          table.glGetString.load(std::memory_order_acquire)(GL_EXTENSIONS));
    }
  }

  bool Has(const char* name) const {
    if (!GlVersionAtLeast(m_Version, 3, 0)) {
      return ExtensionsStringContainsWholeWord(m_Legacy, name);
    }
    auto getStringi = player::g_DispatchTable.glGetStringi.load(std::memory_order_acquire);
    for (GLint i = 0; i < m_Count; ++i) {
      const auto* extension =
          reinterpret_cast<const char*>(getStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
      if (extension != nullptr && std::strcmp(extension, name) == 0) {
        return true;
      }
    }
    return false;
  }

private:
  std::pair<int, int> m_Version;
  GLint m_Count{0};
  const char* m_Legacy{nullptr};
};

ReplayContextCaps QueryCaps() {
  ReplayContextCaps caps;
  auto& table = player::g_DispatchTable;
  const auto version = ParseGlVersionMajorMinor(
      reinterpret_cast<const char*>(table.glGetString.load(std::memory_order_acquire)(GL_VERSION)));
  if (version.first == 0) {
    return caps;
  }
  const ExtensionChecker extensions(version);
  caps.MultiTextureCore = GlVersionAtLeast(version, 1, 3);
  caps.MultiTexture = caps.MultiTextureCore || extensions.Has("GL_ARB_multitexture");
  caps.TextureMaxLevel = GlVersionAtLeast(version, 1, 2) || extensions.Has("GL_SGIS_texture_lod");
  caps.SamplerObjects = GlVersionAtLeast(version, 3, 3) || extensions.Has("GL_ARB_sampler_objects");
  caps.DirectStateAccess =
      GlVersionAtLeast(version, 4, 5) || extensions.Has("GL_ARB_direct_state_access");

  auto getIntegerv = table.glGetIntegerv.load(std::memory_order_acquire);
  bool core = false;
  if (GlVersionAtLeast(version, 3, 0)) {
    GLint flags = 0;
    getIntegerv(GL_CONTEXT_FLAGS, &flags);
    core = (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0;
  }
  if (GlVersionAtLeast(version, 3, 2)) {
    GLint profile = 0;
    getIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    core = core || (profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
  } else if (version.first == 3 && version.second == 1) {
    core = core || !extensions.Has("GL_ARB_compatibility");
  }
  caps.DisplayLists = !core;

  GLint units = 1;
  if (GlVersionAtLeast(version, 2, 0)) {
    getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
  } else if (caps.MultiTexture) {
    getIntegerv(GL_MAX_TEXTURE_UNITS, &units);
  }
  caps.TextureUnits = units > 0 ? units : 1;
  return caps;
}

} // namespace

const ReplayContextCaps& CurrentReplayContextCaps() {
  HGLRC context = player::g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire)();
  if (!t_Cached.Valid || t_Cached.Context != context) {
    t_Cached.Caps = context != nullptr ? QueryCaps() : ReplayContextCaps{};
    t_Cached.Context = context;
    t_Cached.Valid = true;
  }
  return t_Cached.Caps;
}

void InvalidateReplayContextCaps() {
  t_Cached.Valid = false;
}

} // namespace gits::opengl2
