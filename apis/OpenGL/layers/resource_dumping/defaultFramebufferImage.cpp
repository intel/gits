// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "defaultFramebufferImage.h"

#include "configurator.h"
#include "log.h"

#include <stb_image_write.h>

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace gits::opengl {

// GL rows start at the bottom; image rows at the top.
void WriteScreenshot(const std::filesystem::path& directory,
                     unsigned frame,
                     int width,
                     int height,
                     std::vector<uint8_t>& pixels) {
  const size_t stride = static_cast<size_t>(width) * 4;
  std::vector<uint8_t> row(stride);
  for (int y = 0; y < height / 2; ++y) {
    uint8_t* top = pixels.data() + y * stride;
    uint8_t* bottom = pixels.data() + (height - 1 - y) * stride;
    std::memcpy(row.data(), top, stride);
    std::memcpy(top, bottom, stride);
    std::memcpy(bottom, row.data(), stride);
  }
  for (size_t i = 3; i < pixels.size(); i += 4) {
    pixels[i] = 0xFF;
  }

  std::ostringstream name;
  name << "frame" << std::setw(8) << std::setfill('0') << frame;
  int saved = 0;
  std::string fileName = (directory / name.str()).string();
  if (Configurator::Get().common.shared.screenshots.format == ImageFormat::JPEG) {
    fileName += ".jpg";
    constexpr int jpegQuality = 95;
    saved = stbi_write_jpg(fileName.c_str(), width, height, 4, pixels.data(), jpegQuality);
  } else {
    fileName += ".png";
    saved =
        stbi_write_png(fileName.c_str(), width, height, 4, pixels.data(), static_cast<int>(stride));
  }
  if (!saved) {
    LOG_ERROR << "Screenshots: failed to write " << fileName;
  }
}

} // namespace gits::opengl
