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
inline void ${func['stateTrackName']}(ze_result_t return_value, ${make_params_with_types(func['args'])}) {
  (void)return_value;
  %for arg in func['args']:
  (void)${arg['name']};
  %endfor
}

  %endif
%endfor
} // namespace l0
} // namespace gits
