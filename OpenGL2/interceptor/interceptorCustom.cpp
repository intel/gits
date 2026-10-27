// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Hand-written overrides for wglGetProcAddress and glXGetProcAddress(ARB):
// apps use them to fetch every GL function beyond GL1.1/core WGL, so unlike
// every other (generated) trampoline, these can't just forward to the real
// driver and return its result unchanged - doing that would hand the app a raw
// real driver pointer that completely bypasses this interceptor for every call
// made through it. Excluded from the generated interceptorAuto.cpp - see
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

#if defined(_WIN32)
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
#endif

// The pointer to hand the app for `name`, given the real driver's valid `realProc`.
void* InterceptedProcAddress(IRecorder& recorder, const char* name, void* realProc) {
  // Cache this now, while we know it's valid - some apps fetch extension
  // pointers using a temporary/bootstrap context that gets destroyed shortly
  // after (e.g. the common "create dummy context -> fetch ARB functions ->
  // destroy it -> create the real context" pattern). If we didn't cache it
  // here, our own trampoline (handed out below) would instead try to
  // resolve this name again from scratch the first time the app actually
  // calls it - by which point the bootstrap context may already be gone,
  // causing a resolution failure for a function that's genuinely available.
  bool knownFunction = recorder.SetResolvedFunction(name, realProc);

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
  void* ownProc = GetInterceptorProcAddress(name);
  if (ownProc == nullptr) {
    FatalRecorderError("interceptor trampoline lookup", name);
  }
  return ownProc;
}

} // namespace

#if defined(_WIN32)

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
  return reinterpret_cast<PROC>(
      InterceptedProcAddress(recorder, name, reinterpret_cast<void*>(realProc)));
}

#else

__GLXextFuncPtr GlxGetProcAddress(const char* command, const GLubyte* procName) {
  IRecorder& recorder = GetRecorder();
  if (procName == nullptr) {
    return nullptr;
  }

  using GlxGetProcAddressPtr = __GLXextFuncPtr (*)(const GLubyte*);
  auto realGlxGetProcAddress =
      reinterpret_cast<GlxGetProcAddressPtr>(recorder.ResolveRealFunction(command));
  if (realGlxGetProcAddress == nullptr) {
    FatalRecorderError("real glXGetProcAddress resolution", "Function is unavailable");
  }
  __GLXextFuncPtr realProc = realGlxGetProcAddress(procName);
  if (realProc == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<__GLXextFuncPtr>(InterceptedProcAddress(
      recorder, reinterpret_cast<const char*>(procName), reinterpret_cast<void*>(realProc)));
}

#endif

} // namespace gits::opengl2

#if defined(_WIN32)

GL_INTERCEPTOR_EXPORT PROC APIENTRY wglGetProcAddress(LPCSTR name) {
  return gits::opengl2::InvokeRecorderOrDie("wglGetProcAddress",
                                            [&] { return gits::opengl2::WglGetProcAddress(name); });
}

#else

GL_INTERCEPTOR_EXPORT __GLXextFuncPtr glXGetProcAddress(const GLubyte* procName) {
  return gits::opengl2::InvokeRecorderOrDie("glXGetProcAddress", [&] {
    return gits::opengl2::GlxGetProcAddress("glXGetProcAddress", procName);
  });
}

GL_INTERCEPTOR_EXPORT __GLXextFuncPtr glXGetProcAddressARB(const GLubyte* procName) {
  return gits::opengl2::InvokeRecorderOrDie("glXGetProcAddressARB", [&] {
    return gits::opengl2::GlxGetProcAddress("glXGetProcAddressARB", procName);
  });
}

#endif
