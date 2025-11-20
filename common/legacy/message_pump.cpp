// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "apis_iface.h"
#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#include "tools_windows.h"
#endif

#include "gits.h"
#include "message_pump.h"

#include "configurationLib.h"

#ifdef GITS_PLATFORM_WINDOWS
//********************************* Windows Player Window ***********************************

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  static bool moving = false;
  static int xInit = 0, yInit = 0;
  int xPos = 0, yPos = 0;

  switch (msg) {
  case WM_MOUSEMOVE:
    if (moving) {
      xPos = GET_X_LPARAM(lParam);
      yPos = GET_Y_LPARAM(lParam);
      RECT rect;
      GetWindowRect(hwnd, &rect);
      SetWindowPos(hwnd, 0, rect.left + (xPos - xInit), rect.top + (yPos - yInit), 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER);
    }
    break;
  case WM_LBUTTONDOWN:
    xInit = GET_X_LPARAM(lParam);
    yInit = GET_Y_LPARAM(lParam);
    moving = true;
    break;
  case WM_LBUTTONUP:
    moving = false;
    break;
  case WM_KEYDOWN:
    MessagePump::get()->key_down(static_cast<int>(wParam));
    break;
  case WM_CLOSE:
    MessagePump::get()->stop();
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

win_ptr_t CreateWin(int width, int height, int x, int y, bool show) {
  static const char class_name[] = "my_window_class_nameX";
  static bool class_registered = false;

  HINSTANCE hInstance = GetModuleHandle(0);
  if (!class_registered) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = class_name;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (RegisterClassEx(&wc) == 0) {
      throw std::runtime_error("Could not register window class");
    }
    class_registered = true;
  }

  SetLastError(NO_ERROR);
  HWND win;
  DWORD style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  DWORD exStyle = WS_EX_APPWINDOW;

  if (gits::Configurator::Get().common.player.windowMode ==
      gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
    style = WS_POPUP;
    exStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    x = 0;
    y = 0;

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(devMode);
    devMode.dmPelsWidth = width;
    devMode.dmPelsHeight = height;
    devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    if (DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettings(&devMode, CDS_TEST)) {
      ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
      ShowCursor(FALSE);
    } else {
      LOG_WARNING << "Cannot adjust screen resolution to match dimensions of " << width << " x "
                  << height << " pixels for a window.";
    }
  } else if (gits::Configurator::Get().common.player.showWindowBorder) {
    style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
  }

  RECT rect{x, y, x + width, y + height};
  AdjustWindowRectEx(&rect, style, FALSE, exStyle);
  int adjustedWidth = rect.right - rect.left;
  int adjustedHeight = rect.bottom - rect.top;

  win = CreateWindowEx(exStyle, class_name, "The title of my window", style, x, y, adjustedWidth,
                       adjustedHeight, NULL, NULL, hInstance, NULL);

  if (win == 0) {
    throw std::runtime_error("CreateWindowEx failed.\n"
                             "  GetLastError() = " +
                             gits::Win32ErrorToString(GetLastError()));
  }

  if (show && !gits::Configurator::Get().common.player.forceInvisibleWindows) {
    ShowWindow(win, SW_SHOW);
  }

  return win;
}

void RemoveWin(win_ptr_t winptr) {
  DestroyWindow(winptr);
}

void ResizeWin(win_ptr_t winptr, int width, int height) {
  SetLastError(NO_ERROR);
  if (SetWindowPos(winptr, 0, 0, 0, width, height, SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOZORDER) ==
      0) {
    throw std::runtime_error("Window_::set_size failed.\n"
                             "  SetWindowPos returned 0.\n"
                             "  GetLastError() = " +
                             gits::Win32ErrorToString(GetLastError()));
  }
  if (gits::Configurator::Get().common.player.windowMode ==
      gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
    DEVMODE devMode = {};
    devMode.dmSize = sizeof(devMode);
    devMode.dmPelsWidth = width;
    devMode.dmPelsHeight = height;
    devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    if (DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettings(&devMode, CDS_TEST)) {
      ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
      ShowCursor(FALSE);
    } else {
      LOG_WARNING << "Cannot adjust screen resolution to match dimensions of " << width << " x "
                  << height << " pixels for window " << winptr << ".";
    }
  }
}

void MoveWin(win_ptr_t winptr, int x, int y) {
  if (gits::Configurator::Get().common.player.windowMode !=
      gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
    SetLastError(NO_ERROR);
    if (SetWindowPos((HWND)winptr, 0, x, y, 0, 0, SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER) ==
        0) {
      throw std::runtime_error("Window_::set_position failed.\n"
                               "  SetWindowPos returned 0.\n"
                               "  GetLastError() = " +
                               gits::Win32ErrorToString(GetLastError()));
    }
  }
}

void WinVisibility(win_ptr_t winptr, bool show) {
  if (gits::Configurator::Get().common.player.forceInvisibleWindows) {
    show = false;
  }
  ShowWindow(winptr, show ? SW_SHOW : SW_HIDE);
}

void WinTitle(win_ptr_t winptr, const std::string& title) {
  SetLastError(NO_ERROR);
  if (SetWindowText(winptr, title.c_str()) == 0) {
    throw std::runtime_error("Window_::set_title failed.\n"
                             "  SetWindowText returned 0\n"
                             "  GetLastError() = " +
                             gits::Win32ErrorToString(GetLastError()));
  }
}

win_handle_t GetWinHandle(win_ptr_t winptr) {
  return winptr;
}

native_disp_t GetNativeDisplay() {
  return 0;
}

void DestroyPlayerWindowing() {}

#endif

//**************************************** Message Pump *****************************************
MessagePump* MessagePump::current_ = nullptr;
MessagePump::MessagePump() : leave_(false) {
  current_ = this;
}

void MessagePump::stop() {
  leave_ = true;
}

void MessagePump::idle() {}

void MessagePump::key_down(int) {}

MessagePump* MessagePump::get() {
  return current_;
}

MessagePump::~MessagePump() {}

#ifdef GITS_PLATFORM_WINDOWS
//************************************ Windows Message Pump *************************************
void MessagePump::process_messages() {
  for (;;) {
    MSG msg;
    if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
      //message available
      if (GetMessage(&msg, NULL, 0, 0) <= 0) {
        break;
      }

      //TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      idle();
    }

    if (leave_) {
      gits::CGits::Instance().SetPlayerFinish();
      if (gits::Configurator::Get().common.player.windowMode ==
          gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
        ShowCursor(TRUE);
        ChangeDisplaySettings(NULL, 0);
      }
      PostQuitMessage(0);
    }
  }
}

#elif defined GITS_PLATFORM_X11
//************************************   X11 Message Pump   *************************************
static Bool accept_event(Display* display, XEvent* event, XPointer arg) {
  return true;
}

void MessagePump::process_messages() {
  native_disp_t disp = GetNativeDisplay();
  while (true) {
    XEvent event;
    if (gits::CGits::Instance().apis.Has3D() &&
        gits::CGits::Instance().apis.Iface3D().Api() == gits::ApisIface::OpenGL &&
        !gits::Configurator::Get().opengl.player.forceWaylandWindow && disp &&
        XCheckIfEvent((Display*)disp, &event, accept_event, nullptr)) {
      //process events, only KeyPress is requested at window creation time
      if (event.type == KeyPress) {
        if (event.xkey.keycode == 9) { //ESC, TODO: how to get this mapping correctly?
          stop();
        }
      }
    } else {
      idle();
    }
    if (leave_) {
      break;
    }
  }
}
#endif
