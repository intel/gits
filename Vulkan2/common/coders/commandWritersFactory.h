// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandWriter.h"
#include "commandsAuto.h"

namespace gits {
namespace vulkan {

CommandWriter* CreateCommandWriter(Command* command);

} // namespace vulkan
} // namespace gits
