// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written player-side counterpart to common/layer_interface/unsupportedCommandTypes.h's
// UnsupportedCommand/UnsupportedCommandSerializer (recorder-only; write-only). Registered by
// OpenGL2CommandFactory::CreateCommand for CommandId::ID_META_UNSUPPORTED_COMMAND - see
// modern-opengl-backend-architecture.md, "Log a degraded-fidelity warning for unsupported
// commands during replay".

#include "commandRunner.h"

#include <string>

namespace gits::opengl2 {

class UnsupportedCommandRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override;

private:
  std::string m_Name;
};

} // namespace gits::opengl2
