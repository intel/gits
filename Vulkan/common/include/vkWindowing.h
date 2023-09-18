// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "message_pump.h"

namespace gits {
namespace Vulkan {
enum class DisplayProtocol { NONE, XCB, XLIB, WAYLAND };
static DisplayProtocol _displayProtocol;

//********************************* Player Window ***********************************
class Window_ {
  win_ptr_t _window;

public:
  Window_(int width,
          int height,
          int x,
          int y,
          bool show,
          DisplayProtocol dispProtocol = DisplayProtocol::NONE);
  ~Window_();
  void set_position(int x, int y);
  void set_size(int width, int height);
  void set_title(const std::string& title);
  void set_visibility(bool show);
  win_handle_t handle() const;
  native_disp_t native_display();
};

#ifdef GITS_PLATFORM_X11
void WinTitle(win_ptr_t winptr, const std::string& title);
//********************************* X11 Recorder Window *********************************
class xcb_handle {
public:
  xcb_handle(xcb_connection_t* connection, xcb_window_t window);
  void get_dimensions(int& x, int& y, int& width, int& height) const;
  std::pair<int, int> get_pos() const;
  std::pair<int, int> get_size() const;
  bool is_visible() const;
  win_handle_t handle() const {
    return window_;
  }

private:
  xcb_connection_t* connection_;
  xcb_window_t window_;
};

class xlib_handle {
public:
  xlib_handle(Display* display, Window window);
  void get_dimensions(int& x, int& y, int& width, int& height) const;
  std::pair<int, int> get_pos() const;
  std::pair<int, int> get_size() const;
  bool is_visible() const;
  win_handle_t handle() const {
    return window_;
  }

private:
  Display* display_;
  Window window_;
};

#else
//********************************* NonUnix Recorder Window *********************************
class window_handle {
public:
  window_handle(win_handle_t window);
  void get_dimensions(int& x, int& y, int& width, int& height) const;
  std::pair<int, int> get_pos() const;
  std::pair<int, int> get_size() const;
  bool is_visible() const;
  win_handle_t handle() const {
    return window_;
  }

private:
  win_handle_t window_;
};

#endif

} // namespace Vulkan
} // namespace gits
