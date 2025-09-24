// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclRecorderWrapper.h"
#include "openclRecorderSubwrappers.h"
#include "openclFunctionsAuto.h"
#include "openclStateTracking.h"
#include "openclTools.h"
#include "recorder.h"

namespace gits {
namespace OpenCL {
%for name, func in latest_version(functions).items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
void CRecorderWrapper::${cut_version(name, func['version'])}(${make_params(func, with_retval=True, with_types=True)}) const {
  %if func['enabled']:
    %if 'D3D' in name or 'DX9' in name:
  D3DWarning();
    %endif
    %if func.get('recWrap'):
  ${func.get('recWrapName')}(_recorder${make_params(func, with_retval=True, prepend_comma=True, one_line=True)});
    %else:
  CFunction* _token = nullptr;
  if (_recorder.Running()) {
    _token = new C${name}(${make_params(func, with_retval=True, one_line=True)});
    _recorder.Schedule(_token);
  }
      %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${'_token, ' if func.get('passToken') else ''}${make_params(func, with_retval=True, one_line=True)});
      %endif
    %endif
  %else:
  LOG_WARNING << "Function token C${name} not implemented.";
  %endif
}
  %if 'platform' in func:
#endif
  %endif

%endfor
} // namespace OpenCL
} // namespace gits
