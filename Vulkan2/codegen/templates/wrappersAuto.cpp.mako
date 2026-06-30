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

namespace gits {
namespace vulkan {

% for command in commands:
<%
args = generate_args(command)
dispatch_table = get_dispatch_table(command)
define = get_define(command.platform)
%>\
% if define:
#ifdef ${define}
% endif
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
    
	% for param in command.params:
	% if param.is_handle:
	UpdateHandle(manager, command.m_${param.name});
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
	UpdateOutputHandle(manager, command.m_${param.name});
	% endif
	% endfor

	% if command.return_type != 'void':
	command.m_Return.Value = result;
	% endif

    for (Layer* layer : manager.GetPostLayers()) {
	  layer->Post(command);
	}
    
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
