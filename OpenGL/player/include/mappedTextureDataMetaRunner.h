// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Replays a MappedTextureDataMetaCommand: maps each texture level with glMapTexture2DINTEL,
// copies the captured rows into it at the replay driver's own stride, and unmaps it. Needs
// GL_INTEL_map_texture on the replay driver.

#include "commandRunner.h"
#include "mappedTextureDataMetaTypes.h"

namespace gits::opengl {

class MappedTextureDataMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  MappedTextureDataMetaCommand command;
};

} // namespace gits::opengl
