// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "directx.h"
#include "ccodeStructsAuto.h"
#include "ccodeTypes.h"

#include <sstream>

namespace gits {
namespace DirectX {
namespace ccode {

%for structure in structures:
%if not is_custom_struct(structure) and not structure.has_interfaces:
void toCpp(const ${structure.name}& value, CppParameterInfo& info, CppParameterOutput& out) {
  if (!info.isPtr && isZeroInitialized(value)) {
    out.initialization = "";
    out.value = "{}";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  %for param in structure.fields:
  %if not is_trivial_parameter(param):
  <%
    param_name = to_lower_camel_case(param.name) 
  %>\
  ${print_parameter_info(structure, param)}
  CppParameterOutput ${param_name}Out;
  toCpp(value.${param.name}, ${param_name}Info, ${param_name}Out);
  ss << ${param_name}Out.initialization;

  %endif
  %endfor
  std::string name = info.getIndexedName();

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  %for param in structure.fields:
  <%
    param_name = to_lower_camel_case(param.name) 
  %>\
  %if param.type == "WCHAR" and not param.is_pointer:
  ss << "wcscpy_s(" << name << ".${param.name}" << ", " << ${param_name}Out.value << ");" << std::endl;
  %elif param.is_array:
  for (unsigned i = 0; i < ${param_name}Info.size; ++i) {
    ss << name << ".${param.name}[" << i << "] = " << ${param_name}Out.decorator << "${param_name}[" << i << "];" << std::endl;
  }
  %elif is_trivial_parameter(param):
  ss << name << ".${param.name} = " << toStr(value.${param.name}) << ";" << std::endl;
  %else:
  ss << name << ".${param.name} = " << ${param_name}Out.decorator << ${param_name}Out.value << ";" << std::endl;
  %endif
  %endfor

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = "";
}

%endif
%endfor

} // namespace ccode
} // namespace DirectX
} // namespace gits
