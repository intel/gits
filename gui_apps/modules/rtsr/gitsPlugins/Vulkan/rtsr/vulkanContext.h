// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "dispatchTableAuto.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace gits::vulkan {

struct RtsrInstanceContext {
  GITSKey Key{};
  VkInstance Handle{VK_NULL_HANDLE};
  VkInstanceLevelDispatchTable Dispatch{};
};

struct RtsrDeviceContext {
  GITSKey Key{};
  GITSKey PhysicalDeviceKey{};
  VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
  VkDevice Handle{VK_NULL_HANDLE};
  VkDeviceLevelDispatchTable Dispatch{};
  float TimestampPeriod{};
  std::vector<uint32_t> TimestampValidBits;
  std::vector<VkQueueFlags> QueueFlags;
  std::atomic<bool> Retiring{false};
  std::shared_mutex ReplacementMutex;

  bool BeginOperation();
  void EndOperation();
  void Retire();

private:
  std::mutex m_OperationMutex;
  std::condition_variable m_OperationsFinished;
  size_t m_ActiveOperations{};
};

class RtsrContextRegistry {
public:
  void AddInstance(GITSKey key, VkInstance handle, const VkInstanceLevelDispatchTable& dispatch);
  void AddDevice(GITSKey key,
                 GITSKey physicalDeviceKey,
                 VkPhysicalDevice physicalDevice,
                 VkDevice handle,
                 const VkInstanceLevelDispatchTable& instanceDispatch,
                 const VkDeviceLevelDispatchTable& dispatch);

  std::shared_ptr<RtsrDeviceContext> Device(GITSKey key) const;
  std::shared_ptr<RtsrDeviceContext> Device(VkDevice handle) const;
  std::shared_ptr<RtsrDeviceContext> RetireDevice(GITSKey key);

  // Returns true when the last tracked instance was removed.
  bool RemoveInstance(GITSKey key);
  void Clear();

private:
  mutable std::mutex m_Mutex;
  std::unordered_map<GITSKey, RtsrInstanceContext> m_Instances;
  std::unordered_map<GITSKey, std::shared_ptr<RtsrDeviceContext>> m_Devices;
  std::unordered_map<VkDevice, GITSKey> m_DeviceKeys;
};

} // namespace gits::vulkan
