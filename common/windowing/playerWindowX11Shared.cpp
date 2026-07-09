// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerWindowX11Shared.h"

#include "log.h"

#if defined GITS_PLATFORM_X11
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#endif

namespace gits {
namespace windowing {

#if defined GITS_PLATFORM_X11

Display* GetPlayerX11Display() {
  static Display* display = []() {
    XInitThreads();
    Display* opened = XOpenDisplay(nullptr);
    if (opened == nullptr) {
      LOG_ERROR << "Failed to open X11 display";
      GITS_ASSERT(false, "Failed to open X11 display");
    }
    return opened;
  }();
  return display;
}

xcb_connection_t* GetPlayerX11XcbConnection() {
  static xcb_connection_t* connection = []() {
    xcb_connection_t* conn = XGetXCBConnection(GetPlayerX11Display());
    if (conn == nullptr) {
      LOG_ERROR << "Failed to get XCB connection from X11 display";
      GITS_ASSERT(false, "Failed to get XCB connection from X11 display");
    }
    return conn;
  }();
  return connection;
}

#endif

} // namespace windowing
} // namespace gits
