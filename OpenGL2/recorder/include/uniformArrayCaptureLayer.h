// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Post layer that records a UniformArrayMetaCommand (uniformArrayMetaTypes.h) after every
// glGetUniformLocation(ARB) returning an element of a uniform array, with the array's capture-time
// size and the element's index, queried from the capture driver. It is recorded under the second
// key of the wrapper's post-hook key pair (generator_recorder.py's POST_HOOK_KEY_PAIR_COMMANDS),
// so it replays right after the glGetUniformLocation, before any use of the locations on any
// thread.
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
  void Post(glGetUniformLocationARBCommand& command) override;
};

} // namespace gits::opengl2
