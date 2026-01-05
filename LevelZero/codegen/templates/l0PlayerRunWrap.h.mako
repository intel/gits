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
  %if func['runWrapName'] in runwrap:
<% continue %>
  %endif
  %if func.get('runWrap'):
inline void ${func.get('runWrapName')}(${'' if func.get('type') == 'void' else 'Cze_result_t &_return_value, '}\
    %for arg in func['args']:
${get_wrap_type(arg)} &_${arg['name']}${'' if loop.last else ', '}\
    %endfor
) {
${'\  ndrv.Initialize();' if name == 'zeInit' else ''}
  ${'_return_value.Value() = ' if func['type'] != 'void' else ''}drv.${func.get('name')}(${make_params(func, prefix='*_')});
  %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${make_params(func, prefix='*_', with_retval=True)});
  %endif
  %for arg in func['args']:
  %if arg['tag'] == 'out' and (('handle' in arg['type'] or 'CMappedPtr' in arg['wrapType']) and 'ipc' not in arg['type']):
  _${arg['name']}.SaveMapping();
  %endif
  %if arg.get('release', False):
  _${arg['name']}.RemoveMapping();
  %endif
  %endfor
}

  %endif
%endfor
} // namespace l0
} // namespace gits
