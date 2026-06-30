// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureLayer.h"
#include "playerManager.h"
#include "commandSerializersCustom.h"
#include "commandSerializersAuto.h"
#include "commandCodersAuto.h"
#include "log.h"

namespace gits {
namespace vulkan {

SubcaptureLayer::SubcaptureLayer(PlayerManager& playerManager, const std::string& framesStr)
    : Layer("Subcapture"),
      m_SubcaptureRange(framesStr),
      m_GpuReadbackHelper(playerManager),
      m_StateTracking(m_Recorder),
      m_SyncState(m_StateTracking),
      m_ImageLayout(m_StateTracking),
      m_CommandBufferLifecycle(m_StateTracking),
      m_MappedMemory(m_StateTracking) {
  m_StateTracking.SetGpuReadbackHelper(&m_GpuReadbackHelper);
}

// ---- Frame boundary ------------------------------------------------------

void SubcaptureLayer::TriggerRestoreState() {
  m_StateTracking.RestoreState();
}

void SubcaptureLayer::Post(vkQueuePresentKHRCommand& command) {
  // vkQueuePresentKHR consumes (unsignals) all binary semaphores listed in
  // pWaitSemaphores.  Clear isSignaled so we don't incorrectly signal them
  // during state restore.
  if (command.m_pPresentInfo.Value && !command.m_pPresentInfo.HandleKeys.empty()) {
    m_SyncState.OnQueuePresent(*command.m_pPresentInfo.Value, command.m_pPresentInfo.HandleKeys);
  }

  // Remove presented images from their swapchain's acquiredImages set.
  // HandleKeys layout: [waitSemaphoreKeys...][swapchainKeys...]
  if (command.m_pPresentInfo.Value) {
    const VkPresentInfoKHR& pi = *command.m_pPresentInfo.Value;
    const uint32_t swapchainKeyStart = pi.waitSemaphoreCount;
    for (uint32_t i = 0; i < pi.swapchainCount && pi.pImageIndices; ++i) {
      const uint32_t keyIdx = swapchainKeyStart + i;
      if (keyIdx >= command.m_pPresentInfo.HandleKeys.size()) {
        break;
      }
      const uint64_t swapchainKey = command.m_pPresentInfo.HandleKeys[keyIdx];
      auto* sc = m_StateTracking.GetState<SwapchainState>(swapchainKey);
      if (sc) {
        sc->acquiredImages.erase(pi.pImageIndices[i]);
      }
    }
  }

  if (!m_SubcaptureRange.IsEnabled()) {
    return;
  }

  // Fire state restore exactly once, before the first recorded frame.
  if (m_SubcaptureRange.IsRestorePoint()) {
    TriggerRestoreState();
    // After RestoreState the recorder stream is open; the very next FrameEnd
    // will put us inside the range so recording begins below.
  }

  m_SubcaptureRange.FrameEnd();

  if (m_SubcaptureRange.InRange()) {
    // Start (or continue) recording.  Emit a FrameEndCommand so the player of
    // the subcapture can detect frame boundaries.
    if (!m_Recording) {
      m_Recording = true;
      LOG_INFO << "Vulkan2 subcapture: entering recording range";
    }
    FrameEndCommand fec;
    m_Recorder.Record(FrameEndSerializer(fec));
  } else if (m_Recording) {
    // We just left the range — close the stream.
    m_Recording = false;
    m_Recorder.FinishRecording();
    LOG_INFO << "Vulkan2 subcapture: recording range complete, stream closed";
  }
}

// ---- Instance / device ---------------------------------------------------

void SubcaptureLayer::Post(vkCreateInstanceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<InstanceState>();
  state->key = command.m_pInstance.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyInstanceCommand& command) {
  // vkDestroyInstance implicitly invalidates every VkPhysicalDevice obtained
  // from this instance.  Drop their tracked state so a subsequent
  // vkCreateInstance + vkEnumeratePhysicalDevices on the same hardware (which
  // returns the same VkPhysicalDevice handle, and therefore the same recorder
  // key via TryGetKey) re-tracks them under the new instance instead of
  // leaving stale state pointing at the destroyed instance.
  const uint64_t instanceKey = command.m_instance.Key;
  std::vector<uint64_t> orphanedPDs;
  for (const auto& [key, statePtr] : m_StateTracking.GetStates()) {
    if (statePtr->creationCommandId == CommandId::ID_VKENUMERATEPHYSICALDEVICES &&
        statePtr->parentKey == instanceKey) {
      orphanedPDs.push_back(key);
    }
  }
  for (uint64_t pdKey : orphanedPDs) {
    m_StateTracking.RemoveState(pdKey);
  }
  m_StateTracking.RemoveState(instanceKey);
}

void SubcaptureLayer::Post(vkEnumeratePhysicalDevicesCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pPhysicalDevices.Value) {
    return;
  }
  // We do NOT store the recorded vkEnumeratePhysicalDevices blob.  Two reasons:
  //   1. Storing the same encoded command on every PhysicalDeviceState would
  //      cause the enumerate call to be re-emitted N times during state restore
  //      (once per physical device).
  //   2. The encoded blob captures the original VkInstance key, so if the app
  //      destroys and recreates the instance (probe-then-real pattern, common
  //      for extension detection) the blob references a key the player never
  //      registered, and the stale state survives because TryGetKey reuses the
  //      same VkPhysicalDevice handle.
  // Instead, we register a typed-only PhysicalDeviceState (no creation blob)
  // and StateTrackingService::RestorePhysicalDevice lazily synthesizes ONE
  // vkEnumeratePhysicalDevices per live parent instance during restore.
  for (uint32_t i = 0; i < command.m_pPhysicalDevices.Size; ++i) {
    uint64_t key = command.m_pPhysicalDevices.Keys[i];
    auto state = std::make_unique<PhysicalDeviceState>();
    state->key = key;
    state->parentKey = command.m_instance.Key;
    state->creationCommandId = command.GetId();
    if (m_StateTracking.HasState(key)) {
      // Re-enumeration on a (possibly new) instance: refresh parentKey so the
      // synthesis attaches the device to whichever instance is current.
      m_StateTracking.RemoveState(key);
    }
    m_StateTracking.StoreState(std::move(state));
  }
}

// Physical devices obtained via vkEnumeratePhysicalDeviceGroups[KHR] are embedded
// inside VkPhysicalDeviceGroupProperties::physicalDevices[].  The HandleKeys layout
// mirrors UpdateOutputHandle: one key per physical device, in group/slot order.
// Without these handlers games that use the device-group enumeration path would have
// no PhysicalDeviceState entries, causing state restore to fail when vkCreateDevice
// references a physical device key that was never tracked.
void SubcaptureLayer::Post(vkEnumeratePhysicalDeviceGroupsCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pPhysicalDeviceGroupProperties.Value) {
    return;
  }
  uint32_t keyIdx = 0;
  for (uint32_t i = 0; i < command.m_pPhysicalDeviceGroupProperties.Size; ++i) {
    const auto& group = command.m_pPhysicalDeviceGroupProperties.Value[i];
    for (uint32_t j = 0; j < group.physicalDeviceCount; ++j) {
      if (keyIdx >= command.m_pPhysicalDeviceGroupProperties.HandleKeys.size()) {
        // Bail out silently used to hide a real recorder/codegen bug: a group
        // declares more devices than HandleKeys provides, so subsequent groups
        // will be skipped entirely.  Surface it so it can be diagnosed.
        LOG_WARNING << "Vulkan2 subcapture: vkEnumeratePhysicalDeviceGroups HandleKeys size="
                    << command.m_pPhysicalDeviceGroupProperties.HandleKeys.size()
                    << " is smaller than the cumulative physicalDeviceCount; "
                    << "remaining devices in group " << i
                    << " and later groups will not be tracked";
        return;
      }
      const uint64_t key = command.m_pPhysicalDeviceGroupProperties.HandleKeys[keyIdx++];
      if (key && !m_StateTracking.HasState(key)) {
        auto state = std::make_unique<PhysicalDeviceState>();
        state->key = key;
        state->parentKey = command.m_instance.Key;
        StoreState(std::move(state), command);
      }
    }
  }
}

void SubcaptureLayer::Post(vkEnumeratePhysicalDeviceGroupsKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pPhysicalDeviceGroupProperties.Value) {
    return;
  }
  uint32_t keyIdx = 0;
  for (uint32_t i = 0; i < command.m_pPhysicalDeviceGroupProperties.Size; ++i) {
    const auto& group = command.m_pPhysicalDeviceGroupProperties.Value[i];
    for (uint32_t j = 0; j < group.physicalDeviceCount; ++j) {
      if (keyIdx >= command.m_pPhysicalDeviceGroupProperties.HandleKeys.size()) {
        // See vkEnumeratePhysicalDeviceGroups for rationale.
        LOG_WARNING << "Vulkan2 subcapture: vkEnumeratePhysicalDeviceGroupsKHR HandleKeys size="
                    << command.m_pPhysicalDeviceGroupProperties.HandleKeys.size()
                    << " is smaller than the cumulative physicalDeviceCount; "
                    << "remaining devices in group " << i
                    << " and later groups will not be tracked";
        return;
      }
      const uint64_t key = command.m_pPhysicalDeviceGroupProperties.HandleKeys[keyIdx++];
      if (key && !m_StateTracking.HasState(key)) {
        auto state = std::make_unique<PhysicalDeviceState>();
        state->key = key;
        state->parentKey = command.m_instance.Key;
        StoreState(std::move(state), command);
      }
    }
  }
}

void SubcaptureLayer::Post(vkCreateDeviceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DeviceState>();
  state->key = command.m_pDevice.Key;
  state->parentKey = command.m_physicalDevice.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDeviceCommand& command) {
  m_StateTracking.RemoveState(command.m_device.Key);
}

// ---- Memory --------------------------------------------------------------

void SubcaptureLayer::Post(vkAllocateMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DeviceMemoryState>();
  state->key = command.m_pMemory.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pAllocateInfo.Value) {
    state->allocationSize = command.m_pAllocateInfo.Value->allocationSize;
    state->memoryTypeIndex = command.m_pAllocateInfo.Value->memoryTypeIndex;
  }
  // VkMemoryAllocateInfo itself has no handle members, but its pNext chain
  // can carry VkMemoryDedicatedAllocateInfo (image / buffer) and other
  // pNext extensions whose handles are referenced by the captured allocate
  // command's HandleKeys.  The top-level RestoreState loop iterates
  // m_States in unordered map order; without these dependency keys it can
  // emit vkAllocateMemory before the dedicated image / buffer has been
  // restored, and the subcapture player then asserts inside
  // ResolvePNextHandleKeys -> HandleMapService::GetHandle on a key that
  // has not yet been registered.  Promote every non-zero HandleKey to a
  // dependency so RestoreOne restores them first.
  for (uint64_t dep : command.m_pAllocateInfo.HandleKeys) {
    if (dep) {
      state->dependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkFreeMemoryCommand& command) {
  m_StateTracking.RemoveState(command.m_memory.Key);
}

void SubcaptureLayer::Post(vkMapMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_MappedMemory.OnMapMemory(command.m_memory.Key, command.m_offset.Value, command.m_size.Value,
                             command.m_flags.Value);
}

void SubcaptureLayer::Post(vkUnmapMemoryCommand& command) {
  m_MappedMemory.OnUnmapMemory(command.m_memory.Key);
}

// When the player replays a MappedDataMetaCommand it has already written the
// bytes into the mapped host pointer.  We snapshot those bytes here so that
// state restore can re-emit the same command and the subcapture player starts
// with the correct initial memory contents.
void SubcaptureLayer::Post(MappedDataMetaCommand& command) {
  m_MappedMemory.OnMappedData(command);
}

// ---- In-flight command buffer tracking -----------------------------------

void SubcaptureLayer::Post(vkBeginCommandBufferCommand& command) {
  // vkBeginCommandBuffer implicitly resets the CB if it was executable.
  // Clear any stale render-pass tracking for this CB before starting fresh.
  m_ImageLayout.OnResetCommandBuffer(command.m_commandBuffer.Key);
  m_CommandBufferLifecycle.OnBegin(command);
  // For secondary command buffers pBeginInfo->pInheritanceInfo carries
  // renderPass and framebuffer handles encoded into the stored
  // beginCommandBuffer bytes.  When RestoreCommandBuffers later emits those
  // bytes the second player calls GetHandle on every key in HandleKeys -- if
  // either object was not restored the player will assert.  Track them as
  // dependencies (isRecording is true after OnBegin) so RestoreOne skips
  // this CB when a dependency cannot be restored.
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBeginInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkEndCommandBufferCommand& command) {
  m_CommandBufferLifecycle.OnEnd(command);
}

void SubcaptureLayer::Post(vkResetCommandBufferCommand& command) {
  m_ImageLayout.OnResetCommandBuffer(command.m_commandBuffer.Key);
  m_CommandBufferLifecycle.OnReset(command.m_commandBuffer.Key);
}

// ---- Synchronization -----------------------------------------------------

void SubcaptureLayer::Post(vkCreateFenceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<FenceState>();
  state->key = command.m_pFence.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyFenceCommand& command) {
  m_StateTracking.RemoveState(command.m_fence.Key);
}

void SubcaptureLayer::Post(vkQueueSubmitCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnQueueSubmit(command.m_pSubmits.Value, command.m_pSubmits.Size,
                            command.m_pSubmits.HandleKeys, command.m_fence.Key);
}

void SubcaptureLayer::Post(vkQueueSubmit2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                             command.m_pSubmits.HandleKeys, command.m_fence.Key);
}

void SubcaptureLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                             command.m_pSubmits.HandleKeys, command.m_fence.Key);
}

void SubcaptureLayer::Post(vkResetFencesCommand& command) {
  // On VK_ERROR_OUT_OF_*_MEMORY the spec leaves fence states unchanged; do
  // not advance our shadow state in that case or we'll mark fences as reset
  // that the driver actually still considers signalled.
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnResetFences(command.m_pFences.Keys);
}

void SubcaptureLayer::Post(vkGetDeviceQueueCommand& command) {
  if (!command.m_pQueue.Key) {
    return;
  }
  auto state = std::make_unique<QueueState>();
  state->key = command.m_pQueue.Key;
  state->parentKey = command.m_device.Key;
  state->queueFamilyIndex = command.m_queueFamilyIndex.Value;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkGetDeviceQueue2Command& command) {
  if (!command.m_pQueue.Key) {
    return;
  }
  auto state = std::make_unique<QueueState>();
  state->key = command.m_pQueue.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pQueueInfo.Value) {
    state->queueFamilyIndex = command.m_pQueueInfo.Value->queueFamilyIndex;
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateSemaphoreCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SemaphoreState>();
  state->key = command.m_pSemaphore.Key;
  state->parentKey = command.m_device.Key;
  // Detect timeline semaphores via pNext chain.
  if (command.m_pCreateInfo.Value) {
    const auto* pNext = static_cast<const VkBaseInStructure*>(command.m_pCreateInfo.Value->pNext);
    while (pNext) {
      if (pNext->sType == VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO) {
        const auto* typeInfo = reinterpret_cast<const VkSemaphoreTypeCreateInfo*>(pNext);
        if (typeInfo->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
          state->isBinary = false;
        }
        break;
      }
      pNext = pNext->pNext;
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySemaphoreCommand& command) {
  m_StateTracking.RemoveState(command.m_semaphore.Key);
}

void SubcaptureLayer::Post(vkCreateEventCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<EventState>();
  state->key = command.m_pEvent.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyEventCommand& command) {
  m_StateTracking.RemoveState(command.m_event.Key);
}

void SubcaptureLayer::Post(vkSetEventCommand& command) {
  // Host-side signal.  The spec leaves the event unchanged on failure.
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  if (auto* ev = m_StateTracking.GetState<EventState>(command.m_event.Key)) {
    ev->isSignaled = true;
  }
}

void SubcaptureLayer::Post(vkResetEventCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  if (auto* ev = m_StateTracking.GetState<EventState>(command.m_event.Key)) {
    ev->isSignaled = false;
  }
}

// ---- Buffers / images ----------------------------------------------------

void SubcaptureLayer::Post(vkCreateBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<BufferState>();
  state->key = command.m_pBuffer.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    state->bufferSize = command.m_pCreateInfo.Value->size;
    state->usageFlags = command.m_pCreateInfo.Value->usage;
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyBufferCommand& command) {
  m_StateTracking.RemoveState(command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkBindBufferMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto* state = m_StateTracking.GetState<BufferState>(command.m_buffer.Key);
  if (state) {
    state->boundMemoryKey = command.m_memory.Key;
    state->memoryOffset = command.m_memoryOffset.Value;
  }
}

void SubcaptureLayer::Post(vkBindBufferMemory2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // HandleKeys layout per element: [bufferKey, memoryKey]
  const auto& keys = command.m_pBindInfos.HandleKeys;
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const uint64_t bufKey = (keys.size() > i * 2) ? keys[i * 2] : 0;
    const uint64_t memKey = (keys.size() > i * 2 + 1) ? keys[i * 2 + 1] : 0;
    auto* state = m_StateTracking.GetState<BufferState>(bufKey);
    if (state && memKey) {
      state->boundMemoryKey = memKey;
      state->memoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
    }
  }
}

void SubcaptureLayer::Post(vkBindBufferMemory2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  const auto& keys = command.m_pBindInfos.HandleKeys;
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const uint64_t bufKey = (keys.size() > i * 2) ? keys[i * 2] : 0;
    const uint64_t memKey = (keys.size() > i * 2 + 1) ? keys[i * 2 + 1] : 0;
    auto* state = m_StateTracking.GetState<BufferState>(bufKey);
    if (state && memKey) {
      state->boundMemoryKey = memKey;
      state->memoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
    }
  }
}

void SubcaptureLayer::Post(vkCreateImageCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<ImageState>();
  state->key = command.m_pImage.Key;
  state->parentKey = command.m_device.Key;
  const VkImageCreateInfo& ci = *command.m_pCreateInfo.Value;
  state->format = ci.format;
  state->currentLayout = ci.initialLayout;
  state->extent = ci.extent;
  state->mipLevels = ci.mipLevels;
  state->arrayLayers = ci.arrayLayers;
  state->samples = ci.samples;
  state->usageFlags = ci.usage;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyImageCommand& command) {
  m_StateTracking.RemoveState(command.m_image.Key);
}

void SubcaptureLayer::Post(vkBindImageMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto* state = m_StateTracking.GetState<ImageState>(command.m_image.Key);
  if (state) {
    state->boundMemoryKey = command.m_memory.Key;
    state->memoryOffset = command.m_memoryOffset.Value;
  }
}

void SubcaptureLayer::Post(vkBindImageMemory2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // HandleKeys layout per element: [imageKey, memoryKey]
  const auto& keys = command.m_pBindInfos.HandleKeys;
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const uint64_t imgKey = (keys.size() > i * 2) ? keys[i * 2] : 0;
    const uint64_t memKey = (keys.size() > i * 2 + 1) ? keys[i * 2 + 1] : 0;
    auto* state = m_StateTracking.GetState<ImageState>(imgKey);
    if (state && memKey) {
      state->boundMemoryKey = memKey;
      state->memoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
    }
  }
}

void SubcaptureLayer::Post(vkBindImageMemory2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  const auto& keys = command.m_pBindInfos.HandleKeys;
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const uint64_t imgKey = (keys.size() > i * 2) ? keys[i * 2] : 0;
    const uint64_t memKey = (keys.size() > i * 2 + 1) ? keys[i * 2 + 1] : 0;
    auto* state = m_StateTracking.GetState<ImageState>(imgKey);
    if (state && memKey) {
      state->boundMemoryKey = memKey;
      state->memoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
    }
  }
}

void SubcaptureLayer::Post(vkCreateBufferViewCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<BufferViewState>();
  state->key = command.m_pView.Key;
  state->parentKey = command.m_device.Key;
  // VkBufferViewCreateInfo::buffer is the first handle in HandleKeys.
  if (!command.m_pCreateInfo.HandleKeys.empty()) {
    state->dependencyKeys.push_back(command.m_pCreateInfo.HandleKeys[0]);
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyBufferViewCommand& command) {
  m_StateTracking.RemoveState(command.m_bufferView.Key);
}

void SubcaptureLayer::Post(vkCreateImageViewCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<ImageViewState>();
  state->key = command.m_pView.Key;
  state->parentKey = command.m_device.Key;
  // VkImageViewCreateInfo::image is the first handle in HandleKeys.
  // Store in imageKey for other consumers, and in dependencyKeys so RestoreOne
  // ensures the image exists before the view is created.
  state->imageKey =
      command.m_pCreateInfo.HandleKeys.empty() ? 0 : command.m_pCreateInfo.HandleKeys[0];
  if (state->imageKey) {
    state->dependencyKeys.push_back(state->imageKey);
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyImageViewCommand& command) {
  m_StateTracking.RemoveState(command.m_imageView.Key);
}

// ---- Render pass / framebuffer -------------------------------------------

void SubcaptureLayer::Post(vkCreateRenderPassCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<RenderPassState>();
  state->key = command.m_pRenderPass.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->attachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->attachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateRenderPass2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<RenderPassState>();
  state->key = command.m_pRenderPass.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->attachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->attachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateRenderPass2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<RenderPassState>();
  state->key = command.m_pRenderPass.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->attachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->attachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyRenderPassCommand& command) {
  // Same rationale as vkDestroyShaderModule / vkDestroyPipelineLayout: VkRenderPass may be
  // destroyed after VkGraphicsPipelineCreateInfo referenced it (Vulkan permits this once pipelines
  // are built). RemoveState erased the key while pipelines still listed it as a dependency, so
  // RestoreOne aborted vkCreateGraphicsPipelines with "dependency ... no longer tracked". Keep the
  // encoded vkCreateRenderPass* blob and mark destroyed; RestoreOne re-emits create before pipelines.
  // Unlike shader modules we do not schedule vkDestroyRenderPass after restore: vkCmdBeginRenderPass
  // blobs still reference this handle key for the rest of replay.
  auto* state = m_StateTracking.GetState(command.m_renderPass.Key);
  if (state) {
    state->destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateFramebufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<FramebufferState>();
  state->key = command.m_pFramebuffer.Key;
  state->parentKey = command.m_device.Key;
  // VkFramebufferCreateInfo HandleKeys: render pass followed by attachment image views.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->dependencyKeys.push_back(dep);
    }
  }
  // Store attachment image view keys separately (HandleKeys[0] = renderPass,
  // HandleKeys[1..] = pAttachments[i] image view keys in order).
  // Used by ImageLayoutService to resolve attachment index → image key.
  const auto& keys = command.m_pCreateInfo.HandleKeys;
  if (keys.size() > 1) {
    state->attachmentImageViewKeys.assign(keys.begin() + 1, keys.end());
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyFramebufferCommand& command) {
  m_StateTracking.RemoveState(command.m_framebuffer.Key);
}

// ---- Pipelines -----------------------------------------------------------

void SubcaptureLayer::Post(vkCreatePipelineCacheCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<PipelineCacheState>();
  state->key = command.m_pPipelineCache.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyPipelineCacheCommand& command) {
  m_StateTracking.RemoveState(command.m_pipelineCache.Key);
}

void SubcaptureLayer::Post(vkCreatePipelineLayoutCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<PipelineLayoutState>();
  state->key = command.m_pPipelineLayout.Key;
  state->parentKey = command.m_device.Key;
  // Descriptor set layout handles referenced by pSetLayouts.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->dependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyPipelineLayoutCommand& command) {
  // Mark as destroyed but keep the state so it can be transiently re-created
  // as a pipeline dependency during state restore (same rationale as shader
  // modules above).
  auto* state = m_StateTracking.GetState(command.m_pipelineLayout.Key);
  if (state) {
    state->destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateShaderModuleCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<ShaderModuleState>();
  state->key = command.m_pShaderModule.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyShaderModuleCommand& command) {
  // Mark as destroyed but keep the state so it can be transiently re-created
  // as a pipeline dependency during state restore.  Vulkan allows destroying a
  // VkShaderModule after pipeline creation; the pipeline holds the compiled
  // shader internally.  We need the creation data to re-emit the module just
  // before any pipeline that references it, and destroy it afterwards.
  auto* state = m_StateTracking.GetState(command.m_shaderModule.Key);
  if (state) {
    state->destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateGraphicsPipelinesCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  // Collect all handle dependencies (shader modules, pipeline layout, render pass,
  // base pipeline) from the encoded HandleKeys for all create infos in the batch.
  // NOTE: pipelineCache is deliberately NOT treated as a dependency.  It is a pure
  // optimization hint that engines frequently destroy right after building their
  // pipelines; as a hard dependency a destroyed (and thus removed) cache would make
  // every pipeline built from it unrestorable and crash later at vkCmdBindPipeline.
  // A live cache is still restored on its own in RestoreState's first pass, and
  // EmitCreationCommand nulls the cache handle when it is no longer live -- mirroring
  // legacy RestorePipelines, which builds restore pipelines against a temporary cache.
  std::vector<uint64_t> batchDeps;
  for (uint64_t dep : command.m_pCreateInfos.HandleKeys) {
    if (dep) {
      batchDeps.push_back(dep);
    }
  }
  // Collect all output pipeline keys for this batch.  batchPipelineKeys is
  // stored on every sibling PipelineState so that RestoreOne can mark all
  // siblings as restored after the first one emits the shared batch command,
  // preventing N redundant full-batch emissions for a batch of N pipelines.
  std::vector<uint64_t> batchKeys;
  batchKeys.reserve(command.m_createInfoCount.Value);
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    batchKeys.push_back(command.m_pPipelines.Keys[i]);
  }
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto state = std::make_unique<PipelineState>();
    state->key = command.m_pPipelines.Keys[i];
    state->parentKey = command.m_device.Key;
    state->dependencyKeys = batchDeps;
    state->batchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkCreateComputePipelinesCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // Collect handle dependencies (shader module, pipeline layout, base pipeline)
  // so RestoreOne restores them before the pipeline. pipelineCache is intentionally
  // NOT a dependency (see vkCreateGraphicsPipelines): it is an optional hint that may
  // be destroyed before the cut; EmitCreationCommand nulls it when no longer live.
  std::vector<uint64_t> batchDeps;
  for (uint64_t dep : command.m_pCreateInfos.HandleKeys) {
    if (dep) {
      batchDeps.push_back(dep);
    }
  }
  std::vector<uint64_t> batchKeys;
  batchKeys.reserve(command.m_createInfoCount.Value);
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    batchKeys.push_back(command.m_pPipelines.Keys[i]);
  }
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto state = std::make_unique<PipelineState>();
    state->key = command.m_pPipelines.Keys[i];
    state->parentKey = command.m_device.Key;
    state->dependencyKeys = batchDeps;
    state->batchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkCreateRayTracingPipelinesKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // pipelineCache is intentionally NOT a dependency (see vkCreateGraphicsPipelines):
  // an optional hint that may be destroyed before the cut; EmitCreationCommand nulls
  // it when no longer live.
  std::vector<uint64_t> batchDeps;
  for (uint64_t dep : command.m_pCreateInfos.HandleKeys) {
    if (dep) {
      batchDeps.push_back(dep);
    }
  }
  std::vector<uint64_t> batchKeys;
  batchKeys.reserve(command.m_createInfoCount.Value);
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    batchKeys.push_back(command.m_pPipelines.Keys[i]);
  }
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto state = std::make_unique<PipelineState>();
    state->key = command.m_pPipelines.Keys[i];
    state->parentKey = command.m_device.Key;
    state->dependencyKeys = batchDeps;
    state->batchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkCreateRayTracingPipelinesNVCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // pipelineCache is intentionally NOT a dependency (see vkCreateGraphicsPipelines):
  // an optional hint that may be destroyed before the cut; EmitCreationCommand nulls
  // it when no longer live.
  std::vector<uint64_t> batchDeps;
  for (uint64_t dep : command.m_pCreateInfos.HandleKeys) {
    if (dep) {
      batchDeps.push_back(dep);
    }
  }
  std::vector<uint64_t> batchKeys;
  batchKeys.reserve(command.m_createInfoCount.Value);
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    batchKeys.push_back(command.m_pPipelines.Keys[i]);
  }
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto state = std::make_unique<PipelineState>();
    state->key = command.m_pPipelines.Keys[i];
    state->parentKey = command.m_device.Key;
    state->dependencyKeys = batchDeps;
    state->batchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkDestroyPipelineCommand& command) {
  // Same pattern as vkDestroyShaderModule: VkPipeline may still appear in
  // encoded vkCmdBindPipeline blobs after the app destroys it.  Removing state
  // made HasState(pipelineKey) false so RestoreCommandBuffers could not recreate
  // the pipeline before replaying those blobs.  Keep creationCommandBuffer and
  // mark destroyed so RestoreOne can transiently recreate the VkPipeline; we
  // do not emit vkDestroyPipeline after restore (pipelines can stay referenced
  // by executable CBs submitted later, unlike shader modules post-create).
  auto* state = m_StateTracking.GetState(command.m_pipeline.Key);
  if (state) {
    state->destroyed = true;
  }
}

// ---- Descriptors ---------------------------------------------------------

void SubcaptureLayer::Post(vkCreateDescriptorSetLayoutCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorSetLayoutState>();
  state->key = command.m_pSetLayout.Key;
  state->parentKey = command.m_device.Key;
  // Immutable sampler handles referenced in pBindings.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->dependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDescriptorSetLayoutCommand& command) {
  // Mark as destroyed but keep the state.  Descriptor set layouts may be
  // destroyed after vkCreatePipelineLayout (Vulkan allows this).  During state
  // restore we must re-create the layout before re-creating any pipeline layout
  // that references it, so the state must remain available here.
  auto* state = m_StateTracking.GetState(command.m_descriptorSetLayout.Key);
  if (state) {
    state->destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateDescriptorPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorPoolState>();
  state->key = command.m_pDescriptorPool.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::RemoveDescriptorSetsByPool(uint64_t poolKey) {
  std::vector<uint64_t> setKeys;
  for (const auto& [key, statePtr] : m_StateTracking.GetStates()) {
    if (statePtr->creationCommandId == CommandId::ID_VKALLOCATEDESCRIPTORSETS) {
      auto* ds = static_cast<DescriptorSetState*>(statePtr.get());
      if (ds->poolKey == poolKey) {
        setKeys.push_back(key);
      }
    }
  }
  for (uint64_t setKey : setKeys) {
    m_StateTracking.RemoveState(setKey);
    m_StateTracking.GetDescriptorSetUpdateService().RemoveDescriptorSet(setKey);
  }
}

void SubcaptureLayer::Post(vkDestroyDescriptorPoolCommand& command) {
  RemoveDescriptorSetsByPool(command.m_descriptorPool.Key);
  m_StateTracking.RemoveState(command.m_descriptorPool.Key);
}

void SubcaptureLayer::Post(vkResetDescriptorPoolCommand& command) {
  RemoveDescriptorSetsByPool(command.m_descriptorPool.Key);
}

void SubcaptureLayer::Post(vkAllocateDescriptorSetsCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pAllocateInfo.Value) {
    return;
  }

  const uint64_t poolKey =
      command.m_pAllocateInfo.HandleKeys.empty() ? 0 : command.m_pAllocateInfo.HandleKeys[0];

  // Build a single-set VkDescriptorSetAllocateInfo so each stored set state
  // carries an allocation command sized for ONE descriptor set.  Storing the
  // original batch command on every DescriptorSetState would cause RestoreOne
  // to re-emit the N-set vkAllocateDescriptorSets once per set (N times), and
  // every emission consumes pool capacity - exhausting the pool, leaving
  // later sets unallocated, and crashing subsequent vkUpdateDescriptorSets /
  // vkCmdBindDescriptorSets in the recording range when their dstSet key
  // never appears in the player's HandleMap.  Mirrors the per-CB split in
  // CommandBufferLifecycleService::OnAllocate.
  VkDescriptorSetAllocateInfo singleAllocInfo = *command.m_pAllocateInfo.Value;
  singleAllocInfo.descriptorSetCount = 1;

  for (uint32_t i = 0; i < command.m_pDescriptorSets.Size; ++i) {
    auto state = std::make_unique<DescriptorSetState>();
    state->key = command.m_pDescriptorSets.Keys[i];
    state->parentKey = command.m_device.Key;
    state->poolKey = poolKey;

    // HandleKeys layout in the original command: [0]=pool, [1+i]=pSetLayouts[i].
    // Add the layout as a dependency so RestoreOne restores it before emitting
    // vkAllocateDescriptorSets (which references the layout handle).
    const uint32_t layoutIdx = 1 + i;
    uint64_t layoutKey = 0;
    if (layoutIdx < command.m_pAllocateInfo.HandleKeys.size()) {
      layoutKey = command.m_pAllocateInfo.HandleKeys[layoutIdx];
      if (layoutKey) {
        state->dependencyKeys.push_back(layoutKey);
      }
    }

    // Synthetic single-set allocation command for this set only.  pSetLayouts
    // points into the original array slot; Encode reads pSetLayouts[0..count)
    // so we slice by aiming pSetLayouts at element i with descriptorSetCount=1.
    singleAllocInfo.pSetLayouts = command.m_pAllocateInfo.Value->pSetLayouts + i;

    vkAllocateDescriptorSetsCommand singleCmd;
    singleCmd.m_device = command.m_device;
    singleCmd.m_pAllocateInfo.Value = &singleAllocInfo;
    singleCmd.m_pAllocateInfo.HandleKeys = {poolKey, layoutKey};
    // Value must be non-null (HandleArrayOutputArgument encodes a null-flag
    // when null).  Point at the i-th slot of the original output array; the
    // pointer content does not flow through, only Keys/Size do.
    singleCmd.m_pDescriptorSets.Value = command.m_pDescriptorSets.Value + i;
    singleCmd.m_pDescriptorSets.Size = 1;
    singleCmd.m_pDescriptorSets.Keys = {command.m_pDescriptorSets.Keys[i]};
    singleCmd.m_Return.Value = VK_SUCCESS;

    StoreState(std::move(state), singleCmd);
  }
}

void SubcaptureLayer::Post(vkFreeDescriptorSetsCommand& command) {
  // vkFreeDescriptorSets returns VkResult; on failure (VK_ERROR_OUT_OF_*) the
  // descriptor sets remain valid in the pool, so do NOT drop our state for
  // them or a subsequent vkUpdateDescriptorSets / vkCmdBindDescriptorSets
  // referencing the same key would silently misbehave.
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  for (uint32_t i = 0; i < command.m_descriptorSetCount.Value; ++i) {
    const uint64_t setKey = command.m_pDescriptorSets.Keys[i];
    m_StateTracking.RemoveState(setKey);
    m_StateTracking.GetDescriptorSetUpdateService().RemoveDescriptorSet(setKey);
  }
}

void SubcaptureLayer::Post(vkUpdateDescriptorSetsCommand& command) {
  m_StateTracking.GetDescriptorSetUpdateService().TrackUpdate(
      command.m_descriptorWriteCount.Value, command.m_pDescriptorWrites.Value,
      command.m_pDescriptorWrites.HandleKeys, command.m_descriptorCopyCount.Value,
      command.m_pDescriptorCopies.Value, command.m_pDescriptorCopies.HandleKeys);
}

void SubcaptureLayer::Post(vkCreateDescriptorUpdateTemplateCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorUpdateTemplateState>();
  state->key = command.m_pDescriptorUpdateTemplate.Key;
  state->parentKey = command.m_device.Key;
  m_StateTracking.GetDescriptorSetUpdateService().StoreTemplateEntries(
      command.m_pDescriptorUpdateTemplate.Key, command.m_pCreateInfo.Value);
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorUpdateTemplateState>();
  state->key = command.m_pDescriptorUpdateTemplate.Key;
  state->parentKey = command.m_device.Key;
  m_StateTracking.GetDescriptorSetUpdateService().StoreTemplateEntries(
      command.m_pDescriptorUpdateTemplate.Key, command.m_pCreateInfo.Value);
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDescriptorUpdateTemplateCommand& command) {
  m_StateTracking.GetDescriptorSetUpdateService().RemoveTemplateEntries(
      command.m_descriptorUpdateTemplate.Key);
  m_StateTracking.RemoveState(command.m_descriptorUpdateTemplate.Key);
}

void SubcaptureLayer::Post(vkDestroyDescriptorUpdateTemplateKHRCommand& command) {
  m_StateTracking.GetDescriptorSetUpdateService().RemoveTemplateEntries(
      command.m_descriptorUpdateTemplate.Key);
  m_StateTracking.RemoveState(command.m_descriptorUpdateTemplate.Key);
}

void SubcaptureLayer::Post(vkUpdateDescriptorSetWithTemplateCommand& command) {
  // RemapHandles leaves arg.Data intact (GITSKeys) and patches arg.PatchedData,
  // so arg.Data is safe to read here in Post.
  m_StateTracking.GetDescriptorSetUpdateService().TrackTemplateUpdate(
      command.m_descriptorSet.Key, command.m_descriptorUpdateTemplate.Key, command.m_pData.Data);
}

void SubcaptureLayer::Post(vkUpdateDescriptorSetWithTemplateKHRCommand& command) {
  m_StateTracking.GetDescriptorSetUpdateService().TrackTemplateUpdate(
      command.m_descriptorSet.Key, command.m_descriptorUpdateTemplate.Key, command.m_pData.Data);
}

// ---- Sampler ------------------------------------------------------------

void SubcaptureLayer::Post(vkCreateSamplerCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SamplerState>();
  state->key = command.m_pSampler.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySamplerCommand& command) {
  m_StateTracking.RemoveState(command.m_sampler.Key);
}

// ---- Command pool / buffers ----------------------------------------------

void SubcaptureLayer::Post(vkCreateCommandPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<CommandPoolState>();
  state->key = command.m_pCommandPool.Key;
  state->parentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    state->queueFamilyIndex = command.m_pCreateInfo.Value->queueFamilyIndex;
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyCommandPoolCommand& command) {
  m_CommandBufferLifecycle.OnDestroyPool(command.m_commandPool.Key);
  m_StateTracking.RemoveState(command.m_commandPool.Key);
}

void SubcaptureLayer::Post(vkAllocateCommandBuffersCommand& command) {
  m_CommandBufferLifecycle.OnAllocate(command);
}

void SubcaptureLayer::Post(vkFreeCommandBuffersCommand& command) {
  m_CommandBufferLifecycle.OnFree(command.m_pCommandBuffers.Keys);
}

void SubcaptureLayer::Post(vkResetCommandPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_CommandBufferLifecycle.OnResetPool(command.m_commandPool.Key);
}

// ---- vkCmd* dependency tracking ----------------------------------------

void SubcaptureLayer::Post(vkCmdBeginRenderPassCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderPassBegin.HandleKeys);
  // HandleKeys layout for VkRenderPassBeginInfo: [renderPassKey, framebufferKey, ...]
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1]);
  }
}

void SubcaptureLayer::Post(vkCmdBeginRenderPass2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderPassBegin.HandleKeys);
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1]);
  }
}

void SubcaptureLayer::Post(vkCmdBeginRenderPass2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderPassBegin.HandleKeys);
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1]);
  }
}

void SubcaptureLayer::Post(vkCmdEndRenderPassCommand& command) {
  m_ImageLayout.OnEndRenderPass(command.m_commandBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdEndRenderPass2Command& command) {
  m_ImageLayout.OnEndRenderPass(command.m_commandBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdEndRenderPass2KHRCommand& command) {
  m_ImageLayout.OnEndRenderPass(command.m_commandBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdBeginRenderingCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderingInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdBeginRenderingKHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderingInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdBindPipelineCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_pipeline.Key);
}

void SubcaptureLayer::Post(vkCmdBindPipelineShaderGroupNVCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_pipeline.Key);
}

void SubcaptureLayer::Post(vkCmdBindDescriptorSetsCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_layout.Key);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDescriptorSets.Keys);
}

void SubcaptureLayer::Post(vkCmdBindVertexBuffersCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBuffers.Keys);
}

void SubcaptureLayer::Post(vkCmdBindIndexBufferCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkCmdBindVertexBuffers2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBuffers.Keys);
}

void SubcaptureLayer::Post(vkCmdBindVertexBuffers2EXTCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBuffers.Keys);
}

void SubcaptureLayer::Post(vkCmdBindIndexBuffer2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkCmdBindIndexBuffer2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

// ---- vkCmd* transfer / sync / query dependency tracking -----------------
// Every handle referenced inside a recorded command must appear in the CB's
// dependencyKeys so that RestoreOne guarantees the object is restored before
// RestoreCommandBuffers emits the raw command bytes.

void SubcaptureLayer::Post(vkCmdCopyBufferCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcBuffer.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdCopyBuffer2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyBufferInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyBuffer2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyBufferInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdFillBufferCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdUpdateBufferCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdCopyImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcImage.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstImage.Key);
}

void SubcaptureLayer::Post(vkCmdCopyImage2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyImage2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdBlitImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcImage.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstImage.Key);
}

void SubcaptureLayer::Post(vkCmdBlitImage2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBlitImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdBlitImage2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBlitImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdClearColorImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_image.Key);
}

void SubcaptureLayer::Post(vkCmdClearDepthStencilImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_image.Key);
}

void SubcaptureLayer::Post(vkCmdResolveImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcImage.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstImage.Key);
}

void SubcaptureLayer::Post(vkCmdResolveImage2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pResolveImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdResolveImage2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pResolveImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyBufferToImageCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcBuffer.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstImage.Key);
}

void SubcaptureLayer::Post(vkCmdCopyBufferToImage2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyBufferToImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyBufferToImage2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyBufferToImageInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyImageToBufferCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_srcImage.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdCopyImageToBuffer2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyImageToBufferInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdCopyImageToBuffer2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCopyImageToBufferInfo.HandleKeys);
}

// Records the net signaled state an event ends up in after the given (still
// recording) command buffer executes.  Applied to EventState::isSignaled when
// the command buffer is submitted (see SyncStateService::OnQueueSubmit).
void SubcaptureLayer::RecordCmdEventState(uint64_t cbKey, uint64_t eventKey, bool signaled) {
  if (!cbKey || !eventKey) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (cb && cb->isRecording) {
    cb->eventStatesAfterSubmit[eventKey] = signaled;
  }
}

void SubcaptureLayer::Post(vkCmdSetEventCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, true);
}

void SubcaptureLayer::Post(vkCmdSetEvent2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, true);
}

void SubcaptureLayer::Post(vkCmdSetEvent2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, true);
}

void SubcaptureLayer::Post(vkCmdResetEventCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, false);
}

void SubcaptureLayer::Post(vkCmdResetEvent2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, false);
}

void SubcaptureLayer::Post(vkCmdResetEvent2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_event.Key);
  RecordCmdEventState(command.m_commandBuffer.Key, command.m_event.Key, false);
}

void SubcaptureLayer::Post(vkCmdWaitEventsCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pEvents.Keys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBufferMemoryBarriers.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pImageMemoryBarriers.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdDrawIndirectCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkCmdDrawIndexedIndirectCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkCmdDispatchIndirectCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkCmdBeginQueryCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdEndQueryCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdResetQueryPoolCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdWriteTimestampCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdWriteTimestamp2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdWriteTimestamp2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
}

void SubcaptureLayer::Post(vkCmdCopyQueryPoolResultsCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_dstBuffer.Key);
}

void SubcaptureLayer::Post(vkCmdExecuteCommandsCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pCommandBuffers.Keys);
}

// ---- Image layout tracking ---------------------------------------------

void SubcaptureLayer::Post(vkCmdPipelineBarrierCommand& command) {
  m_ImageLayout.OnPipelineBarrier(command.m_pImageMemoryBarriers.Value,
                                  command.m_pImageMemoryBarriers.Size,
                                  command.m_pImageMemoryBarriers.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBufferMemoryBarriers.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pImageMemoryBarriers.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdPipelineBarrier2Command& command) {
  if (command.m_pDependencyInfo.Value) {
    m_ImageLayout.OnPipelineBarrier2(*command.m_pDependencyInfo.Value,
                                     command.m_pDependencyInfo.HandleKeys);
  }
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdPipelineBarrier2KHRCommand& command) {
  if (command.m_pDependencyInfo.Value) {
    m_ImageLayout.OnPipelineBarrier2(*command.m_pDependencyInfo.Value,
                                     command.m_pDependencyInfo.HandleKeys);
  }
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfo.HandleKeys);
}

// ---- Swapchain / surface -------------------------------------------------
void SubcaptureLayer::Post(CreateWindowMetaCommand& command) {
  // Cache the window geometry so it can be associated with the next surface
  // creation command.  The player emits this token before each
  // vkCreate*SurfaceKHR; we store it here and attach it to the SurfaceState
  // so RestoreState can re-emit the window command before the surface.
  m_PendingWindow.x = command.m_X.Value;
  m_PendingWindow.y = command.m_Y.Value;
  m_PendingWindow.width = command.m_Width.Value;
  m_PendingWindow.height = command.m_Height.Value;
  m_PendingWindow.visible = command.m_Visible.Value;
  m_PendingWindow.hwndKey = command.m_Hwnd.Value;
  m_PendingWindow.hinstanceKey = command.m_Hinstance.Value;
  m_PendingWindow.valid = true;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void SubcaptureLayer::Post(vkCreateWin32SurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->key = command.m_pSurface.Key;
  state->parentKey = command.m_instance.Key;
  if (m_PendingWindow.valid) {
    state->windowX = m_PendingWindow.x;
    state->windowY = m_PendingWindow.y;
    state->windowWidth = m_PendingWindow.width;
    state->windowHeight = m_PendingWindow.height;
    state->windowVisible = m_PendingWindow.visible;
    state->hwndKey = m_PendingWindow.hwndKey;
    state->hinstanceKey = m_PendingWindow.hinstanceKey;
    m_PendingWindow.valid = false;
  }
  StoreState(std::move(state), command);
}
#endif

#ifdef GITS_PLATFORM_X11
void SubcaptureLayer::Post(vkCreateXcbSurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->key = command.m_pSurface.Key;
  state->parentKey = command.m_instance.Key;
  if (m_PendingWindow.valid) {
    state->windowX = m_PendingWindow.x;
    state->windowY = m_PendingWindow.y;
    state->windowWidth = m_PendingWindow.width;
    state->windowHeight = m_PendingWindow.height;
    state->windowVisible = m_PendingWindow.visible;
    state->hwndKey = m_PendingWindow.hwndKey;
    state->hinstanceKey = m_PendingWindow.hinstanceKey;
    m_PendingWindow.valid = false;
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateXlibSurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->key = command.m_pSurface.Key;
  state->parentKey = command.m_instance.Key;
  if (m_PendingWindow.valid) {
    state->windowX = m_PendingWindow.x;
    state->windowY = m_PendingWindow.y;
    state->windowWidth = m_PendingWindow.width;
    state->windowHeight = m_PendingWindow.height;
    state->windowVisible = m_PendingWindow.visible;
    state->hwndKey = m_PendingWindow.hwndKey;
    state->hinstanceKey = m_PendingWindow.hinstanceKey;
    m_PendingWindow.valid = false;
  }
  StoreState(std::move(state), command);
}
#endif

void SubcaptureLayer::Post(vkDestroySurfaceKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_surface.Key);
}

void SubcaptureLayer::Post(vkCreateSwapchainKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SwapchainState>();
  state->key = command.m_pSwapchain.Key;
  state->parentKey = command.m_device.Key;
  // Surface must be restored before the swapchain; add it as a dependency.
  if (!command.m_pCreateInfo.HandleKeys.empty() && command.m_pCreateInfo.HandleKeys[0] != 0) {
    state->dependencyKeys.push_back(command.m_pCreateInfo.HandleKeys[0]);
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySwapchainKHRCommand& command) {
  auto* swapchainState = m_StateTracking.GetState<SwapchainState>(command.m_swapchain.Key);
  if (swapchainState) {
    for (auto imageKey : swapchainState->imageKeys) {
      m_StateTracking.RemoveState(imageKey);
    }
  }
  m_StateTracking.RemoveState(command.m_swapchain.Key);
}

void SubcaptureLayer::Post(vkGetSwapchainImagesKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pSwapchainImages.Value) {
    return;
  }
  auto* swapchainState = m_StateTracking.GetState<SwapchainState>(command.m_swapchain.Key);
  if (!swapchainState) {
    return;
  }
  for (uint32_t i = 0; i < command.m_pSwapchainImages.Size; ++i) {
    uint64_t imgKey = command.m_pSwapchainImages.Keys[i];

    // Swapchain images are not explicitly created — track them here.
    if (!m_StateTracking.HasState(imgKey)) {
      swapchainState->imageKeys.push_back(imgKey);
      auto state = std::make_unique<ImageState>();
      state->key = imgKey;
      state->parentKey = command.m_swapchain.Key;
      StoreState(std::move(state), command);
    }
  }
}

void SubcaptureLayer::Post(vkAcquireNextImageKHRCommand& command) {
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_SUBOPTIMAL_KHR) ||
      !command.m_pImageIndex.Value) {
    return;
  }
  auto* sc = m_StateTracking.GetState<SwapchainState>(command.m_swapchain.Key);
  if (sc) {
    sc->acquiredImages.insert(*command.m_pImageIndex.Value);
  }
  // The acquire signals the binary semaphore; track it so state restore
  // re-signals it for any first recorded submit that waits on it.
  m_SyncState.OnImageAcquired(command.m_semaphore.Key);
}

void SubcaptureLayer::Post(vkAcquireNextImage2KHRCommand& command) {
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_SUBOPTIMAL_KHR) ||
      !command.m_pImageIndex.Value || !command.m_pAcquireInfo.Value) {
    return;
  }
  // HandleKeys layout for pAcquireInfo (VkAcquireNextImageInfoKHR handle
  // members, in struct order): [swapchainKey, semaphoreKey, fenceKey].
  if (command.m_pAcquireInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t swapchainKey = command.m_pAcquireInfo.HandleKeys[0];
  auto* sc = m_StateTracking.GetState<SwapchainState>(swapchainKey);
  if (sc) {
    sc->acquiredImages.insert(*command.m_pImageIndex.Value);
  }
  // The acquire signals the binary semaphore; track it (see vkAcquireNextImageKHR).
  if (command.m_pAcquireInfo.HandleKeys.size() > 1) {
    m_SyncState.OnImageAcquired(command.m_pAcquireInfo.HandleKeys[1]);
  }
}

// ---- Query pool ----------------------------------------------------------

void SubcaptureLayer::Post(vkCreateQueryPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<QueryPoolState>();
  state->key = command.m_pQueryPool.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyQueryPoolCommand& command) {
  m_StateTracking.RemoveState(command.m_queryPool.Key);
}

// ---- Acceleration structures ---------------------------------------------

void SubcaptureLayer::Post(vkCreateAccelerationStructureKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<AccelerationStructureState>();
  state->key = command.m_pAccelerationStructure.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyAccelerationStructureKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_accelerationStructure.Key);
}

void SubcaptureLayer::Post(vkCreateAccelerationStructureNVCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<AccelerationStructureState>();
  state->key = command.m_pAccelerationStructure.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyAccelerationStructureNVCommand& command) {
  m_StateTracking.RemoveState(command.m_accelerationStructure.Key);
}

// ---- Deferred operations -------------------------------------------------

void SubcaptureLayer::Post(vkCreateDeferredOperationKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DeferredOperationState>();
  state->key = command.m_pDeferredOperation.Key;
  state->parentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDeferredOperationKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_operation.Key);
}

} // namespace vulkan
} // namespace gits
