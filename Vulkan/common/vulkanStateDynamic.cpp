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

#include "vulkanStateDynamic.h"

namespace gits {
namespace Vulkan {

// Instantiation of static members
std::unordered_map<VkDeviceAddress, std::pair<VkDeviceAddress, std::shared_ptr<CBufferState>>>
    CBufferState::deviceAddressesMap;
std::unordered_map<VkBuffer, std::shared_ptr<CBufferState>>
    CBufferState::shaderDeviceAddressBuffers;

uint64_t CInternalResources::COffscreenAppsSupport::uniqueHandleCounter = 1;

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
} // namespace Vulkan
} // namespace gits
