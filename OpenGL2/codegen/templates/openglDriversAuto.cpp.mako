// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "openglDriversAuto.h"
#include "logging.h"
#include "log.h"
#include "realDriver.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

// Definition of the dispatch table declared in openglDriversAuto.h.
OpenglDrivers drv;

namespace {

// Resolves `name`'s real function pointer: tries the real opengl32.dll's
// static exports first; most functions beyond GL1.1/core WGL aren't
// statically exported at all and can only be resolved via
// wglGetProcAddress (which itself needs a context current to work
// reliably - hence this can fail and needs retrying later, done by each
// resolve-thunk below simply being called again on the next use).
// wglGetProcAddress itself is excluded from the fallback (it's always
// resolvable via the static path, being core WGL) to avoid infinite
// recursion into itself.
void* ResolveDriverFunction(const char* name) {
  void* real = GetRealProcAddress(name);
  if (real == nullptr && strcmp(name, "wglGetProcAddress") != 0) {
    real = reinterpret_cast<void*>(drv.wglGetProcAddress(name));
  }
  if (real == nullptr) {
    // Failed to load it, will retry the next time app calls it.
    // Perhaps the app made a mistake of calling it before a context was available?
    // Perhaps the app did everything properly and we failed for some reason?
    // TODO: distinguish "no context current yet" from a genuine resolution
    // failure with a more specific message (see the old OpenGL backend's
    // get_proc_address(), which checks drv_.gl.Api() == CGlDriver::API_NULL
    // for this same case).
    static std::unordered_set<std::string> alreadyWarnedNames;
    if (alreadyWarnedNames.insert(name).second) {
      EnsureLoggingInitialized();
      LOG_WARNING << "Failed to resolve function: " << name;
    }
  }
  return real;
}

} // namespace

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
${ret} APIENTRY ResolveAndCall_${cmd.name}(${param_decls}) {
  void* real = ResolveDriverFunction("${cmd.name}");
  if (real == nullptr) {
% if is_void:
    return;
% else:
    return {};
% endif
  }
  drv.${cmd.name} = reinterpret_cast<decltype(drv.${cmd.name})>(real);
% if is_void:
  drv.${cmd.name}(${param_names});
% else:
  return drv.${cmd.name}(${param_names});
% endif
}

% endfor
void SetResolved(const char* name, void* value) {
  // This function is rarely run, so doesn't really matter for performance.

  static const std::unordered_map<std::string, void**> nameToMember = {
% for cmd in commands:
    {"${cmd.name}", reinterpret_cast<void**>(&drv.${cmd.name})},
% endfor
  };
  auto it = nameToMember.find(name);
  if (it != nameToMember.end()) {
    *it->second = value;
  }
}

} // namespace gits::opengl2
