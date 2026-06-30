// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"
#include "message_pump.h"
#include "configurator.h"
#include "commandsCustom.h"

#ifdef GITS_PLATFORM_LINUX
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#endif

namespace gits {
namespace vulkan {

uint64_t WindowService::SetWindow(uint32_t protocol,
                                  uint64_t handle,
                                  uint64_t instance,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height,
                                  bool visible) {

  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& state = it->second;
    auto& cfg = Configurator::Get().common.player;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;
    if (state.width != wndWidth || state.height != wndHeight) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::WIN) {
        ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::XLIB) {
        ResizeXlibWindow(state.playbackHandle, m_InstanceMap[instance], wndWidth, wndHeight);
      }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::XCB) {
        ResizeXcbWindow(state.playbackHandle, m_InstanceMap[instance], wndWidth, wndHeight);
      }
#endif
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.visible != visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
#endif
    }
    return state.playbackHandle;
  }

  auto& cfg = Configurator::Get().common.player;
  uint32_t wndPosX = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.x : x;
  uint32_t wndPosY = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.y : y;
  uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
  uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

  uint64_t currentHandle{};
  uint64_t currentInstance{};

  switch (protocol) {
  case CreateWindowMetaCommand::DisplayProtocol::WIN: {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HWND currentHWND = CreateWin(wndWidth, wndHeight, wndPosX, wndPosY, visible);
    WinTitle(currentHWND, "Vulkan-GITS");
    HINSTANCE hInstance =
        reinterpret_cast<HINSTANCE>(GetWindowLongPtr(currentHWND, GWLP_HINSTANCE));
    currentHandle = reinterpret_cast<uint64_t>(currentHWND);
    currentInstance = reinterpret_cast<uint64_t>(hInstance);
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::XLIB: {
#ifdef VK_USE_PLATFORM_XLIB_KHR
    auto winParams = CreateXlibWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::XCB: {
#ifdef VK_USE_PLATFORM_XCB_KHR
    auto winParams = CreateXcbWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::WAYLAND: {
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    auto winParams = CreateWaylandWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  }

  m_WindowMap[handle] = {currentHandle, wndWidth, wndHeight, visible};
  m_InstanceMap[instance] = currentInstance;

  return currentHandle;
}

void WindowService::UpdateWindow(uint64_t handle, int32_t width, int32_t height, bool visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& cfg = Configurator::Get().common.player;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

    auto& state = it->second;
    if (state.width != wndWidth || state.height != wndHeight) {
      ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.visible != visible) {
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
    }
  }
#endif
}

uint64_t WindowService::GetCurrentWindowHandle(uint64_t captureWindow) {
  auto it = m_WindowMap.find(captureWindow);
  if (it != m_WindowMap.end()) {
    return it->second.playbackHandle;
  }
  return 0;
}

uint64_t WindowService::GetCurrentInstance(uint64_t captureInstance) {
  auto it = m_InstanceMap.find(captureInstance);
  if (it != m_InstanceMap.end()) {
    return it->second;
  }
  return 0;
}

#ifdef GITS_PLATFORM_LINUX
std::pair<uint64_t, uint64_t> WindowService::CreateXlibWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  XInitThreads();
  Display* display = XOpenDisplay(nullptr);
  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);
  Visual* visual = DefaultVisual(display, screen);
  int depth = DefaultDepth(display, screen);
  Colormap cmap = XCreateColormap(display, root, visual, AllocNone);

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

  visible = true;
  if (visible) {
    XMapWindow(display, window);
  }
  XFlush(display);

  XMoveWindow(display, window, x, y);
  XFlush(display);

  return std::make_pair(reinterpret_cast<uint64_t>(display), reinterpret_cast<uint64_t>(window));
}

void WindowService::ResizeXlibWindow(uint64_t display,
                                     uint64_t window,
                                     uint32_t width,
                                     uint32_t height) {
  Display* dpy = reinterpret_cast<Display*>(display);
  Window win = reinterpret_cast<Window>(window);
  XResizeWindow(dpy, win, width, height);
  XFlush(dpy);
}

std::pair<uint64_t, uint64_t> WindowService::CreateXcbWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  XInitThreads();
  Display* display = XOpenDisplay(nullptr);
  xcb_connection_t* connection = XGetXCBConnection(display);
  const xcb_setup_t* setup = xcb_get_setup(connection);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  int scr = DefaultScreen(display);
  while (scr-- > 0) {
    xcb_screen_next(&iter);
  }
  xcb_screen_t* screen = iter.data;
  xcb_window_t window = xcb_generate_id(connection);

  uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t valueList[32];
  valueList[0] = screen->black_pixel;
  valueList[1] =
      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  xcb_void_cookie_t cookie = xcb_create_window_checked(
      connection, XCB_COPY_FROM_PARENT, window, screen->root, x, y, width, height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, valueMask, valueList);

  xcb_generic_error_t* error = xcb_request_check(connection, cookie);
  GITS_ASSERT(error == nullptr, "Could not create XCB window");
  free(error);

  const char* netWmPidStr = "_NET_WM_PID";
  xcb_intern_atom_cookie_t cookiePid =
      xcb_intern_atom(connection, 0, strlen(netWmPidStr), netWmPidStr);
  xcb_intern_atom_reply_t* replyPid = xcb_intern_atom_reply(connection, cookiePid, nullptr);
  pid_t pid = getpid();
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, replyPid->atom, XCB_ATOM_CARDINAL,
                      32, 1, &pid);
  free(replyPid);

  // Remove border
  const char* mwmHintsStr = "_MOTIF_WM_HINTS";
  xcb_intern_atom_cookie_t cookieHints =
      xcb_intern_atom(connection, 0, strlen(mwmHintsStr), mwmHintsStr);
  xcb_intern_atom_reply_t* replyHints = xcb_intern_atom_reply(connection, cookieHints, nullptr);
  constexpr uint32_t MWM_HINTS_DECORATIONS = 1L << 1;
  struct MotifHints {
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
  };
  MotifHints hints{};
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = 0;

  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, replyHints->atom, replyHints->atom,
                      32, 5, &hints);
  free(replyHints);
  int flushResult = xcb_flush(connection);
  GITS_ASSERT(flushResult > 0, "Failed to flush XCB commands after removing window border");

  visible = true;
  if (visible) {
    cookie = xcb_map_window_checked(connection, window);
    error = xcb_request_check(connection, cookie);
    GITS_ASSERT(error == nullptr, "Could not map XCB window");
    free(error);
  }

  flushResult = xcb_flush(connection);
  GITS_ASSERT(flushResult > 0, "Failed to flush XCB commands after window creation");

  const int32_t positionValues[] = {x, y};
  xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                       positionValues);
  flushResult = xcb_flush(connection);
  GITS_ASSERT(flushResult > 0, "Failed to flush XCB commands after moving window");

  return std::make_pair(reinterpret_cast<uint64_t>(connection), static_cast<uint64_t>(window));
}

void WindowService::ResizeXcbWindow(uint64_t connection,
                                    uint64_t window,
                                    uint32_t width,
                                    uint32_t height) {
  xcb_connection_t* conn = reinterpret_cast<xcb_connection_t*>(connection);
  xcb_window_t win = static_cast<xcb_window_t>(window);
  const uint32_t values[] = {width, height};
  xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
  xcb_flush(conn);
}

std::pair<uint64_t, uint64_t> WindowService::CreateWaylandWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  static wl_display* display = nullptr;
  static wl_compositor* compositor = nullptr;
  static xdg_wm_base* wmBase = nullptr;

  if (display == nullptr) {
    display = wl_display_connect(nullptr);
    GITS_ASSERT(display != nullptr, "Failed to connect to Wayland display");

    static const wl_registry_listener registryListener = {
        [](void* data, wl_registry* registry, uint32_t name, const char* interface,
           uint32_t version) {
          if (strcmp(interface, wl_compositor_interface.name) == 0) {
            compositor = static_cast<wl_compositor*>(
                wl_registry_bind(registry, name, &wl_compositor_interface, 1));
          } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
            wmBase = static_cast<xdg_wm_base*>(
                wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
          }
        },
        [](void*, wl_registry*, uint32_t) {},
    };

    wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, nullptr);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);

    GITS_ASSERT(compositor != nullptr, "Failed to bind wl_compositor");
    GITS_ASSERT(wmBase != nullptr, "Failed to bind xdg_wm_base");

    static const xdg_wm_base_listener wmBaseListener = {
        [](void*, xdg_wm_base* base, uint32_t serial) { xdg_wm_base_pong(base, serial); },
    };
    xdg_wm_base_add_listener(wmBase, &wmBaseListener, nullptr);
  }

  wl_surface* surface = wl_compositor_create_surface(compositor);
  GITS_ASSERT(surface != nullptr, "Failed to create Wayland surface");

  xdg_surface* xdgSurface = xdg_wm_base_get_xdg_surface(wmBase, surface);
  GITS_ASSERT(xdgSurface != nullptr, "Failed to create xdg_surface");

  static const xdg_surface_listener xdgSurfaceListener = {
      [](void*, xdg_surface* xdgSurf, uint32_t serial) {
        xdg_surface_ack_configure(xdgSurf, serial);
      },
  };
  xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, nullptr);

  xdg_toplevel* toplevel = xdg_surface_get_toplevel(xdgSurface);
  GITS_ASSERT(toplevel != nullptr, "Failed to create xdg_toplevel");
  xdg_toplevel_set_title(toplevel, "Vulkan-GITS");

  wl_surface_commit(surface);
  wl_display_roundtrip(display);

  return std::make_pair(reinterpret_cast<uint64_t>(display), reinterpret_cast<uint64_t>(surface));
}
#endif

} // namespace vulkan
} // namespace gits
