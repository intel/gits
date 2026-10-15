// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "clientArraysMetaTypes.h"
#include "commandRunner.h"

namespace gits::opengl2 {

// Replays a ClientArraysMetaCommand: keeps its new blocks in player-owned memory (dropping the
// released ones) and re-specifies each array's gl*Pointer against its block, with no
// GL_ARRAY_BUFFER bound, so the draw that follows reads replay-process memory instead of the
// recording process's addresses. Blocks outlive the command: later commands reference them.
class ClientArraysMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  ClientArraysMetaCommand command;
};

} // namespace gits::opengl2
