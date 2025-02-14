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

namespace gits {
namespace DirectX {

%for function in functions:
unsigned getSize(const ${function.name}Command& command);
void encode(const ${function.name}Command& command, char* dest);
%endfor

%for interface in interfaces:
%for function in interface.functions:
unsigned getSize(const ${interface.name}${function.name}Command& command);
void encode(const ${interface.name}${function.name}Command& command, char* dest);
%endfor
%endfor
} // namespace DirectX
} // namespace gits
