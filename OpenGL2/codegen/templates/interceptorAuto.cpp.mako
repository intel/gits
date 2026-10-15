// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Trampolines for every intercepted command (see codegen/generator_interceptor.py).
// Each one initializes and caches an exactly typed recorder wrapper, invokes
// it, and contains exceptions at the exported C boundary. Exported through
// the generated interceptorAuto.def rather than per-function dllexport
// annotations.

#include "glTypes.h"
#include "logging.h"
#include "recorderLoader.h"

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
extern "C" ${ret} APIENTRY ${cmd.name}(${param_decls}) {
  using Wrapper = ${ret} (APIENTRY*)(${', '.join(p.type for p in cmd.params)});
  static Wrapper wrapper = reinterpret_cast<Wrapper>(
      gits::opengl2::GetRecorderWrapper("${cmd.name}"));
  return gits::opengl2::InvokeRecorderOrDie(
      "${cmd.name}", [&] { return wrapper(${param_names}); });
}

% endfor
