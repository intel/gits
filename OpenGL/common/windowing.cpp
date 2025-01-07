// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openglDrivers.h"
#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "windowing.h"
#include "config.h"
#include "gits.h"
#include "pragmas.h"
#include "message_pump.h"
#include "openglEnums.h"

#ifdef BUILD_FOR_CCODE
#include "fake_ptbl.h"
#else
#include "ptblLibrary.h"
#endif

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <utility>
#include <sstream>
#include <iomanip>

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <GL/glx.h>
#undef True
#undef XVisualInfo
#endif

EGLDisplay GetEGLDisplay() {
  EGLDisplay eglDisp =
      gits::OpenGL::drv.egl.eglGetDisplay((EGLNativeDisplayType)GetNativeDisplay());
  return eglDisp;
}

#ifdef GITS_PLATFORM_WINDOWS
//********************************* Windows Player Window ***********************************

win_ptr_t CreateWin(
    GLXFBConfig pf, XVisualInfo* xinfo, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWin(width, height, x, y, show);
}

win_ptr_t CreateWin(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWin(width, height, x, y, show);
}

#elif defined GITS_PLATFORM_X11
//********************************* X11/Wayland Player Window ***********************************

// X11
win_ptr_t CreateWinX11(GLint width, GLint height, GLint x, GLint y, bool show) {
  // On linux window can not be created without pixel format
  throw std::runtime_error(EXCEPTION_MESSAGE);
}

win_ptr_t CreateWinX11Impl(
    XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show) {
  int defScreen = DefaultScreen((Display*)GetNativeDisplay());
  Window root = RootWindow((Display*)GetNativeDisplay(), defScreen);
  Colormap cmap = 0;
  Window win = (Window)0;
  if (vi != nullptr) {
    // Visual info is redefined to XVisualInfo_, to allow for forward
    // declaration for the rest of GITS code - this is due to X defining the type
    // as a typedef for an anonymouse struct.
#define XVisualInfo XVisualInfo_
    XVisualInfo* vi_orig = (XVisualInfo*)vi;
    cmap = XCreateColormap((Display*)GetNativeDisplay(), root, vi_orig->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.border_pixel = 0;
    swa.colormap = cmap;
    swa.event_mask = KeyPressMask;
    swa.override_redirect = true;
    win = XCreateWindow((Display*)GetNativeDisplay(), root, x, y, width, height, 0, vi_orig->depth,
                        InputOutput, vi_orig->visual,
                        CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &swa);
#undef XVisualInfo
  }
  if (show) {
    XMapWindow((Display*)GetNativeDisplay(), win);
  }
  return win;
}

win_ptr_t CreateWinX11(
    GLXFBConfig pf, XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWinX11Impl(vi, width, height, x, y, show);
}
win_ptr_t CreateWinX11(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  EGLint visual_id = 0;
  gits::OpenGL::drv.egl.eglGetConfigAttrib(GetEGLDisplay(), (EGLConfig)pf, EGL_NATIVE_VISUAL_ID,
                                           &visual_id);
  // Visual info is redefined to XVisualInfo_, to allow for forward declaration
  // for the rest of GITS code - this is due to X defining the type as a typedef
  // for an anonymous struct.
#define XVisualInfo XVisualInfo_
  int nitems = 0;
  XVisualInfo vitempl;
  vitempl.visualid = visual_id;
  XVisualInfo* vi = XGetVisualInfo((Display*)GetNativeDisplay(), VisualIDMask, &vitempl, &nitems);
#undef XVisualInfo
  return CreateWinX11Impl((XVisualInfo*)vi, width, height, x, y, show);
}

void RemoveWinX11(win_ptr_t winptr) {
  XDestroyWindow((Display*)GetNativeDisplay(), winptr);
}

void ResizeWinX11(win_ptr_t winptr, int width, int height) {
  XResizeWindow((Display*)GetNativeDisplay(), winptr, width, height);
}

void MoveWinX11(win_ptr_t winptr, int x, int y) {
  XMoveWindow((Display*)GetNativeDisplay(), winptr, x, y);
}

void WinVisibilityX11(win_ptr_t winptr, bool show) {}

void WinTitleX11(win_ptr_t winptr, const std::string& title) {
  if (XStoreName((Display*)GetNativeDisplay(), winptr, title.c_str()) == 0) {
    throw std::runtime_error("Window_::set_title failed.\n"
                             "  XStoreName returned 0");
  }
}

win_handle_t GetWinHandleX11(win_ptr_t winptr) {
  return winptr;
}

native_disp_t GetNativeDisplayX11() {
  static Display* dpy = XOpenDisplay(nullptr);
  return (native_disp_t)dpy;
}

// Wayland

wl_compositor* wlCompositor = nullptr;
wl_shell* wlShell = nullptr;

struct wlWinData {
  wl_surface* surf;
  wl_shell_surface* shellSurf;
  int x;
  int y;
  int w;
  int h;
};

std::map<wl_egl_window*, wlWinData> wlWinToSurf;

void wlRegistryAdd(void* data,
                   struct wl_registry* registry,
                   uint32_t name,
                   const char* interface,
                   uint32_t version) {
  if (strcmp(interface, "wl_compositor") == 0) {
    wlCompositor = reinterpret_cast<wl_compositor*>(
        wl_registry_bind(registry, name, &wl_compositor_interface, 1));
  } else if (strcmp(interface, "wl_shell") == 0) {
    wlShell = reinterpret_cast<wl_shell*>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
  }
}

void wlRegistryRemove(void* data, struct wl_registry* registry, uint32_t empty) {
  // Intentionally empty.
}

static const struct wl_registry_listener registryListener = {wlRegistryAdd, wlRegistryRemove};

void InitializeWaylandServer() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  wl_display* dpy = (wl_display*)GetNativeDisplay();
  wl_registry* registry = wl_display_get_registry((wl_display*)dpy);
  wl_registry_add_listener(registry, &registryListener, nullptr);
  wl_display_dispatch(dpy);
  wl_display_roundtrip(dpy);
  if (wlCompositor == nullptr || wlShell == nullptr) {
    Log(ERR) << "Wayland server init failed. wl_compositor: " << wlCompositor
             << ", wl_shell: " << wlShell;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  initialized = true;
}

win_ptr_t CreateWinWaylandEglImpl(GLint width, GLint height, GLint x, GLint y) {
  InitializeWaylandServer();
  wl_surface* surf = wl_compositor_create_surface(wlCompositor);
  if (surf == nullptr) {
    Log(ERR) << "Wayland surface creation failed.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  wl_shell_surface* shellSurf = wl_shell_get_shell_surface(wlShell, surf);
  wl_shell_surface_set_toplevel(shellSurf);

  wl_egl_window* win = wl_egl_window_create(surf, width, height);
  if (win == nullptr) {
    Log(ERR) << "Wayland window creation failed.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  wl_egl_window_resize(win, width, height, x, y);
  wlWinToSurf[win].surf = surf;
  wlWinToSurf[win].shellSurf = shellSurf;
  wlWinToSurf[win].x = x;
  wlWinToSurf[win].y = y;
  wlWinToSurf[win].w = width;
  wlWinToSurf[win].h = height;
  return (win_ptr_t)win;
}

win_ptr_t CreateWinWaylandEgl(GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWinWaylandEglImpl(width, height, x, y);
}

win_ptr_t CreateWinWaylandEgl(
    GLXFBConfig pf, XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWinWaylandEglImpl(width, height, x, y);
}

win_ptr_t CreateWinWaylandEgl(
    EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWinWaylandEglImpl(width, height, x, y);
}

void RemoveWinWaylandEgl(win_ptr_t winptr) {
  wl_egl_window* winptrcast = (wl_egl_window*)winptr;
  wl_egl_window_destroy(winptrcast);
  wl_shell_surface_destroy(wlWinToSurf[winptrcast].shellSurf);
  wl_surface_destroy(wlWinToSurf[winptrcast].surf);
}

void ResizeWinWaylandEgl(win_ptr_t winptr, int width, int height) {
  wl_egl_window* winptrcast = (wl_egl_window*)winptr;
  wlWinToSurf[winptrcast].w = width;
  wlWinToSurf[winptrcast].h = height;
  wl_egl_window_resize(winptrcast, width, height, wlWinToSurf[winptrcast].x,
                       wlWinToSurf[winptrcast].y);
}

void MoveWinWaylandEgl(win_ptr_t winptr, int x, int y) {
  wl_egl_window* winptrcast = (wl_egl_window*)winptr;
  wlWinToSurf[winptrcast].x = x;
  wlWinToSurf[winptrcast].y = y;
  wl_egl_window_resize(winptrcast, wlWinToSurf[winptrcast].w, wlWinToSurf[winptrcast].h, x, y);
}

void WinVisibilityWaylandEgl(win_ptr_t winptr, bool show) {}

void WinTitleWaylandEgl(win_ptr_t winptr, const std::string& title) {}

win_handle_t GetWinHandleWaylandEgl(win_ptr_t winptr) {
  return winptr;
}

native_disp_t GetNativeDisplayWaylandEgl() {
  static wl_display* dpy = wl_display_connect((char*)nullptr);
  if (dpy == nullptr) {
    Log(ERR) << "Wayland display connection failed.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  return (native_disp_t)dpy;
}

win_ptr_t CreateWin(GLint width, GLint height, GLint x, GLint y, bool show) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return CreateWinWaylandEgl(width, height, x, y, show);
  } else {
    return CreateWinX11(width, height, x, y, show);
  }
}

win_ptr_t CreateWin(
    GLXFBConfig pf, XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return CreateWinWaylandEgl(pf, vi, width, height, x, y, show);
  } else {
    return CreateWinX11(pf, vi, width, height, x, y, show);
  }
}
win_ptr_t CreateWin(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return CreateWinWaylandEgl(pf, width, height, x, y, show);
  } else {
    return CreateWinX11(pf, width, height, x, y, show);
  }
}

void RemoveWin(win_ptr_t winptr) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return RemoveWinWaylandEgl(winptr);
  } else {
    return RemoveWinX11(winptr);
  }
}

void ResizeWin(win_ptr_t winptr, int width, int height) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return ResizeWinWaylandEgl(winptr, width, height);
  } else {
    return ResizeWinX11(winptr, width, height);
  }
}

void MoveWin(win_ptr_t winptr, int x, int y) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return MoveWinWaylandEgl(winptr, x, y);
  } else {
    return MoveWinX11(winptr, x, y);
  }
}

void WinVisibility(win_ptr_t winptr, bool show) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return WinVisibilityWaylandEgl(winptr, show);
  } else {
    return WinVisibilityX11(winptr, show);
  }
}

void WinTitle(win_ptr_t winptr, const std::string& title) {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    WinTitleWaylandEgl(winptr, title);
  } else {
    WinTitleX11(winptr, title);
  }
}

win_handle_t GetWinHandle(win_ptr_t winptr) {
  return winptr;
}

native_disp_t GetNativeDisplay() {
  if (gits::Config::Get().opengl.player.forceWaylandWindow) {
    return GetNativeDisplayWaylandEgl();
  } else {
    return GetNativeDisplayX11();
  }
}

void DestroyPlayerWindowing() {}

#endif

Window_::Window_(GLint width, GLint height, GLint x, GLint y, bool show)
    : _window(gits::ptblCreateWin(width, height, x, y, show)) {}

Window_::Window_(
    GLXFBConfig pf, XVisualInfo* vi, GLint width, GLint height, GLint x, GLint y, bool show)
    : _window(gits::ptblCreateWin(pf, vi, width, height, x, y, show)) {}

Window_::Window_(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show)
    : _window(gits::ptblCreateWin(pf, width, height, x, y, show)) {}

Window_::~Window_() {
  try {
    gits::ptblRemoveWin(_window);
  } catch (...) {
    topmost_exception_handler("Window_::~Window_");
  }
}

void Window_::set_position(int x, int y) {
  gits::ptblMoveWin(_window, x, y);
}

void Window_::set_size(int width, int height) {
  gits::ptblResizeWin(_window, width, height);
}

void Window_::set_title(const std::string& title) {
  gits::ptblWinTitle(_window, title);
}

void Window_::set_visibility(bool show) {
  gits::ptblWinVisibility(_window, show);
}

win_handle_t Window_::handle() const {
  return gits::ptblGetWinHandle(_window);
}

#ifdef GITS_PLATFORM_WINDOWS
//********************************* Windows Recorder Window *********************************
window_handle::window_handle(win_ptr_t window) : window_(window) {}

std::pair<int, int> window_handle::get_pos() const {
  std::pair<int, int> pos;
  RECT clientrect;
  GetClientRect((HWND)window_, &clientrect);
  RECT windowrect;
  GetWindowRect((HWND)window_, &windowrect);
  pos.first = windowrect.left + clientrect.left;
  pos.second = windowrect.top + clientrect.top;

  return pos;
}

std::pair<int, int> window_handle::get_size() const {
  std::pair<int, int> size;
  RECT clientrect;
  GetClientRect((HWND)window_, &clientrect);
  size.first = abs(clientrect.right - clientrect.left);
  size.second = abs(clientrect.bottom - clientrect.top);

  return size;
}

void window_handle::get_dimensions(int& x, int& y, int& width, int& height) const {
  RECT clientrect;
  GetClientRect((HWND)window_, &clientrect);
  RECT windowrect;
  GetWindowRect((HWND)window_, &windowrect);
  x = windowrect.left + clientrect.left;
  y = windowrect.top + clientrect.top;
  width = abs(clientrect.right - clientrect.left);
  height = abs(clientrect.bottom - clientrect.top);
}

bool window_handle::is_visible() const {
  return IsWindowVisible((HWND)window_);
}

#elif defined GITS_PLATFORM_X11
//********************************* X11 Recorder Window *********************************
window_handle::window_handle(win_ptr_t window) : window_(window) {}

std::pair<int, int> window_handle::get_pos() const {
  Display* dpy = gits::OpenGL::drv.glx.glXGetCurrentDisplay();
  GLXDrawable drawable = window_;
  XWindowAttributes xwAttr;
  Status ret = XGetWindowAttributes(dpy, (Window)drawable, &xwAttr);
  if (ret == BadDrawable) {
    throw std::runtime_error(
        "window_handle::get_pos failed: XGetWindowAttributes returned BadDrawable.\n");
  } else if (ret == BadWindow) {
    throw std::runtime_error(
        "window_handle::get_pos failed: XGetWindowAttributes returned BadWindow.\n");
  }

  return {xwAttr.x, xwAttr.y};
}

std::pair<int, int> window_handle::get_size() const {
  Display* dpy = gits::OpenGL::drv.glx.glXGetCurrentDisplay();
  GLXDrawable drawable = window_;
  XWindowAttributes xwAttr;
  Status ret = XGetWindowAttributes(dpy, (Window)drawable, &xwAttr);
  if (ret == BadDrawable) {
    throw std::runtime_error(
        "window_handle::get_pos failed: XGetWindowAttributes returned BadDrawable.\n");
  } else if (ret == BadWindow) {
    throw std::runtime_error(
        "window_handle::get_pos failed: XGetWindowAttributes returned BadWindow.\n");
  }

  return {xwAttr.width, xwAttr.height};
}

void window_handle::get_dimensions(int& x, int& y, int& width, int& height) const {
  std::pair<int, int> pos = get_pos();
  std::pair<int, int> size = get_size();

  x = pos.first;
  y = pos.second;
  width = size.first;
  height = size.second;
}

bool window_handle::is_visible() const {
#pragma message("TODO window_handle linux implementation")
  return true;
}
#endif
