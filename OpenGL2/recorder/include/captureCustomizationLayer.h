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
// does not support. The first Pre layer, and a Post layer ahead of EncoderLayer.
class CaptureCustomizationLayer : public Layer {
public:
  CaptureCustomizationLayer() : Layer("CaptureCustomization") {}

  // See WindowGeometryTrackingLayer's identical `using` declarations (member name hiding).
  using Layer::Post;
  using Layer::Pre;

  void Post(wglCreateContextAttribsARBCommand& command) override;
};

} // namespace gits::opengl2
