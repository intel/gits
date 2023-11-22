// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <vector>
#include <sstream>

#include "openglTypes.h"
#include "message_pump.h"

//********************************* Player Window ***********************************
win_ptr_t CreateWin(
    GLXFBConfig pf, XVisualInfo* xinfo, GLint width, GLint height, GLint x, GLint y, bool show);
win_ptr_t CreateWin(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show);

class Window_ {
  win_ptr_t _window;

public:
  Window_(GLint width, GLint height, GLint x, GLint y, bool show);
  Window_(GLXFBConfig pf, XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show);
  Window_(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show);
  Window_(const Window_& other) = delete;
  Window_& operator=(const Window_& other) = delete;
  ~Window_();
  void set_position(int x, int y);
  void set_size(int width, int height);
  void set_title(const std::string& title);
  void set_visibility(bool show);
  win_handle_t handle() const;
};

EGLDisplay GetEGLDisplay();

//********************************* Recorder Window *********************************
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
