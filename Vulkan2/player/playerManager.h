// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "layerAuto.h"
#include "playerLayerManager.h"
#include "dispatchTableAuto.h"
#include "windowService.h"
#include "mapTrackingService.h"

namespace gits {
namespace vulkan {

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

private:
  PlayerManager();

private:
  static PlayerManager* m_Instance;
  PlayerLayerManager m_LayerManager;
  bool m_ExecuteCommands{true};
  dl::SharedObject m_Lib{nullptr};
  PFN_vkGetInstanceProcAddr m_GetInstanceProcAddr{nullptr};
  VkGlobalLevelDispatchTable m_GlobalDispatchTable{};
  std::unordered_map<void*, VkInstanceLevelDispatchTable> m_InstanceDispatchTable{};
  std::unordered_map<void*, VkDeviceLevelDispatchTable> m_DeviceDispatchTable{};

  WindowService m_WindowService;
  MapTrackingService m_MapTrackingService;
};

} // namespace vulkan
} // namespace gits
