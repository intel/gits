// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include <cstdint>

namespace gits {
namespace Vulkan {

std::vector<uint32_t> getPrepareDeviceAddressesForPatchingShaderModuleSource();

std::vector<uint32_t> getPatchDeviceAddressesShaderModuleSource();

} // namespace Vulkan
} // namespace gits
