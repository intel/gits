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

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#undef True
#undef XVisualInfo
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

#include <memory>

namespace gits {
namespace windowing {

namespace {

#if defined GITS_PLATFORM_X11
class PlayerWindowXlib final : public PlayerWindow {
public:
  struct MotifWmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
  };

  PlayerWindowXlib(int x, int y, int width, int height, bool visible) {
    XInitThreads();

    m_Display = XOpenDisplay(nullptr);
    GITS_ASSERT(m_Display != nullptr, "Failed to open Xlib display");

    int screen = DefaultScreen(m_Display);
    Window root = RootWindow(m_Display, screen);
    Visual* visual = DefaultVisual(m_Display, screen);
    int depth = DefaultDepth(m_Display, screen);
    Colormap cmap = XCreateColormap(m_Display, root, visual, AllocNone);

    const bool borderless = !Configurator::Get().common.player.showWindowBorder;

    XSetWindowAttributes attributes;
    attributes.background_pixel = XWhitePixel(m_Display, screen);
    attributes.border_pixel = 0;
    attributes.colormap = cmap;
    attributes.event_mask = KeyPressMask | StructureNotifyMask;

    unsigned long attributeMask = CWBorderPixel | CWColormap | CWBackPixel | CWEventMask;

    m_Window = XCreateWindow(m_Display, root, x, y, width, height,
                             0, // border_width
                             depth, InputOutput, visual, attributeMask, &attributes);

    m_WmDeleteWindow = XInternAtom(m_Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_Display, m_Window, &m_WmDeleteWindow, 1);

    if (borderless) {
      constexpr unsigned long MWM_HINTS_DECORATIONS = 1UL << 1;
      const Atom motifHintsAtom = XInternAtom(m_Display, "_MOTIF_WM_HINTS", False);
      MotifWmHints hints{};
      hints.flags = MWM_HINTS_DECORATIONS;
      hints.decorations = 0;
      XChangeProperty(m_Display, m_Window, motifHintsAtom, motifHintsAtom, 32, PropModeReplace,
                      reinterpret_cast<const unsigned char*>(&hints), 5);
    }

    if (visible) {
      XMapWindow(m_Display, m_Window);
    }
    XFlush(m_Display);

    XMoveWindow(m_Display, m_Window, x, y);
    XFlush(m_Display);
  }

  ~PlayerWindowXlib() override {
    if (m_Display != nullptr) {
      if (m_Window != 0) {
        XDestroyWindow(m_Display, m_Window);
      }
      XCloseDisplay(m_Display);
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
    if (m_Display != nullptr && m_Window != 0) {
      XResizeWindow(m_Display, m_Window, width, height);
      XFlush(m_Display);
    }
  }

  void SetTitle(const std::string& title) override {
    if (m_Display != nullptr && m_Window != 0) {
      XStoreName(m_Display, m_Window, title.c_str());
      XFlush(m_Display);
    }
  }

  std::pair<uint64_t, uint64_t> NativeHandles() const override {
    return std::make_pair(reinterpret_cast<uint64_t>(m_Display),
                          reinterpret_cast<uint64_t>(m_Window));
  }

private:
  Display* m_Display{};
  Window m_Window{};
  Atom m_WmDeleteWindow{};
};
#endif

} // namespace

std::unique_ptr<PlayerWindow> CreatePlayerWindowXlib(
    int x, int y, int width, int height, bool visible) {
#if defined GITS_PLATFORM_X11
  return std::make_unique<PlayerWindowXlib>(x, y, width, height, visible);
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
