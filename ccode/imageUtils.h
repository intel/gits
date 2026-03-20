// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>
#include <string>

/// API-agnostic pixel format for image writing (e.g. maps from DXGI_FORMAT).
enum class Format {
  R8G8B8A8_UNORM,
  R8G8B8A8_UNORM_SRGB,
  B8G8R8X8_UNORM,
  B8G8R8A8_UNORM,
  B8G8R8A8_UNORM_SRGB,
  R10G10B10A2_UNORM,
  Unknown,
};

/// Writes pixel data to a PNG file. Converts to RGB internally; returns false
/// if format is Unknown or write fails. No D3D/DXGI dependency.
bool WriteImage(const std::string& filename,
                const uint8_t* pixelData,
                Format format,
                size_t width,
                size_t height,
                size_t rowPitch);
