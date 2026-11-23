// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/******************************************************************************
This file introduces following macros to be tested in gits code when specific
code for certain platform / architecture is required

platforms:
  * GITS_PLATFORM_WINDOWS
  * GITS_PLATFORM_X11

platform families:
  * GITS_PLATFORM_LINUX - x11
  * GITS_PLATFORM_UNIX  - x11

architectues:
  * GITS_ARCH_X86
  * GITS_ARCH_X64
  * GITS_ARCH_ARM
  * GITS_ARCH_A64

platform x architecture:
  * GITS_PLATFORM_WINDOWS_X64

*******************************************************************************/

#ifdef _WIN32
#define GITS_PLATFORM_WINDOWS
#elif __unix__
#define GITS_PLATFORM_UNIX
#define GITS_PLATFORM_X11
#else
#error "Unsupported OS"
#endif

#if defined __i386__ || defined _M_IX86
#define GITS_ARCH_X86
#elif defined __amd64 || defined __x86_64 || defined _M_X64
#define GITS_ARCH_X64
#elif defined __arm__
#define GITS_ARCH_ARM
#elif defined __aarch64__
#define GITS_ARCH_A64
#endif

#if defined GITS_PLATFORM_X11
#define GITS_PLATFORM_LINUX
#endif

#if defined GITS_PLATFORM_WINDOWS && defined GITS_ARCH_X64
#define GITS_PLATFORM_WINDOWS_X64
#endif

#if defined GITS_PLATFORM_WINDOWS
#define STDCALL __stdcall
#define VISIBLE
#elif defined GITS_PLATFORM_X11
#define STDCALL
#define VISIBLE __attribute__((visibility("default")))
#else
#error "Unknown platform"
#endif
