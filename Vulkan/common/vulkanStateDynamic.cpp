// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
      layout(VK_NULL_HANDLE),
      prepareDeviceAddressesForPatching(VK_NULL_HANDLE),
      patchDeviceAddressesPipeline(VK_NULL_HANDLE) {
  layout = createInternalPipelineLayout(device);
}

VkPipelineLayout InternalPipelinesManager::InternalPipelines::getLayout() {
  return layout;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareDeviceAddressesForPatchingPipeline() {
  if (prepareDeviceAddressesForPatching == VK_NULL_HANDLE) {
    prepareDeviceAddressesForPatching = createInternalPipeline(
        device, layout, getPrepareDeviceAddressesForPatchingShaderModuleSource());
  }

  return prepareDeviceAddressesForPatching;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPatchDeviceAddressesPipeline() {
  if (patchDeviceAddressesPipeline == VK_NULL_HANDLE) {
    patchDeviceAddressesPipeline =
        createInternalPipeline(device, layout, getPatchDeviceAddressesShaderModuleSource());
  }

  return patchDeviceAddressesPipeline;
}

} // namespace Vulkan
} // namespace gits
