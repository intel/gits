// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx.h"
#include <string>

namespace gits {
namespace DirectX {

%for enum in enums:
std::string toStr(${enum.name} value);
%endfor

} // namespace DirectX
} // namespace gits
