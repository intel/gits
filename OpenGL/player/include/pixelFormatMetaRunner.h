// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "pixelFormatMetaTypes.h"

namespace gits::opengl {

// Hands a PixelFormatMetaCommand's attributes to HdcVirtualizationService for the recorded
// thread's next wglSetPixelFormat or wglCreatePbufferARB/EXT (WglSetPixelFormatRunner,
// WglCreatePbufferARBRunner, WglCreatePbufferEXTRunner, commandRunnersCustom.h).
class PixelFormatMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  PixelFormatMetaCommand command;
};

} // namespace gits::opengl
