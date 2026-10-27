// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

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

% for name, token_versions in gl_functions.items():
<%
    # The result should not change depending on which version we use,
    # but we take the latest one just in case.
    token: Token = token_versions[-1]
    params = args_to_str(token.args, '{type} {name_with_array}, ', ', ')
%>\
  GLAPI ${token.return_value.type} GLAPIENTRY ${name}(${params});
% endfor
}
