// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "log.h"
#include "descriptorUpdateTemplateService.h"
#include "deviceDiagnosticService.h"
#include "eventPendingSignalService.h"
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
    std::shared_lock lock(m_DispatchTablesMutex);
    auto it = m_InstanceDispatchTable.find(dispatchKey);
    if (it == m_InstanceDispatchTable.end()) {
      // Mirrors CaptureManager::GetInstanceDispatchTable (Vulkan/recorder/captureManager.h):
      // operator[]'s silent default-insertion would hand back a zero-filled table whose
      // function pointers are all nullptr, so the caller jumps to address 0 on the very next
      // call instead of getting a diagnosable failure. A miss here means this handle's first
      // qword (the loader/ICD dispatch key) does not match any VkInstance/VkPhysicalDevice
      // registered via LoadInstanceFunctions during replay.
      LOG_ERROR << "PlayerManager::GetInstanceDispatchTable: handle=0x" << std::hex
                << reinterpret_cast<uint64_t>(handle) << " dispatchKey=0x"
                << reinterpret_cast<uint64_t>(dispatchKey) << std::dec
                << " has no registered instance dispatch table. The vkCreateInstance for this "
                   "handle's dispatch key was never replayed, or this is a foreign/wrapped "
                   "handle.";
      GITS_ASSERT(it != m_InstanceDispatchTable.end());
      static VkInstanceLevelDispatchTable emptyInstanceDispatchTable{};
      return emptyInstanceDispatchTable;
    }
    return it->second;
  }

  template <typename Handle>
  VkDeviceLevelDispatchTable& GetDeviceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    std::shared_lock lock(m_DispatchTablesMutex);
    auto it = m_DeviceDispatchTable.find(dispatchKey);
    if (it == m_DeviceDispatchTable.end()) {
      // Same class of bug as GetInstanceDispatchTable above, and the same fix as
      // CaptureManager::GetDeviceDispatchTable on the recorder side: fail loudly instead of
      // silently inserting a zero-filled table via operator[] (also unsafe under only the
      // shared/reader lock taken above, since operator[] mutates on insertion). Known replay
      // causes: a queue/command buffer whose loader dispatch data was never associated with a
      // replayed device (e.g. a VkQueue created through a non-standard, vendor-extension path
      // rather than vkGetDeviceQueue/vkAllocateCommandBuffers -- see AliasDeviceDispatchTable
      // below), or a genuinely foreign/wrapped handle.
      LOG_ERROR << "PlayerManager::GetDeviceDispatchTable: handle=0x" << std::hex
                << reinterpret_cast<uint64_t>(handle) << " dispatchKey=0x"
                << reinterpret_cast<uint64_t>(dispatchKey) << std::dec
                << " has no registered device dispatch table. The vkCreateDevice for this "
                   "handle's dispatch key was never replayed, the handle was created via a "
                   "non-standard path that never got aliased to its device's table, or this is "
                   "a foreign/wrapped handle.";
      GITS_ASSERT(it != m_DeviceDispatchTable.end());
      static VkDeviceLevelDispatchTable emptyDeviceDispatchTable{};
      return emptyDeviceDispatchTable;
    }
    return it->second;
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

  EventPendingSignalService& GetEventPendingSignalService() {
    return m_EventPendingSignalService;
  }

  RestoreContentService& GetRestoreContentService() {
    return m_RestoreContentService;
  }

  DeviceDiagnosticService& GetDeviceDiagnosticService() {
    return m_DeviceDiagnosticService;
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
  EventPendingSignalService m_EventPendingSignalService;
  RestoreContentService m_RestoreContentService{*this};
  DeviceDiagnosticService m_DeviceDiagnosticService{*this};
};

} // namespace vulkan
} // namespace gits
