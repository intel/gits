// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandRunnersAuto.h"
#include "commandsAuto.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "handleMapService.h"
#include "handleArgumentUpdaters.h"

namespace gits {
namespace vulkan {

<%
pnext_handle_structs = collect_pnext_handle_structs(structures)
structures_by_name = {s.name: s for s in structures}
%>
% for command in commands:
<%
args = generate_args(command)
define = get_define(command.platform)
dispatch_table = get_dispatch_table(command)
%>\
% if define:
#ifdef ${define}
% endif
void ${command.name}Runner::Run() {
  auto& manager = PlayerManager::Get();
<%
  input_handle_params = []
  for param in command.params:
    if param.is_handle:
      input_handle_params.append(param.name)
    elif param.is_struct_with_handles and not param.is_struct_with_output_handles:
      input_handle_params.append(param.name)
    elif param.is_struct:
      struct_def = structures_by_name.get(param.base_type)
      pnext_member = next((m for m in struct_def.members if m.name == 'pNext'), None) if struct_def else None
      has_pnext = pnext_member is not None and pnext_member.is_const
      if has_pnext and pnext_handle_structs:
        input_handle_params.append(param.name)
%>\
% if input_handle_params:

  // Handles are only registered when commands are dispatched, so in a
  // null-driver run (ExecuteCommands() == false) the map is empty; skip.
  if (manager.ExecuteCommands()) {
    % for name in input_handle_params:
    UpdateHandle(manager, command.m_${name});
    % endfor
  }
% endif

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ${'command.m_Return.Value = ' if command.return_type != 'void' else ''}manager.${dispatch_table}(${f'command.m_{command.params[0].name}.Value' if dispatch_table != 'GetGlobalDispatchTable' else ''}).${command.name}(
      % for arg in args:
      ${arg}${',' if not loop.last else ''}
      % endfor
	);


    % for param in command.params:
    % if param.is_handle_output:
    % if command.return_type != 'void':
<%
    success_condition = ' || '.join(f'command.m_Return.Value == {code}' for code in command.success_codes) if command.success_codes else 'command.m_Return.Value == VK_SUCCESS'
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
    success_condition = ' || '.join(f'command.m_Return.Value == {code}' for code in command.success_codes) if command.success_codes else 'command.m_Return.Value == VK_SUCCESS'
%>
    if (${success_condition}) {
      UpdateOutputHandle(manager, command.m_${param.name});
    }
    % else:
    UpdateOutputHandle(manager, command.m_${param.name});
    % endif
    % endif
    % endfor
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}
% if define:
#endif
% endif
% endfor
} // namespace vulkan
} // namespace gits
