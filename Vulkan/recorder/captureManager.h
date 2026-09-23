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
    auto it = m_InstanceDispatchTable.find(dispatchKey);
    if (it == m_InstanceDispatchTable.end()) {
      // A miss here means this handle's first qword (the loader/ICD dispatch key) does not
      // match any VkInstance/VkPhysicalDevice registered via LoadInstanceFunctions. Calling
      // through a table for a key that was never registered would previously fall through to
      // operator[]'s silent default-insertion, handing back a zero-filled table whose function
      // pointers are all nullptr -- the caller then jumps to address 0 on the very next call.
      // Fail loudly and diagnosably instead (see HandleMapService::GetKeyLocked for the same
      // "log then GITS_ASSERT" convention used for the analogous handle-map miss).
      LOG_ERROR << "CaptureManager::GetInstanceDispatchTable: handle=0x" << std::hex
                << reinterpret_cast<uint64_t>(handle) << " dispatchKey=0x"
                << reinterpret_cast<uint64_t>(dispatchKey) << std::dec
                << " has no registered instance dispatch table. Either this is a "
                   "foreign/wrapped handle from another layer, or the object's loader dispatch "
                   "data was never stamped for this handle.";
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
      // Same class of bug as GetInstanceDispatchTable above: a genuine, non-recoverable
      // "no dispatch table for this handle" miss must fail loudly, not silently insert a
      // zero-filled table via operator[] (which is also unsafe to call while only holding the
      // shared/reader lock taken above -- unordered_map::operator[] mutates on insertion, so a
      // concurrent miss on another thread would be a data race on top of the null-table crash).
      // Known causes: a queue/command buffer whose loader dispatch data was never associated
      // with a device this recorder intercepted (e.g. a handle created through a non-standard,
      // driver-internal path rather than vkGetDeviceQueue/vkAllocateCommandBuffers), or a
      // genuinely foreign/wrapped handle from another layer in the chain.
      LOG_ERROR << "CaptureManager::GetDeviceDispatchTable: handle=0x" << std::hex
                << reinterpret_cast<uint64_t>(handle) << " dispatchKey=0x"
                << reinterpret_cast<uint64_t>(dispatchKey) << std::dec
                << " has no registered device dispatch table. Either this is a "
                   "foreign/wrapped handle from another layer, or the object's loader dispatch "
                   "data does not match any VkDevice this recorder intercepted via "
                   "LoadDeviceFunctions.";
      GITS_ASSERT(it != m_DeviceDispatchTable.end());
      static VkDeviceLevelDispatchTable emptyDeviceDispatchTable{};
      return emptyDeviceDispatchTable;
    }
    return it->second;
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
