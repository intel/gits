// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"

typedef void* ANativeWindow;

#ifdef GITS_PLATFORM_X11
#include <X11/Xlib.h>
#else
typedef struct _XDisplay Display;
typedef unsigned long VisualID;
typedef unsigned long Window;
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#else
typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_visualid_t;
typedef uint32_t xcb_window_t;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif
typedef int BOOL;
#ifndef VOID
typedef void VOID;
#endif
typedef VOID* LPVOID;
#ifdef GITS_PLATFORM_WINDOWS
typedef unsigned long DWORD;
#else
typedef uint32_t DWORD;
#endif
typedef struct HWND__* HWND;
typedef struct HDC__* HDC;
typedef struct HGLRC__* HGLRC;
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HMONITOR__* HMONITOR;
typedef struct D3DKMT_HANDLE__* D3DKMT_HANDLE;
typedef HINSTANCE HMODULE;
#ifdef GITS_PLATFORM_X11
typedef struct _SECURITY_ATTRIBUTES {
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#endif

typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef DWORD COLORREF;
typedef void* HANDLE;
