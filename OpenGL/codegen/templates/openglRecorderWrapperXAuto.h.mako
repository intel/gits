// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

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

    ret_type: str = token_version_data['type']
    has_retval: bool = ret_type != 'void'

    args: list[dict[str,str]] = token_version_data['args']
    retval_and_args: list[dict[str,str]]
    if has_retval:
        retval_and_args = [retval_as_arg(token_version_data)] + args
    else:
        retval_and_args = args  # TODO: `.copy()` ?

    params: str = args_to_str(retval_and_args, '{type} {name_with_array}, ', ', ')
%>\
% if is_iface:  # Whether to generate *IfaceAuto.h or *Auto.h
virtual void ${name}(${params}) const = 0;
% else:
void ${name}(${params}) const override;
% endif  # is_iface
% endfor
