// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "wrappersAuto.h"
#include "commandsAuto.h"
#include "layerAuto.h"
#include "captureManager.h"
#include "handleArgumentUpdaters.h"

#include <mutex>

namespace gits {
namespace vulkan {

namespace {

std::mutex g_Mutex;

} // namespace

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
structures_by_name = {s.name: s for s in structures}
%>
% for command in commands:
<%
args = generate_args(command)
dispatch_table = get_dispatch_table(command)
define = get_define(command.platform)
%>\
% if define:
#ifdef ${define}
% endif
<%
# Precompute the destroy/free target (last handle param) so the UpdateHandle
# loop below can emit a lenient lookup for it.  Strict UpdateHandle is kept
# for every other handle parameter (device, parent pool, ...).
is_destroy_or_free = command.name.startswith('vkDestroy') or command.name.startswith('vkFree')
is_create_or_allocate = command.name.startswith('vkCreate') or command.name.startswith('vkAllocate')
destroy_target_name = None
if is_destroy_or_free:
	for p in reversed(command.params):
		if p.is_handle:
			destroy_target_name = p.name
			break
%>\
${command.return_type} ${command.name}Wrapper(
  % for param in command.params:
	${param.full_type}${',' if not loop.last else ''}
	% endfor
	) {
  % if command.return_type != 'void':
  ${command.return_type} result{};
  % endif

  auto& manager = CaptureManager::Get();
  if (auto recursionDepth = RecursionGuard()) {
    ${command.name}Command command(
	  GITS_GET_THREAD_ID(),
	  % for param in command.params:
	  ${param.name}${',' if not loop.last else ''}
	  % endfor
	  );

	% if is_create_or_allocate or is_destroy_or_free:
	std::lock_guard<std::mutex> lock(g_Mutex);
	% endif

	% for param in command.params:
	% if param.is_handle:
	% if param.name == destroy_target_name:
	UpdateHandleLenient(manager, command.m_${param.name});
	% else:
	UpdateHandle(manager, command.m_${param.name});
	% endif
	% elif param.is_struct_with_handles and not (param.is_pointer and not param.is_const):
	UpdateHandle(manager, command.m_${param.name});
	% elif param.is_struct and not param.is_struct_with_handles:
<%
  struct_def = structures_by_name.get(param.base_type)
  pnext_member = next((m for m in struct_def.members if m.name == 'pNext'), None) if struct_def else None
  has_pnext = pnext_member is not None and pnext_member.is_const
%>\
	% if has_pnext and pnext_handle_structs:
	UpdateHandle(manager, command.m_${param.name});
	% endif
	% endif
	% endfor

	for (Layer* layer : manager.GetPreLayers()) {
	  layer->Pre(command);
	}

	command.m_Key = manager.CreateCommandKey();
	if (!command.m_Skip) {
	  ${'result = ' if command.return_type != 'void' else ''}manager.${dispatch_table}(${f'command.m_{command.params[0].name}.Value' if dispatch_table != 'GetGlobalDispatchTable' else ''}).${command.name}(
		% for arg in args:
		${arg}${',' if not loop.last else ''}
		% endfor
      );
    }

	% for param in command.params:
	% if param.is_handle_output:
	% if command.return_type != 'void':
<%
	success_condition = ' || '.join(f'result == {code}' for code in command.success_codes) if command.success_codes else 'result == VK_SUCCESS'
%>
	if (${success_condition}) {
	  UpdateOutputHandle(manager, command.m_${param.name});
	}
	% else:
	UpdateOutputHandle(manager, command.m_${param.name});
	% endif
	% elif param.is_struct_with_output_handles and param.is_pointer and not param.is_const:
	% if command.return_type != 'void':
<%
	success_condition = ' || '.join(f'result == {code}' for code in command.success_codes) if command.success_codes else 'result == VK_SUCCESS'
%>
	if (${success_condition}) {
	  UpdateOutputHandle(manager, command.m_${param.name});
	}
	% else:
	UpdateOutputHandle(manager, command.m_${param.name});
	% endif
	% elif param.is_struct_with_handles and param.is_pointer and not param.is_const:
	% if command.return_type != 'void':
<%
	success_condition = ' || '.join(f'result == {code}' for code in command.success_codes) if command.success_codes else 'result == VK_SUCCESS'
%>
	if (${success_condition}) {
	  UpdateHandle(manager, command.m_${param.name});
	}
	% else:
	UpdateHandle(manager, command.m_${param.name});
	% endif
	% endif
	% endfor

	% if command.return_type != 'void':
	command.m_Return.Value = result;
	% endif

    for (Layer* layer : manager.GetPostLayers()) {
	  layer->Post(command);
	}

	% if command.name.startswith('vkDestroy') or command.name.startswith('vkFree'):
<%
	destroy_target = None
	for p in reversed(command.params):
		if p.is_handle:
			destroy_target = p
			break
%>\
	% if destroy_target is not None:
	// Drop the recorder-side HandleMapService mapping(s) for the destroyed
	// handle(s).  Without this, a subsequent vkCreate* call that the driver
	// happens to satisfy by recycling the same VkXxx handle would, via
	// UpdateOutputHandle's "reuse existing key" path, end up with the
	// destroyed object's GITSKey -- silently aliasing the new object onto the
	// old state-tracking entry.  See HandleMapService::RemoveHandle for the
	// full rationale.
	% if destroy_target.is_pointer:
	if (command.m_${destroy_target.name}.Value) {
	  for (uint32_t i = 0; i < command.m_${destroy_target.name}.Size; ++i) {
	    HandleMapService::Get().RemoveHandle(
	        reinterpret_cast<uint64_t>(command.m_${destroy_target.name}.Value[i]));
	  }
	}
	% else:
	HandleMapService::Get().RemoveHandle(
	    reinterpret_cast<uint64_t>(command.m_${destroy_target.name}.Value));
	% endif
	% endif
	% endif
    
  } else {
    ${'result = ' if command.return_type != 'void' else ''}manager.${dispatch_table}(${f'{command.params[0].name}' if dispatch_table != 'GetGlobalDispatchTable' else ''}).${command.name}(
		% for param in command.params:
		${param.name}${',' if not loop.last else ''}
		% endfor
		); 
  }
  % if command.return_type != 'void':
  return result;
  % endif
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
