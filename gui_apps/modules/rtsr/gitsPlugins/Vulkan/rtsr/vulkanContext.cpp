// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanContext.h"

#include <vector>

namespace gits::vulkan {

bool RtsrDeviceContext::BeginOperation() {
  std::lock_guard<std::mutex> lock(m_OperationMutex);
  if (Retiring.load()) {
    return false;
  }
  ++m_ActiveOperations;
  return true;
}

void RtsrDeviceContext::EndOperation() {
  std::lock_guard<std::mutex> lock(m_OperationMutex);
  if (m_ActiveOperations != 0 && --m_ActiveOperations == 0) {
    m_OperationsFinished.notify_all();
  }
}

void RtsrDeviceContext::Retire() {
  std::unique_lock<std::mutex> lock(m_OperationMutex);
  Retiring.store(true);
  m_OperationsFinished.wait(lock, [this] { return m_ActiveOperations == 0; });
}

void RtsrContextRegistry::AddInstance(GITSKey key,
                                      VkInstance handle,
                                      const VkInstanceLevelDispatchTable& dispatch) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Instances[key] = {key, handle, dispatch};
}

void RtsrContextRegistry::AddDevice(GITSKey key,
                                    GITSKey physicalDeviceKey,
                                    VkPhysicalDevice physicalDevice,
                                    VkDevice handle,
                                    const VkInstanceLevelDispatchTable& instanceDispatch,
                                    const VkDeviceLevelDispatchTable& dispatch) {
  auto context = std::make_shared<RtsrDeviceContext>();
  context->Key = key;
  context->PhysicalDeviceKey = physicalDeviceKey;
  context->PhysicalDevice = physicalDevice;
  context->Handle = handle;
  context->Dispatch = dispatch;
  if (instanceDispatch.vkGetPhysicalDeviceProperties != nullptr) {
    VkPhysicalDeviceProperties properties{};
    instanceDispatch.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    context->TimestampPeriod = properties.limits.timestampPeriod;
  }
  if (instanceDispatch.vkGetPhysicalDeviceQueueFamilyProperties != nullptr) {
    uint32_t count = 0;
    instanceDispatch.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    if (count != 0) {
      instanceDispatch.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count,
                                                                properties.data());
      properties.resize(count);
    }
    context->TimestampValidBits.reserve(properties.size());
    context->QueueFlags.reserve(properties.size());
    for (const auto& family : properties) {
      context->TimestampValidBits.push_back(family.timestampValidBits);
      context->QueueFlags.push_back(family.queueFlags);
    }
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto old = m_Devices.find(key);
  if (old != m_Devices.end()) {
    m_DeviceKeys.erase(old->second->Handle);
  }
  m_Devices[key] = context;
  m_DeviceKeys[handle] = key;
}

std::shared_ptr<RtsrDeviceContext> RtsrContextRegistry::Device(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto found = m_Devices.find(key);
  return found == m_Devices.end() ? nullptr : found->second;
}

std::shared_ptr<RtsrDeviceContext> RtsrContextRegistry::Device(VkDevice handle) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto key = m_DeviceKeys.find(handle);
  if (key == m_DeviceKeys.end()) {
    return nullptr;
  }
  const auto found = m_Devices.find(key->second);
  return found == m_Devices.end() ? nullptr : found->second;
}

std::shared_ptr<RtsrDeviceContext> RtsrContextRegistry::RetireDevice(GITSKey key) {
  std::shared_ptr<RtsrDeviceContext> context;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto found = m_Devices.find(key);
    if (found == m_Devices.end()) {
      return nullptr;
    }
    context = found->second;
    m_DeviceKeys.erase(context->Handle);
    m_Devices.erase(found);
  }
  context->Retire();
  return context;
}

bool RtsrContextRegistry::RemoveInstance(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Instances.erase(key) != 0 && m_Instances.empty();
}

void RtsrContextRegistry::Clear() {
  std::vector<std::shared_ptr<RtsrDeviceContext>> devices;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    devices.reserve(m_Devices.size());
    for (auto& entry : m_Devices) {
      devices.push_back(entry.second);
    }
    m_DeviceKeys.clear();
    m_Devices.clear();
    m_Instances.clear();
  }
  for (const auto& device : devices) {
    device->Retire();
  }
}

} // namespace gits::vulkan
