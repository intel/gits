// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


/**
 * @file   gitsPluginPrePostAuto.cpp
 *
 * @brief
 */

#include "openglInterceptorExecOverride.h"

extern "C" {

% for name, token_versions in gl_functions.items():
<%
    # The result should not change depending on which version we use,
    # but we take the latest one just in case.
    token: Token = token_versions[-1]

    has_retval: bool = token.return_value.type != 'void'
    delayed_call: bool = token.exec_post_recorder_wrap is True

    params = args_to_str(token.args, '{type} {name_with_array}, ', ', ')
    wrapper_args: str = arg_call(token, add_retval=has_retval)

    # Use a fake retval in the wrapper if the real one is not available yet.
    if delayed_call and has_retval:
        wrapper_args = wrapper_args.replace('return_value', 'true')

    # TODO: Prefix & suffix should follow the same indent logic.
%>\
GLAPI ${token.return_value.type} GLAPIENTRY ${name}(${params})
{
% if token.prefix:
${token.prefix}
% endif
  GITS_ENTRY_GL
% if not delayed_call:
    ${driver_call(token)}
% endif
  GITS_WRAPPER_PRE
    wrapper.${name}${wrapper_args};
  GITS_WRAPPER_POST
% if delayed_call:
    ${driver_call(token)}
% endif
% if has_retval:
  return return_value;
% endif
% if token.suffix:
  ${token.suffix}
% endif
}

% endfor
}
