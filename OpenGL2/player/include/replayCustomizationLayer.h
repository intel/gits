// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits::opengl2 {

// Changes or checks how the stream replays where the replay driver differs from the recording one.
// Checks that keep state are services the layer forwards to, such as ProgramBinaryCheckService.
class ReplayCustomizationLayer : public Layer {
public:
  ReplayCustomizationLayer() : Layer("ReplayCustomization") {}

  // The replay location of each captured uniform location, and the program classic glUniform*()
  // calls set uniforms of, are kept by UniformLocationMapService. A binding is tracked only once
  // GL reports it, since GL keeps the previous one when the call fails.
  void Pre(glGetUniformLocationCommand& command) override;
  void Post(glGetUniformLocationCommand& command) override;
  void Pre(glGetUniformLocationARBCommand& command) override;
  void Post(glGetUniformLocationARBCommand& command) override;
  void Post(glBindProgramPipelineCommand& command) override;
  void Post(glBindProgramPipelineEXTCommand& command) override;
  void Post(glActiveShaderProgramCommand& command) override;
  void Post(glActiveShaderProgramEXTCommand& command) override;

  void Post(glProgramBinaryCommand& command) override;
  void Post(glProgramBinaryOESCommand& command) override;
  void Post(glLinkProgramCommand& command) override;
  void Post(glLinkProgramARBCommand& command) override;
  void Post(glDeleteProgramCommand& command) override;
  void Post(glDeleteObjectARBCommand& command) override;
  void Post(glUseProgramCommand& command) override;
  void Post(glUseProgramObjectARBCommand& command) override;
  void Post(glUseProgramStagesCommand& command) override;

  // A wait that reached the sync object in the recording waits again while replay times out.
  void Pre(glClientWaitSyncCommand& command) override;
  void Post(glClientWaitSyncCommand& command) override;
};

} // namespace gits::opengl2
