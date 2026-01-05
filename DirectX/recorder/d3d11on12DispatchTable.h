// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d11on12.h>

namespace gits {
namespace DirectX {

struct D3D11On12DispatchTable {
  PFN_D3D11ON12_CREATE_DEVICE D3D11On12CreateDevice;
};

} // namespace DirectX
} // namespace gits
