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
#include "interfaceArgumentUpdaters.h"
#include "streams.h"

namespace gits {
namespace DirectX {

<%def name="run_body(function, interfaceName)">
  auto& manager = PlayerManager::Get();
  %if is_xess_function(function) or is_xell_function(function) or is_xefg_function(function):
  auto& ${function.api.name}DispatchTable = manager.GetXessService().${get_xess_dispatch_table(function)};
  %endif
  %if interfaceName:

  UpdateInterface(manager, command.m_Object);
  %endif\

  %for param in function.params:
  %if param.is_interface and not param.is_interface_creation or param.structure_with_interfaces:
  UpdateInterface(manager, command.m_${param.name});
  %endif
  %if param.is_context or (param.name == 'hXeLLContext' and function.name == 'xefgSwapChainSetLatencyReduction'):
  UpdateContext(manager, command.m_${param.name});
  %endif
  %endfor

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  %if is_xess_function(function) or is_xell_function(function) or is_xefg_function(function):
  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      command.m_Result.Value = ${function.api.name}DispatchTable.${function.name}(${command_runner_call_parameters(function)}
    }
    %if is_context_creation(function):

    if (command.m_Result.Value == ${get_success_return_value(function)}) {
      UpdateOutputContext(manager, command.m_${param.name});
    }
    %elif is_interface_creation(function):

    if (command.m_Result.Value == ${get_success_return_value(function)}) {
      %for param in function.params:
      %if param.is_interface_creation:
      UpdateOutputInterface(manager, command.m_${param.name});
      %endif
      %endfor
    }
    %endif
  }
  %else:
  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      ${'command.m_Result.Value = ' if not function.ret.is_void else ''}${'command.m_Object.Value->' if interfaceName else ''}${function.name}(${command_runner_call_parameters(function)}
    }
    %if is_interface_creation(function):

    if (command.m_Result.Value == S_OK) {
      %for param in function.params:
      %if param.is_interface_creation:
      UpdateOutputInterface(manager, command.m_${param.name});
      %endif
      %endfor
    }
    %endif
  }
  %endif

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
</%def>\
%for function in functions:
void ${function.name}Runner::Run() {
  ${run_body(function, '')}\
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void ${interface.name}${function.name}Runner::Run() {
  ${run_body(function, interface.name)}\
}

%endfor
%endfor
} // namespace DirectX
} // namespace gits
