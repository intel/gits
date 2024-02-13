// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
  ${'ze_result_t return_value' if func['enabled'] else 'ze_result_t'}${',' if len(func['args']) > 0 else ''}
  %endif
  %for arg in func['args']:
  ${get_arg_type(arg['name'], arg['type'])}${' ' + arg['name'] if func['enabled'] else ''}${'' if loop.last else ','}
  %endfor
) const {
  %if func['enabled']:
    %if func.get('recWrap'):
  ${func.get('recWrapName')}(_recorder${make_params(func, with_retval=True, prepend_comma=True)});
    %else:
  CFunction* _token = nullptr;
  if (_recorder.Running()) {
    _token = new C${name}(${make_params(func, with_retval=True)});
    _recorder.Schedule(_token);
  }
      %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${'_token, ' if func.get('passToken') else ''}${make_params(func, with_retval=True)});
      %endif
    %endif
  %else:
  Log(WARN) << "Function token C${func.get('name')} not implemented.";
  %endif
}

%endfor

} // namespace l0
} // namespace gits
