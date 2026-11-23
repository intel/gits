// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "xess_fg/xefg_swapchain.h"
#include "xess_fg/xefg_swapchain_d3d12.h"
#include "xess_fg/xefg_swapchain_debug.h"

namespace gits {
namespace DirectX {

struct XefgDispatchTable {
%for function in functions:
%if is_xefg_function(function):
  decltype(${function.name})* ${function.name};
%endif
%endfor
};

} // namespace DirectX
} // namespace gits
