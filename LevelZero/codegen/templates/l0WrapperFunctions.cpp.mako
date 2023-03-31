// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0RecorderWrapper.h"
#include "l0RecorderSubWrappers.h"

#include "l0Functions.h"
#include "l0StateTracking.h"
#include "recorder.h"

namespace gits {
namespace l0 {

%for name, func in functions.items():
%if not is_latest_version(functions, func):
<% continue%>
%endif
void CRecorderWrapper::${func.get('name')}(
  %if func.get('type') != 'void':
  ${'ze_result_t return_value' if func['enabled'] else 'ze_result_t'},
  %endif
  %for arg in func['args']:
  ${arg['type']}${' ' + arg['name'] if func['enabled'] else ''}${'' if loop.last else ','}
  %endfor
) const {
  %if func['enabled']:
    %if func.get('recWrap'):
  ${func.get('recWrapName')}(_recorder,${'' if func.get('type') == 'void' else 'return_value, '}${make_params(func['args'])});
    %else:
  CFunction* _token = nullptr;
  if (_recorder.Running()) {
    _token = new C${name}(${'' if func.get('type') == 'void' else 'return_value, '}${make_params(func['args'])});
    _recorder.Schedule(_token);
  }
      %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${'' if func.get('type') == 'void' else 'return_value, '}${make_params(func['args'])});
      %endif
    %endif
  %endif
}

%endfor

} // namespace l0
} // namespace gits
