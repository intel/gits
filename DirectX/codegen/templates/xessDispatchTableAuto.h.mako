// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "xess.h"
#include "xess_d3d12.h"

namespace gits {
namespace DirectX {

struct XessDispatchTable {
%for function in functions:
%if is_xess_function(function):
  decltype(${function.name})* ${function.name};
%endif
%endfor
};

} // namespace DirectX
} // namespace gits
