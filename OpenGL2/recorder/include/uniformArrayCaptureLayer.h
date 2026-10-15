// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Post layer that records a UniformArrayMetaCommand (uniformArrayMetaTypes.h) after every
// glGetUniformLocation returning an element of a uniform array, with the array's capture-time
// size and the element's index, queried from the capture driver. Its key is reserved after the
// real call, so it replays after the glGetUniformLocation and before any later use of the
// locations on this thread.
#include "commandsAuto.h"
#include "layerAuto.h"

namespace gits::opengl2 {

class UniformArrayCaptureLayer : public Layer {
public:
  UniformArrayCaptureLayer() : Layer("UniformArrayCaptureLayer") {}

  // See WindowGeometryTrackingLayer's identical `using` declarations (member name hiding).
  using Layer::Post;
  using Layer::Pre;

  void Post(glGetUniformLocationCommand& command) override;
};

} // namespace gits::opengl2
