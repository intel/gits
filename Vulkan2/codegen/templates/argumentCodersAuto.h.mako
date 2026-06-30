// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader2.h"
#include "argumentCoders.h"

namespace gits {
namespace vulkan {

% for structure in structures:
<%
define = get_define(structure.platform)
needs_coder = struct_needs_coder(structure, structures)
%>\
% if needs_coder:
% if define:
#ifdef ${define}
% endif
// ${structure.name}
uint32_t GetSize(const ${structure.name}* src, uint32_t count);
void Encode(const ${structure.name}* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const ${structure.name}* dst, uint32_t count, char* src, uint32_t& offset);

// PointerArgument / ArrayArgument overloads for ${structure.name}
uint32_t GetSize(const PointerArgument<${structure.name}>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<${structure.name}>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<${structure.name}>& arg);

uint32_t GetSize(const ArrayArgument<${structure.name}>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<${structure.name}>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<${structure.name}>& arg);
% if define:
#endif
% endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
