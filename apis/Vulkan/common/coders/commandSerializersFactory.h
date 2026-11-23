// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "command.h"

namespace gits {
namespace vulkan {

stream::CommandSerializer* CreateCommandSerializer(Command* command);

} // namespace vulkan
} // namespace gits
