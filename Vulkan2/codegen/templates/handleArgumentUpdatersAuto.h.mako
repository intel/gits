// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "captureManager.h"
#include "handleMapService.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
%>\
% if pnext_handle_structs:
void CollectPNextHandleKeys(std::vector<GITSKey>& keys, const void* pNext);
% endif
% for entry in collect_structs_needing_handle_updater(commands, structures):
<%
struct_name = entry['name']
define = entry['define']
%>
% if define:
#ifdef ${define}
% endif
void CollectHandleKeys(std::vector<GITSKey>& keys, const ${struct_name}& s);
void UpdateHandle(CaptureManager& manager, PointerArgument<${struct_name}>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<${struct_name}>& arg);
% if define:
#endif
% endif
% endfor
} // namespace vulkan
} // namespace gits
