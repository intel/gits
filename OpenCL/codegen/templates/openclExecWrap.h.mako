## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2025 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% subwrappers = open(output_path, 'r').read() %>\
%for name, func in only_enabled(functions).items():
  %if func['recExecWrapName'] not in subwrappers:
    %if func.get('recExecWrap'):
      %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
      %endif
inline ${func.get('type')} ${func.get('recExecWrapName')}(${make_params(func, with_types=True)}) {
  GITS_ENTRY_OCL
  ${'' if func.get('type') == 'void' else 'auto return_value = '}drv.${func.get('name')}(${make_params(func, one_line=True)});
  GITS_WRAPPER_PRE
    wrapper.${func.get('name')}(${make_params(func, with_retval=True, one_line=True)});
  GITS_WRAPPER_POST
  return return_value;
}
      %if 'platform' in func:
#endif
      %endif

    %endif
  %endif
%endfor
} // namespace OpenCL
} // namespace gits
