// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanInternalShaderModules.h
*
* @brief Declaration of SPIR-V data for custom shader modules.
*
*/

#pragma once

#include <vector>
#include "vk_platform.h"

namespace gits {
namespace Vulkan {

std::vector<uint32_t> getPrepareDeviceAddressesForPatchingShaderModuleSource();

std::vector<uint32_t> getPatchDeviceAddressesShaderModuleSource();

} // namespace Vulkan
} // namespace gits
