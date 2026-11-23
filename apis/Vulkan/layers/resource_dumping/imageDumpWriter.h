// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "enumsAuto.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace gits {
namespace vulkan {

// Converts the tightly packed texels of one aspect of a 2D slice to 8-bit RGB, dropping alpha.
// Unsigned normalized values map from 0-1 and signed normalized values from -1-1 to 0-255, and
// floating point values are clamped to 0-1. A single channel, depth and stencil are written to
// red. Depth and stencil are stretched so that the lowest value of the slice is 0 and the highest
// 255, and a slice of one value is written as it is. Returns false for a format and aspect
// without a conversion, whose texels are then written as they are.
bool ConvertToRgb8(VkFormat format,
                   VkImageAspectFlags aspect,
                   const uint8_t* texels,
                   uint32_t width,
                   uint32_t height,
                   std::vector<uint8_t>& rgb);

// Writes an RGB8 image, appending the extension of the format. Returns the written path, or an
// empty one on failure.
std::filesystem::path WriteRgb8Image(const std::filesystem::path& pathWithoutExtension,
                                     ImageFormat format,
                                     uint32_t width,
                                     uint32_t height,
                                     const std::vector<uint8_t>& rgb);

bool WriteBinary(const std::filesystem::path& path, const uint8_t* data, size_t size);

} // namespace vulkan
} // namespace gits
