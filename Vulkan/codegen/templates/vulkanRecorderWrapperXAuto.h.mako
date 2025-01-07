// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for token in vk_functions:
<%
    has_retval: bool = token.return_value.type != 'void'

    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    params = args_to_str(retval_and_args, '{type} {name}{array}, ', ', ')
%>\
% if is_iface:  # Whether to generate *IfaceAuto.h or *Auto.h
virtual void ${token.name}(${params}) const = 0;
% else:
void ${token.name}(${params}) const override;
% endif  # is_iface
% endfor  # function
