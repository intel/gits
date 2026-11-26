// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "driverExports.h"
#include "glDispatchAuto.h"
#include "log.h"

#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl {

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
// recursion into itself. Elsewhere the fallback is glXGetProcAddressARB,
// which libGL exports.
//
// Lazy calls report a failure; speculative lookups (including player warm-up) do not.
void* ResolveDriverFunction(const char* name, bool logFailure = true) {
  void* real = dispatch::GetDriverExport(name);
#if defined(_WIN32)
  if (real == nullptr && strcmp(name, "wglGetProcAddress") != 0) {
    real = reinterpret_cast<void*>(
      g_DispatchTable.wglGetProcAddress.load(std::memory_order_acquire)(name));
    if (IsWglGetProcAddressFailureSentinel(real)) {
      real = nullptr;
    }
  }
#else
  if (real == nullptr && strcmp(name, "glXGetProcAddressARB") != 0) {
    real = reinterpret_cast<void*>(g_DispatchTable.glXGetProcAddressARB.load(
      std::memory_order_acquire)(reinterpret_cast<const GLubyte*>(name)));
  }
#endif
  if (real == nullptr && logFailure) {
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
    is_void = returns_void(cmd)
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
namespace {
// Commands are addressed by their index in `names` through switches rather than through
// per-command functions: thousands of small functions make static analysis of this file slow.
// Each case stays a single block, as static analysis skips functions of more than 10000 blocks.
constexpr unsigned NO_DISPATCH_INDEX = ~0u;

unsigned FindDispatchIndex(const char* name) {
  // A braced map initializer would construct every key inline, which makes this function very
  // slow to compile.
  static const char* const names[] = {
% for cmd in commands:
    "${cmd.name}",
% endfor
  };
  static const std::unordered_map<std::string_view, unsigned> nameToIndex = [] {
    std::unordered_map<std::string_view, unsigned> map;
    map.reserve(std::size(names));
    for (unsigned index = 0; index < std::size(names); ++index) {
      map.emplace(names[index], index);
    }
    return map;
  }();
  const auto it = nameToIndex.find(name);
  return it != nameToIndex.end() ? it->second : NO_DISPATCH_INDEX;
}

void StoreRealFunction(unsigned index, void* pointer) {
  switch (index) {
% for index, cmd in enumerate(commands):
  case ${index}:
    g_DispatchTable.${cmd.name}.store(
      reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(pointer), std::memory_order_release);
    return;
% endfor
  default:
    return;
  }
}

const void* LoadRealFunction(unsigned index) {
  switch (index) {
% for index, cmd in enumerate(commands):
  case ${index}:
    return reinterpret_cast<const void*>(
      g_DispatchTable.${cmd.name}.load(std::memory_order_acquire));
% endfor
  default:
    return nullptr;
  }
}

bool IsRealFunctionResolved(unsigned index) {
  static const void* const resolveThunks[] = {
% for cmd in commands:
    reinterpret_cast<const void*>(&ResolveAndCall_${cmd.name}),
% endfor
  };
  const void* pointer = LoadRealFunction(index);
  return pointer != nullptr && pointer != resolveThunks[index];
}
} // namespace

bool TryCacheRealFunction(const char* name, void* value) {
  const unsigned index = FindDispatchIndex(name);
  if (index == NO_DISPATCH_INDEX) {
    return false;
  }
  StoreRealFunction(index, value);
  return true;
}

bool TryResolveRealFunction(const char* name) {
  const unsigned index = FindDispatchIndex(name);
  if (index == NO_DISPATCH_INDEX) {
    return false;
  }
  if (IsRealFunctionResolved(index)) {
    return true;
  }
  void* real = ResolveDriverFunction(name, /*logFailure=*/false);
  if (real == nullptr) {
    return false;
  }
  StoreRealFunction(index, real);
  return true;
}

void* GetRealFunction(const char* name) {
  return ResolveDriverFunction(name, /*logFailure=*/false);
}

} // namespace gits::opengl
