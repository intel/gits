// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerWindow.h"

#include "configurator.h"
#include "log.h"
#include "platform.h"
#include "playerWindowX11Shared.h"

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef True
#undef XVisualInfo
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysym.h>

#include <xcb/xcb.h>
#include <unistd.h>
#endif

#include <cstring>
#include <memory>
#include <utility>

namespace gits {
namespace windowing {

namespace {

#if defined GITS_PLATFORM_X11
void FlushXcbConnection(xcb_connection_t* connection, const char* context) {
  const int flushResult = xcb_flush(connection);
  GITS_ASSERT(flushResult >= 0, context);
}

class PlayerWindowXcb final : public PlayerWindow {
public:
  PlayerWindowXcb(int x, int y, int width, int height, bool visible) {
    m_Display = GetPlayerX11Display();
    GITS_ASSERT(m_Display != nullptr, "Failed to open X11 display");
    m_Connection = GetPlayerX11XcbConnection();
    GITS_ASSERT(m_Connection != nullptr, "XCB connection is null");

    const auto internAtom = [this](const char* name, xcb_atom_t* atomOut) -> bool {
      xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_Connection, 0, strlen(name), name);
      xcb_generic_error_t* atomError = nullptr;
      xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(m_Connection, cookie, &atomError);
      if (atomError != nullptr) {
        free(atomError);
      }
      if (reply == nullptr) {
        LOG_WARNING << "Failed to intern XCB atom: " << name;
        return false;
      }
      *atomOut = reply->atom;
      free(reply);
      return true;
    };

    const xcb_setup_t* setup = xcb_get_setup(m_Connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    int scr = DefaultScreen(m_Display);
    while (scr-- > 0) {
      xcb_screen_next(&iter);
    }
    xcb_screen_t* screen = iter.data;

    m_Window = xcb_generate_id(m_Connection);

    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t valueList[2];
    valueList[0] = screen->black_pixel;
    valueList[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
                   XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_void_cookie_t cookie = xcb_create_window_checked(
        m_Connection, XCB_COPY_FROM_PARENT, m_Window, screen->root, x, y, width, height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, valueMask, valueList);

    xcb_generic_error_t* error = xcb_request_check(m_Connection, cookie);
    if (error != nullptr) {
      LOG_ERROR << "Could not create XCB window, X error code "
                << static_cast<int>(error->error_code);
      free(error);
      GITS_ASSERT(false, "Could not create XCB window");
    }

    xcb_change_property(m_Connection, XCB_PROP_MODE_REPLACE, m_Window, XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING, 8, 10, "gitsPlayer");

    const char* netWmPidStr = "_NET_WM_PID";
    xcb_atom_t netWmPidAtom{};
    if (internAtom(netWmPidStr, &netWmPidAtom)) {
      pid_t pid = getpid();
      xcb_change_property(m_Connection, XCB_PROP_MODE_REPLACE, m_Window, netWmPidAtom,
                          XCB_ATOM_CARDINAL, 32, 1, &pid);
    }

    if (!Configurator::Get().common.player.showWindowBorder) {
      const char* mwmHintsStr = "_MOTIF_WM_HINTS";
      xcb_atom_t mwmHintsAtom{};
      if (internAtom(mwmHintsStr, &mwmHintsAtom)) {
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

        xcb_change_property(m_Connection, XCB_PROP_MODE_REPLACE, m_Window, mwmHintsAtom,
                            mwmHintsAtom, 32, 5, &hints);
        FlushXcbConnection(m_Connection, "Failed to flush XCB commands after border configuration");
      }
    }

    m_WmDeleteWindow = XInternAtom(m_Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_Display, m_Window, &m_WmDeleteWindow, 1);

    const bool mapWindow = visible && !Configurator::Get().common.player.forceInvisibleWindows;
    if (mapWindow) {
      cookie = xcb_map_window_checked(m_Connection, m_Window);
      error = xcb_request_check(m_Connection, cookie);
      if (error != nullptr) {
        LOG_ERROR << "Could not map XCB window, X error code "
                  << static_cast<int>(error->error_code);
        free(error);
        GITS_ASSERT(false, "Could not map XCB window");
      }
    }

    FlushXcbConnection(m_Connection, "Failed to flush XCB commands after window creation");

    const int32_t positionValues[] = {x, y};
    xcb_configure_window(m_Connection, m_Window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                         reinterpret_cast<const uint32_t*>(positionValues));
    FlushXcbConnection(m_Connection, "Failed to flush XCB commands after moving window");
  }

  ~PlayerWindowXcb() override {
    if (m_Window != 0) {
      xcb_destroy_window(m_Connection, m_Window);
    }
  }

  std::vector<WindowEvent> Poll() override {
    std::vector<WindowEvent> events;

    if (m_Display == nullptr) {
      return events;
    }

    while (XPending(m_Display) > 0) {
      XEvent event;
      XNextEvent(m_Display, &event);

      if (event.type == KeyPress) {
        const KeySym key = XLookupKeysym(&event.xkey, 0);
        switch (key) {
        case XK_Escape:
          events.push_back(WindowEvent::Stop);
          break;
        case XK_space:
          events.push_back(WindowEvent::TogglePause);
          break;
        case XK_i:
        case XK_I:
          events.push_back(WindowEvent::ToggleInteractive);
          break;
        default:
          break;
        }
      } else if (event.type == ClientMessage) {
        if (static_cast<Atom>(event.xclient.data.l[0]) == m_WmDeleteWindow) {
          events.push_back(WindowEvent::Close);
        }
      }
    }

    return events;
  }

  void Resize(uint32_t width, uint32_t height) override {
    if (m_Connection != nullptr && m_Window != 0) {
      const uint32_t values[] = {width, height};
      xcb_configure_window(m_Connection, m_Window,
                           XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      xcb_flush(m_Connection);
    }
  }

  void SetPosition(int32_t x, int32_t y) override {
    if (m_Connection != nullptr && m_Window != 0) {
      const int32_t values[] = {x, y};
      xcb_configure_window(m_Connection, m_Window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                           reinterpret_cast<const uint32_t*>(values));
      xcb_flush(m_Connection);
    }
  }

  void SetVisibility(bool visible) override {
    if (m_Connection == nullptr || m_Window == 0) {
      return;
    }
    if (Configurator::Get().common.player.forceInvisibleWindows) {
      visible = false;
    }
    if (visible) {
      xcb_map_window(m_Connection, m_Window);
    } else {
      xcb_unmap_window(m_Connection, m_Window);
    }
    xcb_flush(m_Connection);
  }

  void SetTitle(const std::string& title) override {
    if (m_Connection != nullptr && m_Window != 0) {
      xcb_change_property(m_Connection, XCB_PROP_MODE_REPLACE, m_Window, XCB_ATOM_WM_NAME,
                          XCB_ATOM_STRING, 8, strlen(title.c_str()), title.c_str());
      xcb_flush(m_Connection);
    }
  }

  std::pair<uint64_t, uint64_t> NativeHandles() const override {
    return std::make_pair(reinterpret_cast<uint64_t>(m_Connection),
                          static_cast<uint64_t>(m_Window));
  }

private:
  Display* m_Display{};
  xcb_connection_t* m_Connection{};
  xcb_window_t m_Window{};
  Atom m_WmDeleteWindow{};
};
#endif

} // namespace

std::unique_ptr<PlayerWindow> CreatePlayerWindowXcb(
    int x, int y, int width, int height, bool visible) {
#if defined GITS_PLATFORM_X11
  return std::make_unique<PlayerWindowXcb>(x, y, width, height, visible);
#else
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  (void)visible;
  return {};
#endif
}

} // namespace windowing
} // namespace gits
