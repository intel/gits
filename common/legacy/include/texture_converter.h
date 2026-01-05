// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>

#include "pragmas.h"

namespace gits {
enum class texel_type {
  A8,
  R8,
  R8snorm,
  R8ui,
  R8i,
  R16,
  R16snorm,
  R16ui,
  R16i,
  R16f,
  RG8,
  RG8snorm,
  RG8ui,
  RG8i,
  B5G6R5,
  A1BGR5,
  A1RGB5,
  ABGR4,
  BGR5A1,
  R32ui,
  R32i,
  R32f,
  RG16,
  RG16snorm,
  RG16ui,
  RG16i,
  RG16f,
  BGR8,
  BGR8i,
  BGR8ui,
  BGR8snorm,
  RGB8,
  RGB8i,
  RGB8ui,
  RGB8snorm,
  RGBA8,
  RGBA8snorm,
  RGBA8ui,
  RGBA8i,
  BGRA8,
  BGRA8i,
  BGRA8snorm,
  BGRA8ui,
  ABGR8,
  ABGR8i,
  ABGR8snorm,
  ABGR8ui,
  RGB10A2,
  RGB10A2ui,
  BGR10A2,
  RG11B10f,
  B10GR11f,
  RG32ui,
  RG32i,
  RG32f,
  RGBA16,
  RGBA16snorm,
  RGBA16ui,
  RGBA16i,
  RGBA16f,
  RGBA32ui,
  RGBA32i,
  RGBA32f,
  X8D24,
  D24,
  D32fS8ui
};

const char* get_texel_format_string(texel_type val);
int get_supported_texels_count();

void convert_texture_data(texel_type input_format,
                          const std::vector<uint8_t>& input_data,
                          texel_type output_format,
                          std::vector<uint8_t>& output_data,
                          int width,
                          int height);

void normalize_texture_data(texel_type type, std::vector<uint8_t>& data, int width, int height);

std::pair<double, double> get_min_max_values(texel_type type,
                                             std::vector<uint8_t>& data,
                                             int width,
                                             int height);

} // namespace gits
