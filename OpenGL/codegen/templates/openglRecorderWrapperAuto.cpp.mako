// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


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

<%
    from typing import Any

    name: str  # Keys are OpenGL function names.
    token_versions_data: list[dict[str,Any]]  # Values are complicated.
    # Each dict in the list contains data for one version of a token.
    # Example:
    # 'glFoo': [{glFoo data}, {glFoo_V1 data}]
    token_version_data: dict[str,Any]  # Data for one version.
%>\
% for name, token_versions_data in gl_functions.items():
<%
    # GITS supports playback of older token versions for compatibility reasons,
    # but it only support recording of the newest version.
    token_version_data: dict[str, Any] = token_versions_data[-1]

    track_state: bool = token_version_data.get('stateTrack') or False
    wrap_rec: bool = token_version_data.get('recWrap') or False
    is_draw_func: bool = is_draw_function(token_version_data)
    is_enabled: bool = token_version_data['enabled']

    cname: str = make_cname(name, token_version_data['version'])
    rec_cond: str = token_version_data.get('recCond') or 'Recording(_recorder)'
    pre_token: str = token_version_data.get('preToken') or ''
    pre_schedule: str = token_version_data.get('preSchedule') or ''

    # Handle inheritance, e.g. glTexImage3DEXT calls glTexImage3D_SD without EXT
    state_track_name: str|None = token_version_data.get('stateTrackName')
    rec_wrap_name: str|None = token_version_data.get('recWrapName')

    ret_type: str = token_version_data['type']
    has_retval: bool = ret_type != 'void'

    args: list[dict[str,str]] = token_version_data['args']
    retval_and_args: list[dict[str,str]]
    if has_retval:
        retval_and_args = [retval_as_arg(token_version_data)] + args
    else:
        retval_and_args = args  # TODO: `.copy()` ?

    params: str = args_to_str(retval_and_args, '{type} {name_with_array}, ', ', ')
    ctor_args: str = args_to_str(retval_and_args, '{name}, ')
    state_track_args: str = ctor_args + 'Recording(_recorder)'
    rec_wrap_args: str = ctor_args + '_recorder'
    ctor_args = ctor_args.strip(', ')
%>\
void CRecorderWrapper::${name}(${params}) const
{
% if is_enabled and not wrap_rec:
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
% if track_state:
  ${state_track_name}_SD(${state_track_args});
% endif  # track_state
% elif is_enabled and wrap_rec:
  GITS_REC_ENTRY_GL
  ${rec_wrap_name}_RECWRAP(${rec_wrap_args});
% else:
  CALL_ONCE [] { Log(WARN) << "function ${name} not implemented"; };
% endif  # is_enabled and (not) wrap_rec
}

% endfor  # for name, token_versions_data
} // namespace OpenGL
} // namespace gits
