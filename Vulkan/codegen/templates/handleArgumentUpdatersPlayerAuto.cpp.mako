// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "handleArgumentUpdatersPlayerAuto.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
%>\
% if pnext_handle_structs:
void ResolvePNextHandleKeys(const std::vector<GITSKey>& keys, uint32_t& idx, std::vector<uint64_t>& handleData, const void* pNext) {
  auto* node = reinterpret_cast<VkBaseOutStructure*>(const_cast<void*>(pNext));
  while (node) {
    switch (node->sType) {
% for structure, handle_members in collect_pnext_handle_structs(structures):
<% define = get_define(structure.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${structure.stype_value}: {
        auto& s = *reinterpret_cast<${structure.name}*>(node);
${generate_child_handle_resolve(handle_members, 's', 0, 8)}
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
needs_pnext_resolve = has_pnext and pnext_handle_structs
# handleData is written to (and its &handleData[...] pointers must survive) only when a pointer/
# array handle is resolved, or a nested pNext chain is walked. When the ONLY reason is the struct's
# own top-level pNext chain, keep the original live guard: reserve only if a pNext is actually
# present at replay time. The reservation size is an upper bound (each key yields at most one slot).
needs_handle_data = entries_need_handle_data(handle_members)
%>\
% if define:
#ifdef ${define}
% endif
void ResolveHandleKeys(const std::vector<GITSKey>& keys, uint32_t& idx, std::vector<uint64_t>& handleData, ${struct_name}& s) {
${generate_child_handle_resolve(handle_members, 's', 0)}
}

void UpdateHandle(PlayerManager& manager, PointerArgument<${struct_name}>& arg) {
  if (!arg.Value || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
% if needs_handle_data:
  arg.HandleData.reserve(arg.HandleKeys.size());
% elif needs_pnext_resolve:
  if (arg.Value->pNext) {
    arg.HandleData.reserve(arg.HandleKeys.size());
  }
% endif
  ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, *arg.Value);
% if needs_pnext_resolve:
  ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value->pNext);
% endif
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<${struct_name}>& arg) {
  if (!arg.Value || arg.Size == 0 || arg.HandleKeys.empty()) {
    return;
  }
  uint32_t idx = 0;
% if needs_handle_data:
  arg.HandleData.reserve(arg.HandleKeys.size());
% elif needs_pnext_resolve:
  for (uint32_t i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].pNext) {
      arg.HandleData.reserve(arg.HandleKeys.size());
      break;
    }
  }
% endif
  for (uint32_t i = 0; i < arg.Size; ++i) {
    ResolveHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i]);
% if needs_pnext_resolve:
    ResolvePNextHandleKeys(arg.HandleKeys, idx, arg.HandleData, arg.Value[i].pNext);
% endif
  }
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
