// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

<%def name="print_body(function, has_obj_key)">\
<%
interface_declaration = get_interface_declaration(function)
obj_key_str = ""
if has_obj_key:
  obj_key_str = ", c.object_.key"
%>\
%if interface_declaration:
// Declare new object
auto& stream = CCodeStream::getInstance();
${interface_declaration}

%endif
// Parameter data
%for param in function.params:
%if not is_trivial_parameter(param):
${print_parameter_info(function, param)}
%endif
%endfor

// Print command
CommandPrinter p(c, "${function.name}"${obj_key_str});
%for param in function.params:
%if is_trivial_parameter(param):
p.addArgumentValue(c.${param.name}_.value);
%else:
p.addArgument(c.${param.name}_, ${to_lower_camel_case(param.name)}Info);
%endif
%endfor
postProcess(p, c);
p.print();

nextCommand();
</%def>

#include "ccodeLayerAuto.h"
#include "ccodeCommandPrinter.h"
#include "ccodeArguments.h"

using namespace gits::DirectX::ccode;

namespace gits {
namespace DirectX {

%for function in functions:
%if function.api == Api.D3D12 or function.api == Api.DXGI:
void CCodeLayer::post(${function.name}Command& c) {
${print_body(function, False)}
}

%endif
%endfor
%for interface in interfaces:
%if interface.api == Api.D3D12 or interface.api == Api.DXGI:
%for function in interface.functions:
void CCodeLayer::post(${interface.name}${function.name}Command& c){
${print_body(function, True)}
}

%endfor
%endif
%endfor
} // namespace DirectX
} // namespace gits