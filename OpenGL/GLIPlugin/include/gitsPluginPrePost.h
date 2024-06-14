// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsPluginPrePost.h
 *
 * @brief
 */

#include "openglRecorderWrapper.h"
#include "gitsPlugin.h"
#include "openglDrivers.h"
#include "openglEnums.h"
#include "gitsPluginPrePostAuto.h"
#include "pragmas.h"

#include <mutex>

#ifdef GITS_PLATFORM_X11
#include <dlfcn.h>
#endif

// Avoid recording API - recursive functions.
extern std::recursive_mutex globalMutex;
extern thread_local uint32_t recursionDepth;
extern const uint32_t disableDepth;

void CloseRecorderIfRequired();
void post_gits_wrapper();

#define GITS_WRAPPER_PRE                                                                           \
  --recursionDepth;                                                                                \
  if (CGitsPlugin::Configuration().common.recorder.enabled && (recursionDepth == 0)) {             \
    try {

#define GITS_WRAPPER_POST                                                                          \
  CloseRecorderIfRequired();                                                                       \
  }                                                                                                \
  catch (...) {                                                                                    \
    topmost_exception_handler(__FUNCTION__);                                                       \
  }                                                                                                \
  }                                                                                                \
  post_gits_wrapper();

#define GITS_WRAPPER_NOT_SUPPORTED(x)                                                              \
  static bool logged = false;                                                                      \
  if (!logged) {                                                                                   \
    logged = true;                                                                                 \
    Log(WARN) << "Not supported function: " << __FUNCTION__;                                       \
  }

void entry();

#define GITS_ENTRY                                                                                 \
  entry();                                                                                         \
  using namespace gits::OpenGL;                                                                    \
  IRecorderWrapper& wrapper = CGitsPlugin::RecorderWrapper();                                      \
  std::unique_lock<std::recursive_mutex> lock(wrapper.GetInterceptorMutex());                      \
  ++recursionDepth;                                                                                \
  (void)wrapper;

#define GITS_ENTRY_GL GITS_ENTRY

extern "C" {} //extern "C"
