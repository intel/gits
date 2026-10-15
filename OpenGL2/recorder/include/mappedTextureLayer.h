// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Pre layer feeding MappedTextureService (mappedTextureService.h) the swaps that capture
// still-mapped textures and the deletions that end their mappings. A swap's capture is recorded
// under the key its wrapper reserved ahead of the swap's own (generator_recorder.py's
// PRE_SWAP_TEXTURE_KEY_COMMANDS), so it replays before that swap presents the frame.
#include "commandsAuto.h"
#include "layerAuto.h"

namespace gits::opengl2 {

class MappedTextureLayer : public Layer {
public:
  MappedTextureLayer() : Layer("MappedTextureLayer") {}

  // See WindowGeometryTrackingLayer's identical `using` declarations (member name hiding).
  using Layer::Post;
  using Layer::Pre;

  void Pre(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
  void Pre(glDeleteTexturesCommand& command) override;
  void Pre(glDeleteTexturesEXTCommand& command) override;
};

} // namespace gits::opengl2
