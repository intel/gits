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
%>
% for name, token_versions_data in gl_functions.items():
% for token_version_data in token_versions_data:
case ${make_id(name, token_version_data['version'])}:
  return new ${make_cname(name, token_version_data['version'])};
% endfor
% endfor
