// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "orderingRecorder.h"
#include "vulkanHeader2.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace vulkan {

// Tracks the surface/swapchain/window relationships needed to record the
// application's live window timeline (visibility/size) during capture. Owned by
// CaptureManager and emits UpdateWindowMetaCommand itself, mirroring
// MapTrackingService.
class WindowTrackingService {
public:
  WindowTrackingService(stream::OrderingRecorder& recorder);

#ifdef VK_USE_PLATFORM_WIN32_KHR
  // Records surface -> Win32 window identity and seeds the last-known window state
  // so per-present polling only emits updates when the window actually changes.
  void StoreSurface(VkSurfaceKHR surface,
                    uint64_t hwnd,
                    uint64_t hinstance,
                    int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height,
                    bool visible);
  // Records swapchain -> surface (so vkQueuePresentKHR can resolve
  // swapchain -> surface -> HWND) and emits an UpdateWindowMetaCommand carrying
  // the swapchain image extent, matching the legacy behaviour at swapchain
  // creation.
  void StoreSwapchain(VkSwapchainKHR swapchain,
                      VkSurfaceKHR surface,
                      uint32_t threadId,
                      int32_t imageWidth,
                      int32_t imageHeight);
  void RemoveSurface(VkSurfaceKHR surface);
  void RemoveSwapchain(VkSwapchainKHR swapchain);
  // Polls the live window state for each presented swapchain and records an
  // UpdateWindowMetaCommand whenever it changed since the last present.
  void UpdateWindowsForPresent(uint32_t threadId, const VkPresentInfoKHR& presentInfo);
#endif

private:
#ifdef VK_USE_PLATFORM_WIN32_KHR
  struct SurfaceWindow {
    uint64_t Hwnd{};
    uint64_t Hinstance{};
  };
  // Last-known geometry/visibility per capture HWND (mirrors legacy CHWNDState).
  struct WindowState {
    int32_t X{};
    int32_t Y{};
    int32_t Width{};
    int32_t Height{};
    bool Visible{};
  };

  std::unordered_map<VkSurfaceKHR, SurfaceWindow> m_SurfaceWindow;
  std::unordered_map<uint64_t, WindowState> m_HwndState;
  std::unordered_map<VkSwapchainKHR, VkSurfaceKHR> m_SwapchainSurface;
#endif
  stream::OrderingRecorder& m_Recorder;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
