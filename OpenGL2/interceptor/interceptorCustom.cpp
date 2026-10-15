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

#include "logging.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "realDriver.h"

#include <string>
#include <unordered_set>

extern "C" PROC APIENTRY wglGetProcAddress(LPCSTR name) {
  gits::opengl2::LogFunctionName("wglGetProcAddress");

  PROC realProc = gits::opengl2::drv.wglGetProcAddress(name);
  if (realProc == nullptr) {
    // The real driver doesn't have this function either - report that
    // faithfully rather than pretending otherwise.
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
  gits::opengl2::SetResolved(name, reinterpret_cast<void*>(realProc));

  // If we generated our own trampoline for this name, hand that out instead
  // of the raw real pointer, so calls made through it are still logged and
  // forwarded through the dispatch table like any other intercepted call.
  // If we didn't generate one (e.g. a function missing from the XML
  // registries we parsed), fall back to the raw real pointer - we can't
  // wrap what we have no generated trampoline for, but the app must still
  // be able to call it.
  PROC ownProc = reinterpret_cast<PROC>(gits::opengl2::GetOwnProcAddress(name));
  if (ownProc == nullptr) {
    static std::unordered_set<std::string> alreadyWarnedNames;
    if (alreadyWarnedNames.insert(name).second) {
      gits::opengl2::EnsureLoggingInitialized();
      LOG_WARNING << "Unknown OpenGL function: " << name << ", passing unmodified driver function.";
    }
    return realProc;
  }
  return ownProc;
}
