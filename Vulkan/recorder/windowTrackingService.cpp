// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowTrackingService.h"
#include "captureManager.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace vulkan {

WindowTrackingService::WindowTrackingService(stream::OrderingRecorder& recorder)
    : m_Recorder(recorder) {}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void WindowTrackingService::StoreSurface(VkSurfaceKHR surface,
                                         uint64_t hwnd,
                                         int32_t x,
                                         int32_t y,
                                         int32_t width,
                                         int32_t height,
                                         bool visible) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SurfaceHwnd[surface] = hwnd;
  m_HwndState[hwnd] = {x, y, width, height, visible};
}

void WindowTrackingService::StoreSwapchain(VkSwapchainKHR swapchain,
                                           VkSurfaceKHR surface,
                                           uint32_t threadId,
                                           int32_t imageWidth,
                                           int32_t imageHeight) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_SurfaceHwnd.find(surface);
  if (it == m_SurfaceHwnd.end()) {
    return;
  }
  m_SwapchainSurface[swapchain] = surface;

  HWND hwnd = reinterpret_cast<HWND>(it->second);
  RECT windowRect{};
  GetWindowRect(hwnd, &windowRect);

  UpdateWindowMetaCommand updateWindowMetaCommand(threadId);
  updateWindowMetaCommand.m_Key = CaptureManager::Get().CreateCommandKey();
  updateWindowMetaCommand.m_Hwnd.Value = it->second;
  updateWindowMetaCommand.m_X.Value = windowRect.left;
  updateWindowMetaCommand.m_Y.Value = windowRect.top;
  updateWindowMetaCommand.m_Width.Value = imageWidth;
  updateWindowMetaCommand.m_Height.Value = imageHeight;
  updateWindowMetaCommand.m_Visible.Value = IsWindowVisible(hwnd) == TRUE;
  m_Recorder.Record(updateWindowMetaCommand.m_Key,
                    new UpdateWindowMetaSerializer(updateWindowMetaCommand));
}

void WindowTrackingService::RemoveSurface(VkSurfaceKHR surface) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SurfaceHwnd.erase(surface);
}

void WindowTrackingService::RemoveSwapchain(VkSwapchainKHR swapchain) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SwapchainSurface.erase(swapchain);
}

void WindowTrackingService::UpdateWindowsForPresent(uint32_t threadId,
                                                    const VkPresentInfoKHR& presentInfo) {
  // Poll the live window state on every present and record an UpdateWindowMetaCommand
  // whenever it changed. This captures the application's real visibility/size timeline
  // (e.g. an Unreal Engine window that is created hidden and shown only after init),
  // which a single sample at surface/swapchain creation cannot capture.
  //
  // Called from CaptureCustomizationLayer::Pre(vkQueuePresentKHRCommand&) so the meta
  // command receives a key smaller than the present command's key (assigned after the
  // Pre layers run in wrappersAuto.cpp.mako), ordering the window update before the
  // present in the stream. Tokens are only recorded on change, so steady-state frames
  // add nothing to the stream.
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (uint32_t i = 0; i < presentInfo.swapchainCount; ++i) {
    auto swapchainIt = m_SwapchainSurface.find(presentInfo.pSwapchains[i]);
    if (swapchainIt == m_SwapchainSurface.end()) {
      continue;
    }
    auto surfaceIt = m_SurfaceHwnd.find(swapchainIt->second);
    if (surfaceIt == m_SurfaceHwnd.end()) {
      continue;
    }
    HWND hwnd = reinterpret_cast<HWND>(surfaceIt->second);

    RECT clientRect;
    if (GetClientRect(hwnd, &clientRect) == FALSE) {
      continue;
    }
    int32_t width = clientRect.right - clientRect.left;
    int32_t height = clientRect.bottom - clientRect.top;
    bool visible = IsWindowVisible(hwnd) == TRUE;
    // Window-frame origin in screen coordinates (matches the surface-create capture
    // convention, where the + clientRect.left/top terms are always zero no-ops).
    RECT windowRect{};
    GetWindowRect(hwnd, &windowRect);
    int32_t x = windowRect.left;
    int32_t y = windowRect.top;

    auto stateIt = m_HwndState.find(surfaceIt->second);
    if (stateIt != m_HwndState.end() && stateIt->second.X == x && stateIt->second.Y == y &&
        stateIt->second.Width == width && stateIt->second.Height == height &&
        stateIt->second.Visible == visible) {
      continue;
    }

    UpdateWindowMetaCommand updateWindowMetaCommand(threadId);
    updateWindowMetaCommand.m_Key = CaptureManager::Get().CreateCommandKey();
    updateWindowMetaCommand.m_Hwnd.Value = surfaceIt->second;
    updateWindowMetaCommand.m_X.Value = x;
    updateWindowMetaCommand.m_Y.Value = y;
    updateWindowMetaCommand.m_Width.Value = width;
    updateWindowMetaCommand.m_Height.Value = height;
    updateWindowMetaCommand.m_Visible.Value = visible;
    m_Recorder.Record(updateWindowMetaCommand.m_Key,
                      new UpdateWindowMetaSerializer(updateWindowMetaCommand));

    m_HwndState[surfaceIt->second] = {x, y, width, height, visible};
  }
}
#endif

} // namespace vulkan
} // namespace gits
