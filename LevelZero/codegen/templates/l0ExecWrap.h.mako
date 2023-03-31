## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% subwrappers = open(output_path, 'r').read() %>\
%for name, func in functions.items():
  %if func['recExecWrapName'] in subwrappers:
<% continue %>
  %endif
  %if func.get('recExecWrap'):
inline ze_result_t ${func.get('recExecWrapName')}(${make_params_with_types(func['args'])}) {
${'     CGitsPlugin::Initialize();\n' if name == 'zeInit' else ''}\
  GITS_ENTRY_L0
${'     wrapper.InitializeDriver();\n' if name == 'zeInit' else ''}\
  auto return_value = driver.${func.get('name')}(${make_params(func['args'])});
  GITS_WRAPPER_PRE
    wrapper.${func.get('name')}(return_value, ${make_params(func['args'])});
  GITS_WRAPPER_POST
  return return_value;
}
  %endif
%endfor
} // namespace l0
} // namespace gits
