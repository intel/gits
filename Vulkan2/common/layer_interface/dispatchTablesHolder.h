// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dispatchTableAuto.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace gits {
namespace vulkan {

// Thread-safe wrapper around the instance and device dispatch tables.
// Shared between CaptureManager and layers that need dispatch access.
class DispatchTablesHolder {
public:
  DispatchTablesHolder(
      std::unordered_map<void*, VkInstanceLevelDispatchTable>& instanceDispatchTables,
      std::unordered_map<void*, VkDeviceLevelDispatchTable>& deviceDispatchTables,
      std::shared_mutex& mutex)
      : m_InstanceDispatchTables(instanceDispatchTables),
        m_DeviceDispatchTables(deviceDispatchTables),
        m_Mutex(mutex) {}

  // Pass dispatchable instance level handle:
  // VkInstance, VkPhysicalDevice, VkSurfaceKHR, etc.
  template <typename Handle>
  VkInstanceLevelDispatchTable* GetInstanceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    std::shared_lock lock(m_Mutex);
    auto it = m_InstanceDispatchTables.find(dispatchKey);
    if (it == m_InstanceDispatchTables.end()) {
      return nullptr;
    }
    return &it->second;
  }

  // Pass dispatchable device level handle:
  // VkDevice, VkQueue, VkCommandBuffer, etc.
  template <typename Handle>
  VkDeviceLevelDispatchTable* GetDeviceDispatchTable(Handle handle) {
    void* dispatchKey = *reinterpret_cast<void**>(handle);
    std::shared_lock lock(m_Mutex);
    auto it = m_DeviceDispatchTables.find(dispatchKey);
    if (it == m_DeviceDispatchTables.end()) {
      return nullptr;
    }
    return &it->second;
  }

private:
  std::unordered_map<void*, VkInstanceLevelDispatchTable>& m_InstanceDispatchTables;
  std::unordered_map<void*, VkDeviceLevelDispatchTable>& m_DeviceDispatchTables;
  std::shared_mutex& m_Mutex;
};

} // namespace vulkan
} // namespace gits
