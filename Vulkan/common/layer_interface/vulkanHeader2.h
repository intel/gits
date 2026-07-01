// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"

#ifdef GITS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define GITS_GET_THREAD_ID() GetCurrentThreadId()
#endif

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <sys/syscall.h>
#include <unistd.h>
#define GITS_GET_THREAD_ID() static_cast<uint32_t>(syscall(SYS_gettid))
#endif

#define VK_ENABLE_BETA_EXTENSIONS

#include "vulkan/vulkan.h"
#include "vulkan/vk_layer.h"
