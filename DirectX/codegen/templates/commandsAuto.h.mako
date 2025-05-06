// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx.h"
#include "command.h"
#include "commandIdsAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'ID3D12GraphicsCommandListOMSetRenderTargets',
    'ID3D12DeviceGetResourceTiling',
    'ID3D12StateObjectPropertiesGetShaderIdentifier',
    'ID3D12ResourceWriteToSubresource',
    'ID3D12ResourceReadFromSubresource',
    'IDMLDeviceCheckFeatureSupport'
]
%>\
<%
def generate_initializer_list(function):
    initializer_list = []
    for param in function.params:
        s = param.name + '_{' + param.name
        if param.sal_size and not param.is_array and not param.is_interface_creation and (not param.is_pointer_to_pointer or param.is_interface):
            s += ', ' + param.sal_size.split(',')[0]
        s += '}'
        initializer_list.append(s)
    return initializer_list
%>\
%for function in functions:
%if not function.name in custom:
<%
params = generate_params(function)
is_result = False if function.ret.is_void else True
params_for_function = generate_params_for_function(function)
initializer_list = generate_initializer_list(function)
%>\
class ${function.name}Command : public Command {
public:
  ${function.name}Command(unsigned threadId${',' if params else ')'}
          %if params:
          %for param in params[:-1]:
          ${param},
          %endfor
          ${params[-1]})
          %endif
      : Command{CommandId::ID_${function.name.upper()}, threadId}${',' if initializer_list else ''}
          %if initializer_list:
          %for param in initializer_list[:-1]:
          ${param},
          %endfor
          ${initializer_list[-1]}
          %endif
  {}
  ${function.name}Command() : Command(CommandId::ID_${function.name.upper()}) {}

public:
  %if is_result:
  Argument<${generate_return(function)}> result_{};
  %endif
  %for param in params_for_function:
  ${param}{};
  %endfor
};

%endif
%endfor

%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
<%
params = generate_params(function)
is_result = False if function.ret.is_void else True
params_for_function = generate_params_for_function(function)
initializer_list = generate_initializer_list(function)
%>\
class ${interface.name}${function.name}Command : public Command {
public:
  ${interface.name}${function.name}Command(unsigned threadId,
          ${interface.name}* object${',' if params else ')'}
          %if params:
          %for param in params[:-1]:
          ${param},
          %endfor
          ${params[-1]})
          %endif
      : Command{CommandId::ID_${interface.name.upper()}_${function.name.upper()}, threadId},
          object_{object}${',' if initializer_list else ''}
          %if initializer_list:
          %for param in initializer_list[:-1]:
          ${param},
          %endfor
          ${initializer_list[-1]}
          %endif
  {}
  ${interface.name}${function.name}Command() : Command(CommandId::ID_${interface.name.upper()}_${function.name.upper()}) {}

public:
  InterfaceArgument<${interface.name}> object_{};
  %if is_result:
  Argument<${generate_return(function)}> result_{};
  %endif
  %for param in params_for_function:
  ${param}{};
  %endfor
};

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
