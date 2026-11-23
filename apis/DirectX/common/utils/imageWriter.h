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

// Keeps the COM multithreaded apartment alive for the lifetime of the process. DirectXTex caches
// its WIC factory in a static, so without this the factory can outlive the apartment (and
// windowscodecs.dll) once the short-lived dump threads exit. Safe to call repeatedly from any
// thread; call before the first DirectXTex WIC use.
void KeepWicComAlive();

bool WriteImage(const std::filesystem::path& outputFileName,
                ImageFormat outputFormat,
                uint8_t* pixelData,
                DXGI_FORMAT pixelFormat,
                size_t width,
                size_t height,
                size_t rowPitch);

} // namespace DirectX
} // namespace gits
