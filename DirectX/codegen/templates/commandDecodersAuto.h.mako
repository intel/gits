// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"

#include <vector>

namespace gits {
namespace DirectX {

%for function in functions:
void decode(char* src, ${function.name}Command& command);
%endfor
%for interface in interfaces:
%for function in interface.functions:
void decode(char* src, ${interface.name}${function.name}Command& command);
%endfor
%endfor
} // namespace DirectX
} // namespace gits
