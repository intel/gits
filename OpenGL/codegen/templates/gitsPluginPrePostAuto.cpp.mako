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

<%
    from typing import Any

    name: str  # Keys are OpenGL function names.
    token_versions_data: list[dict[str,Any]]  # Values are complicated.
    # Each dict in the list contains data for one version of a token.
    # Example:
    # 'glFoo': [{glFoo data}, {glFoo_V1 data}]
%>\
% for name, token_versions_data in gl_functions.items():
<%
    # The result should not change depending on which version we use,
    # but we take the latest one just in case.
    token_version_data: dict[str, Any] = token_versions_data[-1]

    # TODO: Prefix & suffix should follow the same indent logic.
    prefix: str = token_version_data.get('prefix')
    suffix: str = token_version_data.get('suffix')

    ret_type: str = token_version_data['type']
    has_retval: bool = ret_type != 'void'

    params: str = arg_decl(token_version_data, add_retval=False, add_names=True)
    wrapper_args: str = arg_call(token_version_data, add_retval=has_retval)

    delayed_call: bool = token_version_data.get('execPostRecWrap') is True

    # Use a fake retval in the wrapper if the real one is not available yet.
    if delayed_call and has_retval:
        wrapper_args = wrapper_args.replace('return_value', 'true')
%>\
GLAPI ${ret_type} GLAPIENTRY ${name}${params}
{
% if prefix:
${prefix}
% endif
  GITS_ENTRY_GL
% if not delayed_call:
    ${driver_call(name, token_version_data, has_retval)}
% endif
  GITS_WRAPPER_PRE
    wrapper.${name}${wrapper_args};
  GITS_WRAPPER_POST
% if delayed_call:
    ${driver_call(name, token_version_data, has_retval)}
% endif
% if has_retval:
  return return_value;
% endif
% if suffix:
  ${suffix}
% endif
}

% endfor
}
