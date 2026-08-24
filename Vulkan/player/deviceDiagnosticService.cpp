// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "deviceDiagnosticService.h"

#include "configurator.h"
#include "log.h"
#include "playerManager.h"

#include <algorithm>
#include <cstring>
#include <iomanip>

namespace gits {
namespace vulkan {

thread_local std::vector<const char*> DeviceDiagnosticService::tl_DeviceExtensionNames;
thread_local VkPhysicalDeviceFaultFeaturesEXT DeviceDiagnosticService::tl_DeviceFaultFeatures;

namespace {

const char* DeviceFaultAddressTypeName(VkDeviceFaultAddressTypeEXT type) {
  switch (type) {
  case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:
    return "none";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
    return "invalid read";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
    return "invalid write";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
    return "invalid execute";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
    return "unknown instruction pointer";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
    return "invalid instruction pointer";
  case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
    return "instruction pointer fault";
  default:
    return "unknown";
  }
}

const char* PipelineStageName(VkPipelineStageFlagBits stage) {
  switch (stage) {
  case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:
    return "top of pipe";
  case VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT:
    return "draw indirect";
  case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:
    return "vertex input";
  case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
    return "vertex shader";
  case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:
    return "tessellation control shader";
  case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:
    return "tessellation evaluation shader";
  case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
    return "geometry shader";
  case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
    return "fragment shader";
  case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:
    return "early fragment tests";
  case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:
    return "late fragment tests";
  case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
    return "color attachment output";
  case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:
    return "compute shader";
  case VK_PIPELINE_STAGE_TRANSFER_BIT:
    return "transfer";
  case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:
    return "bottom of pipe";
  case VK_PIPELINE_STAGE_HOST_BIT:
    return "host";
  case VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT:
    return "all graphics";
  case VK_PIPELINE_STAGE_ALL_COMMANDS_BIT:
    return "all commands";
  default:
    return "unknown";
  }
}

template <typename T>
void ReplaceOrAppend(std::vector<T>& values, const T& value, auto predicate) {
  const auto it = std::find_if(values.begin(), values.end(), predicate);
  if (it == values.end()) {
    values.push_back(value);
  } else {
    *it = value;
  }
}

} // namespace

DeviceDiagnosticService::DeviceDiagnosticService(PlayerManager& manager) : m_Manager(manager) {
  const auto& playerConfig = Configurator::Get().vulkan.player;
  m_CheckpointsRequested = playerConfig.enableNvDeviceDiagnosticCheckpoints;
  m_DeviceFaultRequested = playerConfig.enableExtDeviceFault;
  if (m_CheckpointsRequested) {
    m_MarkerRing.resize(MarkerRingSize);
  }
}

bool DeviceDiagnosticService::HasExtension(const VkDeviceCreateInfo* createInfo,
                                           const char* extensionName) {
  if (!createInfo || !extensionName) {
    return false;
  }
  for (uint32_t i = 0; i < createInfo->enabledExtensionCount; ++i) {
    if (std::strcmp(createInfo->ppEnabledExtensionNames[i], extensionName) == 0) {
      return true;
    }
  }
  return false;
}

bool DeviceDiagnosticService::HasDeviceFaultFeature(const VkDeviceCreateInfo* createInfo) {
  if (!createInfo) {
    return false;
  }
  for (auto* node = static_cast<const VkBaseInStructure*>(createInfo->pNext); node;
       node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT) {
      return reinterpret_cast<const VkPhysicalDeviceFaultFeaturesEXT*>(node)->deviceFault ==
             VK_TRUE;
    }
  }
  return false;
}

bool DeviceDiagnosticService::IsExtensionSupported(VkPhysicalDevice physicalDevice,
                                                   const char* extensionName) {
  auto& dispatch = m_Manager.GetInstanceDispatchTable(physicalDevice);
  if (!dispatch.vkEnumerateDeviceExtensionProperties) {
    return false;
  }

  uint32_t extensionCount = 0;
  VkResult result = dispatch.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                                  &extensionCount, nullptr);
  if ((result != VK_SUCCESS && result != VK_INCOMPLETE) || extensionCount == 0) {
    return false;
  }

  std::vector<VkExtensionProperties> extensions(extensionCount);
  result = dispatch.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                                         extensions.data());
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    return false;
  }

  return std::any_of(extensions.begin(), extensions.end(), [extensionName](const auto& extension) {
    return std::strcmp(extension.extensionName, extensionName) == 0;
  });
}

bool DeviceDiagnosticService::IsDeviceFaultSupported(VkPhysicalDevice physicalDevice) {
  auto& dispatch = m_Manager.GetInstanceDispatchTable(physicalDevice);
  auto getFeatures = dispatch.vkGetPhysicalDeviceFeatures2
                         ? dispatch.vkGetPhysicalDeviceFeatures2
                         : dispatch.vkGetPhysicalDeviceFeatures2KHR;
  if (!getFeatures) {
    return false;
  }

  VkPhysicalDeviceFaultFeaturesEXT faultFeatures{};
  faultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &faultFeatures;
  getFeatures(physicalDevice, &features);
  return faultFeatures.deviceFault == VK_TRUE;
}

void DeviceDiagnosticService::EnableDeviceFaultFeature(VkDeviceCreateInfo* createInfo) {
  for (auto* node = static_cast<VkBaseOutStructure*>(const_cast<void*>(createInfo->pNext)); node;
       node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT) {
      reinterpret_cast<VkPhysicalDeviceFaultFeaturesEXT*>(node)->deviceFault = VK_TRUE;
      return;
    }
  }

  tl_DeviceFaultFeatures = {};
  tl_DeviceFaultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
  tl_DeviceFaultFeatures.pNext = const_cast<void*>(createInfo->pNext);
  tl_DeviceFaultFeatures.deviceFault = VK_TRUE;
  createInfo->pNext = &tl_DeviceFaultFeatures;
}

void DeviceDiagnosticService::PrepareDeviceCreate(VkPhysicalDevice physicalDevice,
                                                  VkDeviceCreateInfo* createInfo) {
  if (!createInfo || (!m_CheckpointsRequested && !m_DeviceFaultRequested)) {
    return;
  }

  const bool enableCheckpoints =
      m_CheckpointsRequested &&
      IsExtensionSupported(physicalDevice, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
  const bool enableDeviceFault =
      m_DeviceFaultRequested &&
      IsExtensionSupported(physicalDevice, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) &&
      IsDeviceFaultSupported(physicalDevice);

  if (m_CheckpointsRequested && !enableCheckpoints) {
    LOG_WARNING << "Vulkan.Player.NvDeviceDiagnosticCheckpoints was requested, but "
                   "VK_NV_device_diagnostic_checkpoints is not supported by the replay device.";
  }
  if (m_DeviceFaultRequested && !enableDeviceFault) {
    LOG_WARNING << "Vulkan.Player.ExtDeviceFault was requested, but VK_EXT_device_fault with the "
                   "deviceFault feature is not supported by the replay device.";
  }
  if (!enableCheckpoints && !enableDeviceFault) {
    return;
  }

  tl_DeviceExtensionNames.clear();
  if (createInfo->enabledExtensionCount > 0) {
    tl_DeviceExtensionNames.assign(createInfo->ppEnabledExtensionNames,
                                   createInfo->ppEnabledExtensionNames +
                                       createInfo->enabledExtensionCount);
  }
  auto appendExtension = [&](const char* extensionName) {
    if (!HasExtension(createInfo, extensionName)) {
      tl_DeviceExtensionNames.push_back(extensionName);
    }
  };
  if (enableCheckpoints) {
    appendExtension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
  }
  if (enableDeviceFault) {
    appendExtension(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    EnableDeviceFaultFeature(createInfo);
  }
  createInfo->enabledExtensionCount = static_cast<uint32_t>(tl_DeviceExtensionNames.size());
  createInfo->ppEnabledExtensionNames = tl_DeviceExtensionNames.data();
}

void DeviceDiagnosticService::TrackDevice(VkPhysicalDevice physicalDevice,
                                          VkDevice device,
                                          const VkDeviceCreateInfo* createInfo) {
  if ((!m_CheckpointsRequested && !m_DeviceFaultRequested) || !device || !createInfo) {
    return;
  }

  DeviceState state{};
  state.PhysicalDevice = physicalDevice;
  state.Device = device;
  state.CheckpointsEnabled =
      m_CheckpointsRequested &&
      HasExtension(createInfo, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
  state.DeviceFaultEnabled = m_DeviceFaultRequested &&
                             HasExtension(createInfo, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) &&
                             HasDeviceFaultFeature(createInfo);

  if (state.CheckpointsEnabled) {
    auto& dispatch = m_Manager.GetInstanceDispatchTable(physicalDevice);
    if (dispatch.vkGetPhysicalDeviceQueueFamilyProperties) {
      uint32_t familyCount = 0;
      dispatch.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
      std::vector<VkQueueFamilyProperties> properties(familyCount);
      dispatch.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount,
                                                        properties.data());
      state.QueueFamilyFlags.reserve(properties.size());
      for (const auto& property : properties) {
        state.QueueFamilyFlags.push_back(property.queueFlags);
      }
    }
  }

  ReplaceOrAppend(m_Devices, std::move(state),
                  [device](const DeviceState& current) { return current.Device == device; });

  LOG_INFO << "Vulkan device diagnostics enabled: checkpoints="
           << (FindDevice(device)->CheckpointsEnabled ? "true" : "false")
           << ", device fault=" << (FindDevice(device)->DeviceFaultEnabled ? "true" : "false")
           << ".";
}

void DeviceDiagnosticService::UntrackDevice(VkDevice device) {
  std::erase_if(m_Devices, [device](const DeviceState& state) { return state.Device == device; });
  std::erase_if(m_Queues, [device](const QueueState& state) { return state.Device == device; });
  std::erase_if(m_CommandPools,
                [device](const auto& entry) { return entry.second.Device == device; });
  std::erase_if(m_CommandBuffers,
                [device](const auto& entry) { return entry.second.Device == device; });
}

void DeviceDiagnosticService::TrackQueue(VkDevice device, VkQueue queue) {
  if (!device || !queue || !FindDevice(device)) {
    return;
  }
  ReplaceOrAppend(m_Queues, QueueState{device, queue},
                  [queue](const QueueState& state) { return state.Queue == queue; });
}

void DeviceDiagnosticService::TrackCommandPool(VkDevice device,
                                               VkCommandPool commandPool,
                                               uint32_t queueFamilyIndex) {
  if (!device || !commandPool || !FindDevice(device)) {
    return;
  }
  m_CommandPools[commandPool] = CommandPoolState{device, queueFamilyIndex};
}

void DeviceDiagnosticService::UntrackCommandPool(VkCommandPool commandPool) {
  m_CommandPools.erase(commandPool);
  std::erase_if(m_CommandBuffers, [commandPool](const auto& entry) {
    return entry.second.CommandPool == commandPool;
  });
}

void DeviceDiagnosticService::TrackCommandBuffers(VkDevice device,
                                                  VkCommandPool commandPool,
                                                  uint32_t commandBufferCount,
                                                  const VkCommandBuffer* commandBuffers) {
  const DeviceState* deviceState = FindDevice(device);
  if (!device || !commandPool || !commandBuffers || !deviceState) {
    return;
  }

  // Decide once whether this pool's buffers may carry checkpoints, so the
  // per-command path never has to walk pools or query queue families again.
  PFN_vkCmdSetCheckpointNV cmdSetCheckpoint = nullptr;
  const auto commandPoolIt = m_CommandPools.find(commandPool);
  constexpr VkQueueFlags supportedQueues =
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
  if (m_CheckpointsRequested && deviceState->CheckpointsEnabled &&
      commandPoolIt != m_CommandPools.end() &&
      commandPoolIt->second.QueueFamilyIndex < deviceState->QueueFamilyFlags.size() &&
      (deviceState->QueueFamilyFlags[commandPoolIt->second.QueueFamilyIndex] & supportedQueues) !=
          0) {
    cmdSetCheckpoint = m_Manager.GetDeviceDispatchTable(device).vkCmdSetCheckpointNV;
  }

  for (uint32_t i = 0; i < commandBufferCount; ++i) {
    if (!commandBuffers[i]) {
      continue;
    }
    m_CommandBuffers[commandBuffers[i]] = CommandBufferState{device, commandPool, cmdSetCheckpoint};
  }
}

void DeviceDiagnosticService::UntrackCommandBuffers(uint32_t commandBufferCount,
                                                    const VkCommandBuffer* commandBuffers) {
  if (!commandBuffers) {
    return;
  }
  for (uint32_t i = 0; i < commandBufferCount; ++i) {
    m_CommandBuffers.erase(commandBuffers[i]);
  }
}

const DeviceDiagnosticService::DeviceState* DeviceDiagnosticService::FindDevice(
    VkDevice device) const {
  const auto it =
      std::find_if(m_Devices.begin(), m_Devices.end(),
                   [device](const DeviceState& state) { return state.Device == device; });
  return it == m_Devices.end() ? nullptr : &*it;
}

VkDevice DeviceDiagnosticService::ResolveDevice(const void* dispatchableHandle) const {
  if (!dispatchableHandle) {
    return VK_NULL_HANDLE;
  }
  for (const auto& state : m_Devices) {
    if (reinterpret_cast<const void*>(state.Device) == dispatchableHandle) {
      return state.Device;
    }
  }
  for (const auto& state : m_Queues) {
    if (reinterpret_cast<const void*>(state.Queue) == dispatchableHandle) {
      return state.Device;
    }
  }
  const auto commandBufferIt =
      m_CommandBuffers.find(static_cast<VkCommandBuffer>(const_cast<void*>(dispatchableHandle)));
  if (commandBufferIt != m_CommandBuffers.end()) {
    return commandBufferIt->second.Device;
  }
  return VK_NULL_HANDLE;
}

VkQueue DeviceDiagnosticService::ResolveQueue(const void* dispatchableHandle) const {
  const auto it =
      std::find_if(m_Queues.begin(), m_Queues.end(), [dispatchableHandle](const QueueState& state) {
        return reinterpret_cast<const void*>(state.Queue) == dispatchableHandle;
      });
  return it == m_Queues.end() ? VK_NULL_HANDLE : it->Queue;
}

void DeviceDiagnosticService::InsertCheckpoint(VkCommandBuffer commandBuffer,
                                               uint64_t commandKey,
                                               const char* commandName) {
  if (!m_CheckpointsRequested || !commandBuffer) {
    return;
  }

  const auto commandBufferIt = m_CommandBuffers.find(commandBuffer);
  if (commandBufferIt == m_CommandBuffers.end() || !commandBufferIt->second.CmdSetCheckpoint) {
    return;
  }

  CheckpointMarker* marker = &m_MarkerRing[m_NextMarker];
  m_NextMarker = (m_NextMarker + 1) % MarkerRingSize;
  marker->CommandKey = commandKey;
  marker->CommandName = commandName;
  commandBufferIt->second.CmdSetCheckpoint(commandBuffer, marker);
}

const DeviceDiagnosticService::CheckpointMarker* DeviceDiagnosticService::FindMarker(
    const void* markerAddress) const {
  if (!markerAddress || m_MarkerRing.empty()) {
    return nullptr;
  }
  const auto base = reinterpret_cast<uintptr_t>(m_MarkerRing.data());
  const auto address = reinterpret_cast<uintptr_t>(markerAddress);
  const auto span = MarkerRingSize * sizeof(CheckpointMarker);
  if (address < base || address >= base + span ||
      (address - base) % sizeof(CheckpointMarker) != 0) {
    return nullptr;
  }
  return &m_MarkerRing[(address - base) / sizeof(CheckpointMarker)];
}

void DeviceDiagnosticService::OnDeviceLost(const char* commandName,
                                           uint64_t commandKey,
                                           const void* dispatchableHandle) {
  if ((!m_CheckpointsRequested && !m_DeviceFaultRequested) || m_DeviceLostHandled.exchange(true)) {
    return;
  }

  VkDevice lostDevice = ResolveDevice(dispatchableHandle);
  const VkQueue lostQueue = ResolveQueue(dispatchableHandle);
  if (!lostDevice && m_Devices.size() == 1) {
    lostDevice = m_Devices.front().Device;
  }

  LOG_ERROR << "Collecting Vulkan device diagnostics after " << commandName
            << " returned VK_ERROR_DEVICE_LOST at command key " << commandKey << ".";
  if (!lostDevice) {
    LOG_WARNING << "Could not associate the device-lost result with a replay device; diagnostics "
                   "cannot be queried safely.";
    return;
  }
  if (m_CheckpointsRequested) {
    DumpCheckpoints(lostDevice, lostQueue);
  }
  if (m_DeviceFaultRequested) {
    DumpDeviceFaults(lostDevice);
  }
}

void DeviceDiagnosticService::DumpCheckpoints(VkDevice lostDevice, VkQueue lostQueue) {
  const DeviceState* device = FindDevice(lostDevice);
  if (!device || !device->CheckpointsEnabled) {
    LOG_WARNING << "VK_NV_device_diagnostic_checkpoints was not enabled on the lost device.";
    return;
  }

  bool queueFound = false;
  bool checkpointFound = false;
  std::vector<const QueueState*> queues;
  for (const QueueState& queue : m_Queues) {
    if (queue.Device == lostDevice) {
      queues.push_back(&queue);
    }
  }
  std::stable_sort(queues.begin(), queues.end(), [lostQueue](const auto* lhs, const auto* rhs) {
    return lhs->Queue == lostQueue && rhs->Queue != lostQueue;
  });

  for (const QueueState* queueState : queues) {
    const QueueState& queue = *queueState;
    queueFound = true;
    const char* queueRole = queue.Queue == lostQueue ? " (queue reporting device loss)" : "";
    auto& dispatch = m_Manager.GetDeviceDispatchTable(queue.Queue);
    if (!dispatch.vkGetQueueCheckpointDataNV) {
      continue;
    }

    uint32_t checkpointCount = 0;
    dispatch.vkGetQueueCheckpointDataNV(queue.Queue, &checkpointCount, nullptr);
    if (checkpointCount == 0) {
      // Worth saying out loud - a silent skip here reads as "this queue was
      // fine" when it may simply have had nothing submitted yet.
      LOG_ERROR << "VK_NV_device_diagnostic_checkpoints: queue " << queue.Queue << queueRole
                << " reported no checkpoint data.";
      continue;
    }
    std::vector<VkCheckpointDataNV> checkpoints(checkpointCount);
    for (auto& checkpoint : checkpoints) {
      checkpoint.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
    }
    const uint32_t capacity = checkpointCount;
    dispatch.vkGetQueueCheckpointDataNV(queue.Queue, &checkpointCount, checkpoints.data());

    for (uint32_t i = 0; i < std::min(checkpointCount, capacity); ++i) {
      checkpointFound = true;
      const VkCheckpointDataNV& checkpoint = checkpoints[i];
      const CheckpointMarker* found = FindMarker(checkpoint.pCheckpointMarker);
      if (found) {
        const CheckpointMarker& marker = *found;
        LOG_ERROR << "VK_NV_device_diagnostic_checkpoints: queue " << queue.Queue << queueRole
                  << ", " << PipelineStageName(checkpoint.stage) << " stage (0x" << std::hex
                  << static_cast<uint32_t>(checkpoint.stage) << std::dec
                  << ") reached the checkpoint before " << marker.CommandName << " at command key "
                  << marker.CommandKey << ".";
      } else {
        LOG_ERROR << "VK_NV_device_diagnostic_checkpoints: queue " << queue.Queue << queueRole
                  << ", " << PipelineStageName(checkpoint.stage) << " stage (0x" << std::hex
                  << static_cast<uint32_t>(checkpoint.stage) << std::dec
                  << ") returned an application checkpoint marker " << checkpoint.pCheckpointMarker
                  << ".";
      }
    }
  }

  if (!queueFound) {
    LOG_WARNING << "VK_NV_device_diagnostic_checkpoints: no queues were tracked for the lost "
                   "device.";
  } else if (!checkpointFound) {
    LOG_WARNING << "VK_NV_device_diagnostic_checkpoints: the driver returned no checkpoint data.";
  }
}

void DeviceDiagnosticService::DumpDeviceFaults(VkDevice lostDevice) {
  const DeviceState* device = FindDevice(lostDevice);
  if (!device || !device->DeviceFaultEnabled) {
    LOG_WARNING << "VK_EXT_device_fault was not enabled on the lost device.";
    return;
  }

  auto& dispatch = m_Manager.GetDeviceDispatchTable(lostDevice);
  if (!dispatch.vkGetDeviceFaultInfoEXT) {
    LOG_WARNING << "VK_EXT_device_fault was enabled, but vkGetDeviceFaultInfoEXT is unavailable.";
    return;
  }

  VkDeviceFaultCountsEXT counts{};
  counts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
  VkResult result = dispatch.vkGetDeviceFaultInfoEXT(lostDevice, &counts, nullptr);
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    LOG_WARNING << "vkGetDeviceFaultInfoEXT failed while querying fault counts with VkResult "
                << static_cast<int32_t>(result) << ".";
    return;
  }

  std::vector<VkDeviceFaultAddressInfoEXT> addressInfos(counts.addressInfoCount);
  std::vector<VkDeviceFaultVendorInfoEXT> vendorInfos(counts.vendorInfoCount);
  const uint32_t addressCapacity = counts.addressInfoCount;
  const uint32_t vendorCapacity = counts.vendorInfoCount;

  VkDeviceFaultInfoEXT faultInfo{};
  faultInfo.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
  faultInfo.pAddressInfos = addressInfos.empty() ? nullptr : addressInfos.data();
  faultInfo.pVendorInfos = vendorInfos.empty() ? nullptr : vendorInfos.data();
  // Vendor binary data is optional and can be large. The requested diagnostics
  // need the human-readable information and faulting virtual addresses only.
  faultInfo.pVendorBinaryData = nullptr;

  result = dispatch.vkGetDeviceFaultInfoEXT(lostDevice, &counts, &faultInfo);
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    LOG_WARNING << "vkGetDeviceFaultInfoEXT failed while querying fault details with VkResult "
                << static_cast<int32_t>(result) << ".";
    return;
  }

  if (faultInfo.description[0] != '\0') {
    LOG_ERROR << "VK_EXT_device_fault: " << faultInfo.description;
  }
  for (uint32_t i = 0; i < std::min(counts.addressInfoCount, addressCapacity); ++i) {
    const auto& addressInfo = addressInfos[i];
    LOG_ERROR << "VK_EXT_device_fault address " << i << ": "
              << DeviceFaultAddressTypeName(addressInfo.addressType) << " at 0x" << std::hex
              << addressInfo.reportedAddress << std::dec << " (precision "
              << addressInfo.addressPrecision << " bytes).";
  }
  for (uint32_t i = 0; i < std::min(counts.vendorInfoCount, vendorCapacity); ++i) {
    const auto& vendorInfo = vendorInfos[i];
    LOG_ERROR << "VK_EXT_device_fault vendor info " << i << ": " << vendorInfo.description
              << ", code 0x" << std::hex << vendorInfo.vendorFaultCode << ", data 0x"
              << vendorInfo.vendorFaultData << std::dec << ".";
  }
  if (counts.addressInfoCount == 0 && counts.vendorInfoCount == 0 &&
      faultInfo.description[0] == '\0') {
    LOG_WARNING << "VK_EXT_device_fault: the driver returned no fault details.";
  }
}

} // namespace vulkan
} // namespace gits
