// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imageDumpWriter.h"

#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <optional>

namespace gits {
namespace vulkan {

namespace {

enum class Component {
  UNORM8,
  SNORM8,
  UNORM16,
  SNORM16,
  FLOAT16,
  FLOAT32,
};

struct PlainFormat {
  uint32_t Channels{};
  Component Kind{};
  bool Bgr{};
};

std::optional<PlainFormat> GetPlainFormat(VkFormat format) {
  switch (format) {
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SRGB:
    return PlainFormat{1, Component::UNORM8};
  case VK_FORMAT_R8_SNORM:
    return PlainFormat{1, Component::SNORM8};
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SRGB:
    return PlainFormat{2, Component::UNORM8};
  case VK_FORMAT_R8G8_SNORM:
    return PlainFormat{2, Component::SNORM8};
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SRGB:
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    return PlainFormat{4, Component::UNORM8};
  case VK_FORMAT_R8G8B8A8_SNORM:
    return PlainFormat{4, Component::SNORM8};
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
    return PlainFormat{4, Component::UNORM8, true};
  case VK_FORMAT_B8G8R8A8_SNORM:
    return PlainFormat{4, Component::SNORM8, true};
  case VK_FORMAT_R16_UNORM:
    return PlainFormat{1, Component::UNORM16};
  case VK_FORMAT_R16_SNORM:
    return PlainFormat{1, Component::SNORM16};
  case VK_FORMAT_R16_SFLOAT:
    return PlainFormat{1, Component::FLOAT16};
  case VK_FORMAT_R16G16_UNORM:
    return PlainFormat{2, Component::UNORM16};
  case VK_FORMAT_R16G16_SNORM:
    return PlainFormat{2, Component::SNORM16};
  case VK_FORMAT_R16G16_SFLOAT:
    return PlainFormat{2, Component::FLOAT16};
  case VK_FORMAT_R16G16B16A16_UNORM:
    return PlainFormat{4, Component::UNORM16};
  case VK_FORMAT_R16G16B16A16_SNORM:
    return PlainFormat{4, Component::SNORM16};
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return PlainFormat{4, Component::FLOAT16};
  case VK_FORMAT_R32_SFLOAT:
    return PlainFormat{1, Component::FLOAT32};
  case VK_FORMAT_R32G32_SFLOAT:
    return PlainFormat{2, Component::FLOAT32};
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return PlainFormat{4, Component::FLOAT32};
  default:
    return std::nullopt;
  }
}

uint32_t ComponentSize(Component kind) {
  switch (kind) {
  case Component::UNORM8:
  case Component::SNORM8:
    return 1;
  case Component::UNORM16:
  case Component::SNORM16:
  case Component::FLOAT16:
    return 2;
  case Component::FLOAT32:
    return 4;
  }
  return 0;
}

uint8_t ToByte(float value) {
  if (!(value > 0.0f)) {
    return 0;
  }
  if (value >= 1.0f) {
    return 255;
  }
  return static_cast<uint8_t>(value * 255.0f + 0.5f);
}

float SignedToUnit(float value) {
  return (std::max(value, -1.0f) + 1.0f) * 0.5f;
}

float HalfToFloat(uint16_t half) {
  const uint32_t mantissa = half & 0x3ff;
  const int exponent = (half >> 10) & 0x1f;
  float value{};
  if (exponent == 0) {
    value = std::ldexp(static_cast<float>(mantissa), -24);
  } else if (exponent == 31) {
    value = mantissa ? NAN : INFINITY;
  } else {
    value = std::ldexp(static_cast<float>(mantissa | 0x400), exponent - 25);
  }
  return (half & 0x8000) ? -value : value;
}

// Unsigned floats of B10G11R11_UFLOAT_PACK32, with a 5 bit exponent.
float UnsignedSmallFloatToFloat(uint32_t bits, int mantissaBits) {
  const uint32_t mantissa = bits & ((1u << mantissaBits) - 1);
  const int exponent = (bits >> mantissaBits) & 0x1f;
  if (exponent == 0) {
    return std::ldexp(static_cast<float>(mantissa), -14 - mantissaBits);
  }
  if (exponent == 31) {
    return mantissa ? NAN : INFINITY;
  }
  return std::ldexp(static_cast<float>(mantissa | (1u << mantissaBits)),
                    exponent - 15 - mantissaBits);
}

uint8_t ComponentToByte(Component kind, const uint8_t* data) {
  switch (kind) {
  case Component::UNORM8:
    return data[0];
  case Component::SNORM8:
    return ToByte(SignedToUnit(static_cast<int8_t>(data[0]) / 127.0f));
  case Component::UNORM16: {
    uint16_t value{};
    std::memcpy(&value, data, sizeof(value));
    return static_cast<uint8_t>(value >> 8);
  }
  case Component::SNORM16: {
    int16_t value{};
    std::memcpy(&value, data, sizeof(value));
    return ToByte(SignedToUnit(value / 32767.0f));
  }
  case Component::FLOAT16: {
    uint16_t value{};
    std::memcpy(&value, data, sizeof(value));
    return ToByte(HalfToFloat(value));
  }
  case Component::FLOAT32: {
    float value{};
    std::memcpy(&value, data, sizeof(value));
    return ToByte(value);
  }
  }
  return 0;
}

uint32_t ReadPacked32(const uint8_t* data) {
  uint32_t value{};
  std::memcpy(&value, data, sizeof(value));
  return value;
}

bool IsConvertedPacked32(VkFormat format) {
  return format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ||
         format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
         format == VK_FORMAT_B10G11R11_UFLOAT_PACK32;
}

bool IsConvertedDepth(VkFormat format) {
  switch (format) {
  case VK_FORMAT_D16_UNORM:
  case VK_FORMAT_D16_UNORM_S8_UINT:
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D24_UNORM_S8_UINT:
  case VK_FORMAT_D32_SFLOAT:
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return true;
  default:
    return false;
  }
}

bool CanConvertToRgb8(VkFormat format, VkImageAspectFlags aspect) {
  switch (aspect) {
  case VK_IMAGE_ASPECT_DEPTH_BIT:
    return IsConvertedDepth(format);
  case VK_IMAGE_ASPECT_STENCIL_BIT:
    return true;
  case VK_IMAGE_ASPECT_COLOR_BIT:
    return IsConvertedPacked32(format) || GetPlainFormat(format).has_value();
  default:
    return false;
  }
}

bool ConvertPacked32(VkFormat format,
                     const uint8_t* texels,
                     size_t texelCount,
                     std::vector<uint8_t>& rgb) {
  if (!IsConvertedPacked32(format)) {
    return false;
  }
  for (size_t i = 0; i < texelCount; ++i) {
    const uint32_t value = ReadPacked32(texels + i * 4);
    uint8_t* out = &rgb[i * 3];
    if (format == VK_FORMAT_B10G11R11_UFLOAT_PACK32) {
      out[0] = ToByte(UnsignedSmallFloatToFloat(value & 0x7ff, 6));
      out[1] = ToByte(UnsignedSmallFloatToFloat((value >> 11) & 0x7ff, 6));
      out[2] = ToByte(UnsignedSmallFloatToFloat((value >> 22) & 0x3ff, 5));
      continue;
    }
    const uint8_t low = static_cast<uint8_t>((value & 0x3ff) >> 2);
    const uint8_t middle = static_cast<uint8_t>(((value >> 10) & 0x3ff) >> 2);
    const uint8_t high = static_cast<uint8_t>(((value >> 20) & 0x3ff) >> 2);
    const bool rgbOrder = format == VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    out[0] = rgbOrder ? low : high;
    out[1] = middle;
    out[2] = rgbOrder ? high : low;
  }
  return true;
}

void ConvertStencil(const uint8_t* texels, size_t texelCount, std::vector<uint8_t>& rgb) {
  uint8_t lowest = 255;
  uint8_t highest = 0;
  for (size_t i = 0; i < texelCount; ++i) {
    lowest = std::min(lowest, texels[i]);
    highest = std::max(highest, texels[i]);
  }
  const bool stretched = lowest < highest;
  for (size_t i = 0; i < texelCount; ++i) {
    rgb[i * 3] =
        stretched ? ToByte(static_cast<float>(texels[i] - lowest) / (highest - lowest)) : texels[i];
  }
}

bool ConvertDepth(VkFormat format,
                  const uint8_t* texels,
                  size_t texelCount,
                  std::vector<uint8_t>& rgb) {
  std::vector<float> depths(texelCount);
  float lowest = INFINITY;
  float highest = -INFINITY;
  for (size_t i = 0; i < texelCount; ++i) {
    float depth{};
    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D16_UNORM_S8_UINT: {
      uint16_t value{};
      std::memcpy(&value, texels + i * 2, sizeof(value));
      depth = value / 65535.0f;
      break;
    }
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D24_UNORM_S8_UINT:
      depth = (ReadPacked32(texels + i * 4) & 0xffffff) / 16777215.0f;
      break;
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      std::memcpy(&depth, texels + i * 4, sizeof(depth));
      break;
    default:
      return false;
    }
    depths[i] = depth;
    if (std::isfinite(depth)) {
      lowest = std::min(lowest, depth);
      highest = std::max(highest, depth);
    }
  }
  const bool stretched = lowest < highest;
  // The range of finite floats can exceed the largest float.
  const double range = static_cast<double>(highest) - lowest;
  for (size_t i = 0; i < texelCount; ++i) {
    rgb[i * 3] =
        ToByte(stretched ? static_cast<float>((depths[i] - static_cast<double>(lowest)) / range)
                         : depths[i]);
  }
  return true;
}

} // namespace

bool ConvertToRgb8(VkFormat format,
                   VkImageAspectFlags aspect,
                   const uint8_t* texels,
                   uint32_t width,
                   uint32_t height,
                   std::vector<uint8_t>& rgb) {
  if (!CanConvertToRgb8(format, aspect)) {
    return false;
  }
  const size_t texelCount = static_cast<size_t>(width) * height;
  rgb.assign(texelCount * 3, 0);

  if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
    return ConvertDepth(format, texels, texelCount, rgb);
  }
  if (aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
    ConvertStencil(texels, texelCount, rgb);
    return true;
  }
  if (aspect != VK_IMAGE_ASPECT_COLOR_BIT) {
    return false;
  }
  if (ConvertPacked32(format, texels, texelCount, rgb)) {
    return true;
  }

  const std::optional<PlainFormat> plain = GetPlainFormat(format);
  if (!plain) {
    return false;
  }
  const uint32_t componentSize = ComponentSize(plain->Kind);
  const uint32_t texelSize = componentSize * plain->Channels;
  for (size_t i = 0; i < texelCount; ++i) {
    const uint8_t* in = texels + i * texelSize;
    uint8_t channels[3]{};
    for (uint32_t c = 0; c < std::min(plain->Channels, 3u); ++c) {
      channels[c] = ComponentToByte(plain->Kind, in + c * componentSize);
    }
    uint8_t* out = &rgb[i * 3];
    out[0] = plain->Bgr ? channels[2] : channels[0];
    out[1] = channels[1];
    out[2] = plain->Bgr ? channels[0] : channels[2];
  }
  return true;
}

std::filesystem::path WriteRgb8Image(const std::filesystem::path& pathWithoutExtension,
                                     ImageFormat format,
                                     uint32_t width,
                                     uint32_t height,
                                     const std::vector<uint8_t>& rgb) {
  constexpr int channels = 3;
  std::filesystem::path path = pathWithoutExtension;
  // The image is encoded in memory, as the stb file functions do not report write errors.
  std::vector<uint8_t> encoded;
  const auto append = [](void* context, void* data, int size) {
    auto& bytes = *static_cast<std::vector<uint8_t>*>(context);
    const auto* begin = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), begin, begin + size);
  };
  int ok = 0;
  if (format == ImageFormat::JPEG) {
    constexpr int jpegQuality = 95;
    path += ".jpg";
    ok = stbi_write_jpg_to_func(append, &encoded, static_cast<int>(width), static_cast<int>(height),
                                channels, rgb.data(), jpegQuality);
  } else {
    path += ".png";
    ok = stbi_write_png_to_func(append, &encoded, static_cast<int>(width), static_cast<int>(height),
                                channels, rgb.data(), static_cast<int>(width * channels));
  }
  if (!ok || !WriteBinary(path, encoded.data(), encoded.size())) {
    return {};
  }
  return path;
}

bool WriteBinary(const std::filesystem::path& path, const uint8_t* data, size_t size) {
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
  // Closing flushes the buffered bytes, which may fail as well.
  file.close();
  return !file.fail();
}

} // namespace vulkan
} // namespace gits
