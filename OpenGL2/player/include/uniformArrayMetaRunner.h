// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "uniformArrayMetaTypes.h"

namespace gits::opengl2 {

// Replays a UniformArrayMetaCommand: looks the same name up on the replay driver and registers
// every element location of the array with UniformLocationMapService, for the index range the
// capture-time size covers. Elements beyond the replay driver's own array size map to -1.
class UniformArrayMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  UniformArrayMetaCommand command;
};

} // namespace gits::opengl2
