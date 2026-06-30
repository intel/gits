// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"

#include "vulkanHeader2.h"
#include "pluginService.h"
#include "captureLayerManager.h"
#include "dispatchTableAuto.h"
#include "dispatchTablesHolder.h"
#include "mapTrackingService.h"
#include "windowTrackingService.h"
#include "descriptorUpdateTemplateService.h"
#include "orderingRecorder.h"

#include <atomic>
#include <shared_mutex>

namespace gits {
namespace vulkan {

class CaptureManager : public gits::noncopyable {
public:
  static CaptureManager& Get();
  static void Cleanup();

  GITSKey CreateCommandKey() {
    return m_CommandUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  GITSKey CreateHandleKey() {
    return m_HandleUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  VkGlobalLevelDispatchTable& GetGlobalDispatchTable() {
    return m_GlobalDispatchTable;
  }

  template <typename Handle>
  VkInstanceLevelDispatchTable& GetInstanceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    std::shared_lock lock(m_DispatchTablesMutex);
    return m_InstanceDispatchTable[dispatchKey];
  }

  template <typename Handle>
  VkDeviceLevelDispatchTable& GetDeviceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    std::shared_lock lock(m_DispatchTablesMutex);
    return m_DeviceDispatchTable[dispatchKey];
  }

  DispatchTablesHolder& GetDispatchTablesHolder() {
    return *m_DispatchTablesHolder;
  }

  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }

  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

  uint32_t IncrementRecursionDepth() {
    return ++m_RecursionDepth;
  }

  void DecrementRecursionDepth() {
    --m_RecursionDepth;
  }

  void LoadGlobalFunctions(PFN_vkGetInstanceProcAddr getProcAddr);
  void LoadInstanceFunctions(PFN_vkGetInstanceProcAddr getProcAddr, VkInstance instance);
  void LoadDeviceFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device);
  void LoadDeviceFunctions(void* dispatchKey, VkDevice device);

  PFN_vkVoidFunction GetFunctionWrapper(const char* name);

  MapTrackingService& GetMapTrackingService() {
    return *m_MapTrackingService;
  }

  WindowTrackingService& GetWindowTrackingService() {
    return *m_WindowTrackingService;
  }

  DescriptorUpdateTemplateService& GetDescriptorUpdateTemplateService() {
    return m_DescriptorUpdateTemplateService;
  }

private:
  CaptureManager();
  ~CaptureManager();

private:
  static CaptureManager* m_Instance;
  CaptureLayerManager m_LayerManager;
  PluginService m_PluginService;

  std::unique_ptr<stream::OrderingRecorder> m_Recorder;
  //std::atomic<uint32_t> m_RecursionDepth{0};
  static thread_local uint32_t m_RecursionDepth;
  std::atomic<GITSKey> m_CommandUniqueKey{0};
  std::atomic<GITSKey> m_HandleUniqueKey{0};
  VkGlobalLevelDispatchTable m_GlobalDispatchTable{};
  std::shared_mutex m_DispatchTablesMutex;
  std::unordered_map<void*, VkInstanceLevelDispatchTable> m_InstanceDispatchTable{};
  std::unordered_map<void*, VkDeviceLevelDispatchTable> m_DeviceDispatchTable{};
  std::unique_ptr<DispatchTablesHolder> m_DispatchTablesHolder;

  std::unique_ptr<MapTrackingService> m_MapTrackingService;
  std::unique_ptr<WindowTrackingService> m_WindowTrackingService;
  DescriptorUpdateTemplateService m_DescriptorUpdateTemplateService;
};

class RecursionGuard : public gits::noncopyable {
public:
  ~RecursionGuard() {
    try {
      auto& manager = CaptureManager::Get();
      manager.DecrementRecursionDepth();
    } catch (...) {
      topmost_exception_handler("RecursionGuard::~RecursionGuard()");
    }
  }

  operator bool() {
    auto& manager = CaptureManager::Get();
    uint32_t depth = manager.IncrementRecursionDepth();
    return depth == 1;
  }
};

} // namespace vulkan
} // namespace gits
