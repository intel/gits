// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "handleArgumentUpdatersAuto.h"
#include "pNextChainSkip.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
%>\
% if pnext_handle_structs:
void CollectPNextHandleKeys(std::vector<GITSKey>& keys, const void* pNext) {
  const auto* node = reinterpret_cast<const VkBaseInStructure*>(pNext);
  while (node) {
    if (ShouldSkipPNext(node->sType)) {
      node = node->pNext;
      continue;
    }
    switch (node->sType) {
% for structure, handle_members in collect_pnext_handle_structs(structures):
<% define = get_define(structure.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${structure.stype_value}: {
        const auto& s = *reinterpret_cast<const ${structure.name}*>(node);
${generate_child_handle_keys(handle_members, 's', 0, 8)}
        break;
      }
% if define:
#endif
% endif
% endfor
      default:
        break;
    }
    node = node->pNext;
  }
}

% endif
% for entry in collect_structs_needing_handle_updater(commands, structures):
<%
struct_name = entry['name']
structure = entry['structure']
has_pnext = entry['has_pnext']
handle_members = entry['handle_members']
define = entry['define']
%>\
% if define:
#ifdef ${define}
% endif
void CollectHandleKeys(std::vector<GITSKey>& keys, const ${struct_name}& s) {
${generate_child_handle_keys(handle_members, 's', 0)}
}

void UpdateHandle(CaptureManager& manager, PointerArgument<${struct_name}>& arg) {
  if (!arg.Value) {
    return;
  }
  CollectHandleKeys(arg.HandleKeys, *arg.Value);
% if has_pnext and pnext_handle_structs:
  CollectPNextHandleKeys(arg.HandleKeys, arg.Value->pNext);
% endif
}

void UpdateHandle(CaptureManager& manager, ArrayArgument<${struct_name}>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    CollectHandleKeys(arg.HandleKeys, arg.Value[i]);
% if has_pnext and pnext_handle_structs:
    CollectPNextHandleKeys(arg.HandleKeys, arg.Value[i].pNext);
% endif
  }
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
