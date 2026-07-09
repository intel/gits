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

#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <memory>
#include <string>
#include <vector>

namespace gits {
namespace windowing {

namespace {

#if defined GITS_PLATFORM_WINDOWS
constexpr const char* kPlayerWindowClassName = "gits_player_window_class";

class PlayerWindowWin final : public PlayerWindow {
public:
  PlayerWindowWin(int x, int y, int width, int height, bool visible) {
    m_HInstance = GetModuleHandle(nullptr);

    static bool classRegistered = false;
    if (!classRegistered) {
      WNDCLASSEXA windowClass{};
      windowClass.cbSize = sizeof(windowClass);
      windowClass.lpfnWndProc = WndProc;
      windowClass.hInstance = m_HInstance;
      windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
      windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
      windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
      windowClass.lpszClassName = kPlayerWindowClassName;
      windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

      if (RegisterClassExA(&windowClass) == 0) {
        LOG_ERROR << "RegisterClassEx failed, GetLastError() = " << GetLastError();
        GITS_ASSERT(false, "Could not register player window class");
      }
      classRegistered = true;
    }

    DWORD style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    DWORD exStyle = WS_EX_APPWINDOW;
    int posX = x;
    int posY = y;

    if (Configurator::Get().common.player.windowMode == gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
      style = WS_POPUP;
      exStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
      posX = 0;
      posY = 0;

      DEVMODE devMode = {};
      devMode.dmSize = sizeof(devMode);
      devMode.dmPelsWidth = width;
      devMode.dmPelsHeight = height;
      devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      if (ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL) {
        ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
        ShowCursor(FALSE);
      } else {
        LOG_WARNING << "Cannot adjust screen resolution to match dimensions of " << width << " x "
                    << height << " pixels for a window.";
      }
    } else if (Configurator::Get().common.player.showWindowBorder) {
      style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
    }

    RECT rect{posX, posY, posX + width, posY + height};
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    const int adjustedWidth = rect.right - rect.left;
    const int adjustedHeight = rect.bottom - rect.top;

    SetLastError(NO_ERROR);
    m_Hwnd = CreateWindowExA(exStyle, kPlayerWindowClassName, "Vulkan-GITS", style, posX, posY,
                             adjustedWidth, adjustedHeight, nullptr, nullptr, m_HInstance, this);

    if (m_Hwnd == nullptr) {
      LOG_ERROR << "CreateWindowEx failed, GetLastError() = " << GetLastError();
      GITS_ASSERT(false, "CreateWindowEx failed");
    }

    const bool show = visible && !Configurator::Get().common.player.forceInvisibleWindows;
    if (show) {
      ShowWindow(m_Hwnd, SW_SHOW);
    }
  }

  ~PlayerWindowWin() override {
    if (m_Hwnd != nullptr) {
      if (Configurator::Get().common.player.windowMode == gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
        ShowCursor(TRUE);
        ChangeDisplaySettings(nullptr, 0);
      }
      DestroyWindow(m_Hwnd);
      m_Hwnd = nullptr;
    }
  }

  std::vector<WindowEvent> Poll() override {
    MSG msg{};
    while (PeekMessageA(&msg, m_Hwnd, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        m_PendingEvents.push_back(WindowEvent::Stop);
        continue;
      }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    std::vector<WindowEvent> events;
    events.swap(m_PendingEvents);
    return events;
  }

  void Resize(uint32_t width, uint32_t height) override {
    if (m_Hwnd == nullptr) {
      return;
    }

    SetLastError(NO_ERROR);
    if (SetWindowPos(m_Hwnd, nullptr, 0, 0, static_cast<int>(width), static_cast<int>(height),
                     SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOZORDER) == 0) {
      LOG_ERROR << "SetWindowPos failed, GetLastError() = " << GetLastError();
      GITS_ASSERT(false, "SetWindowPos failed");
    }

    if (Configurator::Get().common.player.windowMode == gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
      DEVMODE devMode = {};
      devMode.dmSize = sizeof(devMode);
      devMode.dmPelsWidth = static_cast<DWORD>(width);
      devMode.dmPelsHeight = static_cast<DWORD>(height);
      devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
      if (ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL) {
        ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
        ShowCursor(FALSE);
      } else {
        LOG_WARNING << "Cannot adjust screen resolution to match dimensions of " << width << " x "
                    << height << " pixels for window " << m_Hwnd << ".";
      }
    }
  }

  void SetPosition(int32_t x, int32_t y) override {
    if (m_Hwnd == nullptr) {
      return;
    }
    if (Configurator::Get().common.player.windowMode == gits::WindowMode::EXCLUSIVE_FULLSCREEN) {
      return;
    }
    SetLastError(NO_ERROR);
    if (SetWindowPos(m_Hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) ==
        0) {
      LOG_ERROR << "SetWindowPos failed, GetLastError() = " << GetLastError();
      GITS_ASSERT(false, "SetWindowPos failed");
    }
  }

  void SetVisibility(bool visible) override {
    if (m_Hwnd == nullptr) {
      return;
    }
    if (Configurator::Get().common.player.forceInvisibleWindows) {
      visible = false;
    }
    ShowWindow(m_Hwnd, visible ? SW_SHOW : SW_HIDE);
  }

  void SetTitle(const std::string& title) override {
    if (m_Hwnd == nullptr) {
      return;
    }
    SetLastError(NO_ERROR);
    if (SetWindowTextA(m_Hwnd, title.c_str()) == 0) {
      LOG_ERROR << "SetWindowText failed, GetLastError() = " << GetLastError();
      GITS_ASSERT(false, "SetWindowText failed");
    }
  }

  std::pair<uint64_t, uint64_t> NativeHandles() const override {
    return std::make_pair(reinterpret_cast<uint64_t>(m_HInstance),
                          reinterpret_cast<uint64_t>(m_Hwnd));
  }

  void PushPendingEvent(WindowEvent event) {
    m_PendingEvents.push_back(event);
  }

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PlayerWindowWin* self =
        reinterpret_cast<PlayerWindowWin*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
      auto* createStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
      self = static_cast<PlayerWindowWin*>(createStruct->lpCreateParams);
      SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
      return TRUE;
    }

    if (self == nullptr) {
      return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_KEYDOWN:
      switch (wParam) {
      case VK_ESCAPE:
        self->PushPendingEvent(WindowEvent::Stop);
        break;
      case VK_SPACE:
        self->PushPendingEvent(WindowEvent::TogglePause);
        break;
      case 'I':
      case 'i':
        self->PushPendingEvent(WindowEvent::ToggleInteractive);
        break;
      default:
        break;
      }
      break;
    case WM_CLOSE:
      self->PushPendingEvent(WindowEvent::Close);
      break;
    default:
      return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
  }

  HINSTANCE m_HInstance{};
  HWND m_Hwnd{};
  std::vector<WindowEvent> m_PendingEvents;
};
#endif

} // namespace

std::unique_ptr<PlayerWindow> CreatePlayerWindowWin(
    int x, int y, int width, int height, bool visible) {
#if defined GITS_PLATFORM_WINDOWS
  return std::make_unique<PlayerWindowWin>(x, y, width, height, visible);
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
