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
#include "argumentCodersCustom.h"

namespace gits {
namespace vulkan {

<%
union_selector_types = get_union_selector_types(structures, unions)
%>\
% for union in unions_needing_coder(unions, structures):
<%
udefine = get_define(union.platform)
seltype = union_selector_types.get(union.name, 'uint32_t')
%>\
% if udefine:
#ifdef ${udefine}
% endif
// ${union.name} (selected by ${seltype})
uint32_t GetSize(const ${union.name}* src, uint32_t count, ${seltype} selector);
void Encode(const ${union.name}* src, uint32_t count, char* dst, uint32_t& offset, ${seltype} selector);
void Decode(const ${union.name}* dst, uint32_t count, char* src, uint32_t& offset, ${seltype} selector);
% if udefine:
#endif
% endif
% endfor

% for structure in structures:
<%
define = get_define(structure.platform)
needs_coder = struct_needs_coder(structure, structures, unions)
%>\
% if needs_coder and structure.name not in custom_handle_structs:
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
