// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written class declaration; CreateCommand's body is generated (see
// codegen/templates/openGL2CommandFactoryAuto.cpp.mako and player/openGL2CommandFactoryAuto.cpp)
// - mirrors Vulkan/player/vulkanCommandFactory.h's identical split.

#include "commandFactory.h"
#include "commandRunner.h"

namespace gits::opengl2 {

class OpenGL2CommandFactory : public stream::CommandFactory {
public:
  stream::CommandRunner* CreateCommand(unsigned id) override;
};

} // namespace gits::opengl2
