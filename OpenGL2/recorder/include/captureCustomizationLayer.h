// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "layerAuto.h"

namespace gits::opengl2 {

// Changes what the application sees of the driver while recording, and rejects what the recorder
// does not support. The first Pre layer, and a Post layer ahead of EncoderLayer. Changes that keep
// state are services the layer forwards to.
//
// It hides capabilities through CapabilitySuppressionService. Answered queries are still
// recorded, and their results are not stored. A skipped binary load is not recorded at all.
class CaptureCustomizationLayer : public Layer {
public:
  CaptureCustomizationLayer() : Layer("CaptureCustomization") {}

  // See WindowGeometryTrackingLayer's identical `using` declarations (member name hiding).
  using Layer::Post;
  using Layer::Pre;

  // The number of extensions left after hiding, no program binary formats, and only the SPIR-V
  // shader binary formats.
  void Pre(glGetBooleanvCommand& command) override;
  void Pre(glGetIntegervCommand& command) override;
  void Pre(glGetInteger64vCommand& command) override;
  void Pre(glGetFloatvCommand& command) override;
  void Pre(glGetDoublevCommand& command) override;
  // An index into the current context's filtered extension list selects the same extension in
  // the driver's.
  void Pre(glGetStringiCommand& command) override;
  void Post(glGetStringCommand& command) override;
  void Post(wglGetExtensionsStringARBCommand& command) override;
  void Post(wglGetExtensionsStringEXTCommand& command) override;
  // Loads in a format the driver lists, except SPIR-V shader binaries, which are portable.
  void Pre(glProgramBinaryCommand& command) override;
  void Pre(glProgramBinaryOESCommand& command) override;
  void Pre(glShaderBinaryCommand& command) override;

  void Post(wglCreateContextAttribsARBCommand& command) override;
};

} // namespace gits::opengl2
