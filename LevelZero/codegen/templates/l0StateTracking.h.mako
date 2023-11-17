## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% runwrap = open(output_path, 'r').read() %>\
%for name, func in functions.items():
  %if func['stateTrackName'] in runwrap:
<% continue %>
  %endif
  %if func.get('stateTrack'):
inline void ${func['stateTrackName']}(${make_params(func, with_types=True, with_retval=True)}) {
}

  %endif
%endfor
} // namespace l0
} // namespace gits
