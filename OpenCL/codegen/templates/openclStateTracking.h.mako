## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2025 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
${open(output_path, 'r').read()}\
<% runwrap = open(output_path, 'r').read() %>\
%for name, func in only_enabled(functions).items():
  %if func['stateTrackName'] not in runwrap:
    %if func.get('stateTrack'):
inline void ${func['stateTrackName']}(${make_params(func, with_types=True, with_retval=True)}) {
}

    %endif
  %endif
%endfor
} // namespace OpenCL
} // namespace gits
