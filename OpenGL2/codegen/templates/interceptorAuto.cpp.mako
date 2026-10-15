// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Trampolines for every intercepted command (see codegen/generator_interceptor.py).
// Each one logs its function name and forwards through the generated dispatch
// table (see openglDriversAuto.h). Resolving the real driver function is
// entirely the dispatch table's own concern: each `drv.<name>` slot always
// holds a valid, callable function pointer (either its own self-resolving
// thunk or the resolved real function), so trampolines just call through it
// directly. Exported via the generated interceptorAuto.def, not per-function
// dllexport.

#include "logging.h"
#include "openglDriversAuto.h"

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
extern "C" ${ret} APIENTRY ${cmd.name}(${param_decls}) {
  gits::opengl2::LogFunctionName("${cmd.name}");
% if is_void:
  gits::opengl2::drv.${cmd.name}(${param_names});
% else:
  return gits::opengl2::drv.${cmd.name}(${param_names});
% endif
}

% endfor
