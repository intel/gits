// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "descriptorUpdateTemplateService.h"
#include "fencePendingSignalService.h"
#include "dispatchTableAuto.h"
#include "dispatchTablesHolder.h"
#include "layerAuto.h"
#include "mapTrackingService.h"
#include "playerLayerManager.h"
#include "restoreContentService.h"
#include "windowService.h"

#include <memory>
#include <shared_mutex>
#include "swapchainImageSyncService.h"
#include <unordered_map>

namespace gits {
namespace vulkan {

class PluginService;

class PlayerManager : public gits::noncopyable {
public:
  static PlayerManager& Get();
  static void Destroy() {
    delete m_Instance;
    m_Instance = nullptr;
  }

  ~PlayerManager();

  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

  bool ExecuteCommands() {
    return m_ExecuteCommands;
  }

  void LoadGlobalFunctions();
  void LoadInstanceFunctions(VkInstance instance);
  void LoadDeviceFunctions(void* dispatchKey, VkDevice device);

  DispatchTablesHolder& GetDispatchTablesHolder() {
    return *m_DispatchTablesHolder;
  }

  VkGlobalLevelDispatchTable& GetGlobalDispatchTable() {
    return m_GlobalDispatchTable;
  }

  template <typename Handle>
  VkInstanceLevelDispatchTable& GetInstanceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    return m_InstanceDispatchTable[dispatchKey];
  }

  template <typename Handle>
  VkDeviceLevelDispatchTable& GetDeviceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    return m_DeviceDispatchTable[dispatchKey];
  }

  WindowService& GetWindowService() {
    return m_WindowService;
  }

  MapTrackingService& GetMapTrackingService() {
    return m_MapTrackingService;
  }

  DescriptorUpdateTemplateService& GetDescriptorUpdateTemplateService() {
    return m_DescriptorUpdateTemplateService;
  }

  SwapchainImageSyncService& GetSwapchainImageSyncService() {
    return m_SwapchainImageSyncService;
  }

  FencePendingSignalService& GetFencePendingSignalService() {
    return m_FencePendingSignalService;
  }

  RestoreContentService& GetRestoreContentService() {
    return m_RestoreContentService;
  }

private:
  PlayerManager();

private:
  static PlayerManager* m_Instance;
  PlayerLayerManager m_LayerManager;
  std::unique_ptr<PluginService> m_PluginService;
  bool m_ExecuteCommands{true};
  dl::SharedObject m_Lib{nullptr};
  PFN_vkGetInstanceProcAddr m_GetInstanceProcAddr{nullptr};
  VkGlobalLevelDispatchTable m_GlobalDispatchTable{};
  std::shared_mutex m_DispatchTablesMutex;
  std::unordered_map<void*, VkInstanceLevelDispatchTable> m_InstanceDispatchTable{};
  std::unordered_map<void*, VkDeviceLevelDispatchTable> m_DeviceDispatchTable{};
  std::unique_ptr<DispatchTablesHolder> m_DispatchTablesHolder;

  WindowService m_WindowService;
  MapTrackingService m_MapTrackingService;
  DescriptorUpdateTemplateService m_DescriptorUpdateTemplateService;
  SwapchainImageSyncService m_SwapchainImageSyncService;
  FencePendingSignalService m_FencePendingSignalService;
  RestoreContentService m_RestoreContentService{*this};
};

} // namespace vulkan
} // namespace gits
