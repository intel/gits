// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "wrappersAuto.h"

#include "captureManager.h"
#include "interfaceArgumentUpdaters.h"
#include "commandsAuto.h"
#include "layerAuto.h"

#include <processthreadsapi.h>

namespace gits {
namespace DirectX {

%for function in functions:
<%
params = generate_params(function)
args = generate_args(function)
dispatch_table = get_dispatch_table(function)
update_created = wrappers_update_created(function)
xess_d3d12_init = is_xess_d3d12_init(function)
args_simple = []
for param in function.params:
    args_simple.append(param.name)
%>
${generate_return(function)} ${function.name}Wrapper(${'' if params else ') {'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]}) {
    %endif
  %if not function.ret.is_void:
  ${generate_return(function)} result{};
  %endif

  auto& manager = CaptureManager::get();
  %if 'xess' in function.name:
  if (auto atTopOfStack = AtTopOfStackLocal()) {
  %else:
  if (auto atTopOfStack = AtTopOfStackGlobal()) {
  %endif
    ${function.name}Command command(
        GetCurrentThreadId()${',' if args_simple else ');'}
        %if args_simple:
        %for arg in args_simple[:-1]:
        ${arg},
        %endfor
        ${args_simple[-1]});
        %endif
    %for param in function.params:
    %if param.is_context:
    command.${param.name}_.key = manager.${get_context_map(function)}.getKey(reinterpret_cast<std::uintptr_t>(${param.name}));
    %elif param.is_interface and not param.is_interface_creation and not param.is_const:
    %if not param.sal_size:
    updateInterface(command.${param.name}_, ${param.name});
    %else:
    UpdateInterface<InterfaceArrayArgument<${param.type}>, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %endif
    %elif param.structure_with_interfaces and param.sal_size:
    UpdateInterface<${param.type}s_Argument, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %elif param.structure_with_interfaces:
    UpdateInterface<${param.type}_Argument, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %endif
    %endfor
    %if xess_d3d12_init:
    command.pInitParams_.key = manager.createWrapperKey(); // Used for subcapture restore order
    %endif

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      ${'result = ' if not function.ret.is_void else ''}manager.${dispatch_table}.${function.name}(${'' if args else ');'}
          %if args:
          %for param in args[:-1]:
          ${param},
          %endfor
          ${args[-1]});
          %endif
    }

    %for param in function.params:
    %if param.is_context_output:
    if (result == ${get_success_return_value(function)}) {
      command.${param.name}_.key = manager.createWrapperKey();
      auto context = reinterpret_cast<std::uintptr_t>(*command.${param.name}_.value);
      manager.${get_context_map(function)}.setContext(context, command.${param.name}_.key);
    }
    %endif
    %endfor
    %if not function.ret.is_void:
    %if update_created:
    ${update_created}
    %endif
    command.result_.value = result;
    %endif

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
      ${'result = ' if not function.ret.is_void else ''}manager.${dispatch_table}.${function.name}(${'' if args else ');'}
        %if args:
        %for param in function.params[:-1]:
        ${param.name},
        %endfor
        ${function.params[-1].name});
        %endif
  }
  %if not function.ret.is_void:

  return result;
  %endif
}
%endfor

%for interface in interfaces:
%for function in interface.functions:
<%
params = generate_params(function)
args = generate_args(function)
update_created = wrappers_update_created(function) 
args_simple = []
for param in function.params:
    args_simple.append(param.name)
%>
${generate_return(function)} ${interface.name}Wrapper::${function.name}(${'' if params else ') {'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]}) {
    %endif
  %if not function.ret.is_void:
  ${generate_return(function)} result{};
  %endif

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    ${interface.name}${function.name}Command command(
        GetCurrentThreadId(),
        reinterpret_cast<${interface.name}*>(this)${',' if params else ');'}
        %if args_simple:
        %for arg in args_simple[:-1]:
        ${arg},
        %endfor
        ${args_simple[-1]});
        %endif

    updateInterface(command.object_, this);
    %for param in function.params:
    %if param.is_interface and not param.is_interface_creation and not param.is_const:
    %if not param.sal_size:
    updateInterface(command.${param.name}_, ${param.name});
    %else:
    UpdateInterface<InterfaceArrayArgument<${param.type}>, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %endif
    %elif param.structure_with_interfaces and param.sal_size:
    UpdateInterface<${param.type}s_Argument, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %elif param.structure_with_interfaces:
    UpdateInterface<${param.type}_Argument, ${param.type}> update_${param.name}(command.${param.name}_, ${param.name});
    %endif
    %endfor

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      ${'result = ' if not function.ret.is_void else ''}command.object_.value->${function.name}(${'' if args else ');'}
          %if args:
          %for param in args[:-1]:
          ${param},
          %endfor
          ${args[-1]});
          %endif
    }

    %if not function.ret.is_void:
    %if update_created:
    ${update_created}
    %endif
    command.result_.value = result;

    %endif
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
      ${'result = ' if not function.ret.is_void else ''}getWrappedObject<${interface.name}>()->${function.name}(${'' if args else ');'}
        %if args:
        %for param in function.params[:-1]:
        ${param.name},
        %endfor
        ${function.params[-1].name});
        %endif
  }
  %if not function.ret.is_void:

  return result;
  %endif
}
%endfor
%endfor
} // namespace DirectX
} // namespace gits
