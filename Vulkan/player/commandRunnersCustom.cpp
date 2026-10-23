// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandRunnersCustom.h"
#include "argumentCodersAuto.h"
#include "layerAuto.h"
#include "playerManager.h"

#include "log.h"

#include <cstring>

namespace gits {
namespace vulkan {

void StateRestoreBeginRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void StateRestoreEndRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void FrameEndRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void MarkerUInt64Runner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void CreateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    uint64_t hWnd = manager.GetWindowService().SetWindow(
        m_Command.m_DisplayProtocol.Value, m_Command.m_Hwnd.Value, m_Command.m_Hinstance.Value,
        m_Command.m_X.Value, m_Command.m_Y.Value, m_Command.m_Width.Value, m_Command.m_Height.Value,
        m_Command.m_Visible.Value);

    m_Command.m_Hwnd.Value = hWnd;
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void UpdateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    manager.GetWindowService().UpdateWindow(m_Command.m_Hwnd.Value, m_Command.m_X.Value,
                                            m_Command.m_Y.Value, m_Command.m_Width.Value,
                                            m_Command.m_Height.Value, m_Command.m_Visible.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void MappedDataMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    auto mappedEntry =
        manager.GetMapTrackingService().GetData(m_Command.m_Device.Key, m_Command.m_Memory.Key);
    // If the memory isn't currently mapped, skip the update and warn instead of
    // dereferencing a null/stale pointer.
    if (mappedEntry == nullptr || mappedEntry->Ptr == nullptr) {
      LOG_WARNING << "MappedDataMetaRunner: no live host mapping for device key "
                  << m_Command.m_Device.Key << ", memory key " << m_Command.m_Memory.Key
                  << " - skipping memory update.";
    } else {
      for (const auto& region : m_Command.m_Regions.Regions) {
        if (region.Offset + region.Size > mappedEntry->Size) {
          LOG_WARNING << "MappedDataMetaRunner: region [offset " << region.Offset << ", size "
                      << region.Size << "] exceeds mapped range (" << mappedEntry->Size
                      << ") for device key " << m_Command.m_Device.Key << ", memory key "
                      << m_Command.m_Memory.Key << " - skipping region.";
          continue;
        }
        char* dstPtr = static_cast<char*>(mappedEntry->Ptr) + region.Offset;
        std::memcpy(dstPtr, region.Data, region.Size);
      }
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void RestoreContentManifestRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    manager.GetRestoreContentService().Manifest(m_Command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void RestoreContentDataRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    manager.GetRestoreContentService().OnData(m_Command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

#if !defined(VK_USE_PLATFORM_WIN32_KHR) && defined(VK_USE_PLATFORM_XLIB_KHR)
namespace {

// Layout of VkWin32SurfaceCreateInfoKHR, which the Vulkan headers declare only on Windows.
struct Win32SurfaceCreateInfo {
  VkStructureType sType;
  const void* pNext;
  VkFlags flags;
  void* hinstance;
  void* hwnd;
};

// Reads a PointerArgument<VkWin32SurfaceCreateInfoKHR> the same way the generated decoder does
// and returns the recorded window handle.
uint64_t DecodeWin32SurfaceCreateInfo(char* src, uint32_t& offset) {
  void* pointer{};
  std::memcpy(&pointer, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (pointer == nullptr) {
    return 0;
  }
  Win32SurfaceCreateInfo createInfo{};
  std::memcpy(&createInfo, src + offset, sizeof(createInfo));
  offset += sizeof(createInfo);
  if (createInfo.pNext) {
    DecodePNextChainInput(src, offset, const_cast<void**>(&createInfo.pNext));
  }
  uint32_t keyCount{};
  std::memcpy(&keyCount, src + offset, sizeof(keyCount));
  offset += sizeof(keyCount) + keyCount * sizeof(GITSKey);
  return reinterpret_cast<uint64_t>(createInfo.hwnd);
}

} // namespace

void vkCreateWin32SurfaceKHRAsXlibRunner::DecodeCommand() {
  uint32_t offset = 0;
  Decode(m_Data, offset, command.m_Key);
  Decode(m_Data, offset, command.m_ThreadId);
  Decode(m_Data, offset, command.m_instance);
  m_RecordedHwnd = DecodeWin32SurfaceCreateInfo(m_Data, offset);
  Decode(m_Data, offset, command.m_pAllocator);
  Decode(m_Data, offset, command.m_pSurface);
  Decode(m_Data, offset, command.m_Return);

  m_CreateInfo = {};
  m_CreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  command.m_pCreateInfo.Value = &m_CreateInfo;
}

void vkCreateWin32SurfaceKHRAsXlibRunner::Run() {
  CALL_ONCE[] {
    LOG_INFO << "Replaying Win32 surfaces as Xlib surfaces.";
  };
  // The window is created by an earlier command, so it can only be looked up when this command
  // runs. ReplayCustomizationLayer doesn't remap these live handles again.
  const auto window = PlayerManager::Get().GetWindowService().GetPlaybackWindow(m_RecordedHwnd);
  if (window.first == 0 || window.second == 0) {
    LOG_ERROR << "vkCreateWin32SurfaceKHR: no playback window for the recorded window "
              << m_RecordedHwnd << ", skipping the surface.";
    command.m_Skip = true;
  }
  m_CreateInfo.dpy = reinterpret_cast<Display*>(window.first);
  m_CreateInfo.window = static_cast<Window>(window.second);
  vkCreateXlibSurfaceKHRRunner::Run();
}
#endif

} // namespace vulkan
} // namespace gits
