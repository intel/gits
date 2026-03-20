// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "imageUtils.h"

#include <stb_image_write.h>

#include <vector>

namespace {

void ConvertRgba8ToRgb8(
    const uint8_t* src, size_t width, size_t height, size_t rowPitch, uint8_t* dst) {
  uint8_t* dstPtr = dst;
  for (size_t y = 0; y < height; ++y) {
    const uint8_t* srcRow = src + y * rowPitch;
    for (size_t x = 0; x < width; ++x) {
      *dstPtr++ = *srcRow++; // R
      *dstPtr++ = *srcRow++; // G
      *dstPtr++ = *srcRow++; // B
      srcRow++;              // Skip A
    }
  }
}

void ConvertBgra8ToRgb8(
    const uint8_t* src, size_t width, size_t height, size_t rowPitch, uint8_t* dst) {
  uint8_t* dstPtr = dst;
  for (size_t y = 0; y < height; ++y) {
    const uint8_t* srcRow = src + y * rowPitch;
    for (size_t x = 0; x < width; ++x) {
      uint8_t b = *srcRow++;
      uint8_t g = *srcRow++;
      uint8_t r = *srcRow++;
      srcRow++;      // Skip A
      *dstPtr++ = r; // R
      *dstPtr++ = g; // G
      *dstPtr++ = b; // B
    }
  }
}

void ConvertRgb10a2ToRgb8(
    const uint8_t* src, size_t width, size_t height, size_t rowPitch, uint8_t* dst) {
  uint8_t* dstPtr = dst;
  for (size_t y = 0; y < height; ++y) {
    const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src + y * rowPitch);
    for (size_t x = 0; x < width; ++x) {
      uint32_t pixel = srcRow[x];
      uint8_t r = static_cast<uint8_t>(((pixel >> 0) & 0x3FF) >> 2);
      uint8_t g = static_cast<uint8_t>(((pixel >> 10) & 0x3FF) >> 2);
      uint8_t b = static_cast<uint8_t>(((pixel >> 20) & 0x3FF) >> 2);
      *dstPtr++ = r;
      *dstPtr++ = g;
      *dstPtr++ = b;
    }
  }
}

} // namespace

bool WriteImage(const std::string& filename,
                const uint8_t* pixelData,
                Format format,
                size_t width,
                size_t height,
                size_t rowPitch) {
  std::vector<uint8_t> rgbData(width * height * 3);

  switch (format) {
  case Format::R8G8B8A8_UNORM:
  case Format::R8G8B8A8_UNORM_SRGB:
    ConvertRgba8ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
    break;
  case Format::B8G8R8X8_UNORM:
  case Format::B8G8R8A8_UNORM:
  case Format::B8G8R8A8_UNORM_SRGB:
    ConvertBgra8ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
    break;
  case Format::R10G10B10A2_UNORM:
    ConvertRgb10a2ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
    break;
  case Format::Unknown:
    return false;
  }

  return stbi_write_png(filename.c_str(), static_cast<int>(width), static_cast<int>(height), 3,
                        rgbData.data(), static_cast<int>(width * 3));
}
