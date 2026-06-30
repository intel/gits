// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "handleArgumentUpdatersAuto.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
%>\
% if pnext_handle_structs:
void CollectPNextHandleKeys(std::vector<GITSKey>& keys, const void* pNext) {
  const auto* node = reinterpret_cast<const VkBaseInStructure*>(pNext);
  while (node) {
    switch (node->sType) {
% for structure, handle_members in pnext_handle_structs:
<% define = get_define(structure.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${structure.stype_value}: {
        const auto& s = *reinterpret_cast<const ${structure.name}*>(node);
% for kind, access, length, base_type, member_name in handle_members:
% if kind == 'handle_single':
        keys.push_back(s.${access} != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access})) : 0);
% elif kind == 'handle_typed_uint64':
        keys.push_back(s.${access} != 0 ? HandleMapService::Get().GetKey(s.${access}) : 0);
% elif kind == 'handle_ptr':
        if (s.${access}) {
          keys.push_back(HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(*s.${access})));
        } else {
          keys.push_back(0);
        }
% elif kind == 'handle_array_ptr':
        if (s.${access} && s.${length} > 0) {
          for (uint32_t handleIdx = 0; handleIdx < s.${length}; ++handleIdx) {
            keys.push_back(s.${access}[handleIdx] != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access}[handleIdx])) : 0);
          }
        }
% elif kind == 'handle_fixed_array':
        for (uint32_t handleIdx = 0; handleIdx < s.${length}; ++handleIdx) {
          keys.push_back(s.${access}[handleIdx] != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access}[handleIdx])) : 0);
        }
% endif
% endfor
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
% for kind, access, length, base_type, member_name in handle_members:
% if kind == 'handle_single':
  keys.push_back(s.${access} != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access})) : 0);
% elif kind == 'handle_typed_uint64':
  keys.push_back(s.${access} != 0 ? HandleMapService::Get().GetKey(s.${access}) : 0);
% elif kind == 'handle_ptr':
  if (s.${access}) {
    keys.push_back(HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(*s.${access})));
  } else {
    keys.push_back(0);
  }
% elif kind == 'handle_array_ptr':
  if (s.${access} && s.${length} > 0) {
    for (uint32_t handleIdx = 0; handleIdx < s.${length}; ++handleIdx) {
      keys.push_back(s.${access}[handleIdx] != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access}[handleIdx])) : 0);
    }
  }
% elif kind == 'handle_fixed_array':
  for (uint32_t handleIdx = 0; handleIdx < s.${length}; ++handleIdx) {
    keys.push_back(s.${access}[handleIdx] != VK_NULL_HANDLE ? HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(s.${access}[handleIdx])) : 0);
  }
% elif kind == 'handle_struct_array_ptr':
  if (s.${access} && s.${length} > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.${length}; ++elemIdx) {
      const auto& elem = s.${access}[elemIdx];
${generate_child_handle_keys(member_name)}
    }
  }
% elif kind == 'handle_struct_ptr':
  if (s.${access}) {
    const auto& elem = *s.${access};
${generate_child_handle_keys(member_name)}
  }
% endif
% endfor
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
