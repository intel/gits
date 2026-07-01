// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}


#pragma once


#include "platform.h"
#include <cstdint>
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
#include <windows.h>
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

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#ifdef GITS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef GITS_PLATFORM_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#define VK_ENABLE_BETA_EXTENSIONS

#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_win32.h"
#include "vulkan/vulkan_wayland.h"
#include "vulkan/vulkan_xcb.h"
#include "vulkan/vulkan_xlib.h"
#include "vulkan/vulkan_beta.h"
#include "vulkan/vk_layer.h"

typedef void(VKAPI_PTR* PFN_vkVoidFunction)(void);

typedef uint32_t VkFlags;
% for flag in vulkan_flags:
typedef VkFlags ${flag};
% endfor

typedef uint64_t VkFlags64;
% for flag in vulkan_flags64:
typedef VkFlags64 ${flag};
% endfor

% for extended_enum in vk_extended_enums:
% for enumerator in extended_enum.enumerators:
constexpr auto ${enumerator.name} = static_cast<${extended_enum.name}>(${decimal_str_to_hex(enumerator.value)});
% endfor
% endfor

% for enum in vk_enums:
<%
    parent_type: str = ': VkFlags64 ' if enum.size == 64 else ''
%>\
typedef enum ${enum.name}_ ${parent_type}{
  % for enumerator in enum.enumerators:
  ${enumerator.name} = ${decimal_str_to_hex(enumerator.value)},
  % endfor
} ${enum.name};

% endfor
\
% for struct in vk_structs:
${struct.type} ${struct.name};
% endfor

\
% for token in vk_functions:
<%
    params: str = args_to_str(token.args, '{type} {name}{array}, ', ', ')
%>\
typedef ${token.return_value.type} (VKAPI_PTR *PFN_${token.name}) (${params});
% endfor

#ifdef VK_PROTOTYPES
% for token in vk_functions:
<%
    multiline_params: str = args_to_str(token.args, '{type} {name}{array},\n', ', \n')
%>\
VKAPI_ATTR ${token.return_value.type} VKAPI_CALL ${token.name}(
${multiline_params});

% endfor
#endif
\
% for struct in dependency_ordered(vk_structs):
${struct.type} ${struct.name} {
${fields_to_str(struct.fields, '  {type} {name}{bitfield}{array};\n', '\n')}
};

% endfor
