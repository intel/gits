// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include "platform.h"

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef True
#undef XVisualInfo
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include "wayland-client.h"
#include "wayland-egl.h"
#include <stdint.h>
typedef uint32_t xcb_window_t;
typedef struct xcb_connection_t xcb_connection_t;
typedef XID win_handle_t;
typedef win_handle_t win_ptr_t;
typedef void* native_disp_t;
#elif defined GITS_PLATFORM_WINDOWS
typedef struct HWND__* HWND;
typedef HWND win_handle_t;
typedef win_handle_t win_ptr_t;
typedef void* native_disp_t;
#endif

//********************************* Message Pump *********************************
class MessagePump {
public:
  MessagePump();
  void process_messages();
  void stop();
  virtual ~MessagePump() = 0;

  virtual void idle();
  virtual void key_down(int code);
  static MessagePump* get();

private:
  bool leave_;

  //only single message pump will get messages
  //this should eventually be changing to per'
  //per thread pointer (at least for windows case)
  static MessagePump* current_;

  void operator=(const MessagePump&);
};

win_ptr_t CreateWin(int width, int height, int x, int y, bool show);
void RemoveWin(win_ptr_t handle);
void ResizeWin(win_ptr_t handle, int width, int height);
void MoveWin(win_ptr_t handle, int x, int y);
void WinVisibility(win_ptr_t handle, bool show);
void WinTitle(win_ptr_t handle, const std::string& title);
win_handle_t GetWinHandle(win_ptr_t win);
native_disp_t GetNativeDisplay();
void DestroyPlayerWindowing();
