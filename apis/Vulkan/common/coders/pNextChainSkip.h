// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

namespace gits {
namespace vulkan {

bool ShouldSkipPNext(VkStructureType sType);
void LogSkippedPNext(const char* funcName, VkStructureType sType);

} // namespace vulkan
} // namespace gits
