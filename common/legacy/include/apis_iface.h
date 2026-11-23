// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   apis_iface.h
 *
 * @brief Declaration of the APIs stored in legacy stream headers.
 *
 */

#pragma once

namespace gits {
class ApisIface {
public:
  // Values are stored in stream headers; keep them unchanged.
  enum TApi {
    ApiNotSet = 0,
    OpenGL = 1,    // no longer supported, identifies old streams
    Vulkan = 2,    // no longer supported, identifies old streams
    OpenCL = 3,    // no longer supported, identifies old streams
    LevelZero = 4, // no longer supported, identifies old streams
    DirectX = 5,
  };
};
} // namespace gits
