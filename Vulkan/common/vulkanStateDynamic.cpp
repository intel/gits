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
  if (SD()._devicestates.find(device) == SD()._devicestates.end()) {
    return;
  }

  if (prepareDeviceAddressesForPatchingPipeline) {
    drvVk.vkDestroyPipeline(device, prepareDeviceAddressesForPatchingPipeline, nullptr);
  }
  if (patchDeviceAddressesPipeline) {
    drvVk.vkDestroyPipeline(device, patchDeviceAddressesPipeline, nullptr);
  }
  if (prepareIndirectCopyFor16BitIndexedVerticesPipeline) {
    drvVk.vkDestroyPipeline(device, prepareIndirectCopyFor16BitIndexedVerticesPipeline, nullptr);
  }
  if (prepareIndirectCopyFor32BitIndexedVerticesPipeline) {
    drvVk.vkDestroyPipeline(device, prepareIndirectCopyFor32BitIndexedVerticesPipeline, nullptr);
  }
  if (performIndirectCopyPipeline) {
    drvVk.vkDestroyPipeline(device, performIndirectCopyPipeline, nullptr);
  }
  if (patchShaderGroupHandlesInSBT) {
    drvVk.vkDestroyPipeline(device, patchShaderGroupHandlesInSBT, nullptr);
  }
  if (pipelineLayout) {
    drvVk.vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  }
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
        device, getLayout(), getPrepareDeviceAddressesForPatchingShaderModuleSource());
  }

  return prepareDeviceAddressesForPatchingPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPatchDeviceAddressesPipeline() {
  if (patchDeviceAddressesPipeline == VK_NULL_HANDLE) {
    patchDeviceAddressesPipeline =
        createInternalPipeline(device, getLayout(), getPatchDeviceAddressesShaderModuleSource());
  }

  return patchDeviceAddressesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareIndirectCopyFor16BitIndexedVerticesPipeline() {
  if (prepareIndirectCopyFor16BitIndexedVerticesPipeline == VK_NULL_HANDLE) {
    prepareIndirectCopyFor16BitIndexedVerticesPipeline = createInternalPipeline(
        device, getLayout(), getPrepareIndirectCopyFor16BitIndexedVerticesShaderModuleSource());
  }

  return prepareIndirectCopyFor16BitIndexedVerticesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::
    getPrepareIndirectCopyFor32BitIndexedVerticesPipeline() {
  if (prepareIndirectCopyFor32BitIndexedVerticesPipeline == VK_NULL_HANDLE) {
    prepareIndirectCopyFor32BitIndexedVerticesPipeline = createInternalPipeline(
        device, getLayout(), getPrepareIndirectCopyFor32BitIndexedVerticesShaderModuleSource());
  }

  return prepareIndirectCopyFor32BitIndexedVerticesPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPerformIndirectCopyPipeline() {
  if (performIndirectCopyPipeline == VK_NULL_HANDLE) {
    performIndirectCopyPipeline =
        createInternalPipeline(device, getLayout(), getPerformIndirectCopyShaderModuleSource());
  }

  return performIndirectCopyPipeline;
}

VkPipeline InternalPipelinesManager::InternalPipelines::getPatchShaderGroupHandlesInSBT() {
  if (patchShaderGroupHandlesInSBT == VK_NULL_HANDLE) {
    patchShaderGroupHandlesInSBT = createInternalPipeline(
        device, getLayout(), getPatchShaderGroupHandlesInSBTShaderModuleSource());
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

// Memory aliasing tracker

// Kudos to Piotr Horodecki
bool MemoryAliasingTracker::Range::operator()(Range const& lRange, Range const& rRange) const {
  return lRange.offset < rRange.offset;
}

MemoryAliasingTracker::RangeSetType::iterator MemoryAliasingTracker::GetRange(uint64_t offset) {
  // Get an iterator to the first offset greater than given offset
  auto iterator = MemoryRanges.upper_bound({offset, 0, {}});
  // Decrement to obtain an iterator to the range which includes given offset
  iterator--;
  return iterator;
}

void MemoryAliasingTracker::SplitRange(uint64_t offset) {
  auto existingRangeIterator = GetRange(offset);
  if (existingRangeIterator->offset < offset) {
    // Create two new ranges: left range and right range by cutting existing range at offset
    Range leftRange = {existingRangeIterator->offset, offset - existingRangeIterator->offset,
                       existingRangeIterator->resources};
    Range rightRange = {offset, existingRangeIterator->size - leftRange.size, leftRange.resources};
    assert(leftRange.size > 0);
    assert(rightRange.size > 0);
    // Remove existing range
    MemoryRanges.erase(existingRangeIterator);
    // Insert two new ranges
    MemoryRanges.insert(std::move(leftRange));
    MemoryRanges.insert(std::move(rightRange));
  }
}

void MemoryAliasingTracker::AddResource(uint64_t offset,
                                        uint64_t size,
                                        ResourceIdentifier const& resource) {
  // Split range if needed on the resource beginning.
  SplitRange(offset);

  // Split range if needed on the resource end.
  SplitRange(offset + size);

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.insert(resource);
  }
}

void MemoryAliasingTracker::RemoveResource(uint64_t offset,
                                           uint64_t size,
                                           ResourceIdentifier const& resource) {
  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.erase(resource);
  }

  for (auto current = beginIterator; current->offset < endIterator->offset;) {
    auto next = current;
    next++;

    if ((next->offset < endIterator->offset) && (current->resources == next->resources)) {
      Range newRange = {current->offset, current->size + next->size, current->resources};
      MemoryRanges.erase(current);
      MemoryRanges.erase(next);
      MemoryRanges.insert(std::move(newRange));

      beginIterator = GetRange(offset);
      endIterator = GetRange(offset + size);
      current = beginIterator;
    } else {
      current++;
    }
  }
}

std::set<MemoryAliasingTracker::ResourceIdentifier> MemoryAliasingTracker::
    GetAliasedResourcesForResource(uint64_t offset,
                                   uint64_t size,
                                   ResourceIdentifier const& resource) {
  std::set<ResourceIdentifier> aliasedResources;

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  for (auto i = beginIterator; i->offset < endIterator->offset; ++i) {
    aliasedResources.insert(i->resources.begin(), i->resources.end());
  }

  aliasedResources.erase(resource);

  return aliasedResources;
}

MemoryAliasingTracker::MemoryAliasingTracker(uint64_t size) {
  // Add whole device memory range.
  MemoryRanges.insert({0, size, {}});
  // Add "after whole device memory empty range" (fake) with offset == size of
  // vkDeviceMemory and size == 0. This allows to use SplitRange() method in
  // case of splitting range at the end of adding resource.
  MemoryRanges.insert({size, 0, {}});
}

void MemoryAliasingTracker::AddImage(uint64_t offset, uint64_t size, VkImage image) {
  AddResource(offset, size, {(uint64_t)image, ResourceType::IMAGE});
}

void MemoryAliasingTracker::AddBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  AddResource(offset, size, {(uint64_t)buffer, ResourceType::BUFFER});
}

void MemoryAliasingTracker::AddAccelerationStructure(
    uint64_t offset, uint64_t size, VkAccelerationStructureKHR accelerationStructure) {
  AddResource(offset, size,
              {(uint64_t)accelerationStructure, ResourceType::ACCELERATION_STRUCTURE});
}

void MemoryAliasingTracker::RemoveImage(uint64_t offset, uint64_t size, VkImage image) {
  RemoveResource(offset, size, {(uint64_t)image, ResourceType::IMAGE});
}

void MemoryAliasingTracker::RemoveBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  RemoveResource(offset, size, {(uint64_t)buffer, ResourceType::BUFFER});
}

void MemoryAliasingTracker::RemoveAccelerationStructure(
    uint64_t offset, uint64_t size, VkAccelerationStructureKHR accelerationStructure) {
  RemoveResource(offset, size,
                 {(uint64_t)accelerationStructure, ResourceType::ACCELERATION_STRUCTURE});
}

std::set<MemoryAliasingTracker::ResourceIdentifier> MemoryAliasingTracker::
    GetAliasedResourcesForImage(uint64_t offset, uint64_t size, VkImage image) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)image, ResourceType::IMAGE});
}

std::set<MemoryAliasingTracker::ResourceIdentifier> MemoryAliasingTracker::
    GetAliasedResourcesForBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)buffer, ResourceType::BUFFER});
}

std::set<MemoryAliasingTracker::ResourceIdentifier> MemoryAliasingTracker::
    GetAliasedResourcesForAccelerationStructure(uint64_t offset,
                                                uint64_t size,
                                                VkAccelerationStructureKHR accelerationStructure,
                                                VkBuffer buffer) {
  auto aliasedResources = GetAliasedResourcesForResource(
      offset, size, {(uint64_t)accelerationStructure, ResourceType::ACCELERATION_STRUCTURE});

  std::set<MemoryAliasingTracker::ResourceIdentifier>::iterator it = aliasedResources.begin();
  while (it != aliasedResources.end()) {
    if ((it->second == ResourceType::BUFFER) && (it->first == (uint64_t)buffer)) {
      it = aliasedResources.erase(it);
    } else {
      ++it;
    }
  }

  return aliasedResources;
}

} // namespace Vulkan
} // namespace gits
