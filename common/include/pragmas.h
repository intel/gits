// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"

#define STRINGIZE(x)        STRINGIZE_HELPER(x)
#define STRINGIZE_HELPER(x) #x
#define LINE_STRING         STRINGIZE(__LINE__)

#if defined _MSC_VER // MSVC

#define DO_PRAGMA(x)                   __pragma(x)
#define DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))
#define DISABLE_WARNINGS               __pragma(warning(push, 1)) DISABLE_WARNING(4503)
#define ENABLE_WARNINGS                __pragma(warning(pop))

#elif defined(__clang__) || defined(__GNUC__) // clang or gcc

#define DO_PRAGMA(x)                 _Pragma(#x)
#define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored warningName)
#define DISABLE_WARNINGS                                                                           \
  DO_PRAGMA(GCC diagnostic push)                                                                   \
  DISABLE_WARNING("-Wattributes")                                                                  \
  DISABLE_WARNING("-Wdelete-non-virtual-dtor")                                                     \
  DISABLE_WARNING("-Wdeprecated-declarations")                                                     \
  DISABLE_WARNING("-Wparentheses")                                                                 \
  DISABLE_WARNING("-Wuninitialized")                                                               \
  DISABLE_WARNING("-Wunused-function")                                                             \
  DISABLE_WARNING("-Wunused-local-typedefs")
#define ENABLE_WARNINGS DO_PRAGMA(GCC diagnostic pop)

#else // other compilers

#define DO_PRAGMA(x) _Pragma(#x)
#define DISABLE_WARNINGS
#define ENABLE_WARNINGS

#endif

#define TODO(x) DO_PRAGMA(message(__FILE__ "(" LINE_STRING ") : TODO: " #x))
