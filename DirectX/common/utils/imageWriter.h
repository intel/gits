// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "enumsAuto.h"

#include <filesystem>

namespace gits {
namespace DirectX {

bool WriteImage(const std::filesystem::path& outputFileName,
                ImageFormat outputFormat,
                uint8_t* pixelData,
                DXGI_FORMAT pixelFormat,
                size_t width,
                size_t height,
                size_t rowPitch);

} // namespace DirectX
} // namespace gits
