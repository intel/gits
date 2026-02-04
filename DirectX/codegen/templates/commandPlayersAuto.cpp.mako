// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandPlayersAuto.h"
#include "commandsAuto.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "interfaceArgumentUpdaters.h"
#include "streams.h"

namespace gits {
namespace DirectX {

<%def name="run_body(function, interfaceName)">
  auto& manager = PlayerManager::get();
  %if is_xess_function(function) or is_xell_function(function):
  auto& ${function.api.name}DispatchTable = manager.getXessService().${get_xess_dispatch_table(function)};
  %endif
  %if interfaceName:

  updateInterface(manager, command.object_);
  %endif\

  %for param in function.params:
  %if param.is_interface and not param.is_interface_creation or param.structure_with_interfaces:
  updateInterface(manager, command.${param.name}_);
  %endif
  %if param.is_context:
  updateContext(manager, command.${param.name}_);
  %endif
  %endfor

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  %if is_xess_function(function) or is_xell_function(function):
  if (manager.executeCommands() && command.result_.value == ${get_success_return_value(function)}) {
    if (!command.skip) {
      command.result_.value = ${function.api.name}DispatchTable.${function.name}(${command_runner_call_parameters(function)};
    }
    %if is_context_creation(function):

    if (command.result_.value == ${get_success_return_value(function)}) {
      updateOutputContext(manager, command.${param.name}_);
    }
    %endif
  }
  %else:
  if (manager.executeCommands()) {
    if (!command.skip) {
      ${'command.result_.value = ' if not function.ret.is_void else ''}${'command.object_.value->' if interfaceName else ''}${function.name}(${command_runner_call_parameters(function)}
    }
    %if is_interface_creation(function):

    if (command.result_.value == S_OK) {
      %for param in function.params:
      %if param.is_interface_creation:
      updateOutputInterface(manager, command.${param.name}_);
      %endif
      %endfor
    }
    %endif
  }
  %endif

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
</%def>\
%for function in functions:
void ${function.name}Player::Run() {
  ${run_body(function, '')}\
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void ${interface.name}${function.name}Player::Run() {
  ${run_body(function, interface.name)}\
}

%endfor
%endfor
} // namespace DirectX
} // namespace gits
