// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
  * GITS_PLATFORM_WINDOWS_X86
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

#if defined GITS_PLATFORM_WINDOWS && defined GITS_ARCH_X86
#define GITS_PLATFORM_WINDOWS_X86
#endif
#if defined GITS_PLATFORM_WINDOWS && defined GITS_ARCH_X64
#define GITS_PLATFORM_WINDOWS_X64
#endif

#define GITS_PLATFORM_BIT_WINDOWS 1u
#define GITS_PLATFORM_BIT_X11     2u
#define GITS_PLATFORM_BIT_ALL     ~0u

#define VENDOR_ID_NVIDIA 0x10DE
#define VENDOR_ID_INTEL  0x8086

//#define GITS_PLATFORM_BIT_LINUX        GITS_PLATFORM_BIT_X11
//#define GITS_PLATFORM_BIT_UNIX         GITS_PLATFORM_BIT_LINUX

#if defined GITS_PLATFORM_WINDOWS
#define GITS_PLATFORM_BIT_CURRENT GITS_PLATFORM_BIT_WINDOWS
#define STDCALL                   __stdcall
#define NOINLINE                  __declspec(noinline)
#define NORETURN                  __declspec(noreturn)
#define VISIBLE
#define WEAK
#elif defined GITS_PLATFORM_X11
#define GITS_PLATFORM_BIT_CURRENT GITS_PLATFORM_BIT_X11
#define STDCALL
#define NOINLINE __attribute__((noinline))
#define NORETURN __attribute__((noreturn))
#define VISIBLE  __attribute__((visibility("default")))
#define WEAK     __attribute__((weak))
#else
#error "Unknown platform"
#endif
