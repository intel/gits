// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#define DRAW_FUNCTIONS(a) ${'\\'}
<%
    from typing import Any

    name: str  # OpenGL function name.
    token_version_data: dict[str,Any]  # Data for latest version of a token.
%>\
% for name, token_version_data in draw_functions.items():
<%
    ret_type: str = token_version_data['type']
    if ret_type == 'void':
        ret_type = 'void_t'

    params: str = arg_decl(token_version_data, add_retval=False, add_names=True)
    args: str = arg_call(token_version_data, add_retval=False)
%>\
  DRAW_FUNCTION(a, ${ret_type}, ${name}, ${params}, ${args}) ${'\\'}
% endfor

#define GL_FUNCTIONS(a) ${'\\'}
% for name, token_version_data in nondraw_functions.items():
<%
    ret_type: str = token_version_data['type']
    if ret_type == 'void':
        ret_type = 'void_t'

    params: str = arg_decl(token_version_data, add_retval=False, add_names=True)
    args: str = arg_call(token_version_data, add_retval=False)
%>\
  GL_FUNCTION(a, ${ret_type}, ${name}, ${params}, ${args}) ${'\\'}
% endfor

