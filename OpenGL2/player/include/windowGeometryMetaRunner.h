// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written player-side counterpart to common/layer_interface/windowGeometryMetaTypes.h's
// WindowGeometryMetaCommand/WindowGeometryMetaSerializer. Registered by OpenGL2CommandFactory::
// CreateCommand for CommandId::ID_META_WINDOW_GEOMETRY, mirroring
// MappedDataMetaRunner/UnsupportedCommandRunner's identical hand-written-Runner precedent (see
// that command's own header comment). Unlike a generated *Runner (which replays a single real
// GL/WGL call), this Runner has no real driver call of its own - it forwards straight to
// HdcVirtualizationService::SetGeometry().

#include "commandRunner.h"
#include "windowGeometryMetaTypes.h"

namespace gits::opengl2 {

class WindowGeometryMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  WindowGeometryMetaCommand command;
};

} // namespace gits::opengl2
