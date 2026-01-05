## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2026 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% subwrappers = open(output_path, 'r').read() %>\
%for name, func in latest_version(only_enabled(functions)).items():
  %if func['recWrapName'] not in subwrappers:
    %if func.get('recWrap'):
      %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
      %endif
inline void ${func['recWrapName']}(CRecorder& recorder${make_params(func, with_retval=True, with_types=True, prepend_comma=True)}) {
  if (recorder.Running()) {
    recorder.Schedule(new C${name}(${make_params(func, with_retval=True, one_line=True)}));
  }
      %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${make_params(func, with_retval=True, one_line=True)});
      %endif
}
      %if 'platform' in func:
#endif
      %endif

    %endif
  %endif
%endfor
} // namespace OpenCL
} // namespace gits
