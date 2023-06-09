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
inline void ${func['recWrapName']}(CRecorder& recorder${make_params(func, with_retval=True, with_types=True, prepend_comma=True)}) {
  if (recorder.Running())
  {
    recorder.Schedule(new C${name}(${make_params(func, with_retval=True)}));
  }
  %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${make_params(func, with_retval=True)});
  %endif
}

  %endif
%endfor
} // namespace l0
} // namespace gits
