// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written player-side counterpart to common/layer_interface/mappedDataMetaTypes.h's
// MappedDataMetaCommand/MappedDataMetaSerializer. Registered by OpenGL2CommandFactory::
// CreateCommand for CommandId::ID_META_MAPPED_DATA - see
// docs/development/opengl2-buffer-map-tracking-plan.md's Decision 6. Unlike every generated
// *Runner (which replays a single real GL call whose parameters are already the command's own
// fields), this Runner performs the whole map/write/unmap sequence itself, directly against the
// real driver - glMapBufferRange/glUnmapBuffer are not part of the normal enabled-command
// replay path at all under Decision 3.

#include "commandRunner.h"
#include "mappedDataMetaTypes.h"

namespace gits::opengl2 {

class MappedDataMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  MappedDataMetaCommand command;
};

} // namespace gits::opengl2
