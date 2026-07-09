// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"

#if defined GITS_PLATFORM_X11

struct _XDisplay;
typedef struct _XDisplay Display;
struct xcb_connection_t;

namespace gits {
namespace windowing {

Display* GetPlayerX11Display();
xcb_connection_t* GetPlayerX11XcbConnection();

} // namespace windowing
} // namespace gits

#endif
