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
  %if func['recWrapName'] in subwrappers:
<% continue %>
  %endif
  %if func.get('recWrap'):
    inline void ${func['recWrapName']}(CRecorder& recorder, ze_result_t return_value, \
    %for arg in func['args']:
${arg['type']} ${arg['name']}${') {' if loop.last else ', '}\
    %endfor
  if (recorder.Running())
  {
    recorder.Schedule(new C${name}(return_value, ${make_params(func['args'])}));
  }
  %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(return_value, ${make_params(func['args'])});
  %endif
}

  %endif
%endfor
} // namespace l0
} // namespace gits
