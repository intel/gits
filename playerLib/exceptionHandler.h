// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "exception.h"
#ifdef GITS_PLATFORM_WINDOWS
#include "stackWalker.h"
#endif

#include <utility>

namespace gits {

#ifdef GITS_PLATFORM_WINDOWS
void ShowCallstack(PEXCEPTION_POINTERS exceptionPtr) {
  class StackWalkerToConsole : public StackWalker {
  public:
    StackWalkerToConsole() : StackWalker(OptionsAll, ".") {}
    virtual void OnOutput(LPCSTR szText) {
      LOG_ERROR << szText;
    }
  } sw;
  sw.ShowCallstack(GetCurrentThread(), exceptionPtr->ContextRecord);
}

LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exceptionPtr) {
  ShowExceptionInfo(exceptionPtr);
  ShowCallstack(exceptionPtr);
  return EXCEPTION_CONTINUE_SEARCH;
}

template <typename Func, typename... Args>
int RunNoSEHThrow(Func&& func, Args&&... args) {
  __try {
    return std::forward<Func>(func)(std::forward<Args>(args)...);
  } __except (ExceptionFilter(GetExceptionInformation())) {
    return EXIT_FAILURE;
  }
}
#endif

template <typename Func, typename... Args>
int RunNoThrow(const char* funcName, Func&& func, Args&&... args) {
  try {
#ifdef GITS_PLATFORM_WINDOWS
    return RunNoSEHThrow(std::forward<Func>(func), std::forward<Args>(args)...);
#else
    return std::forward<Func>(func)(std::forward<Args>(args)...);
#endif
  } catch (...) {
    topmost_exception_handler(funcName);
    return EXIT_FAILURE;
  }
}

} // namespace gits
