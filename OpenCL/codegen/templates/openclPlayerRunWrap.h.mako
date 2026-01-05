## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2026 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% runwrap = open(output_path, 'r').read() %>\
%for name, func in functions.items():
  %if func['runWrapName'] not in runwrap:
    %if func.get('runWrap'):
inline void ${func.get('runWrapName')}(${'' if func.get('type') == 'void' else get_wrap_type(func['retV']) + '& _return_value, '}\
      %for arg in func['args']:
${get_wrap_type(arg)} &_${arg['name']}${'' if loop.last else ', '}\
      %endfor
) {
  ${'_return_value.Value() = ' if func['type'] != 'void' else ''}drvOcl.${func.get('name')}(${make_params(func, prefix='*_', one_line=True)});
      %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${make_params(func, prefix='*_', with_retval=True, one_line=True)});
      %endif
      %for arg in func['args']:
        %if arg.get('removeMapping'):
  _${arg['name']}.RemoveMapping();
        %endif
      %endfor
}

    %endif
  %endif
%endfor
} // namespace OpenCL
} // namespace gits
