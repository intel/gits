// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "openglDriversAuto.h"
#include "log.h"
#include "realDriver.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

// Definition of the dispatch table declared in openglDriversAuto.h.
OpenGLDispatchTable g_DispatchTable;

namespace {

// Per the WGL spec, wglGetProcAddress signals failure either by returning NULL or by
// returning one of four documented sentinel values - (void*)1, (void*)2, (void*)3, (void*)-1 -
// some drivers use instead of NULL for a function they don't support. Treating a sentinel as
// a real, resolved pointer would cache it in the dispatch table and eventually crash the
// first time something actually calls through it, since it is not a valid code address.
bool IsWglGetProcAddressFailureSentinel(void* real) {
  return real == nullptr || real == reinterpret_cast<void*>(1) ||
         real == reinterpret_cast<void*>(2) || real == reinterpret_cast<void*>(3) ||
         real == reinterpret_cast<void*>(-1);
}

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
    real = reinterpret_cast<void*>(
      g_DispatchTable.wglGetProcAddress.load(std::memory_order_acquire)(name));
    if (IsWglGetProcAddressFailureSentinel(real)) {
      real = nullptr;
    }
  }
  if (real == nullptr) {
    // Failed to load it, will retry the next time app calls it.
    // Perhaps the app made a mistake of calling it before a context was available?
    // Perhaps the app did everything properly and we failed for some reason?
    // TODO: distinguish "no context current yet" from a genuine resolution
    // failure with a more specific message (see the old OpenGL backend's
    // get_proc_address(), which checks drv_.gl.Api() == CGlDriver::API_NULL
    // for this same case).
    static std::mutex warningMutex;
    static std::unordered_set<std::string> alreadyWarnedNames;
    const std::lock_guard lock(warningMutex);
    if (alreadyWarnedNames.insert(name).second) {
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
  auto realFunction = reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(real);
  g_DispatchTable.${cmd.name}.store(realFunction, std::memory_order_release);
% if is_void:
  realFunction(${param_names});
% else:
  return realFunction(${param_names});
% endif
}

% endfor
bool TryCacheRealFunction(const char* name, void* value) {
  // This function is rarely run, so doesn't really matter for performance.

  static const std::unordered_map<std::string, void (*)(void*)> nameToSetter = {
% for cmd in commands:
    {"${cmd.name}", [](void* pointer) {
         g_DispatchTable.${cmd.name}.store(
           reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(pointer),
           std::memory_order_release);
     }},
% endfor
  };
  auto it = nameToSetter.find(name);
  if (it == nameToSetter.end()) {
    return false;
  }
  it->second(value);
  return true;
}

} // namespace gits::opengl2
