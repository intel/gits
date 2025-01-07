// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}


#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "openglRecorderWrapper.h"
#include "openglLibraryRecorder.h"
#include "recorder.h"
#include "gits.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "exception.h"
#include "log.h"
#include "config.h"
#include "tools.h"

#include "gitsFunctions.h"

#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"
#include "glFunctions.h"
#include "openglCommon.h"
#include "openglTools.h"
#include "stateTracking.h"
#include "openglState.h"
#include "openglRecorderConditions.h"
#include "openglRecorderSubWrappers.h"
#include "openglRecorderPreSchedule.h"

#include <string>
#include <sstream>
#include <algorithm>

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <GL/glx.h>
#undef True
#undef XVisualInfo
#endif

namespace gits {
namespace OpenGL {

% for name, token_versions in gl_functions.items():
<%
    # GITS supports playback of older token versions for compatibility reasons,
    # but it only support recording of the newest version.
    token: Token = token_versions[-1]

    cname: str = make_cname(name, token.version)
    rec_cond: str = token.rec_condition or 'Recording(_recorder)'
    pre_token: str = token.pre_token or ''
    pre_schedule: str = token.pre_schedule or ''

    has_retval: bool = token.return_value.type != 'void'
    is_draw_func: bool = is_draw_function(token.function_type)

    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    params: str = args_to_str(retval_and_args, '{type} {name_with_array}, ', ', ')
    ctor_args: str = args_to_str(retval_and_args, '{name}, ')
    state_track_args: str = ctor_args + 'Recording(_recorder)'
    rec_wrap_args: str = ctor_args + '_recorder'
    ctor_args = ctor_args.strip(', ')
%>\
void CRecorderWrapper::${name}(${params}) const
{
% if token.enabled and not token.recorder_wrap:
  GITS_REC_ENTRY_GL
% if is_draw_func:
  DRAWCALL_WRAPPER_PRE_POST
% endif  # is_draw_func
  
  if(${rec_cond})
  {
% if pre_token:
    _recorder.Schedule(new ${pre_token});
% endif  # pre_token
% if pre_schedule:
    ${pre_schedule};
% endif  # pre_schedule
    _recorder.Schedule(new ${cname}(${ctor_args}));
  }
% if token.state_track:
  ${token.state_track}_SD(${state_track_args});
% endif  # token.state_track
% elif token.enabled and token.recorder_wrap:
  GITS_REC_ENTRY_GL
  ${token.recorder_wrap}_RECWRAP(${rec_wrap_args});
% else:
  CALL_ONCE [] { Log(WARN) << "function ${name} not implemented"; };
% endif  # token.enabled and (not) token.recorder_wrap
}

% endfor  # for name, token_versions
} // namespace OpenGL
} // namespace gits
