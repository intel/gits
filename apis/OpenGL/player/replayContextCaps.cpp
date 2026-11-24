// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayContextCaps.h"

#include "glContextQueries.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "handleArgumentUpdaters.h"
#include "glDispatchAuto.h"

#include <utility>

namespace gits::opengl {

namespace {

struct CachedCaps {
  bool Valid{false};
  const void* Context{nullptr};
  ReplayContextCaps Caps;
};

thread_local CachedCaps t_Cached;

ReplayContextCaps QueryCaps() {
  ReplayContextCaps caps;
  auto& table = g_DispatchTable;
  const auto version = CurrentContextGLVersion();
  if (version.first == 0) {
    return caps;
  }
  const CurrentContextExtensions extensions(version);
  caps.MultiTextureCore = GlVersionAtLeast(version, 1, 3);
  caps.MultiTexture = caps.MultiTextureCore || extensions.Has("GL_ARB_multitexture");
  caps.SecondaryColorCore = GlVersionAtLeast(version, 1, 4);
  caps.SecondaryColor = caps.SecondaryColorCore || extensions.Has("GL_EXT_secondary_color");
  caps.FogCoordCore = GlVersionAtLeast(version, 1, 4);
  caps.FogCoord = caps.FogCoordCore || extensions.Has("GL_EXT_fog_coord");
  caps.BufferObjectsCore = GlVersionAtLeast(version, 1, 5);
  caps.BufferObjects = caps.BufferObjectsCore || extensions.Has("GL_ARB_vertex_buffer_object");
  caps.GenericAttribsCore = GlVersionAtLeast(version, 2, 0);
  caps.GenericAttribs = caps.GenericAttribsCore || extensions.Has("GL_ARB_vertex_shader") ||
                        extensions.Has("GL_ARB_vertex_program");
  caps.ShaderObjectsCore = GlVersionAtLeast(version, 2, 0);
  caps.ShaderObjects = caps.ShaderObjectsCore || extensions.Has("GL_ARB_shader_objects");
  caps.IntegerAttribsCore = GlVersionAtLeast(version, 3, 0);
  caps.IntegerAttribs = caps.IntegerAttribsCore || extensions.Has("GL_EXT_gpu_shader4");
  caps.LongAttribsCore =
      GlVersionAtLeast(version, 4, 1) || extensions.Has("GL_ARB_vertex_attrib_64bit");
  caps.LongAttribs = caps.LongAttribsCore || extensions.Has("GL_EXT_vertex_attrib_64bit");
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
  } else if (version == std::pair{3, 1}) {
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
  GLint coordUnits = 1;
  if (GlVersionAtLeast(version, 2, 0)) {
    // Core and forward-compatible contexts removed it with the client arrays it limits.
    if (!core) {
      getIntegerv(GL_MAX_TEXTURE_COORDS, &coordUnits);
    }
  } else if (caps.MultiTexture) {
    getIntegerv(GL_MAX_TEXTURE_UNITS, &coordUnits);
  }
  caps.TextureCoordUnits = coordUnits > 0 ? coordUnits : 1;
  return caps;
}

} // namespace

const ReplayContextCaps& CurrentReplayContextCaps() {
  const void* context = CurrentContext();
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

} // namespace gits::opengl
