// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


/**
 * @file   gitsPluginPrePostAuto.h
 *
 * @brief
 */

#include "platform.h"
#include "openglTypes.h"

#if defined GITS_PLATFORM_WINDOWS
#define GLAPI
#else
#define GLAPI __attribute__((visibility("default")))
#endif
#define GLAPIENTRY STDCALL

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
    ret_type: str = token_version_data['type']
    params: str = arg_decl(token_version_data, add_retval=False, add_names=True)
%>\
  GLAPI ${ret_type} GLAPIENTRY ${name}${params};
% endfor
}
