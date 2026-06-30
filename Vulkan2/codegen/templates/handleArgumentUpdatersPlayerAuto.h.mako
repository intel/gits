${header}

#pragma once

#include "playerManager.h"
#include "handleMapService.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
%>\
% if pnext_handle_structs:
void ResolvePNextHandleKeys(const std::vector<GITSKey>& keys, uint32_t& idx, std::vector<uint64_t>& handleData, const void* pNext);
% endif
% for entry in collect_structs_needing_handle_updater(commands, structures):
<%
struct_name = entry['name']
define = entry['define']
%>\
% if define:
#ifdef ${define}
% endif
void ResolveHandleKeys(const std::vector<GITSKey>& keys, uint32_t& idx, std::vector<uint64_t>& handleData, ${struct_name}& s);
void UpdateHandle(PlayerManager& manager, PointerArgument<${struct_name}>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<${struct_name}>& arg);
% if define:
#endif
% endif
% endfor
} // namespace vulkan
} // namespace gits
