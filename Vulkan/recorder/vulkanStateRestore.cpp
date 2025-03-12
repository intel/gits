// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateRestore.cpp
*
* @brief Definition of Vulkan State Restore.
*
*/

#include "recorder.h"
#include "vulkanLog.h"
#include "vulkanPreToken.h"
#include "vulkanFunctions.h"
#include "vulkanStateRestore.h"
#include "vulkanStateTracking.h"

void ScheduleTokens(gits::Vulkan::CFunction* token) {
  gits::CRecorder::Instance().Scheduler().Register(token);
}

namespace {

std::map<VkDevice, gits::Vulkan::TemporaryDeviceResourcesStruct> temporaryDeviceResources;
std::map<uint64_t, VkDevice> temporaryDescriptorSetLayouts;
const uint64_t globalTimeoutValue = 3000000000;

} // namespace

gits::Vulkan::SubmittableResourcesStruct const& gits::Vulkan::GetSubmitableResources(
    CScheduler& scheduler, VkDevice device) {
  auto& deviceResources = temporaryDeviceResources[device];
  auto& submitableResources =
      deviceResources.submitableResources[deviceResources.currentResourceIndex];

  // Wait until previous submission ends
  {
    auto result =
        drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("Waiting on a temporary fence failed!");
    }
    drvVk.vkResetFences(device, 1, &submitableResources.fence);

    scheduler.Register(new CvkWaitForFences(VK_SUCCESS, device, 1, &submitableResources.fence,
                                            VK_FALSE, globalTimeoutValue));
    scheduler.Register(new CvkResetFences(VK_SUCCESS, device, 1, &submitableResources.fence));

    // Reset state of all previously used buffers
    deviceResources.freeBuffers.insert(
        deviceResources.usedBuffers[submitableResources.fence].begin(),
        deviceResources.usedBuffers[submitableResources.fence].end());
    deviceResources.usedBuffers[submitableResources.fence].clear();
  }

  // Start recording temporary command buffer
  {
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};
    scheduler.Register(new CvkBeginCommandBuffer(VK_SUCCESS, submitableResources.commandBuffer,
                                                 &commandBufferBeginInfo));
  }

  deviceResources.currentResourceIndex =
      (deviceResources.currentResourceIndex + 1) % deviceResources.submitableResources.size();

  return submitableResources;
}

void gits::Vulkan::SubmitWork(CScheduler& scheduler,
                              SubmittableResourcesStruct const& submitableResources,
                              bool signalFence,
                              std::vector<VkSemaphore> const& semaphoresToSignal) {
  scheduler.Register(new CvkEndCommandBuffer(VK_SUCCESS, submitableResources.commandBuffer));

  // Empty submission used to signal recorder-side fence
  if (signalFence) {
    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, // VkStructureType sType;
        nullptr,                       // const void* pNext;
        0,                             // uint32_t waitSemaphoreCount;
        nullptr,                       // const VkSemaphore* pWaitSemaphores;
        nullptr,                       // const VkPipelineStageFlags* pWaitDstStageMask;
        0,                             // uint32_t commandBufferCount;
        nullptr,                       // const VkCommandBuffer* pCommandBuffers;
        0,                             // uint32_t signalSemaphoreCount;
        nullptr                        // const VkSemaphore* pSignalSemaphores;
    };
    drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
  }
  // Scheduled, "real" submission
  {
    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType sType;
        nullptr,                            // const void* pNext;
        0,                                  // uint32_t waitSemaphoreCount;
        nullptr,                            // const VkSemaphore* pWaitSemaphores;
        nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
        1,                                  // uint32_t commandBufferCount;
        &submitableResources.commandBuffer, // const VkCommandBuffer* pCommandBuffers;
        static_cast<uint32_t>(semaphoresToSignal.size()), // uint32_t signalSemaphoreCount;
        semaphoresToSignal.size() > 0 ? semaphoresToSignal.data()
                                      : nullptr // const VkSemaphore* pSignalSemaphores;
    };
    scheduler.Register(new CvkQueueSubmit(VK_SUCCESS, submitableResources.queue, 1, &submitInfo,
                                          submitableResources.fence));
  }
}

gits::Vulkan::TemporaryBufferStruct gits::Vulkan::CreateTemporaryBuffer(CScheduler& scheduler,
                                                                        VkDevice device,
                                                                        VkDeviceSize size) {
  TemporaryBufferStruct newBuffer = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_WHOLE_SIZE, nullptr};
  static VkPhysicalDeviceMemoryProperties memoryProperties =
      SD()._devicestates[device]->physicalDeviceStateStore->memoryPropertiesCurrent;
  VkMemoryRequirements bufferMemoryRequirements = {0, 0, 0};
  VkMemoryAllocateInfo memoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, 0,
                                             UINT32_MAX};

  // Create local buffer
  {
    VkBufferCreateInfo localBufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
        nullptr,                              // const void* pNext;
        0,                                    // VkBufferCreateFlags flags;
        size,                                 // VkDeviceSize size;
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT, // VkBufferUsageFlags usage;
        VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode sharingMode;
        0,                                    // uint32_t queueFamilyIndexCount;
        nullptr                               // const uint32_t* pQueueFamilyIndices;
    };

    VkResult result =
        drvVk.vkCreateBuffer(device, &localBufferCreateInfo, nullptr, &newBuffer.buffer);
    if (result != VK_SUCCESS) {
      VkLog(ERR) << "Could not create buffer: " << result << "!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    drvVk.vkGetBufferMemoryRequirements(device, newBuffer.buffer, &bufferMemoryRequirements);

    bool allocatedMemory = false;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
      if (isBitSet(bufferMemoryRequirements.memoryTypeBits, (uint64_t)1 << i) &&
          isBitSet(memoryProperties.memoryTypes[i].propertyFlags,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        memoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                              bufferMemoryRequirements.size, i};

        result = drvVk.vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &newBuffer.memory);
        if (result != VK_SUCCESS) {
          VkLog(ERR) << "Could not allocate memory for a buffer using the " << i
                     << " memory type due to: " << result << "!!!";
        } else {
          allocatedMemory = true;
          break;
        }
      }
    }

    if (!allocatedMemory) {
      Log(ERR) << "Could not allocate memory for a buffer!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    result = drvVk.vkBindBufferMemory(device, newBuffer.buffer, newBuffer.memory, 0);
    if (result != VK_SUCCESS) {
      Log(ERR) << "Could not bind memory to a buffer!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    newBuffer.size = size;

    result = drvVk.vkMapMemory(device, newBuffer.memory, 0, VK_WHOLE_SIZE, 0, &newBuffer.mappedPtr);
    if (result != VK_SUCCESS) {
      Log(ERR) << "vkMapMemory() on the temporary buffer returned the following error: " << result;
    }
  }

  // Schedule buffer creation
  {
    VkBufferCreateInfo scheduledBufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
        nullptr,                              // const void* pNext;
        0,                                    // VkBufferCreateFlags flags;
        size,                                 // VkDeviceSize size;
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,     // VkBufferUsageFlags usage;
        VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode sharingMode;
        0,                                    // uint32_t queueFamilyIndexCount;
        nullptr                               // const uint32_t* pQueueFamilyIndices;
    };
    scheduler.Register(new CvkCreateBuffer(VK_SUCCESS, device, &scheduledBufferCreateInfo, nullptr,
                                           &newBuffer.buffer));
    scheduler.Register(
        new CvkAllocateMemory(VK_SUCCESS, device, &memoryAllocateInfo, nullptr, &newBuffer.memory));
    scheduler.Register(
        new CvkBindBufferMemory(VK_SUCCESS, device, newBuffer.buffer, newBuffer.memory, 0));
    scheduler.Register(new CvkMapMemory(VK_SUCCESS, device, newBuffer.memory, 0, VK_WHOLE_SIZE, 0,
                                        &newBuffer.mappedPtr));
  }
  auto& deviceResources = temporaryDeviceResources[device];
  deviceResources.temporaryBuffers[newBuffer.buffer] = newBuffer;
  deviceResources.freeBuffers.insert(newBuffer.buffer);

  return newBuffer;
}

gits::Vulkan::TemporaryBufferStruct gits::Vulkan::GetTemporaryBuffer(
    CScheduler& scheduler, VkDevice device, SubmittableResourcesStruct& submitableResources) {
  auto& deviceResources = temporaryDeviceResources[device];
  if (deviceResources.freeBuffers.empty()) {
    throw std::runtime_error("There are no temporary buffers to restore resources!");
  }

  // All the temporary buffers have the same size so take the first available buffer
  VkBuffer buffer = *deviceResources.freeBuffers.begin();
  TemporaryBufferStruct foundBuffer = deviceResources.temporaryBuffers[buffer];

  deviceResources.usedBuffers[submitableResources.fence].insert(buffer);
  deviceResources.freeBuffers.erase(buffer);

  return foundBuffer;
}

// Instance

void gits::Vulkan::RestoreVkInstances(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& instanceState : sd._instancestates) {
    VkInstance instance = instanceState.first;
    if (IsObjectToSkip((uint64_t)instance)) {
      continue;
    }

    scheduler.Register(new CvkCreateInstance(
        VK_SUCCESS, instanceState.second->instanceCreateInfoData.Value(), nullptr, &instance));
  }
}

// Physical devices

void gits::Vulkan::RestoreVkPhysicalDevices(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<VkInstance, std::vector<VkPhysicalDevice>> physicalDevices;

  for (auto& physicalDeviceState : sd._physicaldevicestates) {
    VkPhysicalDevice physicalDevice = physicalDeviceState.first;
    if (IsObjectToSkip((uint64_t)physicalDevice)) {
      continue;
    }

    physicalDevices[physicalDeviceState.second->instanceStateStore->instanceHandle].push_back(
        physicalDevice);
  }

  // Restore physical devices for all instances
  for (auto& state : physicalDevices) {
    uint32_t physicalDevicesCount = static_cast<uint32_t>(state.second.size());
    scheduler.Register(new CvkEnumeratePhysicalDevices(VK_SUCCESS, state.first,
                                                       &physicalDevicesCount, state.second.data()));
  }
}

// Surface

void gits::Vulkan::RestoreSurfaceKHR(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& surfaceState : sd._surfacekhrstates) {
    VkSurfaceKHR surface = surfaceState.first;
    if (IsObjectToSkip((uint64_t)surface)) {
      continue;
    }

#ifdef GITS_PLATFORM_X11
    VkXcbSurfaceCreateInfoKHR* xcbCreateInfoPtr =
        surfaceState.second->surfaceCreateInfoXcbData.Value();
    VkXlibSurfaceCreateInfoKHR* xlibCreateInfoPtr =
        surfaceState.second->surfaceCreateInfoXlibData.Value();
    VkInstance instance = surfaceState.second->instanceStateStore->instanceHandle;
    if (xcbCreateInfoPtr != nullptr) {
      scheduler.Register(
          new CGitsVkCreateNativeWindow(xcbCreateInfoPtr->connection, xcbCreateInfoPtr->window));
      scheduler.Register(
          new CvkCreateXcbSurfaceKHR(VK_SUCCESS, instance, xcbCreateInfoPtr, nullptr, &surface));
    } else if (xlibCreateInfoPtr != nullptr) {
      scheduler.Register(
          new CGitsVkCreateXlibWindow(xlibCreateInfoPtr->dpy, xlibCreateInfoPtr->window));
      scheduler.Register(
          new CvkCreateXlibSurfaceKHR(VK_SUCCESS, instance, xlibCreateInfoPtr, nullptr, &surface));
    }
#else
    scheduler.Register(new CGitsVkCreateNativeWindow(
        surfaceState.second->surfaceCreateInfoWin32Data.Value()->hinstance,
        surfaceState.second->surfaceCreateInfoWin32Data.Value()->hwnd));
    scheduler.Register(new CvkCreateWin32SurfaceKHR(
        VK_SUCCESS, surfaceState.second->instanceStateStore->instanceHandle,
        surfaceState.second->surfaceCreateInfoWin32Data.Value(), nullptr, &surface));
#endif
  }

#ifdef GITS_PLATFORM_WINDOWS
  scheduler.Register(new CGitsVkEnumerateDisplayMonitors(true));
#endif
}

// Device

void gits::Vulkan::RestoreVkDevices(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceState : sd._devicestates) {
    if (IsObjectToSkip((uint64_t)deviceState.first)) {
      continue;
    }

    VkPhysicalDevice physicalDevice =
        deviceState.second->physicalDeviceStateStore->physicalDeviceHandle;
    VkDevice device = deviceState.first;
    auto& deviceResources = temporaryDeviceResources[device];
    deviceResources.physicalDevice = physicalDevice;

    // Perform queue family properties queries to prevent Validation Layers
    // errors and to select proper queue family for data copies
    {
      uint32_t count = 0;
      drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
      scheduler.Register(
          new CvkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr));
      std::vector<VkQueueFamilyProperties> queueFamilyProperties(count);
      drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count,
                                                     queueFamilyProperties.data());
      scheduler.Register(new CvkGetPhysicalDeviceQueueFamilyProperties(
          physicalDevice, &count, queueFamilyProperties.data()));

      bool queueFound = false;

      for (uint32_t familyIndex = 0; familyIndex < queueFamilyProperties.size(); ++familyIndex) {
        if (isBitSet(queueFamilyProperties[familyIndex].queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
          for (auto& queueState : deviceState.second->queueStateStoreList) {
            if (queueState->queueFamilyIndex == familyIndex) {
              deviceResources.queue = queueState->queueHandle;
              deviceResources.queueFamilyIndex = familyIndex;
              queueFound = true;
              break;
            }
          }
        }
        if (queueFound) {
          break;
        }
      }
    }

    {
      VkPhysicalDeviceMemoryProperties memoryProperties;
      drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
      scheduler.Register(
          new CvkPassPhysicalDeviceMemoryPropertiesGITS(physicalDevice, &memoryProperties));
    }
    scheduler.Register(new CvkCreateDevice(VK_SUCCESS, physicalDevice,
                                           deviceState.second->deviceCreateInfoData.Value(),
                                           nullptr, &device));
  }
}

void gits::Vulkan::RestoreVkQueue(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceState : sd._devicestates) {
    VkDevice device = deviceState.first;

    if (IsObjectToSkip((uint64_t)device)) {
      continue;
    }

    for (auto& queueState : deviceState.second->queueStateStoreList) {
      VkQueue queue = queueState->queueHandle;

      if (IsObjectToSkip((uint64_t)queue) && (queue != temporaryDeviceResources[device].queue)) {
        continue;
      }

      scheduler.Register(new CvkGetDeviceQueue(device, queueState->queueFamilyIndex,
                                               queueState->queueIndex, &queue));
    }
  }
}

// Temporary resources

void gits::Vulkan::PrepareTemporaryResources(CScheduler& scheduler, CStateDynamic& sd) {
  if (temporaryDeviceResources.empty()) {
    return;
  }

  for (auto& deviceResourcesPair : temporaryDeviceResources) {
    VkDevice device = deviceResourcesPair.first;
    auto& deviceResources = deviceResourcesPair.second;

    deviceResources.submitableResources.resize(
        Config::Get().vulkan.recorder.reusableStateRestoreResourcesCount,
        {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE});

    // Create temporary command pool
    {
      VkCommandPoolCreateInfo commandPoolCreateInfo = {
          VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // VkStructureType sType;
          nullptr,                                    // const void* pNext;
          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // VkCommandPoolCreateFlags flags;
          sd._queuestates[deviceResources.queue]->queueFamilyIndex // uint32_t queueFamilyIndex;
      };
      if (drvVk.vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr,
                                    &deviceResources.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create temporary command pool!");
      }
      scheduler.Register(new CvkCreateCommandPool(VK_SUCCESS, device, &commandPoolCreateInfo,
                                                  nullptr, &deviceResources.commandPool));
    }

    for (uint32_t i = 0; i < deviceResources.submitableResources.size(); ++i) {
      // Allocate temporary command buffers
      VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType sType;
          nullptr,                                        // const void* pNext;
          deviceResources.commandPool,                    // VkCommandPool commandPool;
          VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel level;
          1                                               // uint32_t commandBufferCount;
      };
      if (drvVk.vkAllocateCommandBuffers(device, &commandBufferAllocateInfo,
                                         &deviceResources.submitableResources[i].commandBuffer) !=
          VK_SUCCESS) {
        throw std::runtime_error("Could not create temporary command buffer!");
      }
      drvVk.SetDispatchKey(device, deviceResources.submitableResources[i].commandBuffer);
      scheduler.Register(
          new CvkAllocateCommandBuffers(VK_SUCCESS, device, &commandBufferAllocateInfo,
                                        &deviceResources.submitableResources[i].commandBuffer));

      // Create fences to know the command buffer submission state
      VkFenceCreateInfo fenceCreateInfo = {
          VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // VkStructureType sType;
          nullptr,                             // const void* pNext;
          VK_FENCE_CREATE_SIGNALED_BIT         // VkFenceCreateFlags flags;
      };
      if (drvVk.vkCreateFence(device, &fenceCreateInfo, nullptr,
                              &deviceResources.submitableResources[i].fence) != VK_SUCCESS) {
        throw std::runtime_error("Could not create temporary fence!");
      }
      scheduler.Register(new CvkCreateFence(VK_SUCCESS, device, &fenceCreateInfo, nullptr,
                                            &deviceResources.submitableResources[i].fence));

      // Store queue handle
      deviceResources.submitableResources[i].queue = deviceResources.queue;
    }

    // Calculate a max size for temporary buffers used to restore contents of resources.
    // Temporary buffers are created only when needed (in RestoreImageContents(),
    // RestoreBufferContents() and RestoreAccelerationStructureContents()) but their
    // size is checked here for convenience.
    deviceResources.maxBufferSize = 0;

    // Check the size of the largest buffer
    for (auto& bufferAndStatePair : sd._bufferstates) {
      if (IsObjectToSkip((uint64_t)bufferAndStatePair.first)) {
        continue;
      }

      auto& bufferState = bufferAndStatePair.second;
      auto* pCreateInfo = bufferState->bufferCreateInfoData.Value();
      if ((pCreateInfo == nullptr) || (pCreateInfo->size == 0) ||
          (bufferState->binding == nullptr) ||
          (bufferState->deviceStateStore->deviceHandle != device)) {
        continue;
      }

      if ((TBufferStateRestoration::WITH_NON_HOST_VISIBLE_MEMORY_ONLY ==
           Config::Get().vulkan.recorder.crossPlatformStateRestoration.buffers) &&
          (checkMemoryMappingFeasibility(
              device,
              bufferState->binding->deviceMemoryStateStore->memoryAllocateInfoData.Value()
                  ->memoryTypeIndex,
              false))) {
        continue;
      }

      if (pCreateInfo->size > deviceResources.maxBufferSize) {
        deviceResources.maxBufferSize = pCreateInfo->size;
      }
    }

    for (auto& accelerationStructureAndStatePair : sd._accelerationstructurekhrstates) {
      if (IsObjectToSkip((uint64_t)accelerationStructureAndStatePair.first)) {
        continue;
      }

      auto& accelerationStructureState = accelerationStructureAndStatePair.second;

      if ((accelerationStructureState->bufferStateStore->deviceStateStore->deviceHandle !=
           device) ||
          (!accelerationStructureState->buildInfo)) {
        continue;
      }

      if (accelerationStructureState->buildSizeInfo.accelerationStructureSize >
          deviceResources.maxBufferSize) {
        deviceResources.maxBufferSize =
            accelerationStructureState->buildSizeInfo.accelerationStructureSize;
      }
    }

    deviceResources.maxBufferSize =
        std::max(static_cast<VkDeviceSize>(
                     Config::Get().vulkan.recorder.reusableStateRestoreBufferSize * 1000000),
                 deviceResources.maxBufferSize);
  }
}

// Swapchain

void gits::Vulkan::RestoreSwapchainKHR(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& swapchainState : sd._swapchainkhrstates) {
    if (IsObjectToSkip((uint64_t)swapchainState.first)) {
      continue;
    }

    {
      // Schedule fake surface parameter queries to prevent Validation Layers errors
      VkSurfaceKHR surface = swapchainState.second->surfaceKHRStateStore->surfaceKHRHandle;
      VkPhysicalDevice physicalDevice =
          swapchainState.second->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle;

      {
        uint32_t count = 0;
        drvVk.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
        scheduler.Register(new CvkGetPhysicalDeviceSurfaceFormatsKHR(VK_SUCCESS, physicalDevice,
                                                                     surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> surfaceFormat(count);
        drvVk.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count,
                                                   surfaceFormat.data());
        scheduler.Register(new CvkGetPhysicalDeviceSurfaceFormatsKHR(
            VK_SUCCESS, physicalDevice, surface, &count, surfaceFormat.data()));
      }
      {
        uint32_t count = 0;
        drvVk.vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
        scheduler.Register(new CvkGetPhysicalDeviceSurfacePresentModesKHR(
            VK_SUCCESS, physicalDevice, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> presentModes(count);
        drvVk.vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count,
                                                        presentModes.data());
        scheduler.Register(new CvkGetPhysicalDeviceSurfacePresentModesKHR(
            VK_SUCCESS, physicalDevice, surface, &count, presentModes.data()));
      }
      {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        drvVk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                        &surfaceCapabilities);
        scheduler.Register(new CvkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            VK_SUCCESS, physicalDevice, surface, &surfaceCapabilities));
      }
      {
        for (auto& queueState : swapchainState.second->deviceStateStore->queueStateStoreList) {
          VkBool32 supported = false;
          drvVk.vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueState->queueFamilyIndex,
                                                     surface, &supported);
          scheduler.Register(new CvkGetPhysicalDeviceSurfaceSupportKHR(
              VK_SUCCESS, physicalDevice, queueState->queueFamilyIndex, surface, &supported));
        }
      }
    }

    VkSwapchainKHR swapchain = swapchainState.first;
    VkSwapchainCreateInfoKHR swapchainCreateInfo =
        *swapchainState.second->swapchainCreateInfoKHRData.Value();
    uint32_t imageCount = static_cast<uint32_t>(swapchainState.second->imageStateStoreList.size());
    std::vector<VkImage> swapchainImages;
    for (auto& imageState : swapchainState.second->imageStateStoreList) {
      swapchainImages.push_back(imageState->imageHandle);
    }

    if ((VK_NULL_HANDLE != swapchainCreateInfo.oldSwapchain) &&
        (sd._swapchainkhrstates.find(swapchainCreateInfo.oldSwapchain) ==
         sd._swapchainkhrstates.end())) {
      swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    }
    swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    scheduler.Register(
        new CvkCreateSwapchainKHR(VK_SUCCESS, swapchainState.second->deviceStateStore->deviceHandle,
                                  &swapchainCreateInfo, nullptr, &swapchain));
    scheduler.Register(new CvkGetSwapchainImagesKHR(
        VK_SUCCESS, swapchainState.second->deviceStateStore->deviceHandle, swapchain, &imageCount,
        swapchainImages.data()));

    for (auto imageIndex : swapchainState.second->acquiredImages) {
      scheduler.Register(new CvkAcquireNextImageKHR(
          VK_SUCCESS, swapchainState.second->deviceStateStore->deviceHandle, swapchain, 3000000000,
          VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex));
    }
  }
}

// Descriptor pool

void gits::Vulkan::RestoreVkDescriptorPool(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& descriptorPoolState : sd._descriptorpoolstates) {
    VkDescriptorPool descriptorPool = descriptorPoolState.first;
    if (IsObjectToSkip((uint64_t)descriptorPool)) {
      continue;
    }
    scheduler.Register(new CvkCreateDescriptorPool(
        VK_SUCCESS, descriptorPoolState.second->deviceStateStore->deviceHandle,
        descriptorPoolState.second->descriptorPoolCreateInfoData.Value(), nullptr,
        &descriptorPool));
  }
}

// Command pool

void gits::Vulkan::RestoreVkCommandPool(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& commandPoolState : sd._commandpoolstates) {
    VkCommandPool commandPool = commandPoolState.first;
    if (IsObjectToSkip((uint64_t)commandPool)) {
      continue;
    }
    scheduler.Register(new CvkCreateCommandPool(
        VK_SUCCESS, commandPoolState.second->deviceStateStore->deviceHandle,
        commandPoolState.second->commandPoolCreateInfoData.Value(), nullptr, &commandPool));
  }
}

// Sampler

void gits::Vulkan::RestoreSampler(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& samplerState : sd._samplerstates) {
    VkSampler sampler = samplerState.first;
    if (IsObjectToSkip((uint64_t)sampler)) {
      continue;
    }
    scheduler.Register(new CvkCreateSampler(
        VK_SUCCESS, samplerState.second->deviceStateStore->deviceHandle,
        samplerState.second->samplerCreateInfoData.Value(), nullptr, &sampler));
  }
}

// Device memory

void gits::Vulkan::RestoreMemory(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceMemoryState : sd._devicememorystates) {
    VkDeviceMemory memory = deviceMemoryState.first;
    if (IsObjectToSkip((uint64_t)memory)) {
      continue;
    }
    scheduler.Register(new CvkAllocateMemory(
        VK_SUCCESS, deviceMemoryState.second->deviceStateStore->deviceHandle,
        deviceMemoryState.second->memoryAllocateInfoData.Value(), nullptr, &memory));
  }
}

void gits::Vulkan::RestoreMappedMemory(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceMemoryState : sd._devicememorystates) {
    if (IsObjectToSkip((uint64_t)deviceMemoryState.first)) {
      continue;
    }

    if (checkMemoryMappingFeasibility(
            deviceMemoryState.second->deviceStateStore->deviceHandle,
            deviceMemoryState.second->memoryAllocateInfoData.Value()->memoryTypeIndex, false)) {
      if (TMemoryStateRestoration::NONE != Config::Get().vulkan.recorder.memoryRestoration) {
        scheduler.Register(new CGitsVkMemoryRestore(
            deviceMemoryState.second->deviceStateStore->deviceHandle, deviceMemoryState.first,
            deviceMemoryState.second->memoryAllocateInfoData.Value()->allocationSize));
      }
      if (deviceMemoryState.second->IsMapped()) {
        VkDeviceMemory deviceMemory = deviceMemoryState.first;
        auto& mapping = deviceMemoryState.second->mapping;
        void* data = mapping->ppDataData.Value();
        scheduler.Register(
            new CvkMapMemory(VK_SUCCESS, deviceMemoryState.second->deviceStateStore->deviceHandle,
                             deviceMemory, mapping->offsetData.Value(), mapping->sizeData.Value(),
                             mapping->flagsData.Value(), &data));
      }
    }
  }
}

// Image

void gits::Vulkan::RestoreImage(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& imageState : sd._imagestates) {
    VkImage image = imageState.first;
    if (IsObjectToSkip((uint64_t)image)) {
      continue;
    }

    if (nullptr != imageState.second->imageCreateInfoData.Value()) {
      auto imageCreateInfo = *imageState.second->imageCreateInfoData.Value();
      if (Config::Get().vulkan.recorder.crossPlatformStateRestoration.images) {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      }
      scheduler.Register(new CvkCreateImage(VK_SUCCESS,
                                            imageState.second->deviceStateStore->deviceHandle,
                                            &imageCreateInfo, nullptr, &image));
    }
  }
}

void gits::Vulkan::RestoreImageBindings(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& imageState : sd._imagestates) {
    VkImage image = imageState.first;
    if (IsObjectToSkip((uint64_t)image)) {
      continue;
    }

    if (imageState.second->binding) {
      auto it = sd._devicememorystates.find(
          imageState.second->binding->deviceMemoryStateStore->deviceMemoryHandle);
      if ((it != sd._devicememorystates.end()) &&
          imageState.second->binding->deviceMemoryStateStore->GetUniqueStateID() ==
              it->second->GetUniqueStateID()) {
        scheduler.Register(new CvkBindImageMemory(
            VK_SUCCESS, imageState.second->deviceStateStore->deviceHandle, image,
            imageState.second->binding->deviceMemoryStateStore->deviceMemoryHandle,
            imageState.second->binding->memoryOffset));
      }
    }

    if (imageState.second->sparseBindings.size() > 0) {
      std::vector<VkSparseImageOpaqueMemoryBindInfo> sparseImageOpaqueMemoryBindInfos;
      std::vector<VkSparseImageMemoryBindInfo> sparseImageMemoryBindInfos;
      std::vector<VkBindSparseInfo> bindSparseInfos;

      // Reserve memory for elements to avoid vector reallocation which invalidates memory pointers
      sparseImageOpaqueMemoryBindInfos.reserve(imageState.second->sparseBindings.size());
      sparseImageMemoryBindInfos.reserve(imageState.second->sparseBindings.size());

      // Prepare data for sparse memory binding
      for (auto& sparseBinding : imageState.second->sparseBindings) {
        if (sparseBinding.first) {
          {
            VkSparseImageOpaqueMemoryBindInfo sparseImageOpaqueMemoryBindInfo = {
                image,                       // VkImage image;
                1,                           // uint32_t bindCount;
                sparseBinding.first->Value() // const VkSparseMemoryBind* pBinds;
            };
            sparseImageOpaqueMemoryBindInfos.push_back(sparseImageOpaqueMemoryBindInfo);
          }
          {
            VkBindSparseInfo bindSparseInfo = {
                VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, // VkStructureType sType;
                nullptr,                            // const void* pNext;
                0,                                  // uint32_t waitSemaphoreCount;
                nullptr,                            // const VkSemaphore* pWaitSemaphores;
                0,                                  // uint32_t bufferBindCount;
                nullptr, // const VkSparseBufferMemoryBindInfo* pBufferBinds;
                1,       // uint32_t imageOpaqueBindCount;
                &(sparseImageOpaqueMemoryBindInfos
                      .back()), // const VkSparseImageOpaqueMemoryBindInfo* pImageOpaqueBinds;
                0,              // uint32_t imageBindCount;
                nullptr,        // const VkSparseImageMemoryBindInfo* pImageBinds;
                0,              // uint32_t signalSemaphoreCount;
                nullptr         // const VkSemaphore* pSignalSemaphores;
            };
            bindSparseInfos.push_back(bindSparseInfo);
          }
        }
        if (sparseBinding.second) {
          {
            VkSparseImageMemoryBindInfo sparseImageMemoryBindInfo = {
                image,                        // VkImage image;
                1,                            // uint32_t bindCount;
                sparseBinding.second->Value() // const VkSparseImageMemoryBind* pBinds;
            };
            sparseImageMemoryBindInfos.push_back(sparseImageMemoryBindInfo);
          }
          {
            VkBindSparseInfo bindSparseInfo = {
                VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, // VkStructureType sType;
                nullptr,                            // const void* pNext;
                0,                                  // uint32_t waitSemaphoreCount;
                nullptr,                            // const VkSemaphore* pWaitSemaphores;
                0,                                  // uint32_t bufferBindCount;
                nullptr, // const VkSparseBufferMemoryBindInfo* pBufferBinds;
                0,       // uint32_t imageOpaqueBindCount;
                nullptr, // const VkSparseImageOpaqueMemoryBindInfo* pImageOpaqueBinds;
                1,       // uint32_t imageBindCount;
                &(sparseImageMemoryBindInfos
                      .back()), // const VkSparseImageMemoryBindInfo* pImageBinds;
                0,              // uint32_t signalSemaphoreCount;
                nullptr         // const VkSemaphore* pSignalSemaphores;
            };
            bindSparseInfos.push_back(bindSparseInfo);
          }
        }
      }
      if (bindSparseInfos.size() > 0) {
        VkQueue queue = imageState.second->deviceStateStore->queueStateStoreList[0]->queueHandle;
        scheduler.Register(new CvkQueueBindSparse(VK_SUCCESS, queue,
                                                  (uint32_t)bindSparseInfos.size(),
                                                  bindSparseInfos.data(), VK_NULL_HANDLE));
      }
    }
  }
}

// Image view

void gits::Vulkan::RestoreImageView(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<uint64_t, VkDevice> temporaryImages;
  for (auto& imageViewState : sd._imageviewstates) {
    VkImageView imageView = imageViewState.first;
    if (IsObjectToSkip((uint64_t)imageView)) {
      continue;
    }

    auto imageViewCreateInfo = *imageViewState.second->imageViewCreateInfoData.Value();
    auto imageStateIt = sd._imagestates.find(imageViewState.second->imageStateStore->imageHandle);
    if ((imageStateIt == sd._imagestates.end()) ||
        (imageStateIt->second->GetUniqueStateID() !=
         imageViewState.second->imageStateStore->GetUniqueStateID())) {
      VkImage image = (VkImage)imageViewState.second->imageStateStore->GetUniqueStateID();
      imageViewCreateInfo.image = image;
      scheduler.Register(new CvkCreateImage(
          VK_SUCCESS, imageViewState.second->imageStateStore->deviceStateStore->deviceHandle,
          imageViewState.second->imageStateStore->imageCreateInfoData.Value(), nullptr, &image));
      temporaryImages.emplace(imageViewState.second->imageStateStore->GetUniqueStateID(),
                              imageViewState.second->deviceStateStore->deviceHandle);
    }
    scheduler.Register(new CvkCreateImageView(VK_SUCCESS,
                                              imageViewState.second->deviceStateStore->deviceHandle,
                                              &imageViewCreateInfo, nullptr, &imageView));
  }
  for (auto& imageData : temporaryImages) {
    scheduler.Register(new CvkDestroyImage(imageData.second, (VkImage)imageData.first, nullptr));
  }
}

// Buffer

void gits::Vulkan::RestoreBuffer(CScheduler& scheduler, CStateDynamic& sd) {
  std::vector<std::pair<uint64_t, VkBuffer>> sortedBuffersToRestore;
  sortedBuffersToRestore.reserve(sd._bufferstates.size());

  for (auto& bufferAndState : sd._bufferstates) {
    VkBuffer buffer = bufferAndState.first;
    if (IsObjectToSkip((uint64_t)buffer)) {
      continue;
    }

    VkDevice device = bufferAndState.second->deviceStateStore->deviceHandle;
    if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
      continue;
    }
    sortedBuffersToRestore.emplace_back(bufferAndState.second->GetUniqueStateID(), buffer);
  }

  // Buffers which use shader device address and capture/replay opaque handle
  // must be restored in the same order as they were created in the original workload.
  std::sort(sortedBuffersToRestore.begin(), sortedBuffersToRestore.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  for (auto& uniqueIdAndBuffer : sortedBuffersToRestore) {
    auto buffer = uniqueIdAndBuffer.second;
    auto& bufferState = sd._bufferstates[buffer];
    auto device = bufferState->deviceStateStore->deviceHandle;
    {
      auto bufferCreateInfo = *bufferState->bufferCreateInfoData.Value();
      if (TBufferStateRestoration::NONE !=
          Config::Get().vulkan.recorder.crossPlatformStateRestoration.buffers) {
        bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      }
      scheduler.Register(
          new CvkCreateBuffer(VK_SUCCESS, device, &bufferCreateInfo, nullptr, &buffer));
    }
  }
}

void gits::Vulkan::RestoreBufferBindings(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& bufferState : sd._bufferstates) {
    if (IsObjectToSkip((uint64_t)bufferState.first)) {
      continue;
    }

    auto buffer = bufferState.first;
    auto device = bufferState.second->deviceStateStore->deviceHandle;

    if (bufferState.second->binding) {
      auto deviceMemory = bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle;
      scheduler.Register(new CvkBindBufferMemory(VK_SUCCESS, device, buffer, deviceMemory,
                                                 bufferState.second->binding->memoryOffset));
    }

    if (bufferState.second->sparseBindings.size() > 0) {
      std::vector<VkSparseBufferMemoryBindInfo> sparseBufferMemoryBindInfos;
      std::vector<VkBindSparseInfo> bindSparseInfos;

      sparseBufferMemoryBindInfos.reserve(bufferState.second->sparseBindings.size());

      for (auto& sparseBinding : bufferState.second->sparseBindings) {
        {
          VkSparseBufferMemoryBindInfo sparseBufferMemoryBindInfo = {
              buffer,                // VkBuffer buffer;
              1,                     // uint32_t bindCount;
              sparseBinding->Value() // const VkSparseMemoryBind* pBinds;
          };
          sparseBufferMemoryBindInfos.push_back(sparseBufferMemoryBindInfo);
        }
        {
          VkBindSparseInfo bindSparseInfo = {
              VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, // VkStructureType sType;
              nullptr,                            // const void* pNext;
              0,                                  // uint32_t waitSemaphoreCount;
              nullptr,                            // const VkSemaphore* pWaitSemaphores;
              1,                                  // uint32_t bufferBindCount;
              &(sparseBufferMemoryBindInfos
                    .back()), // const VkSparseBufferMemoryBindInfo* pBufferBinds;
              0,              // uint32_t imageOpaqueBindCount;
              nullptr,        // const VkSparseImageOpaqueMemoryBindInfo* pImageOpaqueBinds;
              0,              // uint32_t imageBindCount;
              nullptr,        // const VkSparseImageMemoryBindInfo* pImageBinds;
              0,              // uint32_t signalSemaphoreCount;
              nullptr         // const VkSemaphore* pSignalSemaphores;
          };
          bindSparseInfos.push_back(bindSparseInfo);
        }
      }
      if (bindSparseInfos.size() > 0) {
        VkQueue queue = bufferState.second->deviceStateStore->queueStateStoreList[0]->queueHandle;
        scheduler.Register(new CvkQueueBindSparse(VK_SUCCESS, queue,
                                                  (uint32_t)bindSparseInfos.size(),
                                                  bindSparseInfos.data(), VK_NULL_HANDLE));
      }
    }

    if (bufferState.second->deviceAddress != 0) {
      VkBufferDeviceAddressInfo addressInfo = {
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType sType;
          nullptr,                                      // const void * pNext;
          bufferState.second->bufferHandle              // VkBuffer buffer;
      };
      scheduler.Register(new CvkGetBufferDeviceAddressUnifiedGITS(bufferState.second->deviceAddress,
                                                                  device, &addressInfo));
    }
  }
}

// Buffer view

void gits::Vulkan::RestoreBufferView(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& bufferViewState : sd._bufferviewstates) {
    if (IsObjectToSkip((uint64_t)bufferViewState.first)) {
      continue;
    }

    auto bufferStateIt =
        sd._bufferstates.find(bufferViewState.second->bufferStateStore->bufferHandle);
    if ((bufferStateIt != sd._bufferstates.end()) &&
        (bufferStateIt->second->GetUniqueStateID() ==
         bufferViewState.second->bufferStateStore->GetUniqueStateID())) {
      auto bufferView = bufferViewState.first;
      auto device = bufferViewState.second->deviceStateStore->deviceHandle;
      auto createInfo = bufferViewState.second->bufferViewCreateInfoData.Value();
      scheduler.Register(
          new CvkCreateBufferView(VK_SUCCESS, device, createInfo, nullptr, &bufferView));
    }
  }
}

// Deferred operation

void gits::Vulkan::RestoreDeferredOperations(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deferredOperationAndState : sd._deferredoperationkhrstates) {
    VkDeferredOperationKHR deferredOperation = deferredOperationAndState.first;

    if (IsObjectToSkip((uint64_t)deferredOperation)) {
      continue;
    }

    VkDevice device = deferredOperationAndState.second->deviceStateStore->deviceHandle;
    if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
      continue;
    }

    scheduler.Register(
        new CvkCreateDeferredOperationKHR(VK_SUCCESS, device, nullptr, &deferredOperation));
  }
}

// Acceleration structure

void gits::Vulkan::RestoreAccelerationStructure(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceAndResources : temporaryDeviceResources) {
    VkDevice device = deviceAndResources.first;

    std::vector<std::tuple<VkAccelerationStructureKHR, VkBuffer, VkDeviceMemory>>
        temporaryAccelerationStructures;

    std::vector<std::pair<uint64_t, VkAccelerationStructureKHR>>
        sortedAccelerationStructuresToRestore;
    sortedAccelerationStructuresToRestore.reserve(sd._accelerationstructurekhrstates.size());

    for (auto& accelerationStructureAndState : sd._accelerationstructurekhrstates) {
      auto accelerationStructure = accelerationStructureAndState.first;
      if (IsObjectToSkip((uint64_t)accelerationStructure)) {
        continue;
      }

      if (accelerationStructureAndState.second->bufferStateStore->deviceStateStore->deviceHandle !=
          device) {
        continue;
      }

      sortedAccelerationStructuresToRestore.emplace_back(
          accelerationStructureAndState.second->GetUniqueStateID(), accelerationStructure);
    }

    // Acceleration structures similarly to buffers are restored in the same order
    // as they were created in the original workload.
    std::sort(sortedAccelerationStructuresToRestore.begin(),
              sortedAccelerationStructuresToRestore.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (auto& uniqueIdAndAccelerationStructure : sortedAccelerationStructuresToRestore) {
      auto accelerationStructure = uniqueIdAndAccelerationStructure.second;
      auto& accelerationStructureState = sd._accelerationstructurekhrstates[accelerationStructure];
      auto pCreateInfo = accelerationStructureState->accelerationStructureCreateInfoData.Value();

      scheduler.Register(new CvkCreateAccelerationStructureKHR(VK_SUCCESS, device, pCreateInfo,
                                                               nullptr, &accelerationStructure));
    }
  }
}

// Descriptor set layout

void gits::Vulkan::RestoreDescriptorSetLayout(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& descriptorSetLayoutState : sd._descriptorsetlayoutstates) {
    if (IsObjectToSkip((uint64_t)descriptorSetLayoutState.first)) {
      continue;
    }

    auto descriptorSetLayout = descriptorSetLayoutState.first;
    auto device = descriptorSetLayoutState.second->deviceStateStore->deviceHandle;
    auto createInfo = descriptorSetLayoutState.second->descriptorSetLayoutCreateInfoData.Value();
    scheduler.Register(new CvkCreateDescriptorSetLayout(VK_SUCCESS, device, createInfo, nullptr,
                                                        &descriptorSetLayout));
  }
}

// Descriptor set

void gits::Vulkan::RestoreAllocatedDescriptorSet(CScheduler& scheduler, CStateDynamic& sd) {
  // Prepare descriptor set data to aggregate allocations into smaller number of
  // function calls Map of VkDevices
  //        with a map of VkDescriptorPools
  //              with a map of VkDescriptorSetLayouts
  //                    with information whether descriptor sets were allocated
  //                    using extensions
  //                          with a vector of allocated descriptor sets for a
  //                          given combination of device, pool, layout and info
  //                          if any extension was used during allocation
  std::map<VkDevice, std::map<VkDescriptorPool,
                              std::map<bool, std::pair<std::vector<VkDescriptorSet>,
                                                       std::vector<VkDescriptorSetLayout>>>>>
      descriptorSetAllocationData;

  for (auto& descriptorSetStatePair : sd._descriptorsetstates) {
    if (IsObjectToSkip((uint64_t)descriptorSetStatePair.first)) {
      continue;
    }

    VkDescriptorSet descriptorSet = descriptorSetStatePair.first;
    auto& descriptorSetState = descriptorSetStatePair.second;
    VkDevice device =
        descriptorSetState->descriptorSetLayoutStateStore->deviceStateStore->deviceHandle;
    VkDescriptorPool descriptorPool =
        descriptorSetState->descriptorPoolStateStore->descriptorPoolHandle;
    VkDescriptorSetLayout descriptorSetLayout =
        descriptorSetState->descriptorSetLayoutStateStore->descriptorSetLayoutHandle;

    // Use UniqueID as a descriptor set layout handle and, if necessary, create temporary descriptor set layout
    if ((sd._descriptorsetlayoutstates.find(descriptorSetLayout) ==
         sd._descriptorsetlayoutstates.end()) ||
        (sd._descriptorsetlayoutstates[descriptorSetLayout]->GetUniqueStateID() !=
         descriptorSetState->descriptorSetLayoutStateStore->GetUniqueStateID())) {
      descriptorSetLayout =
          (VkDescriptorSetLayout)
              descriptorSetState->descriptorSetLayoutStateStore->GetUniqueStateID();

      if (temporaryDescriptorSetLayouts.find((uint64_t)descriptorSetLayout) ==
          temporaryDescriptorSetLayouts.end()) {
        auto createInfo = descriptorSetState->descriptorSetLayoutStateStore
                              ->descriptorSetLayoutCreateInfoData.Value();
        scheduler.Register(new CvkCreateDescriptorSetLayout(VK_SUCCESS, device, createInfo, nullptr,
                                                            &descriptorSetLayout));
        temporaryDescriptorSetLayouts.emplace(
            descriptorSetState->descriptorSetLayoutStateStore->GetUniqueStateID(), device);
      }
    }

    auto& setLayoutPair =
        descriptorSetAllocationData[device][descriptorPool]
                                   [descriptorSetState->extensionsDataChain.Value() != nullptr];
    setLayoutPair.first.push_back(descriptorSet);
    setLayoutPair.second.push_back(descriptorSetLayout);
  }

  // Allocate descriptor sets using aggregated data
  for (auto& deviceDataPair : descriptorSetAllocationData) {
    VkDevice device = deviceDataPair.first;

    for (auto& descriptorPoolDataPair : deviceDataPair.second) {
      VkDescriptorPool descriptorPool = descriptorPoolDataPair.first;

      for (auto& usedExtensionsPair : descriptorPoolDataPair.second) {

        // Extensions were used for descriptor set allocation - in such case we restore one descriptor set at a time
        if (usedExtensionsPair.first) {
          auto& setLayoutPair = usedExtensionsPair.second;

          for (size_t i = 0; i < setLayoutPair.first.size(); ++i) {
            VkDescriptorSet descriptorSet = setLayoutPair.first[i];
            VkDescriptorSetLayout descriptorSetLayout = setLayoutPair.second[i];

            const void* pNextChain =
                SD()._descriptorsetstates[descriptorSet]->extensionsDataChain.Value();

            VkDescriptorSetAllocateInfo allocateInfo = {
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType                sType;
                pNextChain,          // const void                   * pNext;
                descriptorPool,      // VkDescriptorPool               descriptorPool;
                1,                   // uint32_t                       descriptorSetCount;
                &descriptorSetLayout // const VkDescriptorSetLayout  * pSetLayouts;
            };

            scheduler.Register(
                new CvkAllocateDescriptorSets(VK_SUCCESS, device, &allocateInfo, &descriptorSet));
          }
        } else {
          // No extensions were used - all the descriptor sets can be allocated in one call
          auto& setLayoutPair = usedExtensionsPair.second;
          std::vector<VkDescriptorSet>& descriptorSets = setLayoutPair.first;
          std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = setLayoutPair.second;

          VkDescriptorSetAllocateInfo allocateInfo = {
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType                sType;
              nullptr,        // const void                   * pNext;
              descriptorPool, // VkDescriptorPool               descriptorPool;
              static_cast<uint32_t>(
                  descriptorSets.size()), // uint32_t                       descriptorSetCount;
              descriptorSetLayouts.data() // const VkDescriptorSetLayout  * pSetLayouts;
          };

          scheduler.Register(new CvkAllocateDescriptorSets(VK_SUCCESS, device, &allocateInfo,
                                                           descriptorSets.data()));
        }
      }
    }
  }
}

namespace {

bool RestoreSamplerDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    const VkDevice device) {
  if (!descriptorData.pImageInfo) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice because image descriptor "
                 "info is null.";
    return false;
  }

  const auto sampler = descriptorData.pImageInfo->Value()->sampler;
  const auto& samplerStateIt = sd._samplerstates.find(sampler);

  if ((samplerStateIt == sd._samplerstates.end()) ||
      (descriptorData.samplerStateStore->GetUniqueStateID() !=
       samplerStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkSampler " << sampler << " doesn't exist.";
    return false;
  }

  descriptorWrite.pImageInfo = descriptorData.pImageInfo->Value();
  return true;
}

bool RestoreCombinedImageSamplerDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    const VkDevice device) {
  if (!descriptorData.pImageInfo) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice because image descriptor "
                 "info is null.";
    return false;
  }

  const auto sampler = descriptorData.pImageInfo->Value()->sampler;
  const auto& samplerStateIt = sd._samplerstates.find(sampler);

  if ((samplerStateIt == sd._samplerstates.end()) ||
      (descriptorData.samplerStateStore->GetUniqueStateID() !=
       samplerStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkSampler " << sampler << " doesn't exist.";
    return false;
  }

  const auto imageView = descriptorData.pImageInfo->Value()->imageView;
  const auto& imageViewStateIt = sd._imageviewstates.find(imageView);

  if ((imageViewStateIt == sd._imageviewstates.end()) ||
      (descriptorData.imageViewStateStore->GetUniqueStateID() !=
       imageViewStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkImageView " << imageView << " doesn't exist.";
    return false;
  }

  const auto image = imageViewStateIt->second->imageStateStore->imageHandle;
  const auto& imageStateIt = sd._imagestates.find(image);

  if ((imageStateIt == sd._imagestates.end()) ||
      (imageStateIt->second->GetUniqueStateID() !=
       imageViewStateIt->second->imageStateStore->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkImage " << image << " doesn't exist.";
    return false;
  }

  descriptorWrite.pImageInfo = descriptorData.pImageInfo->Value();
  return true;
}

bool RestoreImageDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    const VkDevice device) {
  if (!descriptorData.pImageInfo) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice because image descriptor "
                 "info is null.";
    return false;
  }

  const auto imageView = descriptorData.pImageInfo->Value()->imageView;
  const auto& imageViewStateIt = sd._imageviewstates.find(imageView);

  if ((imageViewStateIt == sd._imageviewstates.end()) ||
      (descriptorData.imageViewStateStore->GetUniqueStateID() !=
       imageViewStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkImageView " << imageView << " doesn't exist.";
    return false;
  }

  const auto image = imageViewStateIt->second->imageStateStore->imageHandle;
  const auto& imageStateIt = sd._imagestates.find(image);

  if ((imageStateIt == sd._imagestates.end()) ||
      (imageStateIt->second->GetUniqueStateID() !=
       imageViewStateIt->second->imageStateStore->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkImage " << image << " doesn't exist.";
    return false;
  }

  descriptorWrite.pImageInfo = descriptorData.pImageInfo->Value();
  return true;
}

bool RestoreTexelBufferDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    const VkDevice device) {
  if (!descriptorData.pTexelBufferView) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice because texel buffer "
                 "descriptor info is null.";
    return false;
  }

  const auto texelBufferView = *descriptorData.pTexelBufferView->Value();
  const auto& bufferViewStateIt = sd._bufferviewstates.find(texelBufferView);

  if ((bufferViewStateIt == sd._bufferviewstates.end()) ||
      (descriptorData.bufferViewStateStore->GetUniqueStateID() !=
       bufferViewStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkBufferView " << texelBufferView << " doesn't exist.";
    return false;
  }

  const auto buffer = bufferViewStateIt->second->bufferStateStore->bufferHandle;
  const auto& bufferStateIt = sd._bufferstates.find(buffer);

  if ((bufferStateIt == sd._bufferstates.end()) ||
      (bufferStateIt->second->GetUniqueStateID() !=
       bufferViewStateIt->second->bufferStateStore->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkBuffer " << buffer << " doesn't exist.";
    return false;
  }

  descriptorWrite.pTexelBufferView = descriptorData.pTexelBufferView->Value();
  return true;
}

bool RestoreBufferDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    const VkDevice device) {
  if (!descriptorData.pBufferInfo) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice because buffer "
                 "descriptor info is null.";
    return false;
  }

  const auto buffer = descriptorData.pBufferInfo->Value()->buffer;
  const auto& bufferStateIt = sd._bufferstates.find(buffer);

  if ((bufferStateIt == sd._bufferstates.end()) ||
      (descriptorData.bufferStateStore->GetUniqueStateID() !=
       bufferStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkBuffer " << buffer << " doesn't exist.";
    return false;
  }

  descriptorWrite.pBufferInfo = descriptorData.pBufferInfo->Value();
  return true;
}

bool RestoreAccelerationStructureDescriptorHelper(
    const gits::Vulkan::CStateDynamic& sd,
    const gits::Vulkan::CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData&
        descriptorData,
    VkWriteDescriptorSet& descriptorWrite,
    std::list<VkWriteDescriptorSetAccelerationStructureKHR>& accelerationStructureWrites,
    const VkDevice device) {
  if (!descriptorData.accelerationStructureStateStore) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice acceleration structure "
                 "descriptor info is null.";
    return false;
  }

  const auto accStruct =
      descriptorData.accelerationStructureStateStore->accelerationStructureHandle;
  const auto& accStructStateIt = sd._accelerationstructurekhrstates.find(accStruct);

  if ((accStructStateIt == sd._accelerationstructurekhrstates.end()) ||
      (descriptorData.accelerationStructureStateStore->GetUniqueStateID() !=
       accStructStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used VkAccelerationStructureKHR " << accStruct << " doesn't exist.";
    return false;
  }

  const auto buffer =
      descriptorData.accelerationStructureStateStore->bufferStateStore->bufferHandle;
  const auto& bufferStateIt = sd._bufferstates.find(buffer);

  if ((bufferStateIt == sd._bufferstates.end()) ||
      (descriptorData.accelerationStructureStateStore->bufferStateStore->GetUniqueStateID() !=
       bufferStateIt->second->GetUniqueStateID())) {
    Log(INFO) << "Omitting restore of vkUpdateDescriptorSets for VkDevice " << device
              << " because used buffer " << buffer << " doesn't exist.";
    return false;
  }

  accelerationStructureWrites.emplace_back(VkWriteDescriptorSetAccelerationStructureKHR{
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, // VkStructureType sType;
      nullptr,                                                           // const void*     pNext;
      1, // uint32_t accelerationStructureCount;
      &descriptorData.accelerationStructureStateStore
           ->accelerationStructureHandle // const VkAccelerationStructureKHR* pAccelerationStructures;
  });
  descriptorWrite.pNext = &accelerationStructureWrites.back();
  return true;
}

} // namespace

void gits::Vulkan::RestoreDescriptorSetsUpdates(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& descriptorSetState : sd._descriptorsetstates) {
    if (IsObjectToSkip((uint64_t)descriptorSetState.first)) {
      continue;
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::vector<VkWriteDescriptorSetInlineUniformBlock> uniformBlocks;
    std::list<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureWrites;

    for (auto& descriptorSetBindingIt : descriptorSetState.second->descriptorSetBindings) {
      auto& descriptorSetBinding = descriptorSetBindingIt.second;
      auto device =
          descriptorSetState.second->descriptorPoolStateStore->deviceStateStore->deviceHandle;

      if (descriptorSetBinding.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
        VkWriteDescriptorSetInlineUniformBlock uniformBlock = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK, //VkStructureType sType
            nullptr,                                                     //const void* pNext
            descriptorSetBinding.descriptorCount,                        //uint32_t dataSize
            &descriptorSetBinding.descriptorData[0].inlineUniformBlockData[0], //const void* pData
        };
        uniformBlocks.push_back(uniformBlock);
        VkWriteDescriptorSet descriptorWrite = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // VkStructureType sType;
            &uniformBlocks[uniformBlocks.size() - 1], // const void* pNext;
            descriptorSetState.first,                 // VkDescriptorSet dstSet;
            descriptorSetBindingIt.first,             // uint32_t dstBinding;
            0,                                        // uint32_t dstArrayElement;
            descriptorSetBinding.descriptorCount,     // uint32_t descriptorCount;
            descriptorSetBinding.descriptorType,      // VkDescriptorType descriptorType;
            nullptr,                                  // const VkDescriptorImageInfo* pImageInfo;
            nullptr,                                  // const VkDescriptorBufferInfo* pBufferInfo;
            nullptr                                   // const VkBufferView* pTexelBufferView;
        };
        descriptorWrites.push_back(descriptorWrite);
      } else {
        for (size_t arrayIndex = 0; arrayIndex < descriptorSetBinding.descriptorData.size();
             ++arrayIndex) {
          const auto& currentBindingData = descriptorSetBinding.descriptorData[arrayIndex];

          VkWriteDescriptorSet descriptorWrite = {
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // VkStructureType sType;
              nullptr,                                // const void* pNext;
              descriptorSetState.first,               // VkDescriptorSet dstSet;
              descriptorSetBindingIt.first,           // uint32_t dstBinding;
              (uint32_t)arrayIndex,                   // uint32_t dstArrayElement;
              1,                                      // uint32_t descriptorCount;
              descriptorSetBinding.descriptorType,    // VkDescriptorType descriptorType;
              nullptr,                                // const VkDescriptorImageInfo* pImageInfo;
              nullptr,                                // const VkDescriptorBufferInfo* pBufferInfo;
              nullptr                                 // const VkBufferView* pTexelBufferView;
          };
          bool writeDescriptor = false;

          switch (descriptorSetBinding.descriptorType) {
          case VK_DESCRIPTOR_TYPE_SAMPLER:
            writeDescriptor =
                RestoreSamplerDescriptorHelper(sd, currentBindingData, descriptorWrite, device);
            break;
          case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            writeDescriptor = RestoreCombinedImageSamplerDescriptorHelper(sd, currentBindingData,
                                                                          descriptorWrite, device);
            break;
          case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
          case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
          case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            writeDescriptor =
                RestoreImageDescriptorHelper(sd, currentBindingData, descriptorWrite, device);
            break;
          case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
          case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            writeDescriptor =
                RestoreTexelBufferDescriptorHelper(sd, currentBindingData, descriptorWrite, device);
            break;
          case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
          case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
          case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
          case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            writeDescriptor =
                RestoreBufferDescriptorHelper(sd, currentBindingData, descriptorWrite, device);
            break;
          case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            writeDescriptor = RestoreAccelerationStructureDescriptorHelper(
                sd, currentBindingData, descriptorWrite, accelerationStructureWrites, device);
            break;
          default:
            Log(TRACE) << "Not handled VkDescriptorType enumeration: "
                       << descriptorSetBinding.descriptorType;
            break;
          }

          if (writeDescriptor) {
            descriptorWrites.push_back(descriptorWrite);
          }
        }
      }
    }
    if (descriptorWrites.size() > 0) {
      scheduler.Register(new CvkUpdateDescriptorSets(
          descriptorSetState.second->descriptorPoolStateStore->deviceStateStore->deviceHandle,
          (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr));
    }
  }
}

void gits::Vulkan::RestorePipelineLayout(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& pipelineLayoutState : sd._pipelinelayoutstates) {
    VkPipelineLayout pipelineLayout = pipelineLayoutState.first;
    if (IsObjectToSkip((uint64_t)pipelineLayout) || pipelineLayout == VK_NULL_HANDLE) {
      continue;
    }

    auto info = pipelineLayoutState.second->pipelineLayoutCreateInfoData.Value();
    for (uint32_t i = 0; i < info->setLayoutCount; i++) {
      auto descriptorSetLayout = info->pSetLayouts[i];
      if (descriptorSetLayout == VK_NULL_HANDLE) {
        continue;
      }
      auto& descriptorSetLayoutState = pipelineLayoutState.second->descriptorSetLayoutStates[i];
      // Use UniqueID as a descriptor set layout handle and, if necessary, create temporary descriptor set layout
      if ((sd._descriptorsetlayoutstates.find(descriptorSetLayout) ==
           sd._descriptorsetlayoutstates.end()) ||
          (sd._descriptorsetlayoutstates[descriptorSetLayout]->GetUniqueStateID() !=
           descriptorSetLayoutState->GetUniqueStateID())) {
        descriptorSetLayout = (VkDescriptorSetLayout)descriptorSetLayoutState->GetUniqueStateID();
        const_cast<VkDescriptorSetLayout*>(info->pSetLayouts)[i] = descriptorSetLayout;

        if (temporaryDescriptorSetLayouts.find((uint64_t)descriptorSetLayout) ==
            temporaryDescriptorSetLayouts.end()) {
          auto createInfo = descriptorSetLayoutState->descriptorSetLayoutCreateInfoData.Value();
          auto device = pipelineLayoutState.second->deviceStateStore->deviceHandle;
          scheduler.Register(new CvkCreateDescriptorSetLayout(VK_SUCCESS, device, createInfo,
                                                              nullptr, &descriptorSetLayout));
          temporaryDescriptorSetLayouts.emplace(descriptorSetLayoutState->GetUniqueStateID(),
                                                device);
        }
      }
    }

    scheduler.Register(new CvkCreatePipelineLayout(
        VK_SUCCESS, pipelineLayoutState.second->deviceStateStore->deviceHandle, info, nullptr,
        &pipelineLayout));
  }
}

// Temporary descriptor set layouts

void gits::Vulkan::DestroyTemporaryDescriptorSetLayouts(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& temporaryLayoutPair : temporaryDescriptorSetLayouts) {
    scheduler.Register(new CvkDestroyDescriptorSetLayout(
        temporaryLayoutPair.second, (VkDescriptorSetLayout)temporaryLayoutPair.first, nullptr));
  }

  temporaryDescriptorSetLayouts.clear();
}

// Descriptor update template

void gits::Vulkan::RestoreDescriptorUpdateTemplate(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& descriptorUpdateTemplateState : sd._descriptorupdatetemplatestates) {
    if (IsObjectToSkip((uint64_t)descriptorUpdateTemplateState.first)) {
      continue;
    }

    auto device = descriptorUpdateTemplateState.second->deviceStateStore->deviceHandle;
    VkPipelineLayout createdPipelineLayout = VK_NULL_HANDLE;
    if (descriptorUpdateTemplateState.second->pipelineLayoutStateStore) {
      if (sd._pipelinelayoutstates.find(descriptorUpdateTemplateState.second
                                            ->pipelineLayoutStateStore->pipelineLayoutHandle) ==
          sd._pipelinelayoutstates.end()) {
        createdPipelineLayout =
            descriptorUpdateTemplateState.second->pipelineLayoutStateStore->pipelineLayoutHandle;
      } else if (sd._pipelinelayoutstates[descriptorUpdateTemplateState.second
                                              ->pipelineLayoutStateStore->pipelineLayoutHandle]
                     ->GetUniqueStateID() != descriptorUpdateTemplateState.second
                                                 ->pipelineLayoutStateStore->GetUniqueStateID()) {
        createdPipelineLayout = (VkPipelineLayout)1;
      }

      if (VK_NULL_HANDLE != createdPipelineLayout) {
        scheduler.Register(new CvkCreatePipelineLayout(
            VK_SUCCESS, device,
            descriptorUpdateTemplateState.second->pipelineLayoutStateStore
                ->pipelineLayoutCreateInfoData.Value(),
            nullptr, &createdPipelineLayout));
      }
    }

    VkDescriptorUpdateTemplate updateTemplate = descriptorUpdateTemplateState.first;
    switch (descriptorUpdateTemplateState.second->createdWith) {
    case CreationFunction::KHR_EXTENSION:
      scheduler.Register(new CvkCreateDescriptorUpdateTemplateKHR(
          VK_SUCCESS, device,
          descriptorUpdateTemplateState.second->descriptorUpdateTemplateCreateInfoData.Value(),
          nullptr, &updateTemplate));
      break;
    case CreationFunction::CORE_1_1:
      scheduler.Register(new CvkCreateDescriptorUpdateTemplate(
          VK_SUCCESS, device,
          descriptorUpdateTemplateState.second->descriptorUpdateTemplateCreateInfoData.Value(),
          nullptr, &updateTemplate));
      break;
    default:
      throw std::runtime_error("VkDescriptorUpdateTemplate created with an unknown method!");
    }

    if (VK_NULL_HANDLE != createdPipelineLayout) {
      scheduler.Register(new CvkDestroyPipelineLayout(device, createdPipelineLayout, nullptr));
    }
  }
}

// Pipeline cache

void gits::Vulkan::RestorePipelineCache(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& pipelineCacheState : sd._pipelinecachestates) {
    VkPipelineCache pipelineCache = pipelineCacheState.first;
    if (IsObjectToSkip((uint64_t)pipelineCache)) {
      continue;
    }
    scheduler.Register(new CvkCreatePipelineCache_V1(
        VK_SUCCESS, pipelineCacheState.second->deviceStateStore->deviceHandle,
        pipelineCacheState.second->pipelineCacheCreateInfoData.Value(), nullptr, &pipelineCache));
  }
}

// Shader module

void gits::Vulkan::RestoreShaderModules(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& shaderModuleState : sd._shadermodulestates) {
    VkShaderModule shaderModule = shaderModuleState.first;
    if (IsObjectToSkip((uint64_t)shaderModule)) {
      continue;
    }
    scheduler.Register(new CvkCreateShaderModule(
        VK_SUCCESS, shaderModuleState.second->deviceStateStore->deviceHandle,
        shaderModuleState.second->shaderModuleCreateInfoData.Value(), nullptr, &shaderModule));
  }
}

// Render pass

void gits::Vulkan::RestoreRenderPass(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& renderPassObj : sd._renderpassstates) {
    VkRenderPass renderPass = renderPassObj.first;
    auto& renderPassState = renderPassObj.second;
    if (IsObjectToSkip((uint64_t)renderPass)) {
      continue;
    }
    if (gits::CGits::Instance().apis.Iface3D().CfgRec_IsDrawsRangeMode() &&
        renderPassObj.second->restoreRenderPassStateStore) {
      renderPassState = renderPassObj.second->restoreRenderPassStateStore;
    }

    switch (renderPassState->createdWith) {
    case CreationFunction::KHR_EXTENSION:
      scheduler.Register(new CvkCreateRenderPass2KHR(
          VK_SUCCESS, renderPassState->deviceStateStore->deviceHandle,
          renderPassState->renderPassCreateInfo2Data.Value(), nullptr, &renderPass));
      break;
    case CreationFunction::CORE_1_0:
      scheduler.Register(new CvkCreateRenderPass(
          VK_SUCCESS, renderPassState->deviceStateStore->deviceHandle,
          renderPassState->renderPassCreateInfoData.Value(), nullptr, &renderPass));
      break;
    case CreationFunction::CORE_1_2:
      scheduler.Register(new CvkCreateRenderPass2(
          VK_SUCCESS, renderPassState->deviceStateStore->deviceHandle,
          renderPassState->renderPassCreateInfo2Data.Value(), nullptr, &renderPass));
      break;
    default:
      throw std::runtime_error("VkRenderPass created with an unknown method!");
    }
  }
}

// Pipeline

// First pipeline libraries are restored. These can be used to create other pipelines,
// so they need to be recreated earlier. After that all other (normal) pipelines are restored.
void gits::Vulkan::RestorePipelines(CScheduler& scheduler, CStateDynamic& sd) {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, // VkStructureType sType;
      nullptr,                                      // const void* pNext;
      0,                                            // VkPipelineCacheCreateFlags flags;
      0,                                            // size_t initialDataSize;
      nullptr                                       // const void* pInitialData;
  };

  // Restore pipelines only for restored devices
  for (auto& deviceAndStatePair : temporaryDeviceResources) {
    VkDevice device = deviceAndStatePair.first;

    std::set<VkPipelineLayout> temporaryPipelineLayouts;
    std::set<VkShaderModule> temporaryShaderModules;
    std::set<VkRenderPass> temporaryRenderPasses;

    // Create termporary pipeline cache
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    drvVk.vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
    scheduler.Register(new CvkCreatePipelineCache_V1(VK_SUCCESS, device, &pipelineCacheCreateInfo,
                                                     nullptr, &pipelineCache));

    // Get existing pipeline layout or restore one if needed
    auto getOrRestorePipelineLayout =
        [&](std::shared_ptr<CPipelineLayoutState>& pipelineLayoutStateStore) {
          if (pipelineLayoutStateStore == nullptr) {
            return (VkPipelineLayout)VK_NULL_HANDLE;
          }

          VkPipelineLayout pipelineLayout = pipelineLayoutStateStore->pipelineLayoutHandle;

          if ((sd._pipelinelayoutstates.find(pipelineLayout) == sd._pipelinelayoutstates.end()) ||
              (sd._pipelinelayoutstates[pipelineLayout]->GetUniqueStateID() !=
               pipelineLayoutStateStore->GetUniqueStateID())) {
            pipelineLayout = (VkPipelineLayout)pipelineLayoutStateStore->GetUniqueStateID();

            if (temporaryPipelineLayouts.find(pipelineLayout) == temporaryPipelineLayouts.end()) {

              scheduler.Register(new CvkCreatePipelineLayout(
                  VK_SUCCESS, device,
                  pipelineLayoutStateStore->pipelineLayoutCreateInfoData.Value(), nullptr,
                  &pipelineLayout));
              temporaryPipelineLayouts.insert(pipelineLayout);
            }
          }

          return pipelineLayout;
        };

    // Get existing shader modules or restore missing ones
    auto getOrRestoreShaderModules =
        [&](std::vector<std::shared_ptr<CShaderModuleState>>& shaderModuleStateStoreList) {
          std::vector<VkShaderModule> shaderModules;

          for (auto& shaderModuleState : shaderModuleStateStoreList) {
            if (shaderModuleState == nullptr) {
              shaderModules.push_back(VK_NULL_HANDLE);
              continue;
            }

            VkShaderModule shaderModule = shaderModuleState->shaderModuleHandle;

            if ((sd._shadermodulestates.find(shaderModule) == sd._shadermodulestates.end()) ||
                (sd._shadermodulestates[shaderModule]->GetUniqueStateID() !=
                 shaderModuleState->GetUniqueStateID())) {
              shaderModule = (VkShaderModule)shaderModuleState->GetUniqueStateID();

              if (temporaryShaderModules.find(shaderModule) == temporaryShaderModules.end()) {
                scheduler.Register(new CvkCreateShaderModule(
                    VK_SUCCESS, device, shaderModuleState->shaderModuleCreateInfoData.Value(),
                    nullptr, &shaderModule));
                temporaryShaderModules.insert(shaderModule);
              }
            }

            shaderModules.push_back(shaderModule);
          }

          return shaderModules;
        };

    // Get existing render pass or restore one if needed
    auto getOrRestoreRenderPass = [&](std::shared_ptr<CRenderPassState>& renderPassStateStore) {
      if (renderPassStateStore == nullptr) {
        return (VkRenderPass)VK_NULL_HANDLE;
      }

      VkRenderPass renderPass = renderPassStateStore->renderPassHandle;

      if ((sd._renderpassstates.find(renderPass) == sd._renderpassstates.end()) ||
          (sd._renderpassstates[renderPass]->GetUniqueStateID() !=
           renderPassStateStore->GetUniqueStateID())) {
        renderPass = (VkRenderPass)renderPassStateStore->GetUniqueStateID();

        if (temporaryRenderPasses.find(renderPass) == temporaryRenderPasses.end()) {
          switch (renderPassStateStore->createdWith) {
          case CreationFunction::KHR_EXTENSION:
            scheduler.Register(new CvkCreateRenderPass2KHR(
                VK_SUCCESS, device, renderPassStateStore->renderPassCreateInfo2Data.Value(),
                nullptr, &renderPass));
            break;
          case CreationFunction::CORE_1_0:
            scheduler.Register(new CvkCreateRenderPass(
                VK_SUCCESS, device, renderPassStateStore->renderPassCreateInfoData.Value(), nullptr,
                &renderPass));
            break;
          case CreationFunction::CORE_1_2:
            scheduler.Register(new CvkCreateRenderPass2(
                VK_SUCCESS, device, renderPassStateStore->renderPassCreateInfo2Data.Value(),
                nullptr, &renderPass));
            break;
          default:
            throw std::logic_error("VkRenderPass created with an unknown method!");
          }

          temporaryRenderPasses.emplace(renderPass);
        }
      }
      return renderPass;
    };

    // Prepare create info for graphics pipelines
    auto prepareGraphicsPipelineCreateInfo = [&](std::shared_ptr<CPipelineState> pipelineState) {
      VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
          *pipelineState->graphicsPipelineCreateInfoData;
      graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
      graphicsPipelineCreateInfo.basePipelineIndex = -1;
      graphicsPipelineCreateInfo.layout =
          getOrRestorePipelineLayout(pipelineState->pipelineLayoutStateStore);
      graphicsPipelineCreateInfo.renderPass =
          getOrRestoreRenderPass(pipelineState->renderPassStateStore);

      auto shaderModulesHandles =
          getOrRestoreShaderModules(pipelineState->shaderModuleStateStoreList);
      for (uint32_t i = 0; i < graphicsPipelineCreateInfo.stageCount; ++i) {
        const_cast<VkPipelineShaderStageCreateInfo*>(graphicsPipelineCreateInfo.pStages)[i].module =
            shaderModulesHandles[i];
      }

      return graphicsPipelineCreateInfo;
    };

    // Prepare create info for compute pipelines
    auto prepareComputePipelineCreateInfo = [&](std::shared_ptr<CPipelineState>& pipelineState) {
      VkComputePipelineCreateInfo computePipelineCreateInfo =
          *pipelineState->computePipelineCreateInfoData;
      computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
      computePipelineCreateInfo.basePipelineIndex = -1;
      computePipelineCreateInfo.layout =
          getOrRestorePipelineLayout(pipelineState->pipelineLayoutStateStore);
      computePipelineCreateInfo.stage.module =
          getOrRestoreShaderModules(pipelineState->shaderModuleStateStoreList)[0];
      return computePipelineCreateInfo;
    };

    // Prepare create info for ray tracing pipelines
    auto prepareRayTracingPipelineCreateInfo = [&](std::shared_ptr<CPipelineState>& pipelineState) {
      VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo =
          *pipelineState->rayTracingPipelineCreateInfoData;
      rayTracingPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
      rayTracingPipelineCreateInfo.basePipelineIndex = -1;
      rayTracingPipelineCreateInfo.layout =
          getOrRestorePipelineLayout(pipelineState->pipelineLayoutStateStore);

      auto shaderModulesHandles =
          getOrRestoreShaderModules(pipelineState->shaderModuleStateStoreList);
      for (uint32_t i = 0; i < rayTracingPipelineCreateInfo.stageCount; ++i) {
        const_cast<VkPipelineShaderStageCreateInfo*>(rayTracingPipelineCreateInfo.pStages)[i]
            .module = shaderModulesHandles[i];
      }
      return rayTracingPipelineCreateInfo;
    };

    auto restorePipelines = [&](bool restorePipelineLibraries) {
      std::vector<VkGraphicsPipelineCreateInfo> graphicsPipelinesCreateInfos;
      std::vector<VkPipeline> graphicsPipelines;
      std::vector<VkComputePipelineCreateInfo> computePipelinesCreateInfos;
      std::vector<VkPipeline> computePipelines;
      std::vector<VkRayTracingPipelineCreateInfoKHR> rayTracingPipelinesCreateInfos;
      std::vector<VkPipeline> rayTracingPipelines;

      for (auto& pipelineAndStatePair : sd._pipelinestates) {
        if (IsObjectToSkip((uint64_t)pipelineAndStatePair.first)) {
          continue;
        }

        auto& pipelineState = pipelineAndStatePair.second;
        if (pipelineState->deviceStateStore->deviceHandle != device) {
          continue;
        }

        // Prepare graphics pipeline creation data
        {
          auto* graphicsPipelineCreateInfo = pipelineState->graphicsPipelineCreateInfoData.Value();
          if (graphicsPipelineCreateInfo &&
              (restorePipelineLibraries ==
               (bool)(graphicsPipelineCreateInfo->flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR))) {
            graphicsPipelinesCreateInfos.push_back(
                prepareGraphicsPipelineCreateInfo(pipelineState));
            graphicsPipelines.push_back(pipelineState->pipelineHandle);
          }
        }

        // Prepare compute pipeline creation data
        {
          auto* computePipelineCreateInfo = pipelineState->computePipelineCreateInfoData.Value();
          if (computePipelineCreateInfo &&
              (restorePipelineLibraries ==
               (bool)(computePipelineCreateInfo->flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR))) {
            computePipelinesCreateInfos.push_back(prepareComputePipelineCreateInfo(pipelineState));
            computePipelines.push_back(pipelineState->pipelineHandle);
          }
        }

        // Prepare ray tracing pipeline creation data
        {
          auto* rayTracingPipelineCreateInfo =
              pipelineState->rayTracingPipelineCreateInfoData.Value();
          if (rayTracingPipelineCreateInfo &&
              (restorePipelineLibraries ==
               (bool)(rayTracingPipelineCreateInfo->flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR))) {
            rayTracingPipelinesCreateInfos.push_back(
                prepareRayTracingPipelineCreateInfo(pipelineState));
            rayTracingPipelines.push_back(pipelineState->pipelineHandle);
          }
        }
      }

      // Restore graphics pipelines
      if (!graphicsPipelinesCreateInfos.empty()) {
        scheduler.Register(new CvkCreateGraphicsPipelines(
            VK_SUCCESS, device, pipelineCache,
            static_cast<uint32_t>(graphicsPipelinesCreateInfos.size()),
            graphicsPipelinesCreateInfos.data(), nullptr, graphicsPipelines.data()));
      }
      // Restore compute pipelines
      if (!computePipelinesCreateInfos.empty()) {
        scheduler.Register(new CvkCreateComputePipelines(
            VK_SUCCESS, device, pipelineCache,
            static_cast<uint32_t>(computePipelinesCreateInfos.size()),
            computePipelinesCreateInfos.data(), nullptr, computePipelines.data()));
      }
      // Restore ray tracing pipelines
      if (!rayTracingPipelinesCreateInfos.empty()) {
        scheduler.Register(new CvkCreateRayTracingPipelinesKHR(
            VK_SUCCESS, device, VK_NULL_HANDLE, pipelineCache,
            static_cast<uint32_t>(rayTracingPipelinesCreateInfos.size()),
            rayTracingPipelinesCreateInfos.data(), nullptr, rayTracingPipelines.data()));
      }
    };

    // Prepare creation data and restore pipeline LIBRARIES
    restorePipelines(true);

    // Prepare creation data and restore all other (normal) pipelines
    restorePipelines(false);

    // Destroy temporary resources
    for (auto pipelineLayout : temporaryPipelineLayouts) {
      scheduler.Register(new CvkDestroyPipelineLayout(device, pipelineLayout, nullptr));
    }
    for (auto shaderModule : temporaryShaderModules) {
      scheduler.Register(new CvkDestroyShaderModule(device, shaderModule, nullptr));
    }
    for (auto renderPass : temporaryRenderPasses) {
      scheduler.Register(new CvkDestroyRenderPass(device, renderPass, nullptr));
    }

    scheduler.Register(new CvkDestroyPipelineCache(device, pipelineCache, nullptr));
    drvVk.vkDestroyPipelineCache(device, pipelineCache, nullptr);
  }
}

// Framebuffer

void gits::Vulkan::RestoreFramebuffer(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<uint64_t, VkDevice> temporaryImages;
  std::map<uint64_t, VkDevice> temporaryImageViews;
  std::map<uint64_t, VkDevice> temporaryRenderPasses;

  for (auto& framebufferStatePair : sd._framebufferstates) {
    if (IsObjectToSkip((uint64_t)framebufferStatePair.first)) {
      continue;
    }

    auto& framebufferState = framebufferStatePair.second;
    std::vector<bool> usedUniqueIdForImageView(framebufferState->imageViewStateStoreList.size(),
                                               false);
    bool usedUniqueIdForRenderPass = false;

    // Restore missing image views and images
    for (size_t i = 0; i < framebufferState->imageViewStateStoreList.size(); ++i) {
      auto& imageViewState = framebufferState->imageViewStateStoreList[i];

      if ((sd._imageviewstates.find(imageViewState->imageViewHandle) ==
           sd._imageviewstates.end()) ||
          (sd._imageviewstates[imageViewState->imageViewHandle]->GetUniqueStateID() !=
           imageViewState->GetUniqueStateID())) {
        usedUniqueIdForImageView[i] = true;
        bool usedUniqueIdForImage = false;

        if ((sd._imagestates.find(imageViewState->imageStateStore->imageHandle) ==
             sd._imagestates.end()) ||
            (sd._imagestates[imageViewState->imageStateStore->imageHandle]->GetUniqueStateID() !=
             imageViewState->imageStateStore->GetUniqueStateID())) {
          usedUniqueIdForImage = true;

          // Restore image
          if (temporaryImages.find(imageViewState->imageStateStore->GetUniqueStateID()) ==
              temporaryImages.end()) {
            VkImage image = (VkImage)imageViewState->imageStateStore->GetUniqueStateID();
            VkImageCreateInfo imageCreateInfo;
            if (imageViewState->imageStateStore->imageCreateInfoData.Value()) {
              imageCreateInfo = *imageViewState->imageStateStore->imageCreateInfoData.Value();
            } else if (imageViewState->imageStateStore->swapchainKHRStateStore
                           ->swapchainCreateInfoKHRData.Value()) {
              VkSwapchainCreateInfoKHR* pSwapchainCreateInfoKHR =
                  imageViewState->imageStateStore->swapchainKHRStateStore
                      ->swapchainCreateInfoKHRData.Value();
              imageCreateInfo = {
                  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // VkStructureType          sType;
                  nullptr,                              // const void*              pNext;
                  0,                                    // VkImageCreateFlags       flags;
                  VK_IMAGE_TYPE_2D,                     // VkImageType              imageType;
                  pSwapchainCreateInfoKHR->imageFormat, // VkFormat                 format;
                  {                                     // VkExtent3D               extent;
                   pSwapchainCreateInfoKHR->imageExtent.width,
                   pSwapchainCreateInfoKHR->imageExtent.height, 1},
                  1, // uint32_t                 mipLevels;
                  pSwapchainCreateInfoKHR
                      ->imageArrayLayers,              // uint32_t                 arrayLayers;
                  VK_SAMPLE_COUNT_1_BIT,               // VkSampleCountFlagBits    samples;
                  VK_IMAGE_TILING_OPTIMAL,             // VkImageTiling            tiling;
                  pSwapchainCreateInfoKHR->imageUsage, // VkImageUsageFlags        usage;
                  pSwapchainCreateInfoKHR
                      ->imageSharingMode, // VkSharingMode            sharingMode;
                  pSwapchainCreateInfoKHR
                      ->queueFamilyIndexCount, // uint32_t                 queueFamilyIndexCount;
                  pSwapchainCreateInfoKHR
                      ->pQueueFamilyIndices, // const uint32_t*          pQueueFamilyIndices;
                  VK_IMAGE_LAYOUT_UNDEFINED  // VkImageLayout            initialLayout;
              };
            } else {
              throw std::runtime_error("Cannot restore image contents!");
            }
            scheduler.Register(new CvkCreateImage(
                VK_SUCCESS, imageViewState->imageStateStore->deviceStateStore->deviceHandle,
                &imageCreateInfo, nullptr, &image));
            temporaryImages.emplace(imageViewState->imageStateStore->GetUniqueStateID(),
                                    imageViewState->deviceStateStore->deviceHandle);
          }
        }

        // Restore image view
        if (temporaryImageViews.find(imageViewState->GetUniqueStateID()) ==
            temporaryImageViews.end()) {
          VkImageView imageView = (VkImageView)imageViewState->GetUniqueStateID();
          auto imageViewCreateInfo = *imageViewState->imageViewCreateInfoData.Value();
          if (usedUniqueIdForImage) {
            imageViewCreateInfo.image =
                (VkImage)imageViewState->imageStateStore->GetUniqueStateID();
          }
          scheduler.Register(new CvkCreateImageView(VK_SUCCESS,
                                                    imageViewState->deviceStateStore->deviceHandle,
                                                    &imageViewCreateInfo, nullptr, &imageView));
          temporaryImageViews.emplace(imageViewState->GetUniqueStateID(),
                                      imageViewState->deviceStateStore->deviceHandle);
        }
      }
    }

    // Restore missing render pass
    if ((sd._renderpassstates.find(framebufferState->renderPassStateStore->renderPassHandle) ==
         sd._renderpassstates.end()) ||
        (sd._renderpassstates[framebufferState->renderPassStateStore->renderPassHandle]
             ->GetUniqueStateID() != framebufferState->renderPassStateStore->GetUniqueStateID())) {
      usedUniqueIdForRenderPass = true;

      if (temporaryRenderPasses.find(framebufferState->renderPassStateStore->GetUniqueStateID()) ==
          temporaryRenderPasses.end()) {
        VkRenderPass renderPass =
            (VkRenderPass)framebufferState->renderPassStateStore->GetUniqueStateID();

        switch (framebufferState->renderPassStateStore->createdWith) {
        case CreationFunction::KHR_EXTENSION:
          scheduler.Register(new CvkCreateRenderPass2KHR(
              VK_SUCCESS, framebufferState->deviceStateStore->deviceHandle,
              framebufferState->renderPassStateStore->renderPassCreateInfo2Data.Value(), nullptr,
              &renderPass));
          break;
        case CreationFunction::CORE_1_0:
          scheduler.Register(new CvkCreateRenderPass(
              VK_SUCCESS, framebufferState->deviceStateStore->deviceHandle,
              framebufferState->renderPassStateStore->renderPassCreateInfoData.Value(), nullptr,
              &renderPass));
          break;
        case CreationFunction::CORE_1_2:
          scheduler.Register(new CvkCreateRenderPass2KHR(
              VK_SUCCESS, framebufferState->deviceStateStore->deviceHandle,
              framebufferState->renderPassStateStore->renderPassCreateInfo2Data.Value(), nullptr,
              &renderPass));
          break;
        default:
          throw std::runtime_error("VkRenderPass created with an unknown method!");
        }

        temporaryRenderPasses.emplace(framebufferState->renderPassStateStore->GetUniqueStateID(),
                                      framebufferState->deviceStateStore->deviceHandle);
      }
    }

    // Restore framebuffer
    VkFramebuffer framebuffer = framebufferState->framebufferHandle;
    auto framebufferCreateInfo = *framebufferState->framebufferCreateInfoData.Value();
    if (usedUniqueIdForRenderPass) {
      framebufferCreateInfo.renderPass =
          (VkRenderPass)framebufferState->renderPassStateStore->GetUniqueStateID();
    }
    if (!isBitSet(framebufferState->framebufferCreateInfoData.Value()->flags,
                  VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      for (size_t i = 0; i < framebufferCreateInfo.attachmentCount; ++i) {
        if (usedUniqueIdForImageView[i]) {
          const_cast<VkImageView*>(framebufferCreateInfo.pAttachments)[i] =
              (VkImageView)framebufferState->imageViewStateStoreList[i]->GetUniqueStateID();
        }
      }
    }
    scheduler.Register(new CvkCreateFramebuffer(VK_SUCCESS,
                                                framebufferState->deviceStateStore->deviceHandle,
                                                &framebufferCreateInfo, nullptr, &framebuffer));
  }

  // Delete temporary resources
  for (auto& imageViewData : temporaryImageViews) {
    scheduler.Register(
        new CvkDestroyImageView(imageViewData.second, (VkImageView)imageViewData.first, nullptr));
  }
  for (auto& imageData : temporaryImages) {
    scheduler.Register(new CvkDestroyImage(imageData.second, (VkImage)imageData.first, nullptr));
  }
  for (auto& renderPassData : temporaryRenderPasses) {
    scheduler.Register(new CvkDestroyRenderPass(renderPassData.second,
                                                (VkRenderPass)renderPassData.first, nullptr));
  }
}

// Fence

void gits::Vulkan::RestoreFences(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& fenceState : sd._fencestates) {
    if (IsObjectToSkip((uint64_t)fenceState.first)) {
      continue;
    }
    VkFence fence = fenceState.first;
    auto createInfo = *fenceState.second->fenceCreateInfoData.Value();
    if (fenceState.second->fenceUsed) {
      createInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;
    } else {
      createInfo.flags &= ~VK_FENCE_CREATE_SIGNALED_BIT;
    }
    scheduler.Register(new CvkCreateFence(VK_SUCCESS,
                                          fenceState.second->deviceStateStore->deviceHandle,
                                          &createInfo, nullptr, &fence));
  }
}

// Event

void gits::Vulkan::RestoreEvents(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& eventState : sd._eventstates) {
    if (IsObjectToSkip((uint64_t)eventState.first)) {
      continue;
    }

    VkEvent event = eventState.first;
    scheduler.Register(
        new CvkCreateEvent(VK_SUCCESS, eventState.second->deviceStateStore->deviceHandle,
                           eventState.second->eventCreateInfoData.Value(), nullptr, &event));
    if (eventState.second->eventUsed) {
      scheduler.Register(
          new CvkSetEvent(VK_SUCCESS, eventState.second->deviceStateStore->deviceHandle, event));
    }
  }
}

// Semaphore

void gits::Vulkan::RestoreSemaphores(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<VkDevice, std::vector<VkSemaphore>> semaphoresToSignal;

  for (auto& semaphoreState : sd._semaphorestates) {
    if (IsObjectToSkip((uint64_t)semaphoreState.first)) {
      continue;
    }

    VkDevice device = semaphoreState.second->deviceStateStore->deviceHandle;

    if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
      continue;
    }

    VkSemaphore semaphore = semaphoreState.first;
    auto pCreateInfo = semaphoreState.second->semaphoreCreateInfoData.Value();
    auto semaphoreTypeCreateInfo = (VkSemaphoreTypeCreateInfo*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO);
    if (semaphoreTypeCreateInfo) {
      semaphoreTypeCreateInfo->initialValue = semaphoreState.second->timelineSemaphoreValue;
    }

    if (semaphoreState.second->semaphoreUsed && !semaphoreState.second->isTimeline) {
      semaphoresToSignal[device].push_back(semaphore);
    }
    scheduler.Register(
        new CvkCreateSemaphore(VK_SUCCESS, device, pCreateInfo, nullptr, &semaphore));
  }

  // Signal all semaphores that should be signaled (were specified in
  // pSignalSemaphores list and weren't specified in pWaitSemaphores afterwards
  // in any submit)
  for (auto& deviceSemaphoresPair : semaphoresToSignal) {
    VkDevice device = deviceSemaphoresPair.first;
    auto& semaphores = deviceSemaphoresPair.second;

    if (semaphores.size() > 0) {
      // Get temporary command buffer and begin it
      auto submitableResources = GetSubmitableResources(scheduler, device);

      // End command buffer and submit it (also signal a fence)
      SubmitWork(scheduler, submitableResources, true, semaphores);
    }
  }
}

// Query pool

void gits::Vulkan::RestoreQueryPool(CScheduler& scheduler, CStateDynamic& sd) {
  if (sd._querypoolstates.empty()) {
    return;
  }

  for (auto& deviceResourcesPair : temporaryDeviceResources) {
    VkDevice device = deviceResourcesPair.first;

    // Get temporary command buffer and begin it
    auto submitableResources = GetSubmitableResources(scheduler, device);

    for (auto& queryPoolAndStatePair : sd._querypoolstates) {
      if (IsObjectToSkip((uint64_t)queryPoolAndStatePair.first) ||
          (queryPoolAndStatePair.second->deviceStateStore->deviceHandle != device)) {
        continue;
      }

      VkQueryPool queryPool = queryPoolAndStatePair.first;
      auto& queryPoolState = queryPoolAndStatePair.second;
      scheduler.Register(new CvkCreateQueryPool(VK_SUCCESS, device,
                                                queryPoolState->queryPoolCreateInfoData.Value(),
                                                nullptr, &queryPool));

      uint32_t start = 0;
      uint32_t count = 0;

      // Reset queries (minimize number of vkCmdResetQueryPool() function calls)
      for (uint32_t i = 0; i < queryPoolState->resetQueries.size(); ++i) {
        if (queryPoolState->resetQueries[i]) {
          ++count;
        } else {
          if (count > 0) {
            scheduler.Register(new CvkCmdResetQueryPool(submitableResources.commandBuffer,
                                                        queryPool, start, count));
          }
          count = 0;
          start = i + 1;
        }
      }
      if (count > 0) {
        scheduler.Register(
            new CvkCmdResetQueryPool(submitableResources.commandBuffer, queryPool, start, count));
      }

      // Prepare fake queries
      for (uint32_t i = 0; i < queryPoolState->usedQueries.size(); ++i) {
        if (queryPoolState->usedQueries[i]) {
          if (!queryPoolState->resetQueries[i]) {
            Log(ERR) << "Query " << i << " from pool " << queryPool << " used but wasn't reset!";
          }
          switch (queryPoolState->queryPoolCreateInfoData.Value()->queryType) {
          case VK_QUERY_TYPE_TIMESTAMP:
            scheduler.Register(new CvkCmdWriteTimestamp(submitableResources.commandBuffer,
                                                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                        queryPool, i));
            break;
          case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
          case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
          case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
          case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
          case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV:
            // Not allowed
            break;
          default:
            scheduler.Register(
                new CvkCmdBeginQuery(submitableResources.commandBuffer, queryPool, i, 0));
            scheduler.Register(new CvkCmdEndQuery(submitableResources.commandBuffer, queryPool, i));
            break;
          }
        }
      }
    }

    // End command buffer and submit it (also signal a fence)
    SubmitWork(scheduler, submitableResources, true);
  }
}

// Command buffer

void gits::Vulkan::RestoreAllocatedCommandBuffers(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<VkCommandPool, std::map<VkCommandBufferLevel, std::vector<VkCommandBuffer>>>
      commandBufferAllocateInfoData;

  for (auto& commandBufferState : sd._commandbufferstates) {
    VkCommandBuffer commandBuffer = commandBufferState.first;
    if (IsObjectToSkip((uint64_t)commandBuffer)) {
      continue;
    }
    VkCommandBufferAllocateInfo* allocateInfo =
        commandBufferState.second->commandBufferAllocateInfoData.Value();
    commandBufferAllocateInfoData[allocateInfo->commandPool][allocateInfo->level].push_back(
        commandBuffer);
  }

  for (auto& commandPoolMap : commandBufferAllocateInfoData) {
    auto& commandPoolState = sd._commandpoolstates[commandPoolMap.first];

    for (auto& commandBufferLevelMap : commandPoolMap.second) {
      VkCommandBufferLevel commandBufferLevel = commandBufferLevelMap.first;
      std::vector<VkCommandBuffer>& commandBuffers = commandBufferLevelMap.second;
      VkCommandBufferAllocateInfo allocateInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType      sType;
          nullptr,                                        // const void         * pNext;
          commandPoolState->commandPoolHandle,            // VkCommandPool        commandPool;
          commandBufferLevel,                             // VkCommandBufferLevel level;
          static_cast<uint32_t>(commandBuffers.size()) // uint32_t             commandBufferCount;
      };

      scheduler.Register(new CvkAllocateCommandBuffers(
          VK_SUCCESS, commandPoolState->deviceStateStore->deviceHandle, &allocateInfo,
          commandBuffers.data()));
    }
  }
}

void gits::Vulkan::RestoreCommandBuffers(CScheduler& scheduler, CStateDynamic& sd, bool force) {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  if (Config::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit ||
      (api3dIface.CfgRec_IsSubFrameMode() && !force)) {
    return;
  }

  std::set<VkCommandBuffer> restoredCommandBuffers;

  auto RestoreCommandBuffer = [&scheduler, &sd,
                               &restoredCommandBuffers](VkCommandBuffer commandBuffer) {
    auto& commandBufferState = sd._commandbufferstates[commandBuffer];
    if (!commandBufferState->beginCommandBuffer) {
      return;
    }

    // Check if a command buffer should be restored
    if (commandBufferState->beginCommandBuffer->oneTimeSubmit && commandBufferState->submitted) {
      Log(WARN)
          << "Omitting restore of VKCommandBuffer " << commandBuffer
          << " because it's a ONE_TIME_SUBMIT command buffer and it's been already submitted.";
      return;
    }

    for (auto& bufferState : sd.bindingBuffers[commandBuffer]) {
      auto it = sd._bufferstates.find(bufferState->bufferHandle);
      if ((it == sd._bufferstates.end()) ||
          (it->second->GetUniqueStateID() != bufferState->GetUniqueStateID())) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because used VKBuffer " << bufferState->bufferHandle << " doesn't exist.";
        return;
      }
    }

    // Disable this check when recording streams from "The Surge 2" game
    TODO("Find a way to manage workarounds specific for a given title")
    for (auto& obj : sd.bindingImages[commandBuffer]) {
      auto it = sd._imagestates.find(obj->imageHandle);
      if ((it == sd._imagestates.end()) ||
          (it->second->GetUniqueStateID() != obj->GetUniqueStateID())) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because used VKImage " << obj->imageHandle << " doesn't exist.";
        return;
      }
    }

    for (auto& descriptorSetState : commandBufferState->descriptorSetStateStoreList) {
      auto it = sd._descriptorsetstates.find(descriptorSetState.first);
      if ((it == sd._descriptorsetstates.end()) ||
          (it->second->GetUniqueStateID() != descriptorSetState.second->GetUniqueStateID())) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because used VkDescriptorSet " << descriptorSetState.first
                  << " doesn't exist.";
        return;
      }
    }

    for (auto& pipelineState : commandBufferState->pipelineStateStoreList) {
      auto it = sd._pipelinestates.find(pipelineState.first);
      if ((it == sd._pipelinestates.end()) ||
          (it->second->GetUniqueStateID() != pipelineState.second->GetUniqueStateID())) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because used VkPipeline " << pipelineState.first << " doesn't exist.";
        return;
      }
    }

    for (auto& secondaryCommandBufferState :
         commandBufferState->secondaryCommandBuffersStateStoreList) {
      auto it = sd._commandbufferstates.find(secondaryCommandBufferState.first);
      if ((it == sd._commandbufferstates.end()) ||
          (it->second->GetUniqueStateID() !=
           secondaryCommandBufferState.second->GetUniqueStateID())) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because used secondary VkCommandBuffer " << secondaryCommandBufferState.first
                  << " doesn't exist.";
        return;
      }
      if (!secondaryCommandBufferState.second->ended) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because recording of a used secondary VkCommandBuffer "
                  << secondaryCommandBufferState.first << " is not finished.";
        return;
      }
      if (restoredCommandBuffers.find(secondaryCommandBufferState.first) ==
          restoredCommandBuffers.end()) {
        Log(WARN) << "Omitting restore of VKCommandBuffer " << commandBuffer
                  << " because restoring of a used secondary VkCommandBuffer "
                  << secondaryCommandBufferState.first << " was also omitted.";
        return;
      }
    }

    // Restore command buffer
    restoredCommandBuffers.insert(commandBuffer);
    scheduler.Register(new CvkBeginCommandBuffer(
        VK_SUCCESS, commandBuffer,
        commandBufferState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
    commandBufferState->tokensBuffer.Flush(ScheduleTokens);
    if (commandBufferState->ended) {
      scheduler.Register(new CvkEndCommandBuffer(VK_SUCCESS, commandBuffer));
    }
  };

  // Restore all secondary command buffers
  for (auto& commandBufferState : sd._commandbufferstates) {
    if (IsObjectToSkip((uint64_t)commandBufferState.first)) {
      continue;
    }
    if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
        commandBufferState.second->commandBufferAllocateInfoData.Value()->level) {
      RestoreCommandBuffer(commandBufferState.first);
    }
  }

  // In RenderPass, Draws, Blit and Dispatch recording modes, we restore the command buffer later.
  if (!api3dIface.CfgRec_IsRenderPassMode() && !api3dIface.CfgRec_IsDrawsRangeMode() &&
      !api3dIface.CfgRec_IsBlitRangeMode() && !api3dIface.CfgRec_IsDispatchRangeMode()) {
    // Restore all primary command buffers
    for (auto& commandBufferState : sd._commandbufferstates) {
      if (IsObjectToSkip((uint64_t)commandBufferState.first)) {
        continue;
      }
      if (VK_COMMAND_BUFFER_LEVEL_PRIMARY ==
          commandBufferState.second->commandBufferAllocateInfoData.Value()->level) {
        RestoreCommandBuffer(commandBufferState.first);
      }
    }
  }
}

namespace {
// Image data for all layers, mipmaps and aspects is copied using one big buffer
// Buffer offset for each layer, mipmap and aspect is calculated as the nearest
// multiple of a page size greater than the buffer size calculated so far
VkDeviceSize calculateCurrentOffset(VkDeviceSize numToRound, VkDeviceSize multiple) {
  return ((numToRound + multiple - 1) / multiple) * multiple;
};

// timestamp, handle, isImage
std::vector<std::pair<uint64_t, std::pair<uint64_t, bool>>> GetSortedAliasedResources(
    std::set<std::pair<uint64_t, bool>> const& aliasedResources, gits::Vulkan::CStateDynamic& sd) {
  std::vector<std::pair<uint64_t, std::pair<uint64_t, bool>>> sortedByTimestamp;

  for (auto& resource : aliasedResources) {
    if (resource.second) {
      auto it = sd._imagestates.find((VkImage)resource.first);
      if (it != sd._imagestates.end()) {
        sortedByTimestamp.emplace_back(it->second->timestamp, resource);
      }
    } else {
      auto it = sd._bufferstates.find((VkBuffer)resource.first);
      if (it != sd._bufferstates.end()) {
        sortedByTimestamp.emplace_back(it->second->timestamp, resource);
      }
    }
  }

  std::sort(sortedByTimestamp.begin(), sortedByTimestamp.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });

  return sortedByTimestamp;
}

bool isResourceOmittedFromContentsRestoration(uint64_t resource,
                                              bool isImage,
                                              gits::Vulkan::CStateDynamic& sd) {
  if (isImage) {
    VkImage image = (VkImage)resource;
    auto& imageState = sd._imagestates[image];

    if (!imageState) {
      return true;
    }

    // Don't restore unused images
    if (imageState->currentLayout.empty()) {
      return true;
    }

    // Don't restore multisampled images (spec forbids copying data to/from multisampled images)
    if (!imageState->swapchainKHRStateStore &&
        ((!imageState->imageCreateInfoData.Value()) ||
         (imageState->imageCreateInfoData.Value()->samples != VK_SAMPLE_COUNT_1_BIT &&
          !gits::Config::Get().vulkan.recorder.restoreMultisampleImagesWA))) {
      return true;
    }

    // Skip restoring images whose memory objects are already destroyed
    if (imageState->binding) {
      auto deviceMemory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
      auto it = sd._devicememorystates.find(deviceMemory);
      if ((it == sd._devicememorystates.end()) ||
          (it->second->GetUniqueStateID() !=
           imageState->binding->deviceMemoryStateStore->GetUniqueStateID())) {
        return true;
      }
    } else if (imageState->sparseBindings.empty() &&
               !imageState->swapchainKHRStateStore) { // We don't need bindings in swapchain images.
      return true;
    }

    // Skip restoring images which have all layers and mipmaps in undefined or preinitialized state
    {
      uint32_t arrayLayers;
      uint32_t mipLevels;

      if (imageState->swapchainKHRStateStore) {
        arrayLayers = imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData.Value()
                          ->imageArrayLayers;
        mipLevels = 1;
      } else {
        arrayLayers = imageState->imageCreateInfoData.Value()->arrayLayers;
        mipLevels = imageState->imageCreateInfoData.Value()->mipLevels;
      }

      bool allLayersMipmnapsUndefined = true;
      // If at least one layer/mipmap of an image is in any other layout, than restore such image
      for (uint32_t l = 0; l < arrayLayers; ++l) {
        for (uint32_t m = 0; m < mipLevels; ++m) {
          const auto& currentSlice = imageState->currentLayout[l][m];

          if ((currentSlice.Layout != VK_IMAGE_LAYOUT_UNDEFINED) &&
              (currentSlice.Layout != VK_IMAGE_LAYOUT_PREINITIALIZED)) {
            allLayersMipmnapsUndefined = false;
            break;
          }
        }
      }
      // Otherwise, skip restoration of the image
      // because all its layers and mipmaps are undefined/preinitialized
      if (allLayersMipmnapsUndefined) {
        return true;
      }
    }

    // Check if there are other resources aliased with this one which were used more recently
    if (imageState->binding) {
      auto sortedAliasedResources = GetSortedAliasedResources(
          imageState->binding->deviceMemoryStateStore->aliasingTracker.GetAliasedResourcesForImage(
              imageState->binding->memoryOffset, imageState->binding->memorySizeRequirement, image),
          sd);

      for (auto& resourcePair : sortedAliasedResources) {
        if (resourcePair.first > imageState->timestamp) {
          return true;
        }
      }
    }
  } else {
    VkBuffer buffer = (VkBuffer)resource;
    auto& bufferState = sd._bufferstates[buffer];

    if (!bufferState) {
      return true;
    }

    VkDevice device = bufferState->deviceStateStore->deviceHandle;

    if ((!bufferState->bufferCreateInfoData.Value()) ||
        (bufferState->bufferCreateInfoData.Value()->size == 0) || (!bufferState->binding)) {
      return true;
    }

    if ((gits::TBufferStateRestoration::WITH_NON_HOST_VISIBLE_MEMORY_ONLY ==
         gits::Config::Get().vulkan.recorder.crossPlatformStateRestoration.buffers) &&
        (gits::Vulkan::checkMemoryMappingFeasibility(
            device,
            bufferState->binding->deviceMemoryStateStore->memoryAllocateInfoData.Value()
                ->memoryTypeIndex,
            false))) {
      return true;
    }

    // Skip restoring buffers whose memory objects are already destroyed
    if (bufferState->binding) {
      auto deviceMemory = bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
      auto it = sd._devicememorystates.find(deviceMemory);
      if ((it == sd._devicememorystates.end()) ||
          (it->second->GetUniqueStateID() !=
           bufferState->binding->deviceMemoryStateStore->GetUniqueStateID())) {
        return true;
      }
    } else if (bufferState->sparseBindings.empty()) {
      return true;
    }

    // Check if there are other resources aliased with this one which were used more recently
    if (bufferState->binding) {
      auto sortedAliasedResources = GetSortedAliasedResources(
          bufferState->binding->deviceMemoryStateStore->aliasingTracker
              .GetAliasedResourcesForBuffer(bufferState->binding->memoryOffset,
                                            bufferState->binding->memorySizeRequirement, buffer),
          sd);

      for (auto& resourcePair : sortedAliasedResources) {
        if (resourcePair.first > bufferState->timestamp) {
          return true;
        }
      }
    }
  }

  // Restore a resource
  return false;
}

inline bool useSynchronization2(VkPhysicalDevice physicalDevice, VkDevice device) {
  static const char* synchronization2ExtensionName = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
  static bool useSynchronization2 =
      (gits::Vulkan::isVulkanAPIVersionSupported(1, 3, physicalDevice) ||
       gits::Vulkan::areDeviceExtensionsEnabled(device, 1, &synchronization2ExtensionName)) &&
      gits::Vulkan::isSynchronization2FeatureEnabled(device);
  return useSynchronization2;
}

} // namespace

// Image contents

void gits::Vulkan::RestoreImageContents(CScheduler& scheduler, CStateDynamic& sd) {
  if (!Config::Get().vulkan.recorder.crossPlatformStateRestoration.images) {
    return;
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

  struct ImagesAndBarriersStruct {
    std::vector<std::pair<VkImage, VkDeviceSize>> imagesToRestore;

    // Pre-transfer image layout transitions
    std::vector<VkImageMemoryBarrier> imageMemoryBarriersToTransferSrc; // Recorder side
    std::vector<VkImageMemoryBarrier> imageMemoryBarriersToTransferDst; // Player side

    std::vector<VkImageMemoryBarrier2> imageMemoryBarriersToTransferSrc2; // Recorder side
    std::vector<VkImageMemoryBarrier2> imageMemoryBarriersToTransferDst2; // Player side

    // Post-transfer image layout transitions
    std::vector<VkImageMemoryBarrier> imageMemoryBarriersFromTransferSrc; // Recorder side
    std::vector<VkImageMemoryBarrier> imageMemoryBarriersFromTransferDst; // Player side

    std::vector<VkImageMemoryBarrier2> imageMemoryBarriersFromTransferSrc2; // Recorder side
    std::vector<VkImageMemoryBarrier2> imageMemoryBarriersFromTransferDst2; // Player side
  };

  std::map<VkDevice, ImagesAndBarriersStruct> imagesAndBarriersMap;

  for (auto& deviceAndResourcesPair : temporaryDeviceResources) {
    auto device = deviceAndResourcesPair.first;
    auto physicalDevice = deviceAndResourcesPair.second.physicalDevice;
    auto& parameters = imagesAndBarriersMap[device];

    auto statesCount = sd._imagestates.size();
    if (useSynchronization2(physicalDevice, device)) {
      parameters.imageMemoryBarriersToTransferSrc2.reserve(statesCount);
      parameters.imageMemoryBarriersToTransferDst2.reserve(statesCount);
      parameters.imageMemoryBarriersFromTransferSrc2.reserve(statesCount);
      parameters.imageMemoryBarriersFromTransferDst2.reserve(statesCount);
    } else {
      parameters.imageMemoryBarriersToTransferSrc.reserve(statesCount);
      parameters.imageMemoryBarriersToTransferDst.reserve(statesCount);
      parameters.imageMemoryBarriersFromTransferSrc.reserve(statesCount);
      parameters.imageMemoryBarriersFromTransferDst.reserve(statesCount);
    }
  }

  //////////////////////////////////////////////////////////////////
  // Prepare memory barriers / layout transitions for all images
  //////////////////////////////////////////////////////////////////

  for (auto& imageAndStatePair : sd._imagestates) {
    VkImage image = imageAndStatePair.first;

    // Skip resources due to a minimal state restore
    if (gits::Vulkan::IsObjectToSkip((uint64_t)image)) {
      Log(INFO) << "Omitting restoration of an image " << image
                << " due to a minimal state restore.";
      continue;
    }

    auto& imageState = imageAndStatePair.second;
    auto device = imageState->deviceStateStore->deviceHandle;

    if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
      Log(INFO) << "Omitting restoration of an image " << image
                << " because it was created for an ignored device.";
      continue;
    }

    auto physicalDevice =
        imageState->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle;
    auto addBarrier = [physicalDevice, device](std::vector<VkImageMemoryBarrier>& oldVector,
                                               std::vector<VkImageMemoryBarrier2>& newVector,
                                               VkImageMemoryBarrier2 barrier) {
      if (useSynchronization2(physicalDevice, device)) {
        newVector.push_back(barrier);
      } else {
        oldVector.push_back({
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType         sType;
            nullptr,                                // const void*             pNext;
            (VkAccessFlags)barrier.srcAccessMask,   // VkAccessFlags           srcAccessMask;
            (VkAccessFlags)barrier.dstAccessMask,   // VkAccessFlags           dstAccessMask;
            barrier.oldLayout,                      // VkImageLayout           oldLayout;
            barrier.newLayout,                      // VkImageLayout           newLayout;
            barrier.srcQueueFamilyIndex,            // uint32_t                srcQueueFamilyIndex;
            barrier.dstQueueFamilyIndex,            // uint32_t                dstQueueFamilyIndex;
            barrier.image,                          // VkImage                 image;
            barrier.subresourceRange                // VkImageSubresourceRange subresourceRange;
        });
      }
    };

    auto& deviceResources = temporaryDeviceResources[device];
    auto& parameters = imagesAndBarriersMap[device];

    const bool concurrentSharing = [&imageState]() {
      if (imageState->swapchainKHRStateStore) {
        return imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData.Value()
                   ->imageSharingMode == VK_SHARING_MODE_CONCURRENT;
      } else {
        return imageState->imageCreateInfoData.Value()->sharingMode == VK_SHARING_MODE_CONCURRENT;
      }
    }();

    const uint32_t restoreQueueFamily =
        concurrentSharing ? VK_QUEUE_FAMILY_IGNORED : deviceResources.queueFamilyIndex;

    uint32_t arrayLayers;
    uint32_t mipLevels;
    VkFormat format;

    if (imageState->swapchainKHRStateStore) {
      auto& swapchainCreateInfo = imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData;
      arrayLayers = swapchainCreateInfo.Value()->imageArrayLayers;
      mipLevels = 1;
      format = swapchainCreateInfo.Value()->imageFormat;
    } else {
      arrayLayers = imageState->imageCreateInfoData.Value()->arrayLayers;
      mipLevels = imageState->imageCreateInfoData.Value()->mipLevels;
      format = imageState->imageCreateInfoData.Value()->format;
    }

    if (isResourceOmittedFromContentsRestoration((uint64_t)image, true, sd)) {
      // Don't restore image contents, perform only a layout transition

      for (uint32_t l = 0; l < arrayLayers; ++l) {
        for (uint32_t m = 0; m < mipLevels; ++m) {
          const auto& currentSlice = imageState->currentLayout[l][m];

          // Don't perform layout transition for images in undefined or preinitialized state
          if ((currentSlice.Layout == VK_IMAGE_LAYOUT_UNDEFINED) ||
              (currentSlice.Layout == VK_IMAGE_LAYOUT_PREINITIALIZED)) {
            continue;
          }

          const uint32_t imageQueueFamily =
              concurrentSharing ? VK_QUEUE_FAMILY_IGNORED : currentSlice.QueueFamilyIndex;

          addBarrier(parameters.imageMemoryBarriersFromTransferDst,
                     parameters.imageMemoryBarriersFromTransferDst2,
                     {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, // VkStructureType   sType;
                      nullptr,                                  // const void*       pNext;
                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // VkPipelineStageFlags2 srcStageMask;
                      0,                                    // VkAccessFlags2        srcAccessMask;
                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // VkPipelineStageFlags2 dstStageMask;
                      currentSlice.Access,                  // VkAccessFlags2        dstAccessMask;
                      VK_IMAGE_LAYOUT_UNDEFINED,            // VkImageLayout         oldLayout;
                      currentSlice.Layout,                  // VkImageLayout         newLayout;
                      imageQueueFamily, // uint32_t                srcQueueFamilyIndex;
                      imageQueueFamily, // uint32_t                dstQueueFamilyIndex;
                      image,            // VkImage                 image;
                      {                 // VkImageSubresourceRange subresourceRange;
                       getFormatAspectFlags(format), m, 1, l, 1}});
        }
      }

      continue;
    }

    // Copied data
    VkDeviceSize imageSize = 0;

    for (uint32_t l = 0; l < arrayLayers; ++l) {
      for (uint32_t m = 0; m < mipLevels; ++m) {
        const auto& currentSlice = imageState->currentLayout[l][m];

        // Ignore all the images in undefined or preinitialized state as their contents cannot be restored using the below method
        if ((currentSlice.Layout == VK_IMAGE_LAYOUT_UNDEFINED) ||
            (currentSlice.Layout == VK_IMAGE_LAYOUT_PREINITIALIZED)) {
          continue;
        }

        VkExtent3D imageExtent = {
            std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, m)),
            std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, m)),
            std::max(1u, imageState->depth / (uint32_t)std::pow<uint32_t>(2u, m))};

        VkDeviceSize sliceSize = 0;

        for (uint32_t a = 1; a <= 4; a <<= 1) {
          if (getFormatAspectFlags(format) & a) {

            // Image data for all layers, mipmaps and aspects is copied using
            // one big buffer Buffer offset for each layer, mipmap and aspect is
            // calculated as the nearest multiple of a page size greater than
            // the size calculated so far
            VkDeviceSize currentOffset =
                calculateCurrentOffset(imageSize, GetVirtualMemoryPageSize());
            VkDeviceSize aspectSize =
                getFormatAspectDataSize(format, (VkImageAspectFlagBits)a, imageExtent);
            imageSize = currentOffset + aspectSize;
            sliceSize += aspectSize;
          }
        }

        if (sliceSize == 0) {
          continue;
        }

        // Image barriers for all existing images

        const uint32_t imageQueueFamily =
            concurrentSharing ? VK_QUEUE_FAMILY_IGNORED : currentSlice.QueueFamilyIndex;

        // Recorder-side pre-transfer layout transition
        addBarrier(parameters.imageMemoryBarriersToTransferSrc,
                   parameters.imageMemoryBarriersToTransferSrc2,
                   {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, // VkStructureType    sType;
                    nullptr,                                  // const void*        pNext;
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // VkPipelineStageFlags2  srcStageMask;
                    currentSlice.Access,                  // VkAccessFlags2         srcAccessMask;
                    VK_PIPELINE_STAGE_TRANSFER_BIT,       // VkPipelineStageFlags2  dstStageMask;
                    VK_ACCESS_TRANSFER_READ_BIT,          // VkAccessFlags2         dstAccessMask;
                    currentSlice.Layout,                  // VkImageLayout          oldLayout;
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // VkImageLayout          newLayout;
                    imageQueueFamily,   // uint32_t                srcQueueFamilyIndex;
                    restoreQueueFamily, // uint32_t                dstQueueFamilyIndex;
                    image,              // VkImage                 image;
                    {                   // VkImageSubresourceRange subresourceRange;
                     getFormatAspectFlags(format), m, 1, l, 1}});

        // Player-side pre-transfer layout transition
        addBarrier(parameters.imageMemoryBarriersToTransferDst,
                   parameters.imageMemoryBarriersToTransferDst2,
                   {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, // VkStructureType    sType;
                    nullptr,                                  // const void*        pNext;
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // VkPipelineStageFlags2  srcStageMask;
                    0,                                    // VkAccessFlags2         srcAccessMask;
                    VK_PIPELINE_STAGE_TRANSFER_BIT,       // VkPipelineStageFlags2  dstStageMask;
                    VK_ACCESS_TRANSFER_WRITE_BIT,         // VkAccessFlags2         dstAccessMask;
                    VK_IMAGE_LAYOUT_UNDEFINED,            // VkImageLayout          oldLayout;
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // VkImageLayout          newLayout;
                    restoreQueueFamily, // uint32_t                srcQueueFamilyIndex;
                    restoreQueueFamily, // uint32_t                dstQueueFamilyIndex;
                    image,              // VkImage                 image;
                    {                   // VkImageSubresourceRange subresourceRange;
                     getFormatAspectFlags(format), m, 1, l, 1}});

        // Recorder-side post-transfer layout transitions
        addBarrier(parameters.imageMemoryBarriersFromTransferSrc,
                   parameters.imageMemoryBarriersFromTransferSrc2,
                   {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, // VkStructureType    sType;
                    nullptr,                                  // const void*        pNext;
                    VK_PIPELINE_STAGE_TRANSFER_BIT,       // VkPipelineStageFlags2  srcStageMask;
                    VK_ACCESS_TRANSFER_READ_BIT,          // VkAccessFlags2         srcAccessMask;
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // VkPipelineStageFlags2  dstStageMask;
                    currentSlice.Access,                  // VkAccessFlags2         dstAccessMask;
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // VkImageLayout          oldLayout;
                    currentSlice.Layout,                  // VkImageLayout          newLayout;
                    restoreQueueFamily, // uint32_t                srcQueueFamilyIndex;
                    imageQueueFamily,   // uint32_t                dstQueueFamilyIndex;
                    image,              // VkImage                 image;
                    {                   // VkImageSubresourceRange subresourceRange;
                     getFormatAspectFlags(format), m, 1, l, 1}});

        // Player-side post-transfer layout transition
        addBarrier(parameters.imageMemoryBarriersFromTransferDst,
                   parameters.imageMemoryBarriersFromTransferDst2,
                   {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, // VkStructureType    sType;
                    nullptr,                                  // const void*        pNext;
                    VK_PIPELINE_STAGE_TRANSFER_BIT,       // VkPipelineStageFlags2  srcStageMask;
                    VK_ACCESS_TRANSFER_WRITE_BIT,         // VkAccessFlags2         srcAccessMask;
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // VkPipelineStageFlags2  dstStageMask;
                    currentSlice.Access,                  // VkAccessFlags2         dstAccessMask;
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // VkImageLayout          oldLayout;
                    currentSlice.Layout,                  // VkImageLayout          newLayout;
                    restoreQueueFamily, // uint32_t                srcQueueFamilyIndex;
                    imageQueueFamily,   // uint32_t                dstQueueFamilyIndex;
                    image,              // VkImage                 image;
                    {                   // VkImageSubresourceRange subresourceRange;
                     getFormatAspectFlags(format), m, 1, l, 1}});
      }
    }

    if (imageSize == 0) {
      continue;
    }

    parameters.imagesToRestore.push_back({image, imageSize});
  }

  for (auto& deviceAndResourcesPair : temporaryDeviceResources) {
    auto device = deviceAndResourcesPair.first;
    auto physicalDevice = deviceAndResourcesPair.second.physicalDevice;
    auto& parameters = imagesAndBarriersMap[device];
    auto& imagesToRestore = parameters.imagesToRestore;

    // Sort images by size, from largest to smallest (to minize the number of
    // buffers used to restore contents)
    std::sort(imagesToRestore.begin(), imagesToRestore.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    //////////////////////////////////////////////////////////////////
    // Perform pre-transfer memory barriers / layout transitions
    //////////////////////////////////////////////////////////////////

    if ((parameters.imageMemoryBarriersToTransferSrc.size() > 0) ||
        (parameters.imageMemoryBarriersToTransferDst.size() > 0) ||
        (parameters.imageMemoryBarriersToTransferSrc2.size() > 0) ||
        (parameters.imageMemoryBarriersToTransferDst2.size() > 0)) {
      auto submitableResources = GetSubmitableResources(scheduler, device);

      drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);
      if (useSynchronization2(physicalDevice, device)) {
        VkDependencyInfo srcDependencyInfo = {
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO, // VkStructureType               sType;
            nullptr,                           // const void*                   pNext;
            VK_DEPENDENCY_BY_REGION_BIT,       // VkDependencyFlags             dependencyFlags;
            0,                                 // uint32_t                      memoryBarrierCount;
            nullptr,                           // const VkMemoryBarrier2*       pMemoryBarriers;
            0,       // uint32_t                      bufferMemoryBarrierCount;
            nullptr, // const VkBufferMemoryBarrier2* pBufferMemoryBarriers;
            (uint32_t)parameters.imageMemoryBarriersToTransferSrc2
                .size(), // uint32_t                      imageMemoryBarrierCount;
            parameters.imageMemoryBarriersToTransferSrc2
                .data() // const VkImageMemoryBarrier2*  pImageMemoryBarriers;
        };
        drvVk.vkCmdPipelineBarrier2UnifiedGITS(submitableResources.commandBuffer,
                                               &srcDependencyInfo);
      } else {
        drvVk.vkCmdPipelineBarrier(
            submitableResources.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
            (uint32_t)parameters.imageMemoryBarriersToTransferSrc.size(),
            parameters.imageMemoryBarriersToTransferSrc.data());
      }
      drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

      // Submit
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType;
          nullptr,                            // const void*                 pNext;
          0,                                  // uint32_t                    waitSemaphoreCount;
          nullptr,                            // const VkSemaphore*          pWaitSemaphores;
          nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
          1,                                  // uint32_t                    commandBufferCount;
          &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers;
          0,                                  // uint32_t                    signalSemaphoreCount;
          nullptr                             // const VkSemaphore*          pSignalSemaphores;
      };
      drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
      drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

      if (useSynchronization2(physicalDevice, device)) {
        VkDependencyInfo dstDependencyInfo = {
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO, // VkStructureType                sType;
            nullptr,                           // const void*                    pNext;
            VK_DEPENDENCY_BY_REGION_BIT,       // VkDependencyFlags              dependencyFlags;
            0,                                 // uint32_t                       memoryBarrierCount;
            nullptr,                           // const VkMemoryBarrier2*        pMemoryBarriers;
            0,       // uint32_t                       bufferMemoryBarrierCount;
            nullptr, // const VkBufferMemoryBarrier2*  pBufferMemoryBarriers;
            (uint32_t)parameters.imageMemoryBarriersToTransferDst2
                .size(), // uint32_t                       imageMemoryBarrierCount;
            parameters.imageMemoryBarriersToTransferDst2
                .data() // const VkImageMemoryBarrier2*   pImageMemoryBarriers;
        };

        scheduler.Register(new CGitsVkCmdInsertMemoryBarriers2(submitableResources.commandBuffer,
                                                               &dstDependencyInfo));
      } else {
        scheduler.Register(new CGitsVkCmdInsertMemoryBarriers(
            submitableResources.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
            (uint32_t)parameters.imageMemoryBarriersToTransferDst.size(),
            parameters.imageMemoryBarriersToTransferDst.data()));
      }

      SubmitWork(scheduler, submitableResources);

      parameters.imageMemoryBarriersToTransferSrc.clear();
      parameters.imageMemoryBarriersToTransferDst.clear();
      parameters.imageMemoryBarriersToTransferSrc2.clear();
      parameters.imageMemoryBarriersToTransferDst2.clear();
      parameters.imageMemoryBarriersToTransferSrc.shrink_to_fit();
      parameters.imageMemoryBarriersToTransferDst.shrink_to_fit();
      parameters.imageMemoryBarriersToTransferSrc2.shrink_to_fit();
      parameters.imageMemoryBarriersToTransferDst2.shrink_to_fit();
    }

    //////////////////////////////////////////////////////////////////
    // Create temporary resources
    //////////////////////////////////////////////////////////////////

    // Images are restored in bundles - temporary buffers are created, data is
    // copied for as many images as each buffer can store

    // Check the size of the largest image
    if (imagesToRestore.size() > 0) {
      deviceAndResourcesPair.second.maxBufferSize =
          std::max(imagesToRestore[0].second, deviceAndResourcesPair.second.maxBufferSize);
    }

    // Create temporary buffers used for contents restoration
    if (deviceAndResourcesPair.second.maxBufferSize > 0) {
      uint32_t count = Config::Get().vulkan.recorder.reusableStateRestoreResourcesCount;
      for (uint32_t i = 0; i < count; ++i) {
        CreateTemporaryBuffer(scheduler, device, deviceAndResourcesPair.second.maxBufferSize);
      }
    }

    //////////////////////////////////////////////////////////////////
    // Restore contents of images
    //////////////////////////////////////////////////////////////////

    VkDeviceSize totalSize = 0;
    std::map<VkImage, std::pair<std::vector<VkBufferImageCopy>, std::vector<VkInitializeImageGITS>>>
        initializeImagesMap;
    std::vector<VkInitializeImageDataGITS> initializeImagesVk;

    for (uint32_t i = 0; i < imagesToRestore.size(); ++i) {
      auto& imageState = SD()._imagestates[imagesToRestore[i].first];
      VkImage image = imagesToRestore[i].first;
      uint32_t arrayLayers;
      uint32_t mipLevels;
      VkFormat format;

      if (imageState->swapchainKHRStateStore) {
        auto& swapchainCreateInfo = imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData;
        arrayLayers = swapchainCreateInfo.Value()->imageArrayLayers;
        mipLevels = 1;
        format = swapchainCreateInfo.Value()->imageFormat;
      } else {
        arrayLayers = imageState->imageCreateInfoData.Value()->arrayLayers;
        mipLevels = imageState->imageCreateInfoData.Value()->mipLevels;
        format = imageState->imageCreateInfoData.Value()->format;
      }

      for (uint32_t l = 0; l < arrayLayers; ++l) {
        for (uint32_t m = 0; m < mipLevels; ++m) {
          const auto& currentSlice = imageState->currentLayout[l][m];

          // Ignore all the images in undefined or preinitialized state
          if ((currentSlice.Layout == VK_IMAGE_LAYOUT_UNDEFINED) ||
              (currentSlice.Layout == VK_IMAGE_LAYOUT_PREINITIALIZED)) {
            continue;
          }

          VkExtent3D imageExtent = {
              std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, m)),
              std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, m)),
              std::max(1u, imageState->depth / (uint32_t)std::pow<uint32_t>(2u, m))};

          VkDeviceSize colorOrDepthOffset = UINT64_MAX;
          VkDeviceSize stencilOffset = UINT64_MAX;

          for (uint32_t a = 1; a <= 4; a <<= 1) {
            if (getFormatAspectFlags(format) & a) {

              // Image data for all layers, mipmaps and aspects is copied into a
              // buffer Buffer offset for each image's layer, mipmap and aspect is
              // calculated as the nearest multiple of a page size greater than
              // the size calculated so far
              VkDeviceSize currentOffset =
                  calculateCurrentOffset(totalSize, GetVirtualMemoryPageSize());
              VkDeviceSize size =
                  getFormatAspectDataSize(format, (VkImageAspectFlagBits)a, imageExtent);
              totalSize = currentOffset + size;

              if ((VkImageAspectFlagBits)a != VK_IMAGE_ASPECT_STENCIL_BIT) {
                colorOrDepthOffset = currentOffset;
              } else {
                stencilOffset = currentOffset;
              }

              initializeImagesMap[image].first.emplace_back(VkBufferImageCopy{
                  currentOffset, // VkDeviceSize bufferOffset;
                  0,             // uint32_t bufferRowLength;
                  0,             // uint32_t bufferImageHeight;
                  {
                      // VkImageSubresourceLayers imageSubresource;
                      (VkImageAspectFlags)a, // VkImageAspectFlags aspectMask;
                      m,                     // uint32_t mipLevel;
                      l,                     // uint32_t baseArrayLayer;
                      1                      // uint32_t layerCount;
                  },
                  {
                      // VkOffset3D imageOffset;
                      0, // int32_t x;
                      0, // int32_t y;
                      0  // int32_t z;
                  },
                  imageExtent // VkExtent3D imageExtent;
              });
            }
          }

          initializeImagesMap[image].second.emplace_back(VkInitializeImageGITS{
              colorOrDepthOffset, // VkDeviceSize bufferOffset;
              stencilOffset,      // VkDeviceSize stencilBufferOffset;
              imageExtent,        // VkExtent3D imageExtent;
              m,                  // uint32_t imageMipLevel;
              l,                  // uint32_t imageArrayLayer;
              currentSlice.Layout // VkImageLayout imageLayout;
          });
        }
      }

      {
        VkInitializeImageDataGITS initializeImageData = {
            image,                                // VkImage image;
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // VkImageLayout layout;
            static_cast<uint32_t>(
                initializeImagesMap[image].first.size()), // uint32_t copyRegionsCount;
            initializeImagesMap[image].first.data(),      // const VkBufferImageCopy* pCopyRegions;
            static_cast<uint32_t>(
                initializeImagesMap[image].second.size()), // uint32_t initializeRegionsCount;
            initializeImagesMap[image]
                .second.data() // const VkInitializeImageGITS* pInitializeRegions;
        };
        initializeImagesVk.emplace_back(initializeImageData);
      }

      // Schedule copy operations when the temporary buffer cannot hold data for another image
      // or if we are processing the last image
      if ((totalSize > 0) && ((i == (imagesToRestore.size() - 1)) ||
                              (calculateCurrentOffset(totalSize, GetVirtualMemoryPageSize()) +
                                   imagesToRestore[i + 1].second >
                               deviceAndResourcesPair.second.maxBufferSize))) {

        // Get temporary command buffer and begin it
        auto submitableResources = GetSubmitableResources(scheduler, device);

        // Get temporary buffer to copy whole image data - all layers, mipmaps and aspects
        auto temporaryBuffer = GetTemporaryBuffer(scheduler, device, submitableResources);

        drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);

        // Pre-transfer buffer memory barriers
        VkBufferMemoryBarrier copyToBufferMemoryBarrierPre = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
            nullptr,                                 // const void* pNext
            VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags srcAccessMask
            VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
            temporaryBuffer.buffer,                  // VkBuffer buffer
            0,                                       // VkDeviceSize offset
            VK_WHOLE_SIZE                            // VkDeviceSize size;
        };

        // Post-transfer buffer memory barriers
        VkBufferMemoryBarrier copyToBufferMemoryBarrierPost = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
            nullptr,                                 // const void* pNext
            VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask
            VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
            temporaryBuffer.buffer,                  // VkBuffer buffer
            0,                                       // VkDeviceSize offset
            VK_WHOLE_SIZE                            // VkDeviceSize size;
        };

        // Perform pre-transfer memory barriers, copy data, perform post-transfer memory barriers
        drvVk.vkCmdPipelineBarrier(submitableResources.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                                   nullptr, 1, &copyToBufferMemoryBarrierPre, 0, nullptr);
        for (auto& initializeImage : initializeImagesVk) {
          drvVk.vkCmdCopyImageToBuffer(submitableResources.commandBuffer, initializeImage.image,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporaryBuffer.buffer,
                                       initializeImage.copyRegionsCount,
                                       initializeImage.pCopyRegions);
        }
        drvVk.vkCmdPipelineBarrier(submitableResources.commandBuffer,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                                   &copyToBufferMemoryBarrierPost, 0, nullptr);
        drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

        // Clear memory contents before use
        scheduler.Register(new CGitsVkMemoryReset(device, temporaryBuffer.memory, totalSize,
                                                  temporaryBuffer.mappedPtr));

        // Submit
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType
            nullptr,                            // const void*                 pNext
            0,                                  // uint32_t                    waitSemaphoreCount
            nullptr,                            // const VkSemaphore*          pWaitSemaphores
            nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask
            1,                                  // uint32_t                    commandBufferCount
            &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers
            0,                                  // uint32_t                    signalSemaphoreCount
            nullptr                             // const VkSemaphore*          pSignalSemaphores
        };
        drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
        drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

        // Get image data from memory
        scheduler.Register(new CGitsVkMemoryRestore(device, temporaryBuffer.memory, totalSize,
                                                    temporaryBuffer.mappedPtr));

        // Record state restore into a stream
        scheduler.Register(new CGitsInitializeMultipleImages(
            submitableResources.commandBuffer, temporaryBuffer.buffer, initializeImagesVk));

        // End command buffer and submit it (fence is signaled in the above submission)
        SubmitWork(scheduler, submitableResources);

        totalSize = 0;
        initializeImagesVk.clear();
        initializeImagesMap.clear();
      }
    }

    //////////////////////////////////////////////////////////////////
    // Perform post-transfer memory barriers / layout transitions
    //////////////////////////////////////////////////////////////////
    if ((parameters.imageMemoryBarriersFromTransferSrc.size() > 0) ||
        (parameters.imageMemoryBarriersFromTransferDst.size() > 0) ||
        (parameters.imageMemoryBarriersFromTransferSrc2.size() > 0) ||
        (parameters.imageMemoryBarriersFromTransferDst2.size() > 0)) {
      auto submitableResources = GetSubmitableResources(scheduler, device);

      drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);
      {
        VkDependencyInfo srcDependencyInfo = {
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO, // VkStructureType                sType;
            nullptr,                           // const void                   * pNext;
            VK_DEPENDENCY_BY_REGION_BIT,       // VkDependencyFlags              dependencyFlags;
            0,                                 // uint32_t                       memoryBarrierCount;
            nullptr,                           // const VkMemoryBarrier2       * pMemoryBarriers;
            0,       // uint32_t                       bufferMemoryBarrierCount;
            nullptr, // const VkBufferMemoryBarrier2 * pBufferMemoryBarriers;
            (uint32_t)parameters.imageMemoryBarriersFromTransferSrc2
                .size(), // uint32_t                       imageMemoryBarrierCount;
            parameters.imageMemoryBarriersFromTransferSrc2
                .data() // const VkImageMemoryBarrier2  * pImageMemoryBarriers;
        };
        if (useSynchronization2(physicalDevice, device)) {
          drvVk.vkCmdPipelineBarrier2UnifiedGITS(submitableResources.commandBuffer,
                                                 &srcDependencyInfo);
        } else {
          drvVk.vkCmdPipelineBarrier(
              submitableResources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
              nullptr, (uint32_t)parameters.imageMemoryBarriersFromTransferSrc.size(),
              parameters.imageMemoryBarriersFromTransferSrc.data());
        }
      }
      drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

      // Submit
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType;
          nullptr,                            // const void*                 pNext;
          0,                                  // uint32_t                    waitSemaphoreCount;
          nullptr,                            // const VkSemaphore*          pWaitSemaphores;
          nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
          1,                                  // uint32_t                    commandBufferCount;
          &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers;
          0,                                  // uint32_t                    signalSemaphoreCount;
          nullptr                             // const VkSemaphore*          pSignalSemaphores;
      };
      drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
      drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

      if (useSynchronization2(physicalDevice, device)) {
        VkDependencyInfo dstDependencyInfo = {
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO, // VkStructureType                sType;
            nullptr,                           // const void                   * pNext;
            VK_DEPENDENCY_BY_REGION_BIT,       // VkDependencyFlags              dependencyFlags;
            0,                                 // uint32_t                       memoryBarrierCount;
            nullptr,                           // const VkMemoryBarrier2       * pMemoryBarriers;
            0,       // uint32_t                       bufferMemoryBarrierCount;
            nullptr, // const VkBufferMemoryBarrier2 * pBufferMemoryBarriers;
            (uint32_t)parameters.imageMemoryBarriersFromTransferDst2
                .size(), // uint32_t                       imageMemoryBarrierCount;
            parameters.imageMemoryBarriersFromTransferDst2
                .data() // const VkImageMemoryBarrier2  * pImageMemoryBarriers;
        };
        scheduler.Register(new CGitsVkCmdInsertMemoryBarriers2(submitableResources.commandBuffer,
                                                               &dstDependencyInfo));
      } else {
        scheduler.Register(new CGitsVkCmdInsertMemoryBarriers(
            submitableResources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
            (uint32_t)parameters.imageMemoryBarriersFromTransferDst.size(),
            parameters.imageMemoryBarriersFromTransferDst.data()));
      }

      SubmitWork(scheduler, submitableResources);
    }
  }
}

// Buffer contents

void gits::Vulkan::RestoreBufferContents(CScheduler& scheduler, CStateDynamic& sd) {
  if (TBufferStateRestoration::NONE ==
      Config::Get().vulkan.recorder.crossPlatformStateRestoration.buffers) {
    return;
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

  struct BuffersAndBarriersStruct {
    std::vector<std::pair<VkBuffer, VkDeviceSize>> buffersToRestore;

    // Recorder-side memory barriers
    std::vector<VkBufferMemoryBarrier> dataAcquisitionBufferMemoryBarriersPre;
    std::vector<VkBufferMemoryBarrier> dataAcquisitionBufferMemoryBarriersPost;

    // Player-side memory barriers
    std::vector<VkBufferMemoryBarrier> dataRestorationBufferMemoryBarriersPre;
    std::vector<VkBufferMemoryBarrier> dataRestorationBufferMemoryBarriersPost;
  };

  std::map<VkDevice, BuffersAndBarriersStruct> buffersAndBarriersMap;

  //////////////////////////////////////////////////////////////////
  // Prepare memory barriers
  //////////////////////////////////////////////////////////////////

  for (auto& deviceAndResourcesPair : temporaryDeviceResources) {
    auto device = deviceAndResourcesPair.first;
    auto& parameters = buffersAndBarriersMap[device];

    auto statesCount = sd._bufferstates.size();
    parameters.dataAcquisitionBufferMemoryBarriersPre.reserve(statesCount);
    parameters.dataAcquisitionBufferMemoryBarriersPost.reserve(statesCount);
    parameters.dataRestorationBufferMemoryBarriersPre.reserve(statesCount);
    parameters.dataRestorationBufferMemoryBarriersPost.reserve(statesCount);
  }

  for (auto& bufferAndStatePair : sd._bufferstates) {
    auto dstBuffer = bufferAndStatePair.first;

    // Skip resources due to a minimal state restore
    if (gits::Vulkan::IsObjectToSkip((uint64_t)dstBuffer)) {
      Log(INFO) << "Omitting restoration of a buffer " << dstBuffer
                << " due to a minimal state restore.";
      continue;
    }

    if (isResourceOmittedFromContentsRestoration((uint64_t)dstBuffer, false, sd)) {
      continue;
    }

    auto device = bufferAndStatePair.second->deviceStateStore->deviceHandle;

    if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
      Log(INFO) << "Omitting restoration of a buffer " << dstBuffer
                << " because it was created for an ignored device.";
      continue;
    }

#define ALL_VULKAN_BUFFER_ACCESS_BITS VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT

    auto& parameters = buffersAndBarriersMap[device];

    // Pre-transfer buffer memory barriers
    parameters.dataAcquisitionBufferMemoryBarriersPre.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
        nullptr,                                 // const void* pNext
        ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
        dstBuffer,                               // VkBuffer buffer
        0,                                       // VkDeviceSize offset
        VK_WHOLE_SIZE                            // VkDeviceSize size;
    });

    parameters.dataRestorationBufferMemoryBarriersPre.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
        nullptr,                                 // const void* pNext
        0,                                       // VkAccessFlags srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
        dstBuffer,                               // VkBuffer buffer
        0,                                       // VkDeviceSize offset
        VK_WHOLE_SIZE                            // VkDeviceSize size;
    });

    // Post-transfer buffer memory barriers
    parameters.dataAcquisitionBufferMemoryBarriersPost.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
        nullptr,                                 // const void* pNext
        VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
        ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
        dstBuffer,                               // VkBuffer buffer
        0,                                       // VkDeviceSize offset
        VK_WHOLE_SIZE                            // VkDeviceSize size;
    });

    parameters.dataRestorationBufferMemoryBarriersPost.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
        nullptr,                                 // const void* pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask
        ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
        dstBuffer,                               // VkBuffer buffer
        0,                                       // VkDeviceSize offset
        VK_WHOLE_SIZE                            // VkDeviceSize size;
    });

    parameters.buffersToRestore.push_back(
        {dstBuffer, bufferAndStatePair.second->bufferCreateInfoData.Value()->size});
  }

  for (auto& deviceAndResourcesPair : temporaryDeviceResources) {
    auto device = deviceAndResourcesPair.first;
    auto& deviceResources = deviceAndResourcesPair.second;
    auto& parameters = buffersAndBarriersMap[device];
    auto& buffersToRestore = parameters.buffersToRestore;

    // Sort buffers by size, from largest to smallest (to minize the number of buffers used to restore contents)
    std::sort(buffersToRestore.begin(), buffersToRestore.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    //////////////////////////////////////////////////////////////////
    // Create temporary resources
    //////////////////////////////////////////////////////////////////

    // When image contents restoration was skipped, create temporary buffers here
    if ((deviceResources.temporaryBuffers.empty()) && (buffersToRestore.size() > 0) &&
        deviceResources.maxBufferSize > 0) {
      uint32_t count = Config::Get().vulkan.recorder.reusableStateRestoreResourcesCount;
      for (uint32_t i = 0; i < count; ++i) {
        CreateTemporaryBuffer(scheduler, device, deviceResources.maxBufferSize);
      }
    }

    //////////////////////////////////////////////////////////////////
    // Perform / submit memory barriers
    //////////////////////////////////////////////////////////////////

    if ((parameters.dataAcquisitionBufferMemoryBarriersPre.size() > 0) ||
        (parameters.dataRestorationBufferMemoryBarriersPre.size() > 0)) {
      auto submitableResources = GetSubmitableResources(scheduler, device);

      drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);
      drvVk.vkCmdPipelineBarrier(
          submitableResources.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
          (uint32_t)parameters.dataAcquisitionBufferMemoryBarriersPre.size(),
          parameters.dataAcquisitionBufferMemoryBarriersPre.data(), 0, nullptr);
      drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

      // Submit
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType;
          nullptr,                            // const void*                 pNext;
          0,                                  // uint32_t                    waitSemaphoreCount;
          nullptr,                            // const VkSemaphore*          pWaitSemaphores;
          nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
          1,                                  // uint32_t                    commandBufferCount;
          &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers;
          0,                                  // uint32_t                    signalSemaphoreCount;
          nullptr                             // const VkSemaphore*          pSignalSemaphores;
      };
      drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
      drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

      scheduler.Register(new CGitsVkCmdInsertMemoryBarriers(
          submitableResources.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
          (uint32_t)parameters.dataRestorationBufferMemoryBarriersPre.size(),
          parameters.dataRestorationBufferMemoryBarriersPre.data(), 0, nullptr));

      SubmitWork(scheduler, submitableResources);

      parameters.dataAcquisitionBufferMemoryBarriersPre.clear();
      parameters.dataRestorationBufferMemoryBarriersPre.clear();
      parameters.dataAcquisitionBufferMemoryBarriersPre.shrink_to_fit();
      parameters.dataRestorationBufferMemoryBarriersPre.shrink_to_fit();
    }

    //////////////////////////////////////////////////////////////////
    // Copy data / restore contents
    //////////////////////////////////////////////////////////////////

    VkDeviceSize totalSize = 0;
    std::vector<VkInitializeBufferDataGITS> acquireBuffersData;
    std::vector<VkInitializeBufferDataGITS> restoreBuffersData;

    for (uint32_t i = 0; i < buffersToRestore.size(); ++i) {
      VkBuffer dstBuffer = buffersToRestore[i].first;

      VkDeviceSize currentOffset = calculateCurrentOffset(totalSize, GetVirtualMemoryPageSize());
      VkDeviceSize size = buffersToRestore[i].second;
      totalSize = currentOffset + size;

      // Recorder-side copy
      acquireBuffersData.push_back({dstBuffer,
                                    {
                                        0,             // VkDeviceSize   srcOffset
                                        currentOffset, // VkDeviceSize   dstOffset
                                        size           // VkDeviceSize   size
                                    }});

      // Player-side copy
      restoreBuffersData.push_back({dstBuffer,
                                    {
                                        currentOffset, // VkDeviceSize   srcOffset
                                        0,             // VkDeviceSize   dstOffset
                                        size           // VkDeviceSize   size
                                    }});

      if ((totalSize > 0) && ((i == (buffersToRestore.size() - 1)) ||
                              (calculateCurrentOffset(totalSize, GetVirtualMemoryPageSize()) +
                                   buffersToRestore[i + 1].second >
                               deviceResources.maxBufferSize))) {
        // Get temporary command buffer and begin it
        auto submitableResources = GetSubmitableResources(scheduler, device);

        // Get temporary buffer to copy whole image data - all layers, mipmaps and aspects
        auto temporaryBuffer = GetTemporaryBuffer(scheduler, device, submitableResources);

        drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);

        // Pre-transfer buffer memory barriers
        VkBufferMemoryBarrier dataAcquisitionTemporaryBufferBarrierPre = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
            nullptr,                                 // const void* pNext
            VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags srcAccessMask
            VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
            temporaryBuffer.buffer,                  // VkBuffer buffer
            0,                                       // VkDeviceSize offset
            VK_WHOLE_SIZE                            // VkDeviceSize size;
        };

        // Post-transfer buffer memory barriers
        VkBufferMemoryBarrier dataAcquisitionTemporaryBufferBarrierPost = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
            nullptr,                                 // const void* pNext
            VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask
            VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
            temporaryBuffer.buffer,                  // VkBuffer buffer
            0,                                       // VkDeviceSize offset
            VK_WHOLE_SIZE                            // VkDeviceSize size;
        };

        // Set barriers and acquire data
        drvVk.vkCmdPipelineBarrier(submitableResources.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                                   nullptr, 1, &dataAcquisitionTemporaryBufferBarrierPre, 0,
                                   nullptr);
        for (auto& initializeBuffer : acquireBuffersData) {
          drvVk.vkCmdCopyBuffer(submitableResources.commandBuffer, initializeBuffer.buffer,
                                temporaryBuffer.buffer, 1, &initializeBuffer.bufferCopy);
        }
        drvVk.vkCmdPipelineBarrier(submitableResources.commandBuffer,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                                   &dataAcquisitionTemporaryBufferBarrierPost, 0, nullptr);
        drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

        // Clear memory contents before use
        scheduler.Register(new CGitsVkMemoryReset(device, temporaryBuffer.memory, totalSize,
                                                  temporaryBuffer.mappedPtr));

        // Submit
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType;
            nullptr,                            // const void*                 pNext;
            0,                                  // uint32_t                    waitSemaphoreCount;
            nullptr,                            // const VkSemaphore*          pWaitSemaphores;
            nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
            1,                                  // uint32_t                    commandBufferCount;
            &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers;
            0,                                  // uint32_t                    signalSemaphoreCount;
            nullptr                             // const VkSemaphore*          pSignalSemaphores;
        };
        drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
        drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

        scheduler.Register(new CGitsVkMemoryRestore(device, temporaryBuffer.memory, totalSize,
                                                    temporaryBuffer.mappedPtr));
        scheduler.Register(new CGitsInitializeMultipleBuffers(
            submitableResources.commandBuffer, temporaryBuffer.buffer, restoreBuffersData));

        // End command buffer and submit it (fence is signaled in the above submission)
        SubmitWork(scheduler, submitableResources);

        totalSize = 0;
        acquireBuffersData.clear();
        restoreBuffersData.clear();
      }
    }

    //////////////////////////////////////////////////////////////////
    // Perform / submit memory barriers
    //////////////////////////////////////////////////////////////////

    if ((parameters.dataAcquisitionBufferMemoryBarriersPost.size() > 0) ||
        (parameters.dataRestorationBufferMemoryBarriersPost.size() > 0)) {
      auto submitableResources = GetSubmitableResources(scheduler, device);

      drvVk.vkBeginCommandBuffer(submitableResources.commandBuffer, &commandBufferBeginInfo);
      drvVk.vkCmdPipelineBarrier(
          submitableResources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
          (uint32_t)parameters.dataAcquisitionBufferMemoryBarriersPost.size(),
          parameters.dataAcquisitionBufferMemoryBarriersPost.data(), 0, nullptr);
      drvVk.vkEndCommandBuffer(submitableResources.commandBuffer);

      // Submit
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,      // VkStructureType             sType;
          nullptr,                            // const void*                 pNext;
          0,                                  // uint32_t                    waitSemaphoreCount;
          nullptr,                            // const VkSemaphore*          pWaitSemaphores;
          nullptr,                            // const VkPipelineStageFlags* pWaitDstStageMask;
          1,                                  // uint32_t                    commandBufferCount;
          &submitableResources.commandBuffer, // const VkCommandBuffer*      pCommandBuffers;
          0,                                  // uint32_t                    signalSemaphoreCount;
          nullptr                             // const VkSemaphore*          pSignalSemaphores;
      };
      drvVk.vkQueueSubmit(submitableResources.queue, 1, &submitInfo, submitableResources.fence);
      drvVk.vkWaitForFences(device, 1, &submitableResources.fence, VK_FALSE, globalTimeoutValue);

      scheduler.Register(new CGitsVkCmdInsertMemoryBarriers(
          submitableResources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
          (uint32_t)parameters.dataRestorationBufferMemoryBarriersPost.size(),
          parameters.dataRestorationBufferMemoryBarriersPost.data(), 0, nullptr));

      SubmitWork(scheduler, submitableResources);
    }
  }
}

// Acceleration structure contents

void gits::Vulkan::RestoreAccelerationStructureContents(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceResourcesPair : temporaryDeviceResources) {
    VkDevice device = deviceResourcesPair.first;

    std::vector<std::pair<VkDeviceSize, VkAccelerationStructureKHR>>
        accelerationStructuresToRestore;
    accelerationStructuresToRestore.reserve(sd._accelerationstructurekhrstates.size());

    for (auto& accelerationStructureState : sd._accelerationstructurekhrstates) {
      if (IsObjectToSkip((uint64_t)accelerationStructureState.first)) {
        continue;
      }

      if (temporaryDeviceResources.find(device) == temporaryDeviceResources.end()) {
        continue;
      }

      if ((accelerationStructureState.second->buildSizeInfo.accelerationStructureSize == 0) ||
          (!accelerationStructureState.second->buildInfo &&
           !accelerationStructureState.second->updateInfo &&
           !accelerationStructureState.second->copyInfo)) {
        Log(INFO) << "Omitting restoration of an acceleration structure "
                  << accelerationStructureState.first << ".";
        continue;
      }

      accelerationStructuresToRestore.emplace_back(
          accelerationStructureState.second->buildSizeInfo.accelerationStructureSize,
          accelerationStructureState.first);
    }

    std::sort(accelerationStructuresToRestore.begin(), accelerationStructuresToRestore.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::function<VkAccelerationStructureKHR(
        CAccelerationStructureKHRState & accelerationStructureState, VkCommandBuffer commandBuffer)>
        scheduleTemporaryAccelerationStructureCreation;
    std::function<void(VkAccelerationStructureKHR dst, VkBuffer accelerationStructureBuffer,
                       CAccelerationStructureKHRState::CBuildInfo * buildInfo,
                       VkCommandBuffer commandBuffer)>
        scheduleBuild;
    std::function<void(VkAccelerationStructureKHR dst, VkBuffer accelerationStructureBuffer,
                       CAccelerationStructureKHRState::CCopyInfo * copyInfo,
                       VkCommandBuffer commandBuffer)>
        scheduleCopy;

    std::vector<VkAccelerationStructureKHR> temporaryAccelerationStructures;
    std::vector<VkBuffer> temporaryBuffers;
    std::vector<VkDeviceMemory> temporaryMemoryObjects;

    auto prepareSourceData = [&](COnQueueSubmitEndDataStorage* structStorageData,
                                 CBufferDeviceAddressObjectData& bufferDeviceAddressData) {
      if ((!structStorageData->GetDataSize()) || (!bufferDeviceAddressData._buffer) ||
          (!bufferDeviceAddressData._originalDeviceAddress)) {
        return bufferDeviceAddressData._originalDeviceAddress;
      }

      auto memoryBufferPair = createTemporaryBuffer(
          device, structStorageData->GetDataSize(),
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          nullptr, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      auto memory = (VkDeviceMemory)memoryBufferPair.first->GetUniqueStateID();
      auto buffer = (VkBuffer)memoryBufferPair.second->GetUniqueStateID();
      auto deviceAddress = getBufferDeviceAddress(device, memoryBufferPair.second->bufferHandle);
      VkBufferDeviceAddressInfo deviceAddressInfo = {
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType sType;
          nullptr,                                      // const void* pNext;
          buffer                                        // VkBuffer buffer;
      };

      scheduler.Register(new CvkCreateBuffer(VK_SUCCESS, device,
                                             memoryBufferPair.second->bufferCreateInfoData.Value(),
                                             nullptr, &buffer));
      scheduler.Register(new CvkAllocateMemory(
          VK_SUCCESS, device, memoryBufferPair.first->memoryAllocateInfoData.Value(), nullptr,
          &memory));
      scheduler.Register(new CvkBindBufferMemory(VK_SUCCESS, device, buffer, memory, 0));
      scheduler.Register(
          new CvkGetBufferDeviceAddressUnifiedGITS(deviceAddress, device, &deviceAddressInfo));

      void* ptr;
      scheduler.Register(new CvkMapMemory(
          VK_SUCCESS, device, (VkDeviceMemory)memoryBufferPair.first->GetUniqueStateID(), 0,
          structStorageData->GetDataSize(), 0, &ptr));
      scheduler.Register(new CGitsVkMemoryRestore(device, memory, structStorageData->GetDataSize(),
                                                  structStorageData->GetData()));

      temporaryBuffers.push_back(buffer);
      temporaryMemoryObjects.push_back(memory);

      bufferDeviceAddressData._offset = -structStorageData->GetOffset();
      bufferDeviceAddressData._originalDeviceAddress = deviceAddress;
      bufferDeviceAddressData._buffer = buffer;

      return deviceAddress - structStorageData->GetOffset();
    };

    scheduleBuild = [&](VkAccelerationStructureKHR dst, VkBuffer accelerationStructureBuffer,
                        CAccelerationStructureKHRState::CBuildInfo* buildInfo,
                        VkCommandBuffer commandBuffer) {
      if (!buildInfo) {
        return;
      }

      auto pBuildRangeInfo = buildInfo->buildRangeInfoDataArray.Value();
      auto pBuildGeometryInfo = buildInfo->buildGeometryInfoData.Value();
      pBuildGeometryInfo->scratchData.deviceAddress = 0;

      if (pBuildGeometryInfo->srcAccelerationStructure) {
        if (pBuildGeometryInfo->srcAccelerationStructure !=
            pBuildGeometryInfo->dstAccelerationStructure) {
          pBuildGeometryInfo->srcAccelerationStructure =
              scheduleTemporaryAccelerationStructureCreation(
                  *buildInfo->srcAccelerationStructureStateStore, commandBuffer);
        } else {
          pBuildGeometryInfo->srcAccelerationStructure = dst;
        }
      }
      pBuildGeometryInfo->dstAccelerationStructure = dst;

      if (buildInfo->controlData.executionSide == VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS) {

        for (uint32_t i = 0; i < pBuildGeometryInfo->geometryCount; ++i) {
          VkAccelerationStructureGeometryKHR* pGeometry =
              pBuildGeometryInfo->pGeometries
                  ? (VkAccelerationStructureGeometryKHR*)&pBuildGeometryInfo->pGeometries[i]
                  : (VkAccelerationStructureGeometryKHR*)pBuildGeometryInfo->ppGeometries[i];

          switch (pGeometry->geometryType) {
          case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
            auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
                pGeometry->geometry.triangles.pNext, VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
            if (structStoragePointer && structStoragePointer->pStructStorage) {
              auto* trianglesData = (CVkAccelerationStructureGeometryTrianglesDataKHRData*)
                                        structStoragePointer->pStructStorage;
              if (trianglesData->_vertexData) {
                pGeometry->geometry.triangles.vertexData.deviceAddress =
                    prepareSourceData(trianglesData->_vertexData.get(),
                                      trianglesData->_vertexData->_bufferDeviceAddress);
              }
              if (trianglesData->_indexData) {
                pGeometry->geometry.triangles.indexData.deviceAddress =
                    prepareSourceData(trianglesData->_indexData.get(),
                                      trianglesData->_indexData->_bufferDeviceAddress);
              }
              if (trianglesData->_transformData) {
                pGeometry->geometry.triangles.transformData.deviceAddress =
                    prepareSourceData(trianglesData->_transformData.get(),
                                      trianglesData->_transformData->_bufferDeviceAddress);
              }
            }
            break;
          }
          case VK_GEOMETRY_TYPE_AABBS_KHR: {
            auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
                pGeometry->geometry.aabbs.pNext, VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
            if (structStoragePointer && structStoragePointer->pStructStorage) {
              auto* aabbsData = (CVkAccelerationStructureGeometryAabbsDataKHRData*)
                                    structStoragePointer->pStructStorage;
              if (aabbsData->_data) {
                pGeometry->geometry.aabbs.data.deviceAddress = prepareSourceData(
                    aabbsData->_data.get(), aabbsData->_data->_bufferDeviceAddress);
              }
            }
            break;
          }
          case VK_GEOMETRY_TYPE_INSTANCES_KHR: {
            auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
                pGeometry->geometry.instances.pNext, VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
            if (structStoragePointer && structStoragePointer->pStructStorage) {
              auto* geometryInstancesData = (CVkAccelerationStructureGeometryInstancesDataKHRData*)
                                                structStoragePointer->pStructStorage;
              pGeometry->geometry.instances.data.deviceAddress = prepareSourceData(
                  geometryInstancesData, geometryInstancesData->_bufferDeviceAddress);

              TODO("FINISH IMPLEMENTATION FOR AS BUILDING WITHOUT CAPTURE/REPLAY FEATURES!!!")
              //geometryInstancesData->individualPatcher
              //CGitsVkCmdPatchDeviceAddresses()
            }
            break;
          }
          default:
            throw std::runtime_error("Provided incorrect geometry type!");
            break;
          }
        }

        scheduler.Register(new CvkCmdBuildAccelerationStructuresKHR(
            commandBuffer, 1, pBuildGeometryInfo, &pBuildRangeInfo));

        VkBufferMemoryBarrier bufferMemoryBarrier = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType    sType;
            nullptr,                                 // const void       * pNext;
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, // VkAccessFlags      srcAccessMask;
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, // VkAccessFlags      dstAccessMask;
            VK_QUEUE_FAMILY_IGNORED,     // uint32_t           srcQueueFamilyIndex;
            VK_QUEUE_FAMILY_IGNORED,     // uint32_t           dstQueueFamilyIndex;
            accelerationStructureBuffer, // VkBuffer           buffer;
            0,                           // VkDeviceSize       offset;
            VK_WHOLE_SIZE                // VkDeviceSize       size;
        };
        scheduler.Register(new CvkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0,
            nullptr));
      } else /* VK_COMMAND_EXECUTION_SIDE_HOST_GITS */ {
        throw std::runtime_error(
            "Restoring ray tracing-related resources built on host is not yet supported.");
      }
    };

    scheduleCopy = [&](VkAccelerationStructureKHR dst, VkBuffer accelerationStructureBuffer,
                       CAccelerationStructureKHRState::CCopyInfo* copyInfo,
                       VkCommandBuffer commandBuffer) {
      if (!copyInfo) {
        return;
      }

      auto& srcAccelerationStructureStateStore = copyInfo->srcAccelerationStructureStateStore;
      VkCopyAccelerationStructureInfoKHR info =
          *copyInfo->copyAccelerationStructureInfoData.Value();
      info.dst = dst;
      info.src = scheduleTemporaryAccelerationStructureCreation(*srcAccelerationStructureStateStore,
                                                                commandBuffer);

      if (copyInfo->executionSide == VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS) {
        scheduler.Register(new CvkCmdCopyAccelerationStructureKHR(commandBuffer, &info));
        VkBufferMemoryBarrier bufferMemoryBarrier = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType    sType;
            nullptr,                                 // const void       * pNext;
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, // VkAccessFlags      srcAccessMask;
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, // VkAccessFlags      dstAccessMask;
            VK_QUEUE_FAMILY_IGNORED,     // uint32_t           srcQueueFamilyIndex;
            VK_QUEUE_FAMILY_IGNORED,     // uint32_t           dstQueueFamilyIndex;
            accelerationStructureBuffer, // VkBuffer           buffer;
            0,                           // VkDeviceSize       offset;
            VK_WHOLE_SIZE                // VkDeviceSize       size;
        };
        scheduler.Register(new CvkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0,
            nullptr));
      } else /* VK_COMMAND_EXECUTION_SIDE_HOST_GITS */ {
        scheduler.Register(
            new CvkCopyAccelerationStructureKHR(VK_SUCCESS, device, VK_NULL_HANDLE, &info));
      }
    };

    scheduleTemporaryAccelerationStructureCreation =
        [&](CAccelerationStructureKHRState& accelerationStructureState,
            VkCommandBuffer commandBuffer) {
          VkAccelerationStructureKHR accelerationStructure =
              (VkAccelerationStructureKHR)accelerationStructureState.GetUniqueStateID();

          auto createInfo = *accelerationStructureState.accelerationStructureCreateInfoData.Value();

          auto tmpMemoryBufferPair = createTemporaryBuffer(
              device,
              std::max(accelerationStructureState.buildSizeInfo.accelerationStructureSize,
                       createInfo.size),
              accelerationStructureState.bufferStateStore->bufferCreateInfoData.Value()->usage);
          auto& memoryState = *tmpMemoryBufferPair.first;
          auto& bufferState = *tmpMemoryBufferPair.second;
          auto memory = (VkDeviceMemory)memoryState.GetUniqueStateID();
          auto buffer = (VkBuffer)bufferState.GetUniqueStateID();

          createInfo.offset = 0;
          createInfo.deviceAddress = 0;
          createInfo.buffer = buffer;

          scheduler.Register(new CvkCreateBuffer(
              VK_SUCCESS, device, bufferState.bufferCreateInfoData.Value(), nullptr, &buffer));
          scheduler.Register(new CvkAllocateMemory(
              VK_SUCCESS, device, memoryState.memoryAllocateInfoData.Value(), nullptr, &memory));
          scheduler.Register(new CvkBindBufferMemory(VK_SUCCESS, device, buffer, memory, 0));
          scheduler.Register(new CvkCreateAccelerationStructureKHR(
              VK_SUCCESS, device, &createInfo, nullptr, &accelerationStructure));

          temporaryAccelerationStructures.push_back(accelerationStructure);
          temporaryBuffers.push_back(buffer);
          temporaryMemoryObjects.push_back(memory);

          // Schedule copy commands
          scheduleCopy(accelerationStructure, buffer, accelerationStructureState.copyInfo.get(),
                       commandBuffer);

          // Schedule build commands
          scheduleBuild(accelerationStructure, buffer, accelerationStructureState.buildInfo.get(),
                        commandBuffer);

          // Schedule update commands
          scheduleBuild(accelerationStructure, buffer, accelerationStructureState.updateInfo.get(),
                        commandBuffer);

          return accelerationStructure;
        };

    // Function for scheduling AS building commands
    auto scheduleAccelerationStructuresBuilds = [&](VkAccelerationStructureTypeKHR type) {
      for (auto& accelerationStructurePair : accelerationStructuresToRestore) {
        auto& accelerationStructureState =
            sd._accelerationstructurekhrstates[accelerationStructurePair.second];

        if (accelerationStructureState->accelerationStructureCreateInfoData.Value()->type != type) {
          continue;
        }

        auto submittableResources = GetSubmitableResources(scheduler, device);
        VkCommandBuffer commandBuffer = submittableResources.commandBuffer;

        // Schedule copy commands
        scheduleCopy(accelerationStructurePair.second,
                     accelerationStructureState->bufferStateStore->bufferHandle,
                     accelerationStructureState->copyInfo.get(), commandBuffer);

        // Schedule build commands
        scheduleBuild(accelerationStructurePair.second,
                      accelerationStructureState->bufferStateStore->bufferHandle,
                      accelerationStructureState->buildInfo.get(), commandBuffer);

        // Schedule update commands
        scheduleBuild(accelerationStructurePair.second,
                      accelerationStructureState->bufferStateStore->bufferHandle,
                      accelerationStructureState->updateInfo.get(), commandBuffer);

        drvVk.vkQueueSubmit(submittableResources.queue, 0, nullptr, submittableResources.fence);
        SubmitWork(scheduler, submittableResources);

        scheduler.Register(new CvkWaitForFences(VK_SUCCESS, device, 1, &submittableResources.fence,
                                                VK_FALSE, globalTimeoutValue));

        for (auto as : temporaryAccelerationStructures) {
          scheduler.Register(new CvkDestroyAccelerationStructureKHR(device, as, nullptr));
        }
        temporaryAccelerationStructures.clear();
        for (auto buffer : temporaryBuffers) {
          scheduler.Register(new CvkDestroyBuffer(device, buffer, nullptr));
        }
        temporaryBuffers.clear();
        for (auto memory : temporaryMemoryObjects) {
          scheduler.Register(new CvkFreeMemory(device, memory, nullptr));
        }
        temporaryMemoryObjects.clear();
      }
    };

    // First, restore only bottom level acceleration structures
    scheduleAccelerationStructuresBuilds(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

    // Next, restore top level acceleration structures
    scheduleAccelerationStructuresBuilds(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
  }
}

// State restore finish

void gits::Vulkan::FinishStateRestore(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& deviceState : sd._devicestates) {
    if (IsObjectToSkip((uint64_t)deviceState.first)) {
      continue;
    }

    drvVk.vkDeviceWaitIdle(deviceState.first);
    scheduler.Register(new CvkDeviceWaitIdle(VK_SUCCESS, deviceState.first));
  }

  for (auto& deviceResouresPair : temporaryDeviceResources) {
    VkDevice device = deviceResouresPair.first;

    // Destroy all temporary buffers
    for (auto& bufferParams : deviceResouresPair.second.temporaryBuffers) {
      drvVk.vkDestroyBuffer(device, bufferParams.second.buffer, nullptr);
      drvVk.vkFreeMemory(device, bufferParams.second.memory, nullptr);
      scheduler.Register(new CvkDestroyBuffer(device, bufferParams.second.buffer, nullptr));
      scheduler.Register(new CvkFreeMemory(device, bufferParams.second.memory, nullptr));
    }

    // Destroy temporary fences
    for (auto& submitableResources : deviceResouresPair.second.submitableResources) {
      drvVk.vkDestroyFence(device, submitableResources.fence, nullptr);
      scheduler.Register(new CvkDestroyFence(device, submitableResources.fence, nullptr));
    }

    // Destroy temporary pool (command buffers are also implicitly destroyed)
    drvVk.vkDestroyCommandPool(device, deviceResouresPair.second.commandPool, nullptr);
    scheduler.Register(
        new CvkDestroyCommandPool(device, deviceResouresPair.second.commandPool, nullptr));
  }
}

// Queue submits

void gits::Vulkan::PrepareVkQueueSubmits(CStateDynamic& sd) {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  if (api3dIface.CfgRec_IsSubFrameMode()) {
    CVkSubmitInfoArrayWrap submitInfoForPrepare =
        getSubmitInfoForPrepare(Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector,
                                Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
                                Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);
    if (api3dIface.CfgRec_IsRenderPassMode() || api3dIface.CfgRec_IsBlitRangeMode() ||
        api3dIface.CfgRec_IsDispatchRangeMode()) {
      restoreCommandBufferSettings(Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
                                   submitInfoForPrepare,
                                   Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);
    } else if (api3dIface.CfgRec_IsDrawsRangeMode()) {
      restoreCommandBufferSettings(
          Config::Get().vulkan.recorder.objRange.rangeSpecial.range, submitInfoForPrepare,
          Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode,
          Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector.back());
    }
    auto queueHandle = sd.lastQueueSubmit->queueStateStore->queueHandle;
    if (nullptr != submitInfoForPrepare.submitInfoData.Value()) {
      uint32_t submitCount = (uint32_t)submitInfoForPrepare.submitInfoData.size();
      auto pSubmits = submitInfoForPrepare.submitInfoData.Value();

      VkResult retVal =
          drvVk.vkQueueSubmit(queueHandle, submitCount, pSubmits, sd.lastQueueSubmit->fenceHandle);
      vkQueueSubmit_SD(retVal, queueHandle, submitCount, pSubmits, sd.lastQueueSubmit->fenceHandle,
                       true);
    } else if (nullptr != submitInfoForPrepare.submitInfo2Data.Value()) {
      uint32_t submitCount = (uint32_t)submitInfoForPrepare.submitInfo2Data.size();
      auto pSubmits2 = submitInfoForPrepare.submitInfo2Data.Value();

      VkResult retVal = drvVk.vkQueueSubmit2(queueHandle, submitCount, pSubmits2,
                                             sd.lastQueueSubmit->fenceHandle);
      vkQueueSubmit2_SD(retVal, queueHandle, submitCount, pSubmits2,
                        sd.lastQueueSubmit->fenceHandle, true);
    }
    drvVk.vkQueueWaitIdle(queueHandle);

    CVkSubmitInfoArrayWrap submitInfoForSchedule =
        getSubmitInfoForSchedule(Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector,
                                 Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
                                 Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);
    sd.objectsUsedInQueueSubmit.clear();
    sd.objectsUsedInQueueSubmit = getPointersUsedInQueueSubmit(
        submitInfoForSchedule, Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector,
        Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
        Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);

    for (auto& state : sd._devicestates) {
      drvVk.vkDeviceWaitIdle(state.first);
    }
  }
}

void gits::Vulkan::PostRestoreVkQueueSubmits(CScheduler& scheduler, CStateDynamic& sd) {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();

  if (api3dIface.CfgRec_IsSubFrameMode()) {
    scheduler.Register(new gits::CTokenFrameNumber(CToken::ID_FRAME_START, 1));
    CVkSubmitInfoArrayWrap submitInfoForSchedule =
        getSubmitInfoForSchedule(Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector,
                                 Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
                                 Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);
    auto submitInfoDataValues = submitInfoForSchedule.submitInfoData.Value();
    auto submitInfo2DataValues = submitInfoForSchedule.submitInfo2Data.Value();
    RestoreCommandBuffers(scheduler, sd, true);
    if (api3dIface.CfgRec_IsRenderPassMode() || api3dIface.CfgRec_IsDrawsRangeMode() ||
        api3dIface.CfgRec_IsBlitRangeMode() || api3dIface.CfgRec_IsDispatchRangeMode()) {
      VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
      if (submitInfoDataValues != nullptr) {
        commandBuffer = submitInfoDataValues[0].pCommandBuffers[0];
      } else if (submitInfo2DataValues != nullptr) {
        commandBuffer = submitInfo2DataValues[0].pCommandBufferInfos[0].commandBuffer;
      }
      if (commandBuffer != VK_NULL_HANDLE) {
        auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
        scheduler.Register(new CvkBeginCommandBuffer(
            VK_SUCCESS, commandBuffer,
            commandBufferState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
        if (api3dIface.CfgRec_IsDrawsRangeMode()) {
          commandBufferState->tokensBuffer.ScheduleDraw(
              ScheduleTokens, Config::Get().vulkan.recorder.objRange.rangeSpecial.objVector.back(),
              Config::Get().vulkan.recorder.objRange.rangeSpecial.range);
        } else {
          commandBufferState->tokensBuffer.ScheduleObject(
              ScheduleTokens, Config::Get().vulkan.recorder.objRange.rangeSpecial.range,
              Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode);
        }

        if (commandBufferState->ended) {
          scheduler.Register(new CvkEndCommandBuffer(VK_SUCCESS, commandBuffer));
        }
      }
    }

    if (submitInfoDataValues != nullptr) {
      scheduler.Register(new CvkQueueSubmit(
          VK_SUCCESS, sd.lastQueueSubmit->queueStateStore->queueHandle,
          (uint32_t)submitInfoForSchedule.submitInfoData.size(),
          submitInfoForSchedule.submitInfoData.Value(), sd.lastQueueSubmit->fenceHandle));
    } else if (submitInfo2DataValues != nullptr) {
      scheduler.Register(new CvkQueueSubmit2(
          VK_SUCCESS, sd.lastQueueSubmit->queueStateStore->queueHandle,
          (uint32_t)submitInfoForSchedule.submitInfo2Data.size(),
          submitInfoForSchedule.submitInfo2Data.Value(), sd.lastQueueSubmit->fenceHandle));
    }
  }
}

void gits::Vulkan::StateRestoreInfoStart(CScheduler& scheduler, const char* info) {
  scheduler.Register(new CGitsVkStateRestoreInfo(info));
}

void gits::Vulkan::StateRestoreInfoEnd(CScheduler& scheduler, const char* info, int index) {
  scheduler.Register(new CGitsVkStateRestoreInfo(info, index));
}
