// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Hand-written override for wglGetProcAddress: apps use it to fetch every
// GL function beyond GL1.1/core WGL, so unlike every other (generated)
// trampoline, this one can't just forward to the real driver and return its
// result unchanged - doing that would hand the app a raw real driver
// pointer that completely bypasses this interceptor for every call made
// through it. Excluded from the generated interceptorAuto.cpp - see
// codegen/generator_interceptor.py.

#include "glTypes.h"
#include "interceptorExports.h"
#include "logging.h"
#include "log.h"
#include "recorderLoader.h"

#include <mutex>
#include <string>
#include <unordered_set>

namespace gits::opengl2 {

namespace {
// Per the WGL spec, wglGetProcAddress signals failure either by returning NULL or by
// returning one of four documented sentinel values - (PROC)1, (PROC)2, (PROC)3, (PROC)-1 -
// some drivers use instead of NULL for a function they don't support. Treating a sentinel as
// a real, callable pointer would cache it and eventually crash the first time something
// actually calls through it, since it is not a valid code address.
bool IsWglGetProcAddressFailureSentinel(PROC proc) {
  return proc == nullptr || proc == reinterpret_cast<PROC>(1) ||
         proc == reinterpret_cast<PROC>(2) || proc == reinterpret_cast<PROC>(3) ||
         proc == reinterpret_cast<PROC>(-1);
}
} // namespace

PROC WglGetProcAddress(LPCSTR name) {
  IRecorder& recorder = GetRecorder();
  if (name == nullptr) {
    return nullptr;
  }

  using WglGetProcAddressPtr = PROC(APIENTRY*)(LPCSTR);
  auto realWglGetProcAddress =
      reinterpret_cast<WglGetProcAddressPtr>(recorder.ResolveRealFunction("wglGetProcAddress"));
  if (realWglGetProcAddress == nullptr) {
    FatalRecorderError("real wglGetProcAddress resolution", "Function is unavailable");
  }
  PROC realProc = realWglGetProcAddress(name);
  if (IsWglGetProcAddressFailureSentinel(realProc)) {
    // The real driver doesn't have this function either - report that
    // faithfully (as a real NULL) rather than pretending otherwise, whether the driver told
    // us so via NULL or via one of the documented non-NULL failure sentinels above.
    return nullptr;
  }

  // Cache this now, while we know it's valid - some apps fetch extension
  // pointers using a temporary/bootstrap context that gets destroyed shortly
  // after (e.g. the common "create dummy context -> fetch ARB functions ->
  // destroy it -> create the real context" pattern). If we didn't cache it
  // here, our own trampoline (handed out below) would instead try to
  // resolve this name again from scratch the first time the app actually
  // calls it - by which point the bootstrap context may already be gone,
  // causing a resolution failure for a function that's genuinely available.
  bool knownFunction = recorder.SetResolvedFunction(name, reinterpret_cast<void*>(realProc));

  // If we generated our own trampoline for this name, hand that out instead
  // of the raw real pointer, so calls made through it are still logged and
  // forwarded through the dispatch table like any other intercepted call.
  // If we didn't generate one (e.g. a function missing from the XML
  // registries we parsed), fall back to the raw real pointer - we can't
  // wrap what we have no generated trampoline for, but the app must still
  // be able to call it.
  if (!knownFunction) {
    static std::mutex warningMutex;
    static std::unordered_set<std::string> alreadyWarnedNames;
    const std::lock_guard lock(warningMutex);
    if (alreadyWarnedNames.insert(name).second) {
      EnsureLoggingInitialized();
      LOG_WARNING << "Unknown OpenGL function: " << name << ", passing unmodified driver function.";
    }
    return realProc;
  }
  PROC ownProc = reinterpret_cast<PROC>(GetInterceptorProcAddress(name));
  if (ownProc == nullptr) {
    FatalRecorderError("interceptor trampoline lookup", name);
  }
  return ownProc;
}

} // namespace gits::opengl2

extern "C" PROC APIENTRY wglGetProcAddress(LPCSTR name) {
  return gits::opengl2::InvokeRecorderOrDie("wglGetProcAddress",
                                            [&] { return gits::opengl2::WglGetProcAddress(name); });
}
