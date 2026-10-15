// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "pbufferPixelFormatMetaTypes.h"

namespace gits::opengl2 {

// Hands a PbufferPixelFormatMetaCommand's attributes to PbufferService for the recorded thread's
// next wglCreatePbufferARB/EXT (WglCreatePbufferARBRunner/WglCreatePbufferEXTRunner,
// commandRunnersCustom.h).
class PbufferPixelFormatMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  PbufferPixelFormatMetaCommand command;
};

} // namespace gits::opengl2
