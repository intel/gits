// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsCustom.h"
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

  void StoreSurface(VkSurfaceKHR surface,
                    uint32_t displayProtocol,
                    uint64_t hwnd,
                    uint64_t hinstance,
                    int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height,
                    bool visible);
  void StoreSwapchain(VkSwapchainKHR swapchain,
                      VkSurfaceKHR surface,
                      uint32_t threadId,
                      int32_t imageWidth,
                      int32_t imageHeight);
  void RemoveSurface(VkSurfaceKHR surface);
  void RemoveSwapchain(VkSwapchainKHR swapchain);
  void UpdateWindowsForPresent(uint32_t threadId, const VkPresentInfoKHR& presentInfo);

private:
  struct SurfaceWindow {
    uint32_t Protocol{};
    uint64_t Hwnd{};
    uint64_t Hinstance{};
  };
  struct WindowState {
    int32_t X{};
    int32_t Y{};
    int32_t Width{};
    int32_t Height{};
    bool Visible{};
  };

  bool QueryLiveWindowState(const SurfaceWindow& window,
                            int32_t& x,
                            int32_t& y,
                            int32_t& width,
                            int32_t& height,
                            bool& visible) const;
  void RecordUpdateIfChanged(uint32_t threadId,
                             const SurfaceWindow& window,
                             int32_t x,
                             int32_t y,
                             int32_t width,
                             int32_t height,
                             bool visible);

  std::unordered_map<VkSurfaceKHR, SurfaceWindow> m_SurfaceWindow;
  std::unordered_map<uint64_t, WindowState> m_WindowState;
  std::unordered_map<VkSwapchainKHR, VkSurfaceKHR> m_SwapchainSurface;
  stream::OrderingRecorder& m_Recorder;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
