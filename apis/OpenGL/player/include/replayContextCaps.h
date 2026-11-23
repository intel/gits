// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

namespace gits::opengl {

// What the context current on this thread supports, for player-side helpers that touch GL state
// beyond the replayed commands. They must not generate GL errors the capture did not have, so
// every query or call they make that is not in GL 1.1 is gated on one of these. A `*Core` flag
// says the core entry point exists; otherwise only the ARB/EXT one does (their enums are the
// core values).
struct ReplayContextCaps {
  bool MultiTexture{false}; // GL 1.3 / ARB_multitexture: glActiveTexture, glClientActiveTexture.
  bool MultiTextureCore{false};
  bool SecondaryColor{false}; // GL 1.4 / EXT_secondary_color.
  bool SecondaryColorCore{false};
  bool FogCoord{false}; // GL 1.4 / EXT_fog_coord.
  bool FogCoordCore{false};
  bool BufferObjects{false}; // GL 1.5 / ARB_vertex_buffer_object.
  bool BufferObjectsCore{false};
  bool GenericAttribs{false}; // GL 2.0 / ARB_vertex_shader / ARB_vertex_program.
  bool GenericAttribsCore{false};
  // GL 2.0 / ARB_shader_objects: glGetProgramiv, glGetActiveUniform, glGetUniformLocation.
  bool ShaderObjects{false};
  bool ShaderObjectsCore{false};
  bool IntegerAttribs{false}; // GL 3.0 / EXT_gpu_shader4: glVertexAttribIPointer.
  bool IntegerAttribsCore{false};
  // GL 4.1 / ARB_vertex_attrib_64bit (unsuffixed entry points) / EXT_vertex_attrib_64bit.
  bool LongAttribs{false};
  bool LongAttribsCore{false};
  bool TextureMaxLevel{false};   // GL 1.2 / SGIS_texture_lod: GL_TEXTURE_MAX_LEVEL.
  bool SamplerObjects{false};    // GL 3.3 / ARB_sampler_objects.
  bool DirectStateAccess{false}; // GL 4.5 / ARB_direct_state_access.
  bool DisplayLists{false};      // Neither a core profile nor forward-compatible: GL_LIST_INDEX.
  // Units glActiveTexture accepts: 1 without multitexture, GL_MAX_TEXTURE_UNITS before GL 2.0,
  // GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS from it.
  GLint TextureUnits{1};
  // Texture coordinate sets glClientActiveTexture accepts: 1 without multitexture,
  // GL_MAX_TEXTURE_UNITS before GL 2.0, GL_MAX_TEXTURE_COORDS from it; 1 on a core profile,
  // which has no client arrays.
  GLint TextureCoordUnits{1};
};

// Capabilities of the current context, queried on first use per context and thread. Everything
// false when no context is current.
const ReplayContextCaps& CurrentReplayContextCaps();

// Drops this thread's cached capabilities. Called after a replayed make-current, since a
// destroyed context's handle can be reused for a new context with different capabilities.
void InvalidateReplayContextCaps();

} // namespace gits::opengl
