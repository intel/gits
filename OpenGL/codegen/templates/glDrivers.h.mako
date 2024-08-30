// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#define DRAW_FUNCTIONS(a) ${'\\'}
% for token in draw_functions:
<%
    ret_type: str = token.return_value.type
    if ret_type == 'void':
        ret_type = 'void_t'

    params = args_to_str(token.args, '{type} {name_with_array}, ', ', ')
    args: str = arg_call(token, add_retval=False)
%>\
  DRAW_FUNCTION(a, ${ret_type}, ${token.name}, (${params}), ${args}) ${'\\'}
% endfor

#define GL_FUNCTIONS(a) ${'\\'}
% for token in nondraw_functions:
<%
    ret_type: str = token.return_value.type
    if ret_type == 'void':
        ret_type = 'void_t'

    params = args_to_str(token.args, '{type} {name_with_array}, ', ', ')
    args: str = arg_call(token, add_retval=False)
%>\
  GL_FUNCTION(a, ${ret_type}, ${token.name}, (${params}), ${args}) ${'\\'}
% endfor

