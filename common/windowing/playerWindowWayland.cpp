// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerWindow.h"

#include "log.h"
#include "platform.h"

#if defined GITS_PLATFORM_X11
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"

#include <cstring>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <memory>

namespace gits {
namespace windowing {

namespace {

#if defined GITS_PLATFORM_X11
class PlayerWindowWayland final : public PlayerWindow {
public:
  PlayerWindowWayland(int x, int y, int width, int height, bool visible) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)visible;

    m_Display = wl_display_connect(nullptr);
    GITS_ASSERT(m_Display != nullptr, "Failed to connect to Wayland display");

    m_Registry = wl_display_get_registry(m_Display);
    GITS_ASSERT(m_Registry != nullptr, "Failed to get Wayland registry");

    static const wl_registry_listener registryListener = {
        RegistryGlobal,
        RegistryGlobalRemove,
    };

    wl_registry_add_listener(m_Registry, &registryListener, this);
    wl_display_roundtrip(m_Display);
    wl_display_roundtrip(m_Display);

    GITS_ASSERT(m_Compositor != nullptr, "Failed to bind wl_compositor");
    GITS_ASSERT(m_WmBase != nullptr, "Failed to bind xdg_wm_base");

    static const xdg_wm_base_listener wmBaseListener = {
        WmBasePing,
    };
    xdg_wm_base_add_listener(m_WmBase, &wmBaseListener, this);

    m_Surface = wl_compositor_create_surface(m_Compositor);
    GITS_ASSERT(m_Surface != nullptr, "Failed to create Wayland surface");

    m_XdgSurface = xdg_wm_base_get_xdg_surface(m_WmBase, m_Surface);
    GITS_ASSERT(m_XdgSurface != nullptr, "Failed to create xdg_surface");

    static const xdg_surface_listener xdgSurfaceListener = {
        XdgSurfaceConfigure,
    };
    xdg_surface_add_listener(m_XdgSurface, &xdgSurfaceListener, this);

    m_Toplevel = xdg_surface_get_toplevel(m_XdgSurface);
    GITS_ASSERT(m_Toplevel != nullptr, "Failed to create xdg_toplevel");

    static const xdg_toplevel_listener toplevelListener = {
        ToplevelConfigure,
        ToplevelClose,
        ToplevelConfigureBounds,
        ToplevelWmCapabilities,
    };
    xdg_toplevel_add_listener(m_Toplevel, &toplevelListener, this);
    xdg_toplevel_set_title(m_Toplevel, "Vulkan-GITS");

    wl_surface_commit(m_Surface);
    wl_display_roundtrip(m_Display);
  }

  ~PlayerWindowWayland() override {
    if (m_Keyboard != nullptr) {
      wl_keyboard_destroy(m_Keyboard);
    }

    if (m_Seat != nullptr) {
      wl_seat_destroy(m_Seat);
    }

    if (m_Toplevel != nullptr) {
      xdg_toplevel_destroy(m_Toplevel);
    }

    if (m_XdgSurface != nullptr) {
      xdg_surface_destroy(m_XdgSurface);
    }

    if (m_Surface != nullptr) {
      wl_surface_destroy(m_Surface);
    }

    if (m_WmBase != nullptr) {
      xdg_wm_base_destroy(m_WmBase);
    }

    if (m_Compositor != nullptr) {
      wl_compositor_destroy(m_Compositor);
    }

    if (m_Registry != nullptr) {
      wl_registry_destroy(m_Registry);
    }

    if (m_XkbState != nullptr) {
      xkb_state_unref(m_XkbState);
    }
    if (m_XkbKeymap != nullptr) {
      xkb_keymap_unref(m_XkbKeymap);
    }
    if (m_XkbContext != nullptr) {
      xkb_context_unref(m_XkbContext);
    }

    if (m_Display != nullptr) {
      wl_display_disconnect(m_Display);
    }
  }

  std::vector<WindowEvent> Poll() override {
    std::vector<WindowEvent> out;

    if (m_Display == nullptr) {
      return out;
    }

    while (wl_display_prepare_read(m_Display) != 0) {
      wl_display_dispatch_pending(m_Display);
    }

    wl_display_flush(m_Display);

    pollfd pfd{};
    pfd.fd = wl_display_get_fd(m_Display);
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
      wl_display_read_events(m_Display);
      wl_display_dispatch_pending(m_Display);
    } else {
      wl_display_cancel_read(m_Display);
    }

    out.swap(m_PendingEvents);
    return out;
  }

  void Resize(uint32_t width, uint32_t height) override {
    (void)width;
    (void)height;
  }

  void SetTitle(const std::string& title) override {
    if (m_Toplevel != nullptr && m_Surface != nullptr) {
      xdg_toplevel_set_title(m_Toplevel, title.c_str());
      wl_surface_commit(m_Surface);
      wl_display_flush(m_Display);
    }
  }

  std::pair<uint64_t, uint64_t> NativeHandles() const override {
    return std::make_pair(reinterpret_cast<uint64_t>(m_Display),
                          reinterpret_cast<uint64_t>(m_Surface));
  }

private:
  static void RegistryGlobal(
      void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<PlayerWindowWayland*>(data);
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
      self->m_Compositor = static_cast<wl_compositor*>(
          wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
      self->m_WmBase =
          static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
      self->m_Seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 1));

      static const wl_seat_listener seatListener = {
          SeatCapabilities,
          SeatName,
      };
      wl_seat_add_listener(self->m_Seat, &seatListener, self);
    }
    (void)version;
  }

  static void RegistryGlobalRemove(void* data, wl_registry* registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
  }

  static void WmBasePing(void* data, xdg_wm_base* wmBase, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(wmBase, serial);
  }

  static void XdgSurfaceConfigure(void* data, xdg_surface* xdgSurface, uint32_t serial) {
    auto* self = static_cast<PlayerWindowWayland*>(data);
    xdg_surface_ack_configure(xdgSurface, serial);
    wl_surface_commit(self->m_Surface);
  }

  static void ToplevelConfigure(
      void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states) {
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
    (void)states;
  }

  static void ToplevelClose(void* data, xdg_toplevel* toplevel) {
    (void)toplevel;
    auto* self = static_cast<PlayerWindowWayland*>(data);
    self->m_PendingEvents.push_back(WindowEvent::Close);
  }

  static void ToplevelConfigureBounds(void* data,
                                      xdg_toplevel* toplevel,
                                      int32_t width,
                                      int32_t height) {
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
  }

  static void ToplevelWmCapabilities(void* data, xdg_toplevel* toplevel, wl_array* capabilities) {
    (void)data;
    (void)toplevel;
    (void)capabilities;
  }

  static void SeatCapabilities(void* data, wl_seat* seat, uint32_t capabilities) {
    auto* self = static_cast<PlayerWindowWayland*>(data);
    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && self->m_Keyboard == nullptr) {
      self->m_Keyboard = wl_seat_get_keyboard(seat);
      static const wl_keyboard_listener keyboardListener = {
          KeyboardKeymap, KeyboardEnter,     KeyboardLeave,
          KeyboardKey,    KeyboardModifiers, KeyboardRepeatInfo,
      };
      wl_keyboard_add_listener(self->m_Keyboard, &keyboardListener, self);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && self->m_Keyboard != nullptr) {
      wl_keyboard_destroy(self->m_Keyboard);
      self->m_Keyboard = nullptr;
    }
  }

  static void SeatName(void* data, wl_seat* seat, const char* name) {
    (void)data;
    (void)seat;
    (void)name;
  }

  static void KeyboardKeymap(
      void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {
    (void)keyboard;

    auto* self = static_cast<PlayerWindowWayland*>(data);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
      close(fd);
      return;
    }

    void* keymapData = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (keymapData == MAP_FAILED) {
      close(fd);
      return;
    }

    if (self->m_XkbState != nullptr) {
      xkb_state_unref(self->m_XkbState);
      self->m_XkbState = nullptr;
    }
    if (self->m_XkbKeymap != nullptr) {
      xkb_keymap_unref(self->m_XkbKeymap);
      self->m_XkbKeymap = nullptr;
    }
    if (self->m_XkbContext == nullptr) {
      self->m_XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    self->m_XkbKeymap =
        xkb_keymap_new_from_string(self->m_XkbContext, static_cast<const char*>(keymapData),
                                   XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

    munmap(keymapData, size);
    close(fd);

    if (self->m_XkbKeymap != nullptr) {
      self->m_XkbState = xkb_state_new(self->m_XkbKeymap);
    }
  }

  static void KeyboardEnter(
      void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys) {
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)surface;
    (void)keys;
  }

  static void KeyboardLeave(void* data,
                            wl_keyboard* keyboard,
                            uint32_t serial,
                            wl_surface* surface) {
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)surface;
  }

  static void KeyboardKey(void* data,
                          wl_keyboard* keyboard,
                          uint32_t serial,
                          uint32_t time,
                          uint32_t key,
                          uint32_t state) {
    (void)keyboard;
    (void)serial;
    (void)time;

    auto* self = static_cast<PlayerWindowWayland*>(data);
    if (state != WL_KEYBOARD_KEY_STATE_PRESSED || self->m_XkbState == nullptr) {
      return;
    }

    const xkb_keysym_t keysym = xkb_state_key_get_one_sym(self->m_XkbState, key + 8);
    switch (keysym) {
    case XKB_KEY_Escape:
      self->m_PendingEvents.push_back(WindowEvent::Stop);
      break;
    case XKB_KEY_space:
      self->m_PendingEvents.push_back(WindowEvent::TogglePause);
      break;
    case XKB_KEY_i:
    case XKB_KEY_I:
      self->m_PendingEvents.push_back(WindowEvent::ToggleInteractive);
      break;
    default:
      break;
    }
  }

  static void KeyboardModifiers(void* data,
                                wl_keyboard* keyboard,
                                uint32_t serial,
                                uint32_t modsDepressed,
                                uint32_t modsLatched,
                                uint32_t modsLocked,
                                uint32_t group) {
    (void)keyboard;
    (void)serial;

    auto* self = static_cast<PlayerWindowWayland*>(data);
    if (self->m_XkbState != nullptr) {
      xkb_state_update_mask(self->m_XkbState, modsDepressed, modsLatched, modsLocked, 0, 0, group);
    }
  }

  static void KeyboardRepeatInfo(void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay) {
    (void)data;
    (void)keyboard;
    (void)rate;
    (void)delay;
  }

private:
  wl_display* m_Display{};
  wl_registry* m_Registry{};
  wl_compositor* m_Compositor{};
  wl_seat* m_Seat{};
  wl_keyboard* m_Keyboard{};
  xdg_wm_base* m_WmBase{};
  wl_surface* m_Surface{};
  xdg_surface* m_XdgSurface{};
  xdg_toplevel* m_Toplevel{};
  xkb_context* m_XkbContext{};
  xkb_keymap* m_XkbKeymap{};
  xkb_state* m_XkbState{};
  std::vector<WindowEvent> m_PendingEvents;
};
#endif

} // namespace

std::unique_ptr<PlayerWindow> CreatePlayerWindowWayland(
    int x, int y, int width, int height, bool visible) {
#if defined GITS_PLATFORM_X11
  return std::make_unique<PlayerWindowWayland>(x, y, width, height, visible);
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
