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
% for structure, handle_members in pnext_handle_structs:
<% define = get_define(structure.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${structure.stype_value}: {
        auto& s = *reinterpret_cast<${structure.name}*>(node);
% for kind, access, length, base_type, member_name in handle_members:
% if kind == 'handle_single':
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          s.${access} = key ? reinterpret_cast<${base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
        }
% elif kind == 'handle_typed_uint64':
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          s.${access} = key ? HandleMapService::Get().GetHandle(key) : 0;
        }
% elif kind == 'handle_ptr':
        if (idx < keys.size()) {
          GITSKey key = keys[idx++];
          size_t dataOffset = handleData.size();
          handleData.push_back(key ? HandleMapService::Get().GetHandle(key) : 0);
          s.${access} = reinterpret_cast<${base_type}*>(&handleData[dataOffset]);
        }
% elif kind == 'handle_array_ptr':
        if (s.${access} && s.${length} > 0) {
          size_t dataOffset = handleData.size();
          handleData.resize(handleData.size() + s.${length});
          for (uint32_t handleIdx = 0; handleIdx < s.${length} && idx < keys.size(); ++handleIdx) {
            GITSKey key = keys[idx++];
            handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
          }
          s.${access} = reinterpret_cast<${base_type}*>(&handleData[dataOffset]);
        }
% elif kind == 'handle_fixed_array':
        for (uint32_t handleIdx = 0; handleIdx < s.${length} && idx < keys.size(); ++handleIdx) {
          GITSKey key = keys[idx++];
          s.${access}[handleIdx] = key ? reinterpret_cast<${base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
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
<%
def struct_needs_handle_data(handle_members):
    """Check if any handle member kind requires handleData storage."""
    for kind, access, length, base_type, member_name in handle_members:
        if kind in ('handle_ptr', 'handle_array_ptr'):
            return True
        if kind in ('handle_struct_ptr', 'handle_struct_array_ptr'):
            child_handles = member_name
            for child_kind, *_ in child_handles:
                if child_kind in ('handle_ptr', 'handle_array_ptr'):
                    return True
    return False
%>\
% for entry in collect_structs_needing_handle_updater(commands, structures):
<%
struct_name = entry['name']
structure = entry['structure']
has_pnext = entry['has_pnext']
handle_members = entry['handle_members']
define = entry['define']
needs_handle_data = struct_needs_handle_data(handle_members)
needs_pnext_resolve = has_pnext and pnext_handle_structs
%>\
% if define:
#ifdef ${define}
% endif
void ResolveHandleKeys(const std::vector<GITSKey>& keys, uint32_t& idx, std::vector<uint64_t>& handleData, ${struct_name}& s) {
% for kind, access, length, base_type, member_name in handle_members:
% if kind == 'handle_single':
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.${access} = key ? reinterpret_cast<${base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
  }
% elif kind == 'handle_typed_uint64':
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    s.${access} = key ? HandleMapService::Get().GetHandle(key) : 0;
  }
% elif kind == 'handle_ptr':
  if (idx < keys.size()) {
    GITSKey key = keys[idx++];
    size_t dataOffset = handleData.size();
    handleData.push_back(key ? HandleMapService::Get().GetHandle(key) : 0);
    s.${access} = reinterpret_cast<${base_type}*>(&handleData[dataOffset]);
  }
% elif kind == 'handle_array_ptr':
  if (s.${access} && s.${length} > 0) {
    size_t dataOffset = handleData.size();
    handleData.resize(handleData.size() + s.${length});
    for (uint32_t handleIdx = 0; handleIdx < s.${length} && idx < keys.size(); ++handleIdx) {
      GITSKey key = keys[idx++];
      handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
    }
    s.${access} = reinterpret_cast<${base_type}*>(&handleData[dataOffset]);
  }
% elif kind == 'handle_fixed_array':
  for (uint32_t handleIdx = 0; handleIdx < s.${length} && idx < keys.size(); ++handleIdx) {
    GITSKey key = keys[idx++];
    s.${access}[handleIdx] = key ? reinterpret_cast<${base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
  }
% elif kind == 'handle_struct_ptr':
<%
    ptr_member_name = access
    ptr_base_type = base_type
    child_handles = member_name
%>
  if (s.${ptr_member_name}) {
    auto& elem = const_cast<${ptr_base_type}&>(*s.${ptr_member_name});
% for child_kind, child_access, child_length, child_base_type, child_member_name in child_handles:
% if child_kind == 'handle_single':
    if (idx < keys.size()) {
      GITSKey key = keys[idx++];
      elem.${child_access} = key ? reinterpret_cast<${child_base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
    }
% elif child_kind == 'handle_array_ptr':
    if (elem.${child_access} && elem.${child_length} > 0) {
      size_t dataOffset = handleData.size();
      handleData.resize(handleData.size() + elem.${child_length});
      for (uint32_t handleIdx = 0; handleIdx < elem.${child_length} && idx < keys.size(); ++handleIdx) {
        GITSKey key = keys[idx++];
        handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
      }
      elem.${child_access} = reinterpret_cast<${child_base_type}*>(&handleData[dataOffset]);
    }
% endif
% endfor
  }
% elif kind == 'handle_struct_array_ptr':
<%
    arr_member_name = access
    arr_length = length
    arr_base_type = base_type
    child_handles = member_name
%>
  if (s.${arr_member_name} && s.${arr_length} > 0) {
    for (uint32_t elemIdx = 0; elemIdx < s.${arr_length}; ++elemIdx) {
      auto& elem = const_cast<${arr_base_type}&>(s.${arr_member_name}[elemIdx]);
% for child_kind, child_access, child_length, child_base_type, child_member_name in child_handles:
% if child_kind == 'handle_single':
      if (idx < keys.size()) {
        GITSKey key = keys[idx++];
        elem.${child_access} = key ? reinterpret_cast<${child_base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;
      }
% elif child_kind == 'handle_array_ptr':
      if (elem.${child_access} && elem.${child_length} > 0) {
        size_t dataOffset = handleData.size();
        handleData.resize(handleData.size() + elem.${child_length});
        for (uint32_t handleIdx = 0; handleIdx < elem.${child_length} && idx < keys.size(); ++handleIdx) {
          GITSKey key = keys[idx++];
          handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;
        }
        elem.${child_access} = reinterpret_cast<${child_base_type}*>(&handleData[dataOffset]);
      }
% endif
% endfor
    }
  }
% endif
% endfor
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
