// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imageWriter.h"
#include "log.h"
#include "configurator.h"

#include <DirectXTex.h>
#include <wincodec.h>
#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace gits {
namespace DirectX {
static void convertRgba8ToRgb8(
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

static void convertBgra8ToRgb8(
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

static void covertRgb10a2ToRgb8(
    const uint8_t* src, size_t width, size_t height, size_t rowPitch, uint8_t* dst) {
  uint8_t* dstPtr = dst;
  for (size_t y = 0; y < height; ++y) {
    const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src + y * rowPitch);
    for (size_t x = 0; x < width; ++x) {
      uint32_t pixel = srcRow[x];
      uint8_t r =
          static_cast<uint8_t>(((pixel >> 0) & 0x3FF) >> 2); // R from bits 9-0, truncate to 8-bit
      uint8_t g = static_cast<uint8_t>(((pixel >> 10) & 0x3FF) >>
                                       2); // G from bits 19-10, truncate to 8-bit
      uint8_t b = static_cast<uint8_t>(((pixel >> 20) & 0x3FF) >>
                                       2); // B from bits 29-20, truncate to 8-bit
      *dstPtr++ = r;
      *dstPtr++ = g;
      *dstPtr++ = b;
    }
  }
}

bool writeImage(const std::filesystem::path& outputFileName,
                ImageFormat outputFormat,
                uint8_t* pixelData,
                DXGI_FORMAT pixelFormat,
                size_t width,
                size_t height,
                size_t rowPitch) {
  bool useStbImage =
      Configurator::IsPlayer() && Configurator::Get().directx.player.portability.useStbImage;
  auto outputFile = outputFileName;
  if (outputFormat == ImageFormat::PNG) {
    outputFile += ".png";
  } else {
    outputFile += ".jpg";
  }

  if (useStbImage) {
    // Use STB Image to dump textures (for portability)

    // Convert pixel data to RGB8
    std::vector<uint8_t> rgbData(width * height * 3);
    switch (pixelFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      convertRgba8ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
      break;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      convertBgra8ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
      break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
      covertRgb10a2ToRgb8(pixelData, width, height, rowPitch, rgbData.data());
      break;
    }

    // Write the image using STB Image
    int result = 0;
    if (outputFormat == ImageFormat::PNG) {
      result =
          stbi_write_png(outputFile.string().c_str(), width, height, 3, rgbData.data(), width * 3);
    } else if (outputFormat == ImageFormat::JPEG) {
      result = stbi_write_jpg(outputFile.string().c_str(), width, height, 3, rgbData.data(), 90);
    }
    if (!result) {
      LOG_ERROR << "Dumping " + outputFile.string() + " failed: stb_image write error";
      return false;
    }
  } else {
    // Use DirectX Tex SDK to save images with WIC
    ::DirectX::Image image{};
    image.width = width;
    image.height = height;
    image.format = pixelFormat;
    image.rowPitch = rowPitch;
    image.slicePitch = rowPitch * height;
    image.pixels = pixelData;

    auto codec = ::DirectX::WIC_CODEC_PNG;
    auto* pixelFormat = &GUID_WICPixelFormat48bppRGB;
    if (outputFormat == ImageFormat::JPEG) {
      codec = ::DirectX::WIC_CODEC_JPEG;
      pixelFormat = nullptr;
    }

    HRESULT hr{0};
    auto saveToWICFile = [&]() {
      hr = SaveToWICFile(image, ::DirectX::WIC_FLAGS_FORCE_SRGB, GetWICCodec(codec),
                         outputFile.wstring().c_str(), pixelFormat);
    };
    saveToWICFile();
    static thread_local bool initialized = false;
    if (!initialized && hr != S_OK) {
      CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      saveToWICFile();
      initialized = true;
    }
    if (hr != S_OK) {
      if (hr == 0x80070032) {
        LOG_ERROR << "Dumping " + outputFile.string() + " failed: format not supported";
      } else {
        LOG_ERROR << "Dumping " + outputFile.string() + " failed: 0x" << std::hex << hr << std::dec;
      }
      return false;
    }
  }
  return true;
}

} // namespace DirectX
} // namespace gits
