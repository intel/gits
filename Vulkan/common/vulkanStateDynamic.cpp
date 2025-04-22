// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateDynamic.cpp
*
* @brief Definition of Vulkan common part library implementation.
*
*/

#include "vulkanTools.h"
#include "vulkanStateDynamic.h"
#include "vulkanInternalShaderModules.h"

namespace gits {
namespace Vulkan {

// Instantiation of static members

uint64_t CInternalResources::COffscreenAppsSupport::uniqueHandleCounter = 1;

std::unordered_map<VkDeviceAddress, VkAccelerationStructureKHR>
    CAccelerationStructureKHRState::deviceAddresses;
uint32_t CAccelerationStructureKHRState::globalAccelerationStructureBuildCommandIndex = 0;

std::set<CBufferState::DeviceAddressRangeState, CBufferState::DeviceAddressRangeState>
    CBufferState::deviceAddresses;
std::unordered_map<VkDeviceAddress, VkBuffer> CBufferState::deviceAddressesQuickLook;

// BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
// Please, (un)comment all the areas with the above token together, at the same time
//
// std::unordered_map<VkBuffer, std::shared_ptr<CBufferState>> CBufferState::shaderDeviceAddressBuffers;

//------------------------------- STATE DYNAMIC ---------------------------------
CStateDynamic::CStateDynamic()
    : currentlyAllocatedMemoryAll(0),
      currentlyAllocatedMemoryGPU(0),
      currentlyAllocatedMemoryCPU_GPU(0),
      currentlyMappedMemory(0),
      depthRangeUnrestrictedEXTEnabled(false),
      stateRestoreFinished(false) {}

CStateDynamic::~CStateDynamic() {}

CStateDynamic& CStateDynamic::Get() {
  static CStateDynamic* ptr = new CStateDynamic;
  return *ptr;
}

// Defined here due to circular dependency between CImageState and CSwapchainKHRState
std::set<uint64_t> CSwapchainKHRState::GetMappedPointers() {
  std::set<uint64_t> pointers;
  pointers.insert((uint64_t)deviceStateStore->deviceHandle);
  for (auto obj : deviceStateStore->GetMappedPointers()) {
    pointers.insert((uint64_t)obj);
  }
  pointers.insert((uint64_t)surfaceKHRStateStore->surfaceKHRHandle);
  for (auto obj : surfaceKHRStateStore->GetMappedPointers()) {
    pointers.insert((uint64_t)obj);
  }
  for (auto obj : swapchainCreateInfoKHRData.GetMappedPointers()) {
    pointers.insert((uint64_t)obj);
  }
  for (auto& imageState : imageStateStoreList) {
    pointers.insert((uint64_t)imageState->imageHandle);
    for (auto obj : imageState->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
  }
  return pointers;
}

InternalPipelinesManager::InternalPipelines::InternalPipelines(VkDevice _device)
    : device(_device),
      descriptorSetLayoutState(),
      pipelineLayout(VK_NULL_HANDLE),
      prepareDeviceAddressesForPatchingPipeline(VK_NULL_HANDLE),
      patchDeviceAddressesPipeline(VK_NULL_HANDLE),
      prepareIndirectCopyFor16BitIndexedVerticesPipeline(VK_NULL_HANDLE),
      prepareIndirectCopyFor32BitIndexedVerticesPipeline(VK_NULL_HANDLE),
      performIndirectCopyPipeline(VK_NULL_HANDLE),
      patchShaderGroupHandlesInSBT(VK_NULL_HANDLE) {}

InternalPipelinesManager::InternalPipelines::~InternalPipelines() {
  drvVk.vkDestroyPipeline(device, prepareDeviceAddressesForPatchingPipeline, nullptr);
  drvVk.vkDestroyPipeline(device, patchDeviceAddressesPipeline, nullptr);
  drvVk.vkDestroyPipeline(device, prepareIndirectCopyFor16BitIndexedVerticesPipeline, nullptr);
  drvVk.vkDestroyPipeline(device, prepareIndirectCopyFor32BitIndexedVerticesPipeline, nullptr);
  drvVk.vkDestroyPipeline(device, performIndirectCopyPipeline, nullptr);
  drvVk.vkDestroyPipeline(device, patchShaderGroupHandlesInSBT, nullptr);

  drvVk.vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  if (descriptorSetLayoutState) {
    drvVk.vkDestroyDescriptorSetLayout(device, descriptorSetLayoutState->descriptorSetLayoutHandle,
                                       nullptr);
  }
}

InternalPipelinesManager::InternalPipelines::InternalPipelines(const InternalPipelines& other)
    : device(other.device),
      descriptorSetLayoutState(other.descriptorSetLayoutState),
      pipelineLayout(other.pipelineLayout),
      prepareDeviceAddressesForPatchingPipeline(other.prepareDeviceAddressesForPatchingPipeline),
      patchDeviceAddressesPipeline(other.patchDeviceAddressesPipeline),
      prepareIndirectCopyFor16BitIndexedVerticesPipeline(
          other.prepareIndirectCopyFor16BitIndexedVerticesPipeline),
      prepareIndirectCopyFor32BitIndexedVerticesPipeline(
          other.prepareIndirectCopyFor32BitIndexedVerticesPipeline),
      performIndirectCopyPipeline(other.performIndirectCopyPipeline),
      patchShaderGroupHandlesInSBT(other.patchShaderGroupHandlesInSBT) {}

InternalPipelinesManager::InternalPipelines& InternalPipelinesManager::InternalPipelines::operator=(
    const InternalPipelines& other) {
  if (this == &other) {
    return *this;
  }
  device = other.device;
  descriptorSetLayoutState = other.descriptorSetLayoutState;
  pipelineLayout = other.pipelineLayout;
  prepareDeviceAddressesForPatchingPipeline = other.prepareDeviceAddressesForPatchingPipeline;
  patchDeviceAddressesPipeline = other.patchDeviceAddressesPipeline;
  prepareIndirectCopyFor16BitIndexedVerticesPipeline =
      other.prepareIndirectCopyFor16BitIndexedVerticesPipeline;
  prepareIndirectCopyFor32BitIndexedVerticesPipeline =
      other.prepareIndirectCopyFor32BitIndexedVerticesPipeline;
  performIndirectCopyPipeline = other.performIndirectCopyPipeline;
  patchShaderGroupHandlesInSBT = other.patchShaderGroupHandlesInSBT;
  return *this;
}

std::shared_ptr<CDescriptorSetLayoutState> InternalPipelinesManager::InternalPipelines::
    getDescriptorSetLayoutState() {
  if (!descriptorSetLayoutState) {
    descriptorSetLayoutState = createInternalDescriptorSetLayout(device);
  }
  return descriptorSetLayoutState;
}

VkPipelineLayout InternalPipelinesManager::InternalPipelines::getLayout() {
  if (pipelineLayout == VK_NULL_HANDLE) {
    auto descriptorSetLayout = getDescriptorSetLayoutState()->descriptorSetLayoutHandle;
    pipelineLayout = createInternalPipelineLayout(device, 1, &descriptorSetLayout);
  }
  return pipelineLayout;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareDeviceAddressesForPatchingPipeline() {
  if (prepareDeviceAddressesForPatchingPipeline == VK_NULL_HANDLE) {
    prepareDeviceAddressesForPatchingPipeline = createInternalPipeline(
        device, pipelineLayout, getPrepareDeviceAddressesForPatchingShaderModuleSource());
  }

  return prepareDeviceAddressesForPatchingPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPatchDeviceAddressesPipeline() {
  if (patchDeviceAddressesPipeline == VK_NULL_HANDLE) {
    patchDeviceAddressesPipeline =
        createInternalPipeline(device, pipelineLayout, getPatchDeviceAddressesShaderModuleSource());
  }

  return patchDeviceAddressesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareIndirectCopyFor16BitIndexedVerticesPipeline() {
  if (prepareIndirectCopyFor16BitIndexedVerticesPipeline == VK_NULL_HANDLE) {
    prepareIndirectCopyFor16BitIndexedVerticesPipeline = createInternalPipeline(
        device, pipelineLayout, getPrepareIndirectCopyFor16BitIndexedVerticesShaderModuleSource());
  }

  return prepareIndirectCopyFor16BitIndexedVerticesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareIndirectCopyFor32BitIndexedVerticesPipeline() {
  if (prepareIndirectCopyFor32BitIndexedVerticesPipeline == VK_NULL_HANDLE) {
    prepareIndirectCopyFor32BitIndexedVerticesPipeline = createInternalPipeline(
        device, pipelineLayout, getPrepareIndirectCopyFor32BitIndexedVerticesShaderModuleSource());
  }

  return prepareIndirectCopyFor32BitIndexedVerticesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPerformIndirectCopyPipeline() {
  if (performIndirectCopyPipeline == VK_NULL_HANDLE) {
    performIndirectCopyPipeline =
        createInternalPipeline(device, pipelineLayout, getPerformIndirectCopyShaderModuleSource());
  }

  return performIndirectCopyPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPatchShaderGroupHandlesInSBT() {
  if (patchShaderGroupHandlesInSBT == VK_NULL_HANDLE) {
    patchShaderGroupHandlesInSBT = createInternalPipeline(
        device, pipelineLayout, getPatchShaderGroupHandlesInSBTShaderModuleSource());
  }

  return patchShaderGroupHandlesInSBT;
}

CPipelineState::CShaderGroupHandlesManagement::~CShaderGroupHandlesManagement() {
  try {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    if (memoryBufferPair.second) {
      auto device = memoryBufferPair.second->deviceStateStore->deviceHandle;
      if (SD()._devicestates.find(device) != SD()._devicestates.end()) {
        auto buffer = memoryBufferPair.second->bufferHandle;
        drvVk.vkDestroyBuffer(device, buffer, nullptr);
      }
    }
    if (memoryBufferPair.first) {
      auto device = memoryBufferPair.first->deviceStateStore->deviceHandle;
      if (SD()._devicestates.find(device) != SD()._devicestates.end()) {
        auto memory = memoryBufferPair.first->deviceMemoryHandle;
        drvVk.vkFreeMemory(device, memory, nullptr);
      }
    }
  } catch (...) {
    topmost_exception_handler("CShaderGroupHandlesManagement::~CShaderGroupHandlesManagement()");
  }
}

} // namespace Vulkan
} // namespace gits
