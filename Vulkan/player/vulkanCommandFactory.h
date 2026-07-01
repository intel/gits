// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "commandFactory.h"

namespace gits {
namespace vulkan {

class VulkanCommandFactory : public stream::CommandFactory {
public:
  stream::CommandRunner* CreateCommand(unsigned id) override;
};

} // namespace vulkan
} // namespace gits
