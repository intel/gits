// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "iunknownWrapper.h"
#include "directx.h"

#include <initguid.h>

namespace gits {
namespace DirectX {

%for function in functions:
<%
params = generate_params(function)
%>
  ${generate_return(function)} ${function.name}Wrapper(${'' if params else ');'}
      %if params:
      %for param in params[:-1]:
      ${param},
      %endfor
      ${params[-1]});
      %endif
%endfor

%for interface in interfaces:
class ${interface.name}Wrapper : public ${interface.base_name}Wrapper {
public:
  ${interface.name}Wrapper(REFIID riid, IUnknown* object) : ${interface.base_name}Wrapper(riid, object) {
    insertIID(__uuidof(${interface.name}));
  }
%for function in interface.functions:
<%
params = generate_params(function)
%>
  virtual ${generate_return(function)} ${function.name}(${'' if params else ');'}
      %if params:
      %for param in params[:-1]:
      ${param},
      %endfor
      ${params[-1]});
      %endif
  %endfor
};

%endfor
} // namespace DirectX
} // namespace gits
