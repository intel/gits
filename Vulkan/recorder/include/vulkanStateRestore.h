// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateRestore.h
*
* @brief Declaration of Vulkan State Restore.
*/

#pragma once

#include "state.h"
#include "scheduler.h"
#include "vulkanStateDynamic.h"

namespace gits {
namespace Vulkan {

struct SubmittableResourcesStruct {
  VkQueue queue;
  VkCommandBuffer commandBuffer;
  VkFence fence;
};

struct TemporaryBufferStruct {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  void* mappedPtr;
};

struct TemporaryDeviceResourcesStruct {
  VkPhysicalDevice physicalDevice;
  VkQueue queue;
  uint32_t queueFamilyIndex;
  VkCommandPool commandPool;
  uint32_t currentResourceIndex;
  VkDeviceSize maxBufferSize;
  std::vector<SubmittableResourcesStruct> submitableResources;
  std::map<VkBuffer, TemporaryBufferStruct> temporaryBuffers;
  std::set<VkBuffer> freeBuffers;
  std::map<VkFence, std::set<VkBuffer>> usedBuffers;
};

SubmittableResourcesStruct const& GetSubmitableResources(CScheduler& scheduler, VkDevice device);
void SubmitWork(CScheduler& scheduler,
                SubmittableResourcesStruct const& submitableResources,
                bool signalFence = false,
                std::vector<VkSemaphore> const& semaphoresToSignal = std::vector<VkSemaphore>());

TemporaryBufferStruct CreateTemporaryBuffer(CScheduler& scheduler,
                                            VkDevice device,
                                            VkDeviceSize size);

TemporaryBufferStruct GetTemporaryBuffer(CScheduler& scheduler,
                                         VkDevice device,
                                         SubmittableResourcesStruct& submitableResources);

void RestoreVkInstances(CScheduler& scheduler, CStateDynamic& sd);
void RestoreVkPhysicalDevices(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSurfaceKHR(CScheduler& scheduler, CStateDynamic& sd);
void RestoreVkDevices(CScheduler& scheduler, CStateDynamic& sd);
void RestoreVkQueue(CScheduler& scheduler, CStateDynamic& sd);
void PrepareTemporaryResources(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSwapchainKHR(CScheduler& scheduler, CStateDynamic& sd);
void RestoreVkDescriptorPool(CScheduler& scheduler, CStateDynamic& sd);
void RestoreVkCommandPool(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSampler(CScheduler& scheduler, CStateDynamic& sd);
void RestoreMemory(CScheduler& scheduler, CStateDynamic& sd);
void RestoreMappedMemory(CScheduler& scheduler, CStateDynamic& sd);
void RestoreImage(CScheduler& scheduler, CStateDynamic& sd);
void RestoreImageBindings(CScheduler& scheduler, CStateDynamic& sd);
void RestoreImageView(CScheduler& scheduler, CStateDynamic& sd);
void RestoreBuffer(CScheduler& scheduler, CStateDynamic& sd);
void RestoreBufferBindings(CScheduler& scheduler, CStateDynamic& sd);
void RestoreBufferView(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDeferredOperations(CScheduler& scheduler, CStateDynamic& sd);
void RestoreAccelerationStructure(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDescriptorSetLayout(CScheduler& scheduler, CStateDynamic& sd);
void RestoreAllocatedDescriptorSet(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDescriptorSetsUpdates(CScheduler& scheduler, CStateDynamic& sd);
void RestorePipelineLayout(CScheduler& scheduler, CStateDynamic& sd);
void DestroyTemporaryDescriptorSetLayouts(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDescriptorUpdateTemplate(CScheduler& scheduler, CStateDynamic& sd);
void RestorePipelineCache(CScheduler& scheduler, CStateDynamic& sd);
void RestoreShaderModules(CScheduler& scheduler, CStateDynamic& sd);
void RestoreRenderPass(CScheduler& scheduler, CStateDynamic& sd);
void RestorePipelines(CScheduler& scheduler, CStateDynamic& sd);
void RestoreFramebuffer(CScheduler& scheduler, CStateDynamic& sd);
void RestoreFences(CScheduler& scheduler, CStateDynamic& sd);
void RestoreEvents(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSemaphores(CScheduler& scheduler, CStateDynamic& sd);
void RestoreQueryPool(CScheduler& scheduler, CStateDynamic& sd);
void RestoreAllocatedCommandBuffers(CScheduler& scheduler, CStateDynamic& sd);
void RestoreCommandBuffers(CScheduler& scheduler, CStateDynamic& sd, bool force = false);
void RestoreImageContents(CScheduler& scheduler, CStateDynamic& sd);
void RestoreBufferContents(CScheduler& scheduler, CStateDynamic& sd);
void RestoreAccelerationStructureContents(CScheduler& scheduler, CStateDynamic& sd);
void FinishStateRestore(CScheduler& scheduler, CStateDynamic& sd);
void PrepareVkQueueSubmits(CStateDynamic& sd);
void PostRestoreVkQueueSubmits(CScheduler& scheduler, CStateDynamic& sd);
void StateRestoreInfoStart(CScheduler& scheduler, const char* info);
void StateRestoreInfoEnd(CScheduler& scheduler, const char* info, int index);

/**
    * @brief Library state getter class
    *
    * gits::Vulkan::CState class is responsible for restoring Vulkan objects in apropriate order.
    */
class CState : public gits::CState {
public:
  CState() {}
  // Get state is not being used in Vulkan API. Objects are queried and
  // scheduled in one step in "Schedule" function.
  void Get() {}
  void Schedule(CScheduler& scheduler) const {
    auto& sd = SD();
    StateRestoreInfoStart(scheduler, "Restoring resources...");
    RestoreVkInstances(scheduler, sd);
    RestoreVkPhysicalDevices(scheduler, sd);
    RestoreSurfaceKHR(scheduler, sd);
    RestoreVkDevices(scheduler, sd);
    RestoreVkQueue(scheduler, sd);
    PrepareTemporaryResources(scheduler, sd);
    RestoreSwapchainKHR(scheduler, sd);
    RestoreVkDescriptorPool(scheduler, sd);
    RestoreVkCommandPool(scheduler, sd);
    RestoreSampler(scheduler, sd);
    RestoreImage(scheduler, sd);
    RestoreBuffer(scheduler, sd);
    RestoreMemory(scheduler, sd);
    RestoreMappedMemory(scheduler, sd);
    RestoreImageBindings(scheduler, sd);
    RestoreBufferBindings(scheduler, sd);
    RestoreImageView(scheduler, sd);
    RestoreBufferView(scheduler, sd);
    RestoreDeferredOperations(scheduler, sd);
    RestoreAccelerationStructure(scheduler, sd);
    RestoreDescriptorSetLayout(scheduler, sd);
    RestoreAllocatedDescriptorSet(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Resources restored", 0);
    StateRestoreInfoStart(scheduler, "Restoring descriptor set bindings...");
    RestoreDescriptorSetsUpdates(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Descriptor set bindings restored", 1);
    StateRestoreInfoStart(scheduler, "Restoring shader resources...");
    RestorePipelineLayout(scheduler, sd);
    DestroyTemporaryDescriptorSetLayouts(scheduler, sd);
    RestoreDescriptorUpdateTemplate(scheduler, sd);
    RestorePipelineCache(scheduler, sd);
    RestoreShaderModules(scheduler, sd);
    RestoreRenderPass(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Shader resources restored", 2);
    StateRestoreInfoStart(scheduler, "Restoring and compiling pipeline objects...");
    RestorePipelines(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Pipeline objects restored and compiled", 3);
    StateRestoreInfoStart(scheduler, "Restoring additional resources...");
    RestoreFramebuffer(scheduler, sd);
    RestoreFences(scheduler, sd);
    RestoreEvents(scheduler, sd);
    RestoreSemaphores(scheduler, sd);
    RestoreQueryPool(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Additional resources restored", 4);
    StateRestoreInfoStart(scheduler, "Restoring recorded command buffers...");
    RestoreAllocatedCommandBuffers(scheduler, sd);
    RestoreCommandBuffers(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Recorded command buffers restored", 5);
    StateRestoreInfoStart(scheduler, "Restoring contents of images...");
    RestoreImageContents(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Contents of images restored", 6);
    StateRestoreInfoStart(scheduler, "Restoring contents of buffers...");
    RestoreBufferContents(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Contents of buffers restored", 7);
    StateRestoreInfoStart(scheduler, "Restoring contents of acceleration structures...");
    RestoreAccelerationStructureContents(scheduler, sd);
    StateRestoreInfoEnd(scheduler, "Contents of acceleration structures restored", 7);
    FinishStateRestore(scheduler, sd);
  }

  void Prepare() const {
    auto& sd = SD();
    PrepareVkQueueSubmits(sd);
  }

  void PostSchedule(CScheduler& scheduler) const {
    auto& sd = SD();
    PostRestoreVkQueueSubmits(scheduler, sd);
  }

  void Finish(CScheduler& scheduler) const {}
};
} // namespace Vulkan
} // namespace gits
