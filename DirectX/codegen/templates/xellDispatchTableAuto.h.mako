// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "xell/xell.h"
#include "xell/xell_d3d12.h"

namespace gits {
namespace DirectX {

struct XellDispatchTable {
%for function in functions:
%if is_xell_function(function):
  decltype(${function.name})* ${function.name};
%endif
%endfor
};

} // namespace DirectX
} // namespace gits
