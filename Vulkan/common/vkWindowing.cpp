// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "vkWindowing.h"

#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#elif defined GITS_PLATFORM_X11
#include <unistd.h>
#define XVisualInfo XVisualInfo_
#undef XVisualInfo

#include <xcb/xcb.h>
#include <cstring>
#include "log.h"
#include "gits.h"
#endif

namespace gits {
namespace Vulkan {

#if defined GITS_PLATFORM_X11

Display* GetNativeDisplayXLIB_() {
  XInitThreads();
  return XOpenDisplay(nullptr);
}

native_disp_t GetNativeDisplayXCB() {
  static Display* dpy = XOpenDisplay(nullptr);
  return XGetXCBConnection(dpy);
}

native_disp_t GetNativeDisplay() {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return GetNativeDisplayXLIB_();
  } else {
    return GetNativeDisplayXCB();
  }
}

void RemoveWin(win_ptr_t winptr) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();
  xcb_destroy_window(connection, (xcb_window_t)winptr);
  xcb_flush(connection);
}

void ResizeWinXCB(win_ptr_t winptr, int width, int height) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();
  const uint32_t values[] = {(uint32_t)width, (uint32_t)height};

  xcb_configure_window(connection, (xcb_window_t)winptr,
                       XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);

  xcb_flush(connection);
}

void ResizeWinXLIB(win_ptr_t winptr, int width, int height) {
  Display* display = (Display*)GetNativeDisplay();
  XResizeWindow(display, winptr, width, height);
  XFlush(display);
}

void ResizeWin(win_ptr_t winptr, int width, int height) {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return ResizeWinXLIB(winptr, width, height);
  } else {
    return ResizeWinXCB(winptr, width, height);
  }
}

void MoveWinXCB(win_ptr_t winptr, int x, int y) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();
  const int32_t values[] = {x, y};

  xcb_configure_window(connection, (xcb_window_t)winptr, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                       (const uint32_t*)values);

  xcb_flush(connection);
}

void MoveWinXLIB(win_ptr_t winptr, int x, int y) {
  Display* display = (Display*)GetNativeDisplay();
  XMoveWindow(display, winptr, x, y);
  XFlush(display);
}

void MoveWin(win_ptr_t winptr, int x, int y) {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return MoveWinXLIB(winptr, x, y);
  } else {
    return MoveWinXCB(winptr, x, y);
  }
}

void WinVisibilityXCB(win_ptr_t winptr, bool show) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();
  if (show) {
    xcb_map_window(connection, (xcb_window_t)winptr);
  } else {
    xcb_unmap_window(connection, (xcb_window_t)winptr);
  }
  xcb_flush(connection);
}

void WinVisibilityXLIB(win_ptr_t winptr, bool show) {
  Display* display = (Display*)GetNativeDisplay();
  if (show) {
    XMapWindow(display, winptr);
  } else {
    XUnmapWindow(display, winptr);
  }
  XFlush(display);
}

void WinVisibility(win_ptr_t winptr, bool show) {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return WinVisibilityXLIB(winptr, show);
  } else {
    return WinVisibilityXCB(winptr, show);
  }
}

void WinTitleXCB(win_ptr_t winptr, const std::string& title) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();

  xcb_change_property(connection,            // connection
                      XCB_PROP_MODE_REPLACE, // mode (append, replace, etc.)
                      (xcb_window_t)winptr,  // window
                      XCB_ATOM_WM_NAME,      // property (what do you want to change)
                      XCB_ATOM_STRING,       // type of the property
                      8,                     // format (how many bits one data element has)
                      strlen(title.c_str()), // data length (how many elements)
                      title.c_str());        // data

  xcb_flush(connection);
}

void WinTitleXLIB(win_ptr_t winptr, const std::string& title) {
  if (XStoreName((Display*)GetNativeDisplay(), winptr, title.c_str()) == 0) {
    throw std::runtime_error("Window_::set_title failed.\n"
                             "  XStoreName returned 0");
  }
  XFlush((Display*)GetNativeDisplay());
}

void WinTitle(win_ptr_t winptr, const std::string& title) {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return WinTitleXLIB(winptr, title);
  } else {
    return WinTitleXCB(winptr, title);
  }
}

void RemoveWinBorder(win_ptr_t winptr) {
  if (Configurator::Get().common.player.showWindowBorder) {
    return;
  }

  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();

  const char* mwm_hints_str = "_MOTIF_WM_HINTS";
  // Request atom by name.
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(connection,
                      0,                     // return valid atom id if atom exists? 1=no 0=yes
                      strlen(mwm_hints_str), // atom name length
                      mwm_hints_str);        // atom name
  // Receive the requested atom.
  xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie,
                                                         nullptr); // pointer to pointer to error

#define MWM_HINTS_DECORATIONS (1L << 1)
  struct MotifHints {
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
  } hints = {}; // Value-initialize members so they are 0.

  hints.flags = MWM_HINTS_DECORATIONS; // We will be changing decorations.
  hints.decorations = 0;               // Set all of the decorations bitflags to false.

  xcb_change_property(connection,            // connection
                      XCB_PROP_MODE_REPLACE, // mode (append, replace, etc.)
                      (xcb_window_t)winptr,  // window
                      reply->atom,           // property (what do you want to change)
                      reply->atom,           // type of the property
                      32,                    // format (how many bits one data element has)
                      5,                     // data length (5 elements of 32 bits each)
                      &hints);               // data (Motif WM hints struct in this case)

  free(reply);

  xcb_flush(connection);
}

win_ptr_t CreateWinXCB(int width, int height, int x, int y, bool show) {
  xcb_connection_t* connection = (xcb_connection_t*)GetNativeDisplay();
  const xcb_setup_t* setup = xcb_get_setup(connection);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  int scr = DefaultScreen((Display*)GetNativeDisplayXLIB_());

  while (scr-- > 0) {
    xcb_screen_next(&iter);
  }

  xcb_screen_t* screen = iter.data;

  xcb_window_t window = xcb_generate_id(connection);

  uint32_t value_mask, value_list[32];
  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen->black_pixel;
  value_list[1] =
      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  xcb_void_cookie_t cookie;
  xcb_generic_error_t* error;

  cookie = xcb_create_window_checked(connection,                    // connection
                                     XCB_COPY_FROM_PARENT,          // depth
                                     window,                        // window id
                                     screen->root,                  // parent window
                                     x,                             // x
                                     y,                             // y
                                     width,                         // width
                                     height,                        // height
                                     0,                             // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT, // window class
                                     screen->root_visual,           // visual
                                     value_mask,  // value mask (which values will be on the list)
                                     value_list); // value list
  if ((error = xcb_request_check(connection, cookie))) {
    Log(ERR) << "Could not create a window.";
    free(error);
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // Set pid property to window
  const char* net_wm_pid_str = "_NET_WM_PID";
  xcb_intern_atom_cookie_t cookie_pid =
      xcb_intern_atom(connection, 0, strlen(net_wm_pid_str), net_wm_pid_str);
  xcb_intern_atom_reply_t* reply_pid = xcb_intern_atom_reply(connection, cookie_pid, nullptr);
  pid_t pid = getpid();
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, reply_pid->atom, XCB_ATOM_CARDINAL,
                      32, 1, &pid);
  free(reply_pid);

  RemoveWinBorder(window);

  cookie = xcb_map_window_checked(connection, window);
  if ((error = xcb_request_check(connection, cookie))) {
    Log(ERR) << "Could not map a window.";
    free(error);
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  if (xcb_flush(connection) <= 0) {
    Log(WARN) << "Could not flush commands to X server after window creation. "
                 "This might potentially cause some windowing-related issues.";
  }

  // Some window managers ignore window position specified during window
  // creation but respect position specified in window property changes. That's
  // why we need the artificial move to ensure correct location of the window.
  MoveWin(window, x, y);

  return window;
}

win_ptr_t CreateWinXLIB(int width, int height, int x, int y, bool show) {
  int screen;
  Display* display;
  Visual* visual;
  Colormap cmap = 0;
  int depth;
  display = (Display*)GetNativeDisplay();
  screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);
  visual = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);
  cmap = XCreateColormap(display, root, visual, AllocNone);
  XSetWindowAttributes attributes;
  attributes.background_pixel = XWhitePixel(display, screen);
  attributes.border_pixel = 0;
  attributes.colormap = cmap;
  attributes.override_redirect = true;
  attributes.event_mask = KeyPressMask;

  Window window = XCreateWindow(
      display, root, x, y, width, height,
      0, // border_width
      depth, InputOutput, visual,
      CWBorderPixel | CWColormap | CWBackPixel | CWEventMask | CWOverrideRedirect, &attributes);

  XMapWindow(display, window);
  XFlush(display);
  // Some window managers ignore window position specified during window
  // creation but respect position specified in window property changes. That's
  // why we need the artificial move to ensure correct location of the window.
  MoveWin(window, x, y);
  return window;
}

win_ptr_t CreateWin(int width, int height, int x, int y, bool show) {
  if (_displayProtocol == DisplayProtocol::XLIB) {
    return CreateWinXLIB(width, height, x, y, show);
  } else {
    return CreateWinXCB(width, height, x, y, show);
  }
}

win_handle_t GetWinHandle(win_ptr_t win) {
  return win;
}

#endif

Window_::Window_(int width, int height, int x, int y, bool show, DisplayProtocol dispProtocol) {
  _displayProtocol = dispProtocol;
  _window = CreateWin(width, height, x, y, show);
}
Window_::~Window_() {
  RemoveWin(_window);
}

void Window_::set_position(int x, int y) {
  MoveWin(_window, x, y);
}

void Window_::set_size(int width, int height) {
  ResizeWin(_window, width, height);
}

void Window_::set_title(const std::string& title) {
  WinTitle(_window, title);
}

void Window_::set_visibility(bool show) {
  WinVisibility(_window, show);
}

win_handle_t Window_::handle() const {
  return GetWinHandle(_window);
}

native_disp_t Window_::native_display() {
  return GetNativeDisplay();
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

xcb_handle::xcb_handle(xcb_connection_t* connection, xcb_window_t window)
    : connection_(connection), window_(window) {}

void xcb_handle::get_dimensions(int& x, int& y, int& width, int& height) const {
  xcb_get_geometry_cookie_t geom_cookie;
  xcb_generic_error_t* err = nullptr;
  xcb_get_geometry_reply_t* geom;

  geom_cookie = xcb_get_geometry(connection_, window_);
  geom = xcb_get_geometry_reply(connection_, geom_cookie, &err);
  if (geom) {
    width = geom->width;
    height = geom->height;
  } else {
    width = height = 0;
  }
  free(err);
  free(geom);

  x = y = 0;
}

bool xcb_handle::is_visible() const {
#pragma message("TODO xcb_handle linux implementation")
  return true;
}

xlib_handle::xlib_handle(Display* display, Window window) : display_(display), window_(window) {}

void xlib_handle::get_dimensions(int& x, int& y, int& width, int& height) const {
  XWindowAttributes xwAttr;
  XGetWindowAttributes(display_, window_, &xwAttr);
  width = xwAttr.width;
  height = xwAttr.height;

  x = xwAttr.x;
  y = xwAttr.y;
}

bool xlib_handle::is_visible() const {
#pragma message("TODO xlib_handle linux implementation")
  return true;
}

#endif

} // namespace Vulkan
} // namespace gits
