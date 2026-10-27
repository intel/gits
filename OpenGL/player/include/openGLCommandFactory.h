// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written class declaration; CreateCommand's body is generated (see
// codegen/templates/openGLCommandFactoryAuto.cpp.mako and player/openGLCommandFactoryAuto.cpp)
// - mirrors Vulkan/player/vulkanCommandFactory.h's identical split.

#include "commandFactory.h"
#include "commandRunner.h"

namespace gits::opengl {

class OpenGLCommandFactory : public stream::CommandFactory {
public:
  stream::CommandRunner* CreateCommand(unsigned id) override;
};

} // namespace gits::opengl
