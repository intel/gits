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
#include "configurator.h"
#include "log.h"
#include "subcaptureFatal.h"

#include <algorithm>
#include <map>

namespace gits {
namespace vulkan {

SubcaptureLayer::SubcaptureLayer(PlayerManager& playerManager,
                                 const std::string& framesStr,
                                 bool analysisMode)
    : Layer("Subcapture"),
      m_AnalysisMode(analysisMode),
      m_SubcaptureRange(framesStr),
      m_Recorder(!analysisMode),
      m_GpuReadbackHelper(playerManager),
      m_StateTracking(m_Recorder),
      m_SyncState(m_StateTracking),
      m_ImageLayout(m_StateTracking),
      m_CommandBufferLifecycle(m_StateTracking),
      m_MappedMemory(m_StateTracking) {
  m_StateTracking.SetGpuReadbackHelper(&m_GpuReadbackHelper);
  if (m_AnalysisMode) {
    // Analysis pass: collect in-range object usage; never restore.
    m_AnalyzerService = std::make_unique<AnalyzerService>(m_StateTracking, m_SubcaptureRange);
    m_AnalyzerRaytracingService =
        std::make_unique<AnalyzerRaytracingService>(m_StateTracking, m_GpuReadbackHelper);
    m_RaytracingOptimizationService = std::make_unique<RaytracingOptimizationService>();
    m_AnalyzerService->SetRaytracingService(m_AnalyzerRaytracingService.get());
    m_AnalyzerService->SetOptimizationService(m_RaytracingOptimizationService.get());
    // Drive the analyzer's TLAS instance readback and flush the chain-reduction graph at
    // submit time, once the staged builds/copies have executed. Flushing at submit gives
    // the chain graph true GPU execution order across command buffers.
    m_StateTracking.SetSubmittedCommandBufferCallback(
        [this](uint64_t cbKey, uint64_t submitQueueKey) {
          m_AnalyzerRaytracingService->ReadStagedTlasInstances(cbKey, submitQueueKey);
          m_RaytracingOptimizationService->OnQueueSubmit(cbKey);
        });
  } else {
    // Recording pass: gate restore by the analysis results (a no-op that
    // restores everything when optimization is off or the analysis set is
    // empty / absent).
    m_StateTracking.SetAnalyzerResults(&m_AnalyzerResults);
  }
}

// ---- Frame boundary ------------------------------------------------------

void SubcaptureLayer::TriggerRestoreState() {
  m_StateTracking.RestoreState();
}

void SubcaptureLayer::Post(vkQueuePresentKHRCommand& command) {
  // vkQueuePresentKHR consumes (unsignals) all binary semaphores listed in
  // pWaitSemaphores.  Clear IsSignaled so we don't incorrectly signal them
  // during state restore.
  if (command.m_pPresentInfo.Value && !command.m_pPresentInfo.HandleKeys.empty()) {
    m_SyncState.OnQueuePresent(*command.m_pPresentInfo.Value, command.m_pPresentInfo.HandleKeys);
  }

  // Remove presented images from their swapchain's AcquiredImages set, and
  // complete any pending EXCLUSIVE queue-family-ownership transfer for each
  // successfully presented image.
  //
  // For the common exclusive-swapchain pattern (separate graphics and present
  // queue families) the application only ever records the release half of the
  // ownership-transfer barrier (srcFamily=graphics, dstFamily=present) on its
  // own (graphics) command buffer - there is no application-recorded acquire
  // barrier, because vkQueuePresentKHR itself performs that acquire
  // implicitly on the presentation engine's behalf (Vulkan spec, "Queue
  // Family Ownership Transfer"). Without resolving it here, such images stay
  // ExclusiveOwnershipPending forever (see NoteExclusiveQueueFamilyTransfer),
  // which permanently blocks EmitImageLayoutTransitions() from emitting their
  // UNDEFINED -> PRESENT_SRC_KHR restore barrier at subcapture time.
  // HandleKeys layout: [waitSemaphoreKeys...][swapchainKeys...]
  if (command.m_pPresentInfo.Value) {
    const VkPresentInfoKHR& pi = *command.m_pPresentInfo.Value;
    const uint32_t swapchainKeyStart = pi.waitSemaphoreCount;
    uint32_t presentQueueFamily = UINT32_MAX;
    if (auto* queueState = m_StateTracking.GetState<QueueState>(command.m_queue.Key)) {
      presentQueueFamily = queueState->QueueFamilyIndex;
    }
    for (uint32_t i = 0; i < pi.swapchainCount && pi.pImageIndices; ++i) {
      const uint32_t keyIdx = swapchainKeyStart + i;
      if (keyIdx >= command.m_pPresentInfo.HandleKeys.size()) {
        break;
      }
      const uint64_t swapchainKey = command.m_pPresentInfo.HandleKeys[keyIdx];
      auto* sc = m_StateTracking.GetState<SwapchainState>(swapchainKey);
      if (sc) {
        sc->AcquiredImages.erase(pi.pImageIndices[i]);
      }
      // Per-image present result: pResults[i] when the caller asked for it,
      // otherwise the single VkResult returned by vkQueuePresentKHR applies
      // to every swapchain in the batch.
      const VkResult presentResult = pi.pResults ? pi.pResults[i] : command.m_Return.Value;
      const bool presentedOk = (presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR);
      if (presentedOk && sc && pi.pImageIndices[i] < sc->ImageKeys.size()) {
        m_ImageLayout.CompletePendingTransferOnPresent(sc->ImageKeys[pi.pImageIndices[i]],
                                                       presentQueueFamily);
      }
    }
  }

  if (!m_SubcaptureRange.IsEnabled()) {
    return;
  }

  // Analysis pass: advance the frame counter and dump the analysis file when
  // the range ends.  No restore and no recording happen here.
  if (m_AnalysisMode) {
    const bool wasInRange = m_SubcaptureRange.InRange();
    m_SubcaptureRange.FrameEnd();
    const bool nowInRange = m_SubcaptureRange.InRange();
    if (wasInRange && !nowInRange && m_AnalyzerService) {
      m_AnalyzerService->DumpAnalysisFile();
    }
    return;
  }

  // Fire state restore exactly once, before the first recorded frame.
  if (m_SubcaptureRange.IsRestorePoint()) {
    TriggerRestoreState();
    // After RestoreState the recorder stream is open; the very next FrameEnd
    // will put us inside the range so recording begins below.
  }

  m_SubcaptureRange.FrameEnd();

  // RecordingLayer (registered immediately after SubcaptureLayer in the post
  // order) is the sole owner of stream output: it records the in-range
  // vkQueuePresentKHR, emits the trailing FrameEnd marker, and finalizes the
  // stream once the range closes.  This handler must therefore only advance the
  // range.  Emitting a FrameEnd marker or calling FinishRecording() here would
  // run *before* RecordingLayer's present handler and close the stream one
  // command too early, dropping the final vkQueuePresentKHR (and duplicating
  // FrameEnd markers) for every subcapture.
  if (m_SubcaptureRange.InRange()) {
    if (!m_Recording) {
      m_Recording = true;
      LOG_INFO << "Vulkan subcapture: entering recording range";
    }
  } else if (m_Recording) {
    m_Recording = false;
    LOG_INFO << "Vulkan subcapture: recording range complete";
  }
}

// ---- Instance / device ---------------------------------------------------

void SubcaptureLayer::Post(vkCreateInstanceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<InstanceState>();
  state->Key = command.m_pInstance.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyInstanceCommand& command) {
  // vkDestroyInstance implicitly invalidates every VkPhysicalDevice obtained
  // from this instance.  Drop their tracked state so a subsequent
  // vkCreateInstance + vkEnumeratePhysicalDevices on the same hardware (which
  // returns the same VkPhysicalDevice handle, and therefore the same recorder
  // key via TryGetKey) re-tracks them under the new instance instead of
  // leaving stale state pointing at the Destroyed instance.
  const uint64_t instanceKey = command.m_instance.Key;
  std::vector<uint64_t> orphanedPDs;
  for (const auto& [key, statePtr] : m_StateTracking.GetStates()) {
    if (statePtr->CreationCommandId == CommandId::ID_VKENUMERATEPHYSICALDEVICES &&
        statePtr->ParentKey == instanceKey) {
      orphanedPDs.push_back(key);
    }
  }
  for (uint64_t pdKey : orphanedPDs) {
    m_StateTracking.RemoveState(pdKey);
  }
  m_StateTracking.RemoveState(instanceKey);
}

void SubcaptureLayer::Post(vkEnumeratePhysicalDevicesCommand& command) {
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_INCOMPLETE) ||
      !command.m_pPhysicalDevices.Value) {
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
    state->Key = key;
    state->ParentKey = command.m_instance.Key;
    state->CreationCommandId = command.GetId();
    if (m_StateTracking.HasState(key)) {
      // Re-enumeration on a (possibly new) instance: refresh ParentKey so the
      // synthesis attaches the device to whichever instance is current.
      m_StateTracking.RemoveState(key);
    }
    m_StateTracking.StoreState(std::move(state));
  }
}

// Physical devices obtained via vkEnumeratePhysicalDeviceGroups[KHR] are embedded inside
// VkPhysicalDeviceGroupProperties::physicalDevices[]. The HandleKeys layout mirrors
// UpdateOutputHandle: one key per physical device, in group/slot order. Without these
// handlers, games using the device-group enumeration path would have no
// PhysicalDeviceState entries and state restore would fail on vkCreateDevice.
namespace {
template <typename TCommand>
void TrackPhysicalDeviceGroups(StateTrackingService& stateTracking,
                               const TCommand& command,
                               const char* apiName) {
  uint32_t keyIdx = 0;
  const auto& groupProps = command.m_pPhysicalDeviceGroupProperties;
  for (uint32_t i = 0; i < groupProps.Size; ++i) {
    const auto& group = groupProps.Value[i];
    for (uint32_t j = 0; j < group.physicalDeviceCount; ++j) {
      if (keyIdx >= groupProps.HandleKeys.size()) {
        // A group declaring more devices than HandleKeys provides is a recorder/codegen
        // bug. Log it rather than silently skipping the remaining groups.
        LOG_WARNING << "Vulkan subcapture: " << apiName
                    << " HandleKeys size=" << groupProps.HandleKeys.size()
                    << " is smaller than the cumulative physicalDeviceCount; "
                    << "remaining devices in group " << i
                    << " and later groups will not be tracked";
        return;
      }
      const uint64_t key = groupProps.HandleKeys[keyIdx++];
      if (!key) {
        continue;
      }
      auto state = std::make_unique<PhysicalDeviceState>();
      state->Key = key;
      state->ParentKey = command.m_instance.Key;
      // Typed-only state (no creation blob): normalizing the command id routes the device
      // through RestorePhysicalDevice, which synthesizes one robust
      // vkEnumeratePhysicalDevices per live instance, and lets vkDestroyInstance drop it.
      state->CreationCommandId = CommandId::ID_VKENUMERATEPHYSICALDEVICES;
      if (stateTracking.HasState(key)) {
        // Re-enumeration on a possibly new instance: refresh ParentKey.
        stateTracking.RemoveState(key);
      }
      stateTracking.StoreState(std::move(state));
    }
  }
}
} // namespace

void SubcaptureLayer::Post(vkEnumeratePhysicalDeviceGroupsCommand& command) {
  // VK_INCOMPLETE still yields valid handles, and dropping them would leave the device
  // with no PhysicalDeviceState. Mirrors Post(vkEnumeratePhysicalDevicesCommand).
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_INCOMPLETE) ||
      !command.m_pPhysicalDeviceGroupProperties.Value) {
    return;
  }
  TrackPhysicalDeviceGroups(m_StateTracking, command, "vkEnumeratePhysicalDeviceGroups");
}

void SubcaptureLayer::Post(vkEnumeratePhysicalDeviceGroupsKHRCommand& command) {
  // See Post(vkEnumeratePhysicalDeviceGroupsCommand): accept VK_INCOMPLETE too.
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_INCOMPLETE) ||
      !command.m_pPhysicalDeviceGroupProperties.Value) {
    return;
  }
  TrackPhysicalDeviceGroups(m_StateTracking, command, "vkEnumeratePhysicalDeviceGroupsKHR");
}

void SubcaptureLayer::Post(vkCreateDeviceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DeviceState>();
  state->Key = command.m_pDevice.Key;
  state->ParentKey = command.m_physicalDevice.Key;
  if (command.m_pCreateInfo.Value) {
    const auto& ci = *command.m_pCreateInfo.Value;
    for (uint32_t i = 0; i < ci.enabledExtensionCount; ++i) {
      const char* extensionName = ci.ppEnabledExtensionNames[i];
      if (!extensionName) {
        continue;
      }
      if (strcmp(extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) == 0) {
        state->HasTimelineSemaphoreKHR = true;
      } else if (strcmp(extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
        state->HasBufferDeviceAddressKHR = true;
      } else if (strcmp(extensionName, VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
        state->HasBufferDeviceAddressEXT = true;
      }
    }
  }
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
  state->Key = command.m_pMemory.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pAllocateInfo.Value) {
    state->AllocationSize = command.m_pAllocateInfo.Value->allocationSize;
    state->MemoryTypeIndex = command.m_pAllocateInfo.Value->memoryTypeIndex;
    // Capture the memory-side opaque capture/replay address (if present) so a build input
    // backed by this memory can be recreated at the same device address.
    const auto* pNext = static_cast<const VkBaseInStructure*>(command.m_pAllocateInfo.Value->pNext);
    while (pNext) {
      if (pNext->sType == VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO) {
        state->OpaqueCaptureAddress =
            reinterpret_cast<const VkMemoryOpaqueCaptureAddressAllocateInfo*>(pNext)
                ->opaqueCaptureAddress;
        break;
      }
      pNext = pNext->pNext;
    }
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
      state->DependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkFreeMemoryCommand& command) {
  // RestoreBlasChain needs the AS backing memory, so it is flagged Destroyed rather than
  // erased. Other memory is erased as before.
  auto* state = m_StateTracking.GetState<DeviceMemoryState>(command.m_memory.Key);
  if (state && state->AsBacking) {
    state->Destroyed = true;
    return;
  }
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

void SubcaptureLayer::Post(vkMapMemory2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pMemoryMapInfo.Value) {
    return;
  }
  const VkMemoryMapInfo& info = *command.m_pMemoryMapInfo.Value;
  // HandleKeys[0] is the recorder-side key for VkMemoryMapInfo::memory.
  if (command.m_pMemoryMapInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t memoryKey = command.m_pMemoryMapInfo.HandleKeys[0];
  m_MappedMemory.OnMapMemory(memoryKey, info.offset, info.size, info.flags);
}

void SubcaptureLayer::Post(vkMapMemory2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pMemoryMapInfo.Value) {
    return;
  }
  const VkMemoryMapInfo& info = *command.m_pMemoryMapInfo.Value;
  if (command.m_pMemoryMapInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t memoryKey = command.m_pMemoryMapInfo.HandleKeys[0];
  m_MappedMemory.OnMapMemory(memoryKey, info.offset, info.size, info.flags);
}

void SubcaptureLayer::Post(vkUnmapMemory2Command& command) {
  if (!command.m_pMemoryUnmapInfo.Value || command.m_pMemoryUnmapInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t memoryKey = command.m_pMemoryUnmapInfo.HandleKeys[0];
  m_MappedMemory.OnUnmapMemory(memoryKey);
}

void SubcaptureLayer::Post(vkUnmapMemory2KHRCommand& command) {
  if (!command.m_pMemoryUnmapInfo.Value || command.m_pMemoryUnmapInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t memoryKey = command.m_pMemoryUnmapInfo.HandleKeys[0];
  m_MappedMemory.OnUnmapMemory(memoryKey);
}

// When the player replays a MappedDataMetaCommand it has already written the
// bytes into the mapped host pointer.  We snapshot those bytes here so that
// state restore can re-emit the same command and the subcapture player starts
// with the correct initial memory contents.
void SubcaptureLayer::Post(MappedDataMetaCommand& command) {
  m_MappedMemory.OnMappedData(command);
}

void SubcaptureLayer::Post(RestoreContentDataCommand& command) {
  // The player uploads image content through barriers it records itself, so the layouts it
  // leaves behind never reach this layer as commands. Reconstructing them from the manifest
  // would have to mirror every failure path the upload can take, so take the layouts the
  // upload actually applied instead. A data token can flush a batch, so drain after each.
  for (const auto& [imageKey, layout] :
       PlayerManager::Get().GetRestoreContentService().DrainAppliedImageLayouts()) {
    auto* image = m_StateTracking.GetState<ImageState>(imageKey);
    if (image) {
      image->CurrentLayout = layout;
    }
  }
}

// ---- In-flight command buffer tracking -----------------------------------

void SubcaptureLayer::Post(vkBeginCommandBufferCommand& command) {
  // vkBeginCommandBuffer implicitly resets the CB if it was executable.
  // Clear any stale render-pass tracking for this CB before starting fresh.
  m_ImageLayout.OnResetCommandBuffer(command.m_commandBuffer.Key);
  m_CommandBufferLifecycle.OnBegin(command);
  // For secondary command buffers pBeginInfo->pInheritanceInfo carries
  // renderPass and framebuffer handles encoded into the stored
  // BeginCommandBuffer bytes.  When RestoreCommandBuffers later emits those
  // bytes the second player calls GetHandle on every key in HandleKeys -- if
  // either object was not restored the player will assert.  Track them as
  // dependencies (IsRecording is true after OnBegin) so RestoreOne skips
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
  state->Key = command.m_pFence.Key;
  state->ParentKey = command.m_device.Key;
  // Honor the create-time signaled flag: a fence created with
  // VK_FENCE_CREATE_SIGNALED_BIT starts signaled, so if it is never used or
  // reset before the cut it must still be restored signaled.  Mirrors the
  // legacy CFenceState constructor (fenceUsed = flags & SIGNALED_BIT).
  if (command.m_pCreateInfo.Value &&
      (command.m_pCreateInfo.Value->flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
    state->IsSignaled = true;
  }
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
                            command.m_pSubmits.HandleKeys, command.m_fence.Key,
                            command.m_queue.Key);
  // Commit each submitted CB's buffered image-layout transitions to
  // ImageState::CurrentLayout in submission order (layouts take effect at
  // submit time, not record time).
  m_ImageLayout.OnQueueSubmit(command.m_pSubmits.Value, command.m_pSubmits.Size,
                              command.m_pSubmits.HandleKeys);
}

void SubcaptureLayer::Post(vkQueueSubmit2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                             command.m_pSubmits.HandleKeys, command.m_fence.Key,
                             command.m_queue.Key);
  m_ImageLayout.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                               command.m_pSubmits.HandleKeys);
}

void SubcaptureLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_SyncState.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                             command.m_pSubmits.HandleKeys, command.m_fence.Key,
                             command.m_queue.Key);
  m_ImageLayout.OnQueueSubmit2(command.m_pSubmits.Value, command.m_pSubmits.Size,
                               command.m_pSubmits.HandleKeys);
}

void SubcaptureLayer::Post(vkQueueBindSparseCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // vkQueueBindSparse signals its fence (if supplied) when the bind completes.
  // Track it so a fence signaled by a pre-cut sparse bind is restored signaled
  // (see Post(vkAcquireNextImageKHRCommand)).  Mirrors legacy
  // vkQueueBindSparse_SD setting fenceUsed=true.  NOTE: the per-VkBindSparseInfo
  // pSignalSemaphores are intentionally not tracked here, matching legacy, which
  // also tracks only the fence for sparse binds.
  m_SyncState.OnFenceSignaled(command.m_fence.Key);
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
  state->Key = command.m_pQueue.Key;
  state->ParentKey = command.m_device.Key;
  state->QueueFamilyIndex = command.m_queueFamilyIndex.Value;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkGetDeviceQueue2Command& command) {
  if (!command.m_pQueue.Key) {
    return;
  }
  auto state = std::make_unique<QueueState>();
  state->Key = command.m_pQueue.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pQueueInfo.Value) {
    state->QueueFamilyIndex = command.m_pQueueInfo.Value->queueFamilyIndex;
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateSemaphoreCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SemaphoreState>();
  state->Key = command.m_pSemaphore.Key;
  state->ParentKey = command.m_device.Key;
  // Detect timeline semaphores via pNext chain.
  if (command.m_pCreateInfo.Value) {
    const auto* pNext = static_cast<const VkBaseInStructure*>(command.m_pCreateInfo.Value->pNext);
    while (pNext) {
      if (pNext->sType == VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO) {
        const auto* typeInfo = reinterpret_cast<const VkSemaphoreTypeCreateInfo*>(pNext);
        if (typeInfo->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
          state->IsBinary = false;
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

void SubcaptureLayer::Post(vkSignalSemaphoreCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pSignalInfo.Value ||
      command.m_pSignalInfo.HandleKeys.empty()) {
    return;
  }
  // HandleKeys[0] is the recorder-side key for VkSemaphoreSignalInfo::semaphore.
  m_SyncState.OnSignalSemaphore(command.m_pSignalInfo.HandleKeys[0],
                                command.m_pSignalInfo.Value->value);
}

void SubcaptureLayer::Post(vkSignalSemaphoreKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pSignalInfo.Value ||
      command.m_pSignalInfo.HandleKeys.empty()) {
    return;
  }
  m_SyncState.OnSignalSemaphore(command.m_pSignalInfo.HandleKeys[0],
                                command.m_pSignalInfo.Value->value);
}

void SubcaptureLayer::Post(vkCreateEventCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<EventState>();
  state->Key = command.m_pEvent.Key;
  state->ParentKey = command.m_device.Key;
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
    ev->IsSignaled = true;
  }
}

void SubcaptureLayer::Post(vkResetEventCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  if (auto* ev = m_StateTracking.GetState<EventState>(command.m_event.Key)) {
    ev->IsSignaled = false;
  }
}

// ---- Buffers / images ----------------------------------------------------

void SubcaptureLayer::Post(vkCreateBufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<BufferState>();
  state->Key = command.m_pBuffer.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    state->BufferSize = command.m_pCreateInfo.Value->size;
    state->UsageFlags = command.m_pCreateInfo.Value->usage;
    // Detect a capture/replay opaque address via pNext chain (unlike AS,
    // VkBufferCreateInfo has no direct opaqueCaptureAddress field).
    const auto* pNext = static_cast<const VkBaseInStructure*>(command.m_pCreateInfo.Value->pNext);
    while (pNext) {
      if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO) {
        const auto* addrInfo =
            reinterpret_cast<const VkBufferOpaqueCaptureAddressCreateInfo*>(pNext);
        state->OpaqueCaptureAddress = addrInfo->opaqueCaptureAddress;
        break;
      }
      pNext = pNext->pNext;
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyBufferCommand& command) {
  // An AS storage buffer is flagged Destroyed rather than erased - see
  // Post(vkFreeMemoryCommand&). Other buffers are erased as before.
  m_StateTracking.GetDeviceAddressTracking().Untrack(command.m_buffer.Key);
  auto* state = m_StateTracking.GetState<BufferState>(command.m_buffer.Key);
  if (state && state->AsBacking) {
    state->Destroyed = true;
    return;
  }
  m_StateTracking.RemoveState(command.m_buffer.Key);
}

void SubcaptureLayer::Post(vkBindBufferMemoryCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto* state = m_StateTracking.GetState<BufferState>(command.m_buffer.Key);
  if (state) {
    state->BoundMemoryKey = command.m_memory.Key;
    state->MemoryOffset = command.m_memoryOffset.Value;
    TrackBoundBufferDeviceAddress(state->Key);
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
      state->BoundMemoryKey = memKey;
      state->MemoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
      TrackBoundBufferDeviceAddress(bufKey);
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
      state->BoundMemoryKey = memKey;
      state->MemoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
      TrackBoundBufferDeviceAddress(bufKey);
    }
  }
}

void SubcaptureLayer::TrackBoundBufferDeviceAddress(uint64_t bufferKey) {
  auto* state = m_StateTracking.GetState<BufferState>(bufferKey);
  if (!state || !(state->UsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
    return;
  }

  const VkDeviceAddress address =
      m_GpuReadbackHelper.QueryBufferDeviceAddress(state->ParentKey, bufferKey);
  GITS_ASSERT(address);

  state->DeviceAddress = address;
  m_StateTracking.GetDeviceAddressTracking().Track(address, bufferKey, state->BufferSize);
}

void SubcaptureLayer::Post(vkCreateImageCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<ImageState>();
  state->Key = command.m_pImage.Key;
  state->ParentKey = command.m_device.Key;
  const VkImageCreateInfo& ci = *command.m_pCreateInfo.Value;
  state->Format = ci.format;
  state->CurrentLayout = ci.initialLayout;
  state->Extent = ci.extent;
  state->MipLevels = ci.mipLevels;
  state->ArrayLayers = ci.arrayLayers;
  state->Samples = ci.samples;
  state->UsageFlags = ci.usage;
  state->SharingMode = ci.sharingMode;
  state->Disjoint = (ci.flags & VK_IMAGE_CREATE_DISJOINT_BIT) != 0;
  if (ci.sharingMode == VK_SHARING_MODE_CONCURRENT && ci.pQueueFamilyIndices != nullptr) {
    state->ConcurrentFamilies.assign(ci.pQueueFamilyIndices,
                                     ci.pQueueFamilyIndices + ci.queueFamilyIndexCount);
  }
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
    state->BoundMemoryKey = command.m_memory.Key;
    state->MemoryOffset = command.m_memoryOffset.Value;
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
      state->BoundMemoryKey = memKey;
      state->MemoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
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
      state->BoundMemoryKey = memKey;
      state->MemoryOffset = command.m_pBindInfos.Value[i].memoryOffset;
    }
  }
}

namespace {
template <typename TCommand>
void TrackBufferDeviceAddress(StateTrackingService& stateTracking, TCommand& command) {
  if (command.m_Return.Value == 0 || command.m_pInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t bufferKey = command.m_pInfo.HandleKeys[0];
  auto* state = stateTracking.GetState<BufferState>(bufferKey);
  if (!state) {
    return;
  }
  state->DeviceAddress = command.m_Return.Value;
  stateTracking.GetDeviceAddressTracking().Track(command.m_Return.Value, bufferKey,
                                                 state->BufferSize);
}
} // namespace

void SubcaptureLayer::Post(vkGetBufferDeviceAddressCommand& command) {
  TrackBufferDeviceAddress(m_StateTracking, command);
}

void SubcaptureLayer::Post(vkGetBufferDeviceAddressKHRCommand& command) {
  TrackBufferDeviceAddress(m_StateTracking, command);
}

void SubcaptureLayer::Post(vkGetBufferDeviceAddressEXTCommand& command) {
  TrackBufferDeviceAddress(m_StateTracking, command);
}

void SubcaptureLayer::Post(vkCreateBufferViewCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<BufferViewState>();
  state->Key = command.m_pView.Key;
  state->ParentKey = command.m_device.Key;
  // VkBufferViewCreateInfo::buffer is the first handle in HandleKeys.
  if (!command.m_pCreateInfo.HandleKeys.empty()) {
    state->DependencyKeys.push_back(command.m_pCreateInfo.HandleKeys[0]);
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
  state->Key = command.m_pView.Key;
  state->ParentKey = command.m_device.Key;
  // HandleKeys layout: [0] = VkImageViewCreateInfo::image,
  //                    [1] = VkSamplerYcbcrConversionInfo::conversion (pNext, if present).
  state->ImageKey =
      command.m_pCreateInfo.HandleKeys.empty() ? 0 : command.m_pCreateInfo.HandleKeys[0];
  if (state->ImageKey) {
    state->DependencyKeys.push_back(state->ImageKey);
  }
  state->YcbcrConversionKey =
      command.m_pCreateInfo.HandleKeys.size() > 1 ? command.m_pCreateInfo.HandleKeys[1] : 0;
  if (state->YcbcrConversionKey) {
    state->DependencyKeys.push_back(state->YcbcrConversionKey);
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
  state->Key = command.m_pRenderPass.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->AttachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->AttachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateRenderPass2Command& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<RenderPassState>();
  state->Key = command.m_pRenderPass.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->AttachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->AttachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateRenderPass2KHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<RenderPassState>();
  state->Key = command.m_pRenderPass.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value && command.m_pCreateInfo.Value->pAttachments) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->AttachmentFinalLayouts.reserve(ci.attachmentCount);
    for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
      state->AttachmentFinalLayouts.push_back(ci.pAttachments[i].finalLayout);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyRenderPassCommand& command) {
  // Same rationale as vkDestroyShaderModule / vkDestroyPipelineLayout: VkRenderPass may be
  // Destroyed after VkGraphicsPipelineCreateInfo referenced it (Vulkan permits this once pipelines
  // are built). RemoveState erased the key while pipelines still listed it as a dependency, so
  // RestoreOne aborted vkCreateGraphicsPipelines with "dependency ... no longer tracked". Keep the
  // encoded vkCreateRenderPass* blob and mark Destroyed; RestoreOne re-emits create before pipelines.
  // Unlike shader modules we do not schedule vkDestroyRenderPass after restore: vkCmdBeginRenderPass
  // blobs still reference this handle key for the rest of replay.
  auto* state = m_StateTracking.GetState(command.m_renderPass.Key);
  if (state) {
    state->Destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateFramebufferCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<FramebufferState>();
  state->Key = command.m_pFramebuffer.Key;
  state->ParentKey = command.m_device.Key;
  // VkFramebufferCreateInfo HandleKeys: render pass followed by attachment image views.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->DependencyKeys.push_back(dep);
    }
  }
  // Store attachment image view keys separately (HandleKeys[0] = renderPass,
  // HandleKeys[1..] = pAttachments[i] image view keys in order).
  // Used by ImageLayoutService to resolve attachment index ? image key.
  const auto& keys = command.m_pCreateInfo.HandleKeys;
  if (keys.size() > 1) {
    state->AttachmentImageViewKeys.assign(keys.begin() + 1, keys.end());
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
  state->Key = command.m_pPipelineCache.Key;
  state->ParentKey = command.m_device.Key;
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
  state->Key = command.m_pPipelineLayout.Key;
  state->ParentKey = command.m_device.Key;
  // Descriptor set layout handles referenced by pSetLayouts.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->DependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyPipelineLayoutCommand& command) {
  // Mark as Destroyed but keep the state so it can be transiently re-created
  // as a pipeline dependency during state restore (same rationale as shader
  // modules above).
  auto* state = m_StateTracking.GetState(command.m_pipelineLayout.Key);
  if (state) {
    state->Destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateShaderModuleCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<ShaderModuleState>();
  state->Key = command.m_pShaderModule.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyShaderModuleCommand& command) {
  // Mark as Destroyed but keep the state so it can be transiently re-created
  // as a pipeline dependency during state restore.  Vulkan allows destroying a
  // VkShaderModule after pipeline creation; the pipeline holds the compiled
  // shader internally.  We need the creation data to re-emit the module just
  // before any pipeline that references it, and destroy it afterwards.
  auto* state = m_StateTracking.GetState(command.m_shaderModule.Key);
  if (state) {
    state->Destroyed = true;
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
  // pipelines; as a hard dependency a Destroyed (and thus removed) cache would make
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
  // Collect all output pipeline keys for this batch.  BatchPipelineKeys is
  // stored on every sibling PipelineState so that RestoreOne can mark all
  // siblings as restored after the first one emits the shared batch command,
  // preventing N redundant full-batch emissions for a batch of N pipelines.
  std::vector<uint64_t> batchKeys;
  batchKeys.reserve(command.m_createInfoCount.Value);
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    const uint64_t pipelineKey = command.m_pPipelines.Keys[i];
    if (pipelineKey) {
      batchKeys.push_back(pipelineKey);
    }
  }
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    const uint64_t pipelineKey = command.m_pPipelines.Keys[i];
    // A zero key marks a VK_NULL_HANDLE pipeline slot (e.g. an element skipped by
    // VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT).  Do not track it:
    // a Key 0 state would sort to the front of m_States and be restored first.
    if (!pipelineKey) {
      continue;
    }
    auto state = std::make_unique<PipelineState>();
    state->Key = pipelineKey;
    state->ParentKey = command.m_device.Key;
    state->DependencyKeys = batchDeps;
    state->BatchPipelineKeys = batchKeys;
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
  // be Destroyed before the cut; EmitCreationCommand nulls it when no longer live.
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
    state->Key = command.m_pPipelines.Keys[i];
    state->ParentKey = command.m_device.Key;
    state->DependencyKeys = batchDeps;
    state->BatchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkCreateRayTracingPipelinesKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // pipelineCache is intentionally NOT a dependency (see vkCreateGraphicsPipelines):
  // an optional hint that may be Destroyed before the cut; EmitCreationCommand nulls
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
    state->Key = command.m_pPipelines.Keys[i];
    state->ParentKey = command.m_device.Key;
    state->DependencyKeys = batchDeps;
    state->BatchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkCreateRayTracingPipelinesNVCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  // pipelineCache is intentionally NOT a dependency (see vkCreateGraphicsPipelines):
  // an optional hint that may be Destroyed before the cut; EmitCreationCommand nulls
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
    state->Key = command.m_pPipelines.Keys[i];
    state->ParentKey = command.m_device.Key;
    state->DependencyKeys = batchDeps;
    state->BatchPipelineKeys = batchKeys;
    StoreState(std::move(state), command);
  }
}

void SubcaptureLayer::Post(vkDestroyPipelineCommand& command) {
  // Same pattern as vkDestroyShaderModule: VkPipeline may still appear in
  // encoded vkCmdBindPipeline blobs after the app destroys it.  Removing state
  // made HasState(pipelineKey) false so RestoreCommandBuffers could not recreate
  // the pipeline before replaying those blobs.  Keep CreationCommandBuffer and
  // mark Destroyed so RestoreOne can transiently recreate the VkPipeline; we
  // do not emit vkDestroyPipeline after restore (pipelines can stay referenced
  // by executable CBs submitted later, unlike shader modules post-create).
  auto* state = m_StateTracking.GetState(command.m_pipeline.Key);
  if (state) {
    state->Destroyed = true;
  }
}

// ---- Descriptors ---------------------------------------------------------

void SubcaptureLayer::Post(vkCreateDescriptorSetLayoutCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorSetLayoutState>();
  state->Key = command.m_pSetLayout.Key;
  state->ParentKey = command.m_device.Key;
  // Immutable sampler handles referenced in pBindings.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->DependencyKeys.push_back(dep);
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDescriptorSetLayoutCommand& command) {
  // Mark as Destroyed but keep the state.  Descriptor set layouts may be
  // Destroyed after vkCreatePipelineLayout (Vulkan allows this).  During state
  // restore we must re-create the layout before re-creating any pipeline layout
  // that references it, so the state must remain available here.
  auto* state = m_StateTracking.GetState(command.m_descriptorSetLayout.Key);
  if (state) {
    state->Destroyed = true;
  }
}

void SubcaptureLayer::Post(vkCreateDescriptorPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorPoolState>();
  state->Key = command.m_pDescriptorPool.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::RemoveDescriptorSetsByPool(uint64_t poolKey) {
  // Reset / destroy reclaims every set allocated from this pool.  Walk only the
  // pool's own set index (maintained by Post(vkAllocateDescriptorSets) /
  // Post(vkFreeDescriptorSets)) so the cost is O(sets in this pool) rather than a
  // scan over every tracked object -- vkResetDescriptorPool is one of the hottest
  // calls in descriptor-churn-heavy titles (tens of millions of calls).
  auto* poolState = m_StateTracking.GetState<DescriptorPoolState>(poolKey);
  if (!poolState) {
    // Unknown / already-destroyed pool (e.g. a reset on a pool whose creation
    // failed or whose key was never tracked): nothing to remove.
    return;
  }
  // Empty-pool early-out: ~most resets target pools with no live sets, so skip
  // the (already empty) walk entirely.  The index is kept in lockstep with the
  // tracked sets, so empty here means there is genuinely nothing to remove.
  if (poolState->AllocatedSetKeys.empty()) {
    return;
  }
  for (uint64_t setKey : poolState->AllocatedSetKeys) {
    m_StateTracking.RemoveState(setKey);
    m_StateTracking.GetDescriptorSetUpdateService().RemoveDescriptorSet(setKey);
  }
  poolState->AllocatedSetKeys.clear();
  // vkResetDescriptorPool frees every set at once; the pool itself stays alive,
  // so zero the live count but keep PeakLiveSets (the all-time high-water mark).
  poolState->LiveSets = 0;
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

  // Sets allocated with a pNext chain (e.g. variable descriptor counts) cannot
  // be merged into a single batched restore allocation; flag them so state
  // restore allocates them individually.
  const bool allocHasPNext = command.m_pAllocateInfo.Value->pNext != nullptr;

  // Track per-pool live/peak set counts so state restore can size the re-created
  // pool from observed demand rather than a blind multiplier.  The same pool
  // state owns the per-pool set index used by RemoveDescriptorSetsByPool; grab
  // it once here and feed both.  std::map is node-based so this pointer stays
  // valid across the StoreState insertions below.
  auto* poolState = m_StateTracking.GetState<DescriptorPoolState>(poolKey);
  if (poolState) {
    poolState->LiveSets += command.m_pDescriptorSets.Size;
    if (poolState->LiveSets > poolState->PeakLiveSets) {
      poolState->PeakLiveSets = poolState->LiveSets;
    }
    // Grow the index's bucket count once for the whole batch.
    poolState->AllocatedSetKeys.reserve(poolState->AllocatedSetKeys.size() +
                                        command.m_pDescriptorSets.Size);
  }

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
    const uint64_t setKey = command.m_pDescriptorSets.Keys[i];
    auto state = std::make_unique<DescriptorSetState>();
    state->Key = setKey;
    state->ParentKey = command.m_device.Key;
    state->PoolKey = poolKey;
    // Index this set under its pool so reset/destroy can reclaim it without a
    // full state scan.  Only real (stored) keys are indexed; StoreState skips
    // key 0, so guarding here keeps the index == the set of tracked sets.
    // Keys are unique per allocated set, so this never inserts a duplicate.
    if (poolState && setKey) {
      poolState->AllocatedSetKeys.insert(setKey);
    }

    // HandleKeys layout in the original command: [0]=pool, [1+i]=pSetLayouts[i].
    // Add the layout as a dependency so RestoreOne restores it before emitting
    // vkAllocateDescriptorSets (which references the layout handle).
    const uint32_t layoutIdx = 1 + i;
    uint64_t layoutKey = 0;
    if (layoutIdx < command.m_pAllocateInfo.HandleKeys.size()) {
      layoutKey = command.m_pAllocateInfo.HandleKeys[layoutIdx];
      if (layoutKey) {
        state->DependencyKeys.push_back(layoutKey);
      }
    }
    state->LayoutKey = layoutKey;
    state->HasAllocPNext = allocHasPNext;

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
  auto* poolState = m_StateTracking.GetState<DescriptorPoolState>(command.m_descriptorPool.Key);
  if (poolState) {
    poolState->LiveSets = (command.m_descriptorSetCount.Value <= poolState->LiveSets)
                              ? poolState->LiveSets - command.m_descriptorSetCount.Value
                              : 0;
  }
  for (uint32_t i = 0; i < command.m_descriptorSetCount.Value; ++i) {
    const uint64_t setKey = command.m_pDescriptorSets.Keys[i];
    m_StateTracking.RemoveState(setKey);
    m_StateTracking.GetDescriptorSetUpdateService().RemoveDescriptorSet(setKey);
    // Keep the per-pool set index consistent.  unordered_set::erase is O(1)
    // average, so titles that free sets one at a time stay cheap.
    if (poolState) {
      poolState->AllocatedSetKeys.erase(setKey);
    }
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
  state->Key = command.m_pDescriptorUpdateTemplate.Key;
  state->ParentKey = command.m_device.Key;
  // VkDescriptorUpdateTemplateCreateInfo HandleKeys: [descriptorSetLayout,
  // pipelineLayout].  Either may be 0 (VK_NULL_HANDLE) depending on templateType.
  // Track the non-zero ones as dependencies so RestoreOne creates them before the
  // template and the analyzer keeps them in the restore closure (otherwise a
  // restored vkCreateDescriptorUpdateTemplate would reference a pruned layout key
  // and the player would assert in HandleMapService::GetHandle).
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->DependencyKeys.push_back(dep);
    }
  }
  m_StateTracking.GetDescriptorSetUpdateService().StoreTemplateEntries(
      command.m_pDescriptorUpdateTemplate.Key, command.m_pCreateInfo.Value);
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<DescriptorUpdateTemplateState>();
  state->Key = command.m_pDescriptorUpdateTemplate.Key;
  state->ParentKey = command.m_device.Key;
  // See vkCreateDescriptorUpdateTemplate: track non-zero descriptorSetLayout /
  // pipelineLayout keys so they survive analyzer pruning and restore ordering.
  for (uint64_t dep : command.m_pCreateInfo.HandleKeys) {
    if (dep) {
      state->DependencyKeys.push_back(dep);
    }
  }
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

void SubcaptureLayer::NotifyTemplateUpdateHandles(uint64_t templateKey,
                                                  const std::vector<char>& dataBytes) {
  if (!m_AnalyzerService) {
    return;
  }
  std::vector<uint64_t> embeddedKeys;
  m_StateTracking.GetDescriptorSetUpdateService().CollectTemplateUpdateHandleKeys(
      templateKey, dataBytes, embeddedKeys);
  m_AnalyzerService->AddObjectsForRestore(embeddedKeys);
}

void SubcaptureLayer::Post(vkUpdateDescriptorSetWithTemplateCommand& command) {
  // RemapHandles leaves arg.Data intact (GITSKeys) and patches arg.PatchedData,
  // so arg.Data is safe to read here in Post.
  auto& dsus = m_StateTracking.GetDescriptorSetUpdateService();
  dsus.TrackTemplateUpdate(command.m_descriptorSet.Key, command.m_descriptorUpdateTemplate.Key,
                           command.m_pData.Data);
  NotifyTemplateUpdateHandles(command.m_descriptorUpdateTemplate.Key, command.m_pData.Data);
}

void SubcaptureLayer::Post(vkUpdateDescriptorSetWithTemplateKHRCommand& command) {
  auto& dsus = m_StateTracking.GetDescriptorSetUpdateService();
  dsus.TrackTemplateUpdate(command.m_descriptorSet.Key, command.m_descriptorUpdateTemplate.Key,
                           command.m_pData.Data);
  NotifyTemplateUpdateHandles(command.m_descriptorUpdateTemplate.Key, command.m_pData.Data);
}

// ---- Sampler ------------------------------------------------------------

void SubcaptureLayer::Post(vkCreateSamplerCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SamplerState>();
  state->Key = command.m_pSampler.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySamplerCommand& command) {
  m_StateTracking.RemoveState(command.m_sampler.Key);
}

// ---- Sampler YCbCr conversion --------------------------------------------

void SubcaptureLayer::Post(vkCreateSamplerYcbcrConversionCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SamplerYcbcrConversionState>();
  state->Key = command.m_pYcbcrConversion.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkCreateSamplerYcbcrConversionKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SamplerYcbcrConversionState>();
  state->Key = command.m_pYcbcrConversion.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySamplerYcbcrConversionCommand& command) {
  m_StateTracking.RemoveState(command.m_ycbcrConversion.Key);
}

void SubcaptureLayer::Post(vkDestroySamplerYcbcrConversionKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_ycbcrConversion.Key);
}

// ---- Video sessions ------------------------------------------------------

void SubcaptureLayer::Post(vkCreateVideoSessionKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<VideoSessionState>();
  state->Key = command.m_pVideoSession.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyVideoSessionKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_videoSession.Key);
}

void SubcaptureLayer::Post(vkBindVideoSessionMemoryKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto* state = m_StateTracking.GetState<VideoSessionState>(command.m_videoSession.Key);
  if (!state) {
    return;
  }
  uint32_t size = GetSize(command);
  state->BindCommandBuffer.resize(size);
  Encode(command, state->BindCommandBuffer.data());
  for (auto key : command.m_pBindSessionMemoryInfos.HandleKeys) {
    if (key) {
      state->MemoryKeys.push_back(key);
    }
  }
}

void SubcaptureLayer::Post(vkCreateVideoSessionParametersKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<VideoSessionParametersState>();
  state->Key = command.m_pVideoSessionParameters.Key;
  state->ParentKey = command.m_device.Key;
  // CollectHandleKeys order for VkVideoSessionParametersCreateInfoKHR:
  //   keys[0] = videoSessionParametersTemplate, keys[1] = videoSession
  const auto& keys = command.m_pCreateInfo.HandleKeys;
  if (keys.size() >= 2 && keys[1]) {
    state->DependencyKeys.push_back(keys[1]); // videoSession must be restored first
  }
  if (keys.size() >= 1 && keys[0]) {
    state->DependencyKeys.push_back(keys[0]); // template, if present
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyVideoSessionParametersKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_videoSessionParameters.Key);
}

// ---- Command pool / buffers ----------------------------------------------

void SubcaptureLayer::Post(vkCreateCommandPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<CommandPoolState>();
  state->Key = command.m_pCommandPool.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    state->QueueFamilyIndex = command.m_pCreateInfo.Value->queueFamilyIndex;
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
  // HandleKeys layout for VkRenderPassBeginInfo: [renderPassKey, framebufferKey,
  // imagelessAttachmentKeys...].  keys[2..] are the image-view keys from
  // VkRenderPassAttachmentBeginInfo (only present for imageless framebuffers).
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1],
                                    {keys.begin() + 2, keys.end()});
  }
}

void SubcaptureLayer::Post(vkCmdBeginRenderPass2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderPassBegin.HandleKeys);
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1],
                                    {keys.begin() + 2, keys.end()});
  }
}

void SubcaptureLayer::Post(vkCmdBeginRenderPass2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pRenderPassBegin.HandleKeys);
  const auto& keys = command.m_pRenderPassBegin.HandleKeys;
  if (keys.size() >= 2) {
    m_ImageLayout.OnBeginRenderPass(command.m_commandBuffer.Key, keys[0], keys[1],
                                    {keys.begin() + 2, keys.end()});
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
// DependencyKeys so that RestoreOne guarantees the object is restored before
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
// recording) command buffer executes.  Applied to EventState::IsSignaled when
// the command buffer is submitted (see SyncStateService::OnQueueSubmit).
void SubcaptureLayer::RecordCmdEventState(uint64_t cbKey, uint64_t eventKey, bool signaled) {
  if (!cbKey || !eventKey) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (cb && cb->IsRecording) {
    cb->EventStatesAfterSubmit[eventKey] = signaled;
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
  // An event wait carries the same VkImageMemoryBarrier array as an explicit
  // pipeline barrier and can transition image layouts (e.g. TRANSFER_DST ->
  // SHADER_READ_ONLY).  These transitions must feed image-layout tracking, or
  // ImageState::CurrentLayout goes stale and state restore transitions images
  // to the wrong layout (VUID-vkCmdDraw-None-09600).
  m_ImageLayout.OnPipelineBarrier(command.m_commandBuffer.Key, command.m_pImageMemoryBarriers.Value,
                                  command.m_pImageMemoryBarriers.Size,
                                  command.m_pImageMemoryBarriers.HandleKeys);
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
  m_StateTracking.GetQueryPoolStateService().OnCmdUseQuery(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_query.Value);
}

void SubcaptureLayer::Post(vkCmdEndQueryCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_StateTracking.GetQueryPoolStateService().OnCmdUseQuery(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_query.Value);
}

void SubcaptureLayer::Post(vkCmdResetQueryPoolCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_StateTracking.GetQueryPoolStateService().OnCmdResetQueryPool(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_firstQuery.Value,
      command.m_queryCount.Value);
}

void SubcaptureLayer::Post(vkCmdWriteTimestampCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_StateTracking.GetQueryPoolStateService().OnCmdUseQuery(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_query.Value);
}

void SubcaptureLayer::Post(vkCmdWriteTimestamp2Command& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_StateTracking.GetQueryPoolStateService().OnCmdUseQuery(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_query.Value);
}

void SubcaptureLayer::Post(vkCmdWriteTimestamp2KHRCommand& command) {
  m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key,
                                                 command.m_queryPool.Key);
  m_StateTracking.GetQueryPoolStateService().OnCmdUseQuery(
      command.m_commandBuffer.Key, command.m_queryPool.Key, command.m_query.Value);
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
  // Fold each secondary's buffered query effects and image-layout transitions
  // into the primary so they are applied when the primary is submitted.
  for (uint64_t secondaryKey : command.m_pCommandBuffers.Keys) {
    m_StateTracking.GetQueryPoolStateService().MergeSecondary(command.m_commandBuffer.Key,
                                                              secondaryKey);
    m_ImageLayout.MergeSecondary(command.m_commandBuffer.Key, secondaryKey);
    m_StateTracking.MergeSecondaryAsInputReadbacks(command.m_commandBuffer.Key, secondaryKey);
    if (m_AnalyzerRaytracingService) {
      m_AnalyzerRaytracingService->MergeSecondary(command.m_commandBuffer.Key, secondaryKey);
    }
    if (m_RaytracingOptimizationService) {
      m_RaytracingOptimizationService->MergeSecondary(command.m_commandBuffer.Key, secondaryKey);
    }
  }
}

// ---- Acceleration structure build/copy dependency tracking -------------
// Beyond ordinary CB-dependency tracking, a build snapshots its own raw command bytes onto
// the destination AS state(s) for the rebuild-from-inputs content restore, and folds every
// source-AS key it can resolve into that AS's DependencyKeys. That single list is then
// consumed generically by RestoreOne's dependency walk and AnalyzerService::AddClosure.
namespace {
void ResolveAndTrackBufferAddress(StateTrackingService& stateTracking,
                                  VkDeviceAddress address,
                                  std::vector<uint64_t>& depKeys) {
  if (address == 0) {
    return;
  }
  auto found = stateTracking.GetDeviceAddressTracking().FindContaining(address);
  if (found) {
    depKeys.push_back(found->first);
  }
}

void CollectGeometryInputBufferKeys(StateTrackingService& stateTracking,
                                    const VkAccelerationStructureGeometryKHR& geometry,
                                    std::vector<uint64_t>& depKeys) {
  switch (geometry.geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
    const VkAccelerationStructureGeometryTrianglesDataKHR& tri = geometry.geometry.triangles;
    ResolveAndTrackBufferAddress(stateTracking, tri.vertexData.deviceAddress, depKeys);
    if (tri.indexType != VK_INDEX_TYPE_NONE_KHR) {
      ResolveAndTrackBufferAddress(stateTracking, tri.indexData.deviceAddress, depKeys);
    }
    ResolveAndTrackBufferAddress(stateTracking, tri.transformData.deviceAddress, depKeys);
    break;
  }
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    ResolveAndTrackBufferAddress(stateTracking, geometry.geometry.aabbs.data.deviceAddress,
                                 depKeys);
    break;
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    if (!geometry.geometry.instances.arrayOfPointers) {
      ResolveAndTrackBufferAddress(stateTracking, geometry.geometry.instances.data.deviceAddress,
                                   depKeys);
    }
    break;
  default:
    break;
  }
}

// A raw [Start, End) byte interval within an input buffer, before merging.
struct RawInputRegion {
  VkDeviceSize Start{};
  VkDeviceSize End{};
};

// Append the exact referenced byte range(s) of one geometry to regionsByBuffer, keyed by the
// owning buffer. Everything the range math needs - stride/count/format plus the
// per-geometry build range - is available at build-record time. Vertex spans round the last
// vertex up to a full stride, so they may run one trailing stride past a tightly-packed
// buffer. MergeInputRegions clamps that to the buffer size.
void ComputeGeometryInputRegions(StateTrackingService& stateTracking,
                                 const VkAccelerationStructureGeometryKHR& geometry,
                                 const VkAccelerationStructureBuildRangeInfoKHR& range,
                                 std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer) {
  auto addRegion = [&](VkDeviceAddress address, VkDeviceSize extraOffset, VkDeviceSize size) {
    if (address == 0 || size == 0) {
      return;
    }
    auto found = stateTracking.GetDeviceAddressTracking().FindContaining(address);
    if (!found) {
      return;
    }
    const VkDeviceSize start = found->second + extraOffset;
    regionsByBuffer[found->first].push_back({start, start + size});
  };

  switch (geometry.geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
    const VkAccelerationStructureGeometryTrianglesDataKHR& tri = geometry.geometry.triangles;
    if (tri.transformData.deviceAddress != 0) {
      addRegion(tri.transformData.deviceAddress, range.transformOffset,
                sizeof(VkTransformMatrixKHR));
    }
    if (tri.indexType != VK_INDEX_TYPE_NONE_KHR) {
      const VkDeviceSize elem = (tri.indexType == VK_INDEX_TYPE_UINT16) ? 2 : 4;
      addRegion(tri.indexData.deviceAddress, range.primitiveOffset,
                static_cast<VkDeviceSize>(range.primitiveCount) * 3 * elem);
      // Indexed: vertices are addressed at vertexData + stride*(firstVertex + index), where
      // maxVertex is already the highest *effective* index (VUID-...-10774), so firstVertex
      // must not be added on top of it. Span: [firstVertex, maxVertex + 1) strides.
      addRegion(tri.vertexData.deviceAddress,
                static_cast<VkDeviceSize>(range.firstVertex) * tri.vertexStride,
                (static_cast<VkDeviceSize>(tri.maxVertex) + 1 - range.firstVertex) *
                    tri.vertexStride);
    } else {
      // Non-indexed: vertices are addressed at
      // vertexData + primitiveOffset + stride*(firstVertex + i), i in [0, primitiveCount*3).
      addRegion(tri.vertexData.deviceAddress,
                range.primitiveOffset +
                    static_cast<VkDeviceSize>(range.firstVertex) * tri.vertexStride,
                static_cast<VkDeviceSize>(range.primitiveCount) * 3 * tri.vertexStride);
    }
    break;
  }
  case VK_GEOMETRY_TYPE_AABBS_KHR: {
    const VkAccelerationStructureGeometryAabbsDataKHR& aabbs = geometry.geometry.aabbs;
    addRegion(aabbs.data.deviceAddress, range.primitiveOffset,
              static_cast<VkDeviceSize>(range.primitiveCount) * aabbs.stride);
    break;
  }
  case VK_GEOMETRY_TYPE_INSTANCES_KHR: {
    const VkAccelerationStructureGeometryInstancesDataKHR& inst = geometry.geometry.instances;
    if (!inst.arrayOfPointers) {
      addRegion(inst.data.deviceAddress, range.primitiveOffset,
                static_cast<VkDeviceSize>(range.primitiveCount) *
                    sizeof(VkAccelerationStructureInstanceKHR));
    }
    // arrayOfPointers instance data is not captured (parity with the analyzer's
    // TLAS->BLAS discovery, which also skips it).
    break;
  }
  default:
    break;
  }
}

// Merge overlapping/adjacent raw regions into minimal sorted intervals, clamped to the
// buffer size. Hash is left 0, filled at submit-time readback.
std::vector<CapturedBuildInputRegion> MergeInputRegions(std::vector<RawInputRegion> regions,
                                                        VkDeviceSize bufferSize) {
  std::vector<CapturedBuildInputRegion> merged;
  if (regions.empty()) {
    return merged;
  }
  std::sort(regions.begin(), regions.end(),
            [](const RawInputRegion& a, const RawInputRegion& b) { return a.Start < b.Start; });
  VkDeviceSize curStart = regions[0].Start;
  VkDeviceSize curEnd = regions[0].End;
  // A region may run one trailing vertex-stride past a tightly-packed buffer. Clamping is
  // safe, since the GPU cannot read past the buffer end either.
  auto flush = [&]() {
    VkDeviceSize s = std::min(curStart, bufferSize);
    VkDeviceSize e = std::min(curEnd, bufferSize);
    if (e > s) {
      merged.push_back({s, e - s, 0});
    }
  };
  for (size_t i = 1; i < regions.size(); ++i) {
    if (regions[i].Start <= curEnd) {
      curEnd = std::max(curEnd, regions[i].End);
    } else {
      flush();
      curStart = regions[i].Start;
      curEnd = regions[i].End;
    }
  }
  flush();
  return merged;
}
} // namespace

void SubcaptureLayer::Pre(vkCmdBuildAccelerationStructuresKHRCommand& command) {
  // Record the input-buffer readback copies into the application's command buffer before the
  // build. Relevant only for subcapture run (not analysis).
  if (m_AnalysisMode || !m_AnalyzerResults.UseAsChainRestore()) {
    return;
  }
  const uint32_t infoCount = command.m_infoCount.Value;
  if (infoCount == 0 || !command.m_pInfos.Value ||
      command.m_pInfos.HandleKeys.size() < 2 * static_cast<size_t>(infoCount)) {
    return;
  }
  const uint64_t cbKey = command.m_commandBuffer.Key;
  auto* cbState = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cbState) {
    return;
  }
  auto* poolState = m_StateTracking.GetState<CommandPoolState>(cbState->PoolKey);
  if (!poolState) {
    return;
  }
  const uint64_t deviceKey = poolState->ParentKey;
  auto* devState = m_StateTracking.GetState<DeviceState>(deviceKey);
  if (!devState) {
    return;
  }
  const uint64_t physDevKey = devState->ParentKey;

  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkAccelerationStructureBuildGeometryInfoKHR& info = command.m_pInfos.Value[i];
    const uint64_t dstKey = command.m_pInfos.HandleKeys[2 * i + 1];
    if (!dstKey) {
      continue;
    }
    std::map<uint64_t, std::vector<RawInputRegion>> regionsByBuffer;
    for (uint32_t g = 0; g < info.geometryCount; ++g) {
      const VkAccelerationStructureGeometryKHR* geometry = nullptr;
      if (info.pGeometries) {
        geometry = &info.pGeometries[g];
      } else if (info.ppGeometries && info.ppGeometries[g]) {
        geometry = info.ppGeometries[g];
      }
      if (!geometry) {
        continue;
      }
      VkAccelerationStructureBuildRangeInfoKHR range{};
      if (i < command.m_ppBuildRangeInfos.Data.size() &&
          g < command.m_ppBuildRangeInfos.Data[i].size()) {
        range = command.m_ppBuildRangeInfos.Data[i][g];
      }
      ComputeGeometryInputRegions(m_StateTracking, *geometry, range, regionsByBuffer);
    }
    if (regionsByBuffer.empty()) {
      continue;
    }

    PendingAsInputReadback pending;
    pending.AsKey = dstKey;
    pending.CommandKey = command.m_Key;
    for (auto& [bufKey, raw] : regionsByBuffer) {
      // Destroyed buffers are retained only when they back an acceleration structure, and
      // their handles are dead, so a stale device-address hit means "no such buffer".
      auto* buf = m_StateTracking.GetState<BufferState>(bufKey);
      if (!buf || buf->Destroyed || buf->BufferSize == 0 || buf->BoundMemoryKey == 0) {
        continue;
      }
      auto* mem = m_StateTracking.GetState<DeviceMemoryState>(buf->BoundMemoryKey);
      if (!mem || mem->Destroyed) {
        continue;
      }
      CapturedBuildInputBuffer cbuf;
      cbuf.BufferKey = bufKey;
      cbuf.Size = buf->BufferSize;
      cbuf.BufferOpaqueCaptureAddress = buf->OpaqueCaptureAddress;
      cbuf.MemoryOpaqueCaptureAddress = mem->OpaqueCaptureAddress;
      cbuf.MemoryTypeIndex = mem->MemoryTypeIndex;
      cbuf.MemoryOffset = buf->MemoryOffset;
      cbuf.BaseDeviceAddress = buf->DeviceAddress;
      cbuf.Regions = MergeInputRegions(std::move(raw), buf->BufferSize);
      if (cbuf.Regions.empty()) {
        continue;
      }
      StagedInputReadback staging;
      if (!m_GpuReadbackHelper.StageBufferRegions(deviceKey, physDevKey, cbKey, bufKey,
                                                  cbuf.Regions, staging)) {
        LOG_WARNING << "Vulkan subcapture: failed to stage acceleration structure build input copy "
                       "(buffer key="
                    << bufKey << "); rebuild may be incomplete if this buffer is freed";
        continue;
      }
      pending.Buffers.push_back(std::move(cbuf));
      pending.Staging.push_back(staging);
    }
    if (!pending.Buffers.empty()) {
      cbState->AsInputReadbacksAfterSubmit.push_back(std::move(pending));
    }
  }
}

void SubcaptureLayer::RequireBlasChainForRaytracing(const char* commandName) {
  if (m_AnalysisMode || !m_SubcaptureRange.BeforeRange() ||
      !m_AnalyzerResults.CaptureAsBuildInputs() || m_AnalyzerResults.HasBlasChain()) {
    return;
  }
  FatalSubcaptureError(
      std::string("the stream calls ") + commandName +
      " before the subcapture range, which needs the analysis pass so the "
      "rtas chain can be captured. Run with Common.Player.Subcapture.Optimize=true, or set "
      "Common.Player.Subcapture.Vulkan.CaptureASBuildInputs=false to restore "
      "acceleration structures from serialized blobs (replayable only on this GPU and driver)");
}

void SubcaptureLayer::Post(vkCmdBuildAccelerationStructuresKHRCommand& command) {
  const uint32_t infoCount = command.m_infoCount.Value;
  if (infoCount == 0 || !command.m_pInfos.Value ||
      command.m_pInfos.HandleKeys.size() < 2 * static_cast<size_t>(infoCount)) {
    return;
  }

  RequireBlasChainForRaytracing("vkCmdBuildAccelerationStructuresKHR");

  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pInfos.HandleKeys);

  std::vector<uint64_t> dstKeys;
  for (uint32_t i = 0; i < infoCount; ++i) {
    uint64_t dstKey = command.m_pInfos.HandleKeys[2 * i + 1];
    if (dstKey) {
      dstKeys.push_back(dstKey);
    }
  }

  uint32_t sz = GetSize(command);
  std::vector<char> encoded(sz);
  Encode(command, encoded.data());
  const CommandId cmdId = command.GetId();

  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkAccelerationStructureBuildGeometryInfoKHR& info = command.m_pInfos.Value[i];
    uint64_t srcKey = command.m_pInfos.HandleKeys[2 * i];
    uint64_t dstKey = command.m_pInfos.HandleKeys[2 * i + 1];
    if (!dstKey) {
      continue;
    }
    auto* dstState = m_StateTracking.GetState<AccelerationStructureState>(dstKey);
    if (!dstState) {
      continue;
    }
    RefuseGenericAccelerationStructure(dstKey, dstState->Type);

    // Analysis pass: feed the chain-reduction graph with this pre-range BLAS build/update.
    // TLAS builds are skipped, being rebuilt in-range with their BLASes pulled in via
    // GetReferencedBlases, as are in-range and post-range builds, which replay live.
    if (m_AnalysisMode && m_RaytracingOptimizationService && m_SubcaptureRange.BeforeRange() &&
        info.type != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR) {
      const bool isUpdate = info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
      m_RaytracingOptimizationService->RecordBuild(command.m_commandBuffer.Key, command.m_Key,
                                                   dstKey, srcKey, isUpdate);
    }

    // Resolve all buffers this build touches for command-buffer lifecycle tracking, but
    // deliberately not for the AS's DependencyKeys: the app may destroy the scratch and
    // geometry buffers right after a one-time build, and gating AS creation or the analyzer
    // closure on them would wrongly drop the AS.
    std::vector<uint64_t> cbDepKeys;
    if (srcKey && srcKey != dstKey) {
      cbDepKeys.push_back(srcKey);
    }
    ResolveAndTrackBufferAddress(m_StateTracking, info.scratchData.deviceAddress, cbDepKeys);

    // Resolve each geometry's input buffers for command-buffer lifecycle tracking. Scratch is
    // excluded, being regenerated fresh at restore. Also note whether this is an
    // array-of-pointers TLAS build, which cannot be rebuilt at restore.
    bool arrayOfPointers = false;
    for (uint32_t g = 0; g < info.geometryCount; ++g) {
      const VkAccelerationStructureGeometryKHR* geometry = nullptr;
      if (info.pGeometries) {
        geometry = &info.pGeometries[g];
      } else if (info.ppGeometries && info.ppGeometries[g]) {
        geometry = info.ppGeometries[g];
      }
      if (!geometry) {
        continue;
      }
      CollectGeometryInputBufferKeys(m_StateTracking, *geometry, cbDepKeys);
      if (geometry->geometryType == VK_GEOMETRY_TYPE_INSTANCES_KHR &&
          geometry->geometry.instances.arrayOfPointers != VK_FALSE) {
        arrayOfPointers = true;
      }
    }

    for (uint64_t dep : cbDepKeys) {
      m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, dep);
    }
    // An update-mode source acceleration structure (src != dst) is a real dependency: it is
    // persistent, unlike an input buffer, and must be restored and rebuilt before this one.
    if (srcKey && srcKey != dstKey &&
        m_StateTracking.GetState<AccelerationStructureState>(srcKey)) {
      dstState->DependencyKeys.push_back(srcKey);
    }
    dstState->LastBuildCommandId = cmdId;
    dstState->LastBuildCommandBytes = encoded;
    dstState->LastBuildSiblingAsKeys = dstKeys;
    dstState->ArrayOfPointersInstances = arrayOfPointers;
  }

  // Recording pass: keep the whole command's bytes for chain replay if it is a
  // retained BLAS op (no-op in analysis mode / without a loaded BlasChain).
  m_StateTracking.StoreRetainedAsCommandBytes(command.m_Key, encoded, /*isCopy=*/false);
}

// VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR is currently not supported.
void SubcaptureLayer::RefuseGenericAccelerationStructure(uint64_t asKey,
                                                         VkAccelerationStructureTypeKHR type) {
  if (type != VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR) {
    return;
  }
  if (!m_SubcaptureRange.BeforeRange() && !m_SubcaptureRange.InRange()) {
    return; // after the range: cannot affect the subcapture
  }
  FatalSubcaptureError(
      "the stream writes acceleration structure key=" + std::to_string(asKey) +
      ", which was created with VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR. Subcapture restores "
      "only top-level and bottom-level structures");
}

void SubcaptureLayer::RefuseUnsupportedRaytracingCommand(const char* commandName) {
  if (!m_SubcaptureRange.BeforeRange() && !m_SubcaptureRange.InRange()) {
    return; // after the range: cannot affect the subcapture
  }
  FatalSubcaptureError(
      std::string("the stream calls ") + commandName +
      ", which subcapture does not track. Acceleration structure and micromap content "
      "written by it cannot be restored, and structures and buffers it reads are invisible "
      "to the analysis pass, so they may be dropped from the restore set. Only "
      "vkCmdBuildAccelerationStructuresKHR and vkCmdCopyAccelerationStructureKHR are "
      "supported so far; subcapturing this stream is not possible yet");
}

// Untracked writers of acceleration structure or micromap content. See the declarations
// for why they are refused rather than handled.
void SubcaptureLayer::Post(vkCmdBuildAccelerationStructuresIndirectKHRCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdBuildAccelerationStructuresIndirectKHR");
}

void SubcaptureLayer::Post(vkBuildAccelerationStructuresKHRCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkBuildAccelerationStructuresKHR");
}

void SubcaptureLayer::Post(vkCopyAccelerationStructureKHRCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCopyAccelerationStructureKHR");
}

void SubcaptureLayer::Post(vkCmdCopyMemoryToAccelerationStructureKHRCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdCopyMemoryToAccelerationStructureKHR");
}

void SubcaptureLayer::Post(vkCopyMemoryToAccelerationStructureKHRCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCopyMemoryToAccelerationStructureKHR");
}

void SubcaptureLayer::Post(vkCmdBuildAccelerationStructureNVCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdBuildAccelerationStructureNV");
}

void SubcaptureLayer::Post(vkCmdCopyAccelerationStructureNVCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdCopyAccelerationStructureNV");
}

void SubcaptureLayer::Post(vkCmdBuildMicromapsEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdBuildMicromapsEXT");
}

void SubcaptureLayer::Post(vkBuildMicromapsEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkBuildMicromapsEXT");
}

void SubcaptureLayer::Post(vkCmdCopyMicromapEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdCopyMicromapEXT");
}

void SubcaptureLayer::Post(vkCopyMicromapEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCopyMicromapEXT");
}

void SubcaptureLayer::Post(vkCmdCopyMemoryToMicromapEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCmdCopyMemoryToMicromapEXT");
}

void SubcaptureLayer::Post(vkCopyMemoryToMicromapEXTCommand& command) {
  RefuseUnsupportedRaytracingCommand("vkCopyMemoryToMicromapEXT");
}

void SubcaptureLayer::Post(vkCmdCopyAccelerationStructureKHRCommand& command) {
  RequireBlasChainForRaytracing("vkCmdCopyAccelerationStructureKHR");

  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pInfo.HandleKeys);

  // HandleKeys mirrors VkCopyAccelerationStructureInfoKHR field order: [src, dst].
  const uint64_t dstKey =
      command.m_pInfo.HandleKeys.size() >= 2 ? command.m_pInfo.HandleKeys[1] : 0;
  auto* dstState = m_StateTracking.GetState<AccelerationStructureState>(dstKey);
  if (dstState) {
    RefuseGenericAccelerationStructure(dstKey, dstState->Type);
  }

  // Recording pass: keep this copy's bytes for chain replay if it is a retained
  // BLAS op (CLONE/COMPACT). Encode once.
  {
    uint32_t sz = GetSize(command);
    std::vector<char> encoded(sz);
    Encode(command, encoded.data());
    m_StateTracking.StoreRetainedAsCommandBytes(command.m_Key, encoded, /*isCopy=*/true);
  }

  // Analysis pass: feed the chain-reduction graph with this pre-range copy, which can only be
  // CLONE/COMPACT here (the serialize variants are separate commands). TLAS destinations are
  // skipped, not being part of the BLAS restore.
  if (m_AnalysisMode && m_RaytracingOptimizationService && m_SubcaptureRange.BeforeRange() &&
      command.m_pInfo.Value && command.m_pInfo.HandleKeys.size() >= 2 && dstState &&
      dstState->Type != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR) {
    m_RaytracingOptimizationService->RecordCopy(command.m_commandBuffer.Key, command.m_Key, dstKey,
                                                command.m_pInfo.HandleKeys[0],
                                                command.m_pInfo.Value->mode);
  }
}

namespace {
void ResolveShaderBindingTableRegion(StateTrackingService& stateTracking,
                                     const PointerArgument<VkStridedDeviceAddressRegionKHR>& region,
                                     std::vector<uint64_t>& depKeys) {
  if (region.Value) {
    ResolveAndTrackBufferAddress(stateTracking, region.Value->deviceAddress, depKeys);
  }
}
} // namespace

void SubcaptureLayer::Post(vkCmdTraceRaysKHRCommand& command) {
  std::vector<uint64_t> depKeys;
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pRaygenShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pMissShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pHitShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pCallableShaderBindingTable, depKeys);
  for (uint64_t dep : depKeys) {
    m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, dep);
  }
}

void SubcaptureLayer::Post(vkCmdTraceRaysIndirectKHRCommand& command) {
  std::vector<uint64_t> depKeys;
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pRaygenShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pMissShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pHitShaderBindingTable, depKeys);
  ResolveShaderBindingTableRegion(m_StateTracking, command.m_pCallableShaderBindingTable, depKeys);
  ResolveAndTrackBufferAddress(m_StateTracking, command.m_indirectDeviceAddress.Value, depKeys);
  for (uint64_t dep : depKeys) {
    m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, dep);
  }
}

// ---- Image layout tracking ---------------------------------------------

void SubcaptureLayer::Post(vkCmdPipelineBarrierCommand& command) {
  m_ImageLayout.OnPipelineBarrier(command.m_commandBuffer.Key, command.m_pImageMemoryBarriers.Value,
                                  command.m_pImageMemoryBarriers.Size,
                                  command.m_pImageMemoryBarriers.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pBufferMemoryBarriers.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pImageMemoryBarriers.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdPipelineBarrier2Command& command) {
  if (command.m_pDependencyInfo.Value) {
    m_ImageLayout.OnPipelineBarrier2(command.m_commandBuffer.Key, *command.m_pDependencyInfo.Value,
                                     command.m_pDependencyInfo.HandleKeys);
  }
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfo.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdPipelineBarrier2KHRCommand& command) {
  if (command.m_pDependencyInfo.Value) {
    m_ImageLayout.OnPipelineBarrier2(command.m_commandBuffer.Key, *command.m_pDependencyInfo.Value,
                                     command.m_pDependencyInfo.HandleKeys);
  }
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfo.HandleKeys);
}

void SubcaptureLayer::TrackWaitEvents2ImageLayouts(uint64_t cbKey,
                                                   const VkDependencyInfo* dependencyInfos,
                                                   uint32_t infoCount,
                                                   const std::vector<uint64_t>& handleKeys) {
  if (dependencyInfos == nullptr) {
    return;
  }
  // CollectHandleKeys (handleArgumentUpdatersAuto.cpp) appends, per dependency
  // info, the buffer-barrier buffer keys followed by the image-barrier image
  // keys, and the array updater concatenates those across all events.  A
  // VkDependencyInfo pNext chain could in theory contribute extra keys, which
  // would desync the per-element offsets below; guard against that by checking
  // the total against the barrier counts and skipping layout tracking (rather
  // than risk mis-assigning a key) when they disagree.
  uint64_t expectedKeys = 0;
  for (uint32_t i = 0; i < infoCount; ++i) {
    expectedKeys += dependencyInfos[i].bufferMemoryBarrierCount;
    expectedKeys += dependencyInfos[i].imageMemoryBarrierCount;
  }
  if (expectedKeys != handleKeys.size()) {
    return;
  }
  uint32_t offset = 0;
  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkDependencyInfo& info = dependencyInfos[i];
    const uint32_t elemKeyCount = info.bufferMemoryBarrierCount + info.imageMemoryBarrierCount;
    // OnPipelineBarrier2 expects this element's keys laid out as
    // [buffers][images], which is exactly this slice of the flat array.
    const std::vector<uint64_t> elemKeys(handleKeys.begin() + offset,
                                         handleKeys.begin() + offset + elemKeyCount);
    m_ImageLayout.OnPipelineBarrier2(cbKey, info, elemKeys);
    offset += elemKeyCount;
  }
}

void SubcaptureLayer::Post(vkCmdWaitEvents2Command& command) {
  // A sync2 event wait carries its image-layout transitions inside the
  // per-event VkDependencyInfo array, just like vkCmdPipelineBarrier2.  Without
  // feeding these into image-layout tracking, ImageState::CurrentLayout goes
  // stale and state restore transitions images to the wrong layout
  // (VUID-vkCmdDraw-None-09600) -- the sync2 counterpart of the sync1
  // vkCmdWaitEvents fix above.
  TrackWaitEvents2ImageLayouts(command.m_commandBuffer.Key, command.m_pDependencyInfos.Value,
                               command.m_pDependencyInfos.Size,
                               command.m_pDependencyInfos.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pEvents.Keys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfos.HandleKeys);
}

void SubcaptureLayer::Post(vkCmdWaitEvents2KHRCommand& command) {
  TrackWaitEvents2ImageLayouts(command.m_commandBuffer.Key, command.m_pDependencyInfos.Value,
                               command.m_pDependencyInfos.Size,
                               command.m_pDependencyInfos.HandleKeys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pEvents.Keys);
  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pDependencyInfos.HandleKeys);
}

// ---- Swapchain / surface -------------------------------------------------
void SubcaptureLayer::Post(CreateWindowMetaCommand& command) {
  // Cache the window geometry so it can be associated with the next surface
  // creation command.  The player emits this token before each
  // vkCreate*SurfaceKHR; we store it here and attach it to the SurfaceState
  // so RestoreState can re-emit the window command before the surface.
  m_PendingWindow.Protocol = command.m_DisplayProtocol.Value;
  m_PendingWindow.X = command.m_X.Value;
  m_PendingWindow.Y = command.m_Y.Value;
  m_PendingWindow.Width = command.m_Width.Value;
  m_PendingWindow.Height = command.m_Height.Value;
  m_PendingWindow.Visible = command.m_Visible.Value;
  m_PendingWindow.HwndKey = command.m_Hwnd.Value;
  m_PendingWindow.HinstanceKey = command.m_Hinstance.Value;
  m_PendingWindow.Valid = true;
}

void SubcaptureLayer::Post(UpdateWindowMetaCommand& command) {
  // The recorder emits an UpdateWindowMetaCommand whenever the application's
  // window changes (per present), keyed by HWND.  SurfaceState holds the window
  // geometry that RestoreSurface re-emits at the subcapture boundary; without
  // folding these updates in, restore would resurrect the day-one geometry
  // captured at vkCreate*SurfaceKHR (e.g. a hidden / wrong-size / mispositioned
  // window) instead of the actual state at the restore point.  Patch every
  // matching SurfaceState in place so the latest values win, mirroring how
  // ImageState/MappedMemory absorb their latest mutations during the pre-range
  // pass.
  //
  // Value-space caveat: CreateWindowMetaRunner rewrites the command's hwnd to the
  // player's runtime HWND (commandRunnersCustom.cpp) *before* the post-layers
  // run, so SurfaceState::HwndKey holds the PLAYBACK hwnd.  UpdateWindowMetaRunner
  // performs no such rewrite, so this command still carries the original CAPTURE
  // hwnd.  Translate through WindowService -- the same capture->playback map the
  // CreateWindowMeta runner populated -- before matching; fall back to the raw
  // value when no mapping exists (e.g. a non-executing pass, where HwndKey is the
  // capture hwnd too).
  const uint64_t playbackHwnd =
      PlayerManager::Get().GetWindowService().GetCurrentWindowHandle(command.m_Hwnd.Value);
  const uint64_t targetHwnd = playbackHwnd != 0 ? playbackHwnd : command.m_Hwnd.Value;
  for (const auto& [key, statePtr] : m_StateTracking.GetStates()) {
    auto* surf = dynamic_cast<SurfaceState*>(statePtr.get());
    if (surf == nullptr || surf->HwndKey != targetHwnd) {
      continue;
    }
    surf->WindowX = command.m_X.Value;
    surf->WindowY = command.m_Y.Value;
    surf->WindowWidth = command.m_Width.Value;
    surf->WindowHeight = command.m_Height.Value;
    surf->WindowVisible = command.m_Visible.Value;
  }
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void SubcaptureLayer::Post(vkCreateWin32SurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->Key = command.m_pSurface.Key;
  state->ParentKey = command.m_instance.Key;
  if (m_PendingWindow.Valid) {
    state->Protocol = m_PendingWindow.Protocol;
    state->WindowX = m_PendingWindow.X;
    state->WindowY = m_PendingWindow.Y;
    state->WindowWidth = m_PendingWindow.Width;
    state->WindowHeight = m_PendingWindow.Height;
    state->WindowVisible = m_PendingWindow.Visible;
    state->HwndKey = m_PendingWindow.HwndKey;
    state->HinstanceKey = m_PendingWindow.HinstanceKey;
    m_PendingWindow.Valid = false;
  }
  StoreState(std::move(state), command);
}
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
void SubcaptureLayer::Post(vkCreateXcbSurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->Key = command.m_pSurface.Key;
  state->ParentKey = command.m_instance.Key;
  if (m_PendingWindow.Valid) {
    state->Protocol = m_PendingWindow.Protocol;
    state->WindowX = m_PendingWindow.X;
    state->WindowY = m_PendingWindow.Y;
    state->WindowWidth = m_PendingWindow.Width;
    state->WindowHeight = m_PendingWindow.Height;
    state->WindowVisible = m_PendingWindow.Visible;
    state->HwndKey = m_PendingWindow.HwndKey;
    state->HinstanceKey = m_PendingWindow.HinstanceKey;
    m_PendingWindow.Valid = false;
  }
  StoreState(std::move(state), command);
}
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
void SubcaptureLayer::Post(vkCreateXlibSurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->Key = command.m_pSurface.Key;
  state->ParentKey = command.m_instance.Key;
  if (m_PendingWindow.Valid) {
    state->Protocol = m_PendingWindow.Protocol;
    state->WindowX = m_PendingWindow.X;
    state->WindowY = m_PendingWindow.Y;
    state->WindowWidth = m_PendingWindow.Width;
    state->WindowHeight = m_PendingWindow.Height;
    state->WindowVisible = m_PendingWindow.Visible;
    state->HwndKey = m_PendingWindow.HwndKey;
    state->HinstanceKey = m_PendingWindow.HinstanceKey;
    m_PendingWindow.Valid = false;
  }
  StoreState(std::move(state), command);
}
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
void SubcaptureLayer::Post(vkCreateWaylandSurfaceKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<SurfaceState>();
  state->Key = command.m_pSurface.Key;
  state->ParentKey = command.m_instance.Key;
  if (m_PendingWindow.Valid) {
    state->Protocol = m_PendingWindow.Protocol;
    state->WindowX = m_PendingWindow.X;
    state->WindowY = m_PendingWindow.Y;
    state->WindowWidth = m_PendingWindow.Width;
    state->WindowHeight = m_PendingWindow.Height;
    state->WindowVisible = m_PendingWindow.Visible;
    state->HwndKey = m_PendingWindow.HwndKey;
    state->HinstanceKey = m_PendingWindow.HinstanceKey;
    m_PendingWindow.Valid = false;
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
  state->Key = command.m_pSwapchain.Key;
  state->ParentKey = command.m_device.Key;
  // Surface must be restored before the swapchain; add it as a dependency.
  if (!command.m_pCreateInfo.HandleKeys.empty() && command.m_pCreateInfo.HandleKeys[0] != 0) {
    state->DependencyKeys.push_back(command.m_pCreateInfo.HandleKeys[0]);
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroySwapchainKHRCommand& command) {
  auto* swapchainState = m_StateTracking.GetState<SwapchainState>(command.m_swapchain.Key);
  if (swapchainState) {
    for (auto imageKey : swapchainState->ImageKeys) {
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

  // Swapchain images have no vkCreateImageCommand of their own, so
  // MaySubmitImageOperationOnQueueFamily (stateTrackingService.cpp) would otherwise see
  // every swapchain ImageState with the ImageState defaults (EXCLUSIVE, no
  // ConcurrentFamilies) even when the application requested
  // VK_SHARING_MODE_CONCURRENT in VkSwapchainCreateInfoKHR. Decode the swapchain's
  // retained creation command (the same encode/decode round-trip RestoreImage uses
  // for CreationCommandBuffer) to recover the real imageSharingMode /
  // pQueueFamilyIndices and propagate them onto each image below, so family-routing
  // for swapchain images works the same as for regular ones.
  VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  std::vector<uint32_t> concurrentFamilies;
  if (!swapchainState->CreationCommandBuffer.empty()) {
    std::vector<char> scratch = swapchainState->CreationCommandBuffer;
    vkCreateSwapchainKHRCommand createCmd;
    Decode(scratch.data(), createCmd);
    if (createCmd.m_pCreateInfo.Value) {
      imageSharingMode = createCmd.m_pCreateInfo.Value->imageSharingMode;
      if (imageSharingMode == VK_SHARING_MODE_CONCURRENT &&
          createCmd.m_pCreateInfo.Value->pQueueFamilyIndices != nullptr) {
        concurrentFamilies.assign(createCmd.m_pCreateInfo.Value->pQueueFamilyIndices,
                                  createCmd.m_pCreateInfo.Value->pQueueFamilyIndices +
                                      createCmd.m_pCreateInfo.Value->queueFamilyIndexCount);
      }
    }
  }

  for (uint32_t i = 0; i < command.m_pSwapchainImages.Size; ++i) {
    uint64_t imgKey = command.m_pSwapchainImages.Keys[i];

    // Swapchain images are not explicitly created - track them here.
    if (!m_StateTracking.HasState(imgKey)) {
      swapchainState->ImageKeys.push_back(imgKey);
      auto state = std::make_unique<ImageState>();
      state->Key = imgKey;
      state->ParentKey = command.m_swapchain.Key;
      state->SharingMode = imageSharingMode;
      state->ConcurrentFamilies = concurrentFamilies;
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
    sc->AcquiredImages.insert(*command.m_pImageIndex.Value);
  }
  // The acquire signals the binary semaphore; track it so state restore
  // re-signals it for any first recorded submit that waits on it.
  m_SyncState.OnImageAcquired(command.m_semaphore.Key);
  // The acquire also signals the fence (if supplied).  Without tracking it, a
  // fence signaled by a pre-cut acquire is restored unsignaled and a first
  // recorded vkGetFenceStatus / vkWaitForFences poll on it hangs forever.
  m_SyncState.OnFenceSignaled(command.m_fence.Key);
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
    sc->AcquiredImages.insert(*command.m_pImageIndex.Value);
  }
  // The acquire signals the binary semaphore; track it (see vkAcquireNextImageKHR).
  if (command.m_pAcquireInfo.HandleKeys.size() > 1) {
    m_SyncState.OnImageAcquired(command.m_pAcquireInfo.HandleKeys[1]);
  }
  // The acquire also signals the fence (HandleKeys[2]) if supplied; track it so
  // a pre-cut acquire fence is restored signaled (see vkAcquireNextImageKHR).
  if (command.m_pAcquireInfo.HandleKeys.size() > 2) {
    m_SyncState.OnFenceSignaled(command.m_pAcquireInfo.HandleKeys[2]);
  }
}

// ---- Query pool ----------------------------------------------------------

void SubcaptureLayer::Post(vkCreateQueryPoolCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<QueryPoolState>();
  state->Key = command.m_pQueryPool.Key;
  state->ParentKey = command.m_device.Key;
  const uint64_t poolKey = command.m_pQueryPool.Key;
  uint32_t queryType = 0;
  uint32_t queryCount = 0;
  if (command.m_pCreateInfo.Value) {
    queryType = static_cast<uint32_t>(command.m_pCreateInfo.Value->queryType);
    queryCount = command.m_pCreateInfo.Value->queryCount;
  }
  StoreState(std::move(state), command);
  m_StateTracking.GetQueryPoolStateService().OnCreateQueryPool(poolKey, queryType, queryCount);
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
  state->Key = command.m_pAccelerationStructure.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->Type = ci.type;
    state->Offset = ci.offset;
    state->Size = ci.size;
    // Unlike VkBuffer, the opaque capture/replay address is a direct
    // create-info field here, not a pNext extension struct.
    state->OpaqueCaptureAddress = ci.deviceAddress;
    // A successful capture/replay create places the acceleration structure at this exact
    // device address. Track it immediately so analysis of an already-subcaptured stream can
    // resolve TLAS instance references even when that stream predates the explicit address
    // query emitted by state restore below.
    if (ci.deviceAddress != 0) {
      state->DeviceAddress = ci.deviceAddress;
      m_StateTracking.GetDeviceAddressTracking().TrackAccelerationStructure(ci.deviceAddress,
                                                                            state->Key);
    }
  }
  // VkAccelerationStructureCreateInfoKHR::buffer is the only handle member.
  if (!command.m_pCreateInfo.HandleKeys.empty()) {
    state->BufferKey = command.m_pCreateInfo.HandleKeys[0];
    state->DependencyKeys.push_back(state->BufferKey);
    // Flag the storage buffer, and the memory it is bound to (binding always precedes AS
    // creation), so their states survive the application's destroy/free and RestoreBlasChain
    // can re-create them transiently.
    if (auto* bufState = m_StateTracking.GetState<BufferState>(state->BufferKey)) {
      bufState->AsBacking = true;
      if (auto* memState = m_StateTracking.GetState<DeviceMemoryState>(bufState->BoundMemoryKey)) {
        memState->AsBacking = true;
      }
    }
  }
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyAccelerationStructureKHRCommand& command) {
  // A TLAS instance reference must never resolve to a dead AS. Chain replay reaches a
  // destroyed source by key, not by address, so it keeps working.
  m_StateTracking.GetDeviceAddressTracking().UntrackAccelerationStructure(
      command.m_accelerationStructure.Key);
  // Keep the state (flagged Destroyed) instead of erasing it. An application that compacts a
  // BLAS destroys the uncompacted intermediate right after the copy that reads it. Without
  // the state the chain replay has no creation blob for that source and would emit the copy
  // with an unmapped source handle.
  auto* state =
      m_StateTracking.GetState<AccelerationStructureState>(command.m_accelerationStructure.Key);
  if (state) {
    state->Destroyed = true;
  }
}

void SubcaptureLayer::Post(vkGetAccelerationStructureDeviceAddressKHRCommand& command) {
  if (command.m_Return.Value == 0 || command.m_pInfo.HandleKeys.empty()) {
    return;
  }
  const uint64_t asKey = command.m_pInfo.HandleKeys[0];
  auto* state = m_StateTracking.GetState<AccelerationStructureState>(asKey);
  if (!state) {
    return;
  }
  state->DeviceAddress = command.m_Return.Value;
  m_StateTracking.GetDeviceAddressTracking().TrackAccelerationStructure(command.m_Return.Value,
                                                                        asKey);
}

void SubcaptureLayer::Post(vkCreateAccelerationStructureNVCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<AccelerationStructureState>();
  state->Key = command.m_pAccelerationStructure.Key;
  state->ParentKey = command.m_device.Key;
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
  state->Key = command.m_pDeferredOperation.Key;
  state->ParentKey = command.m_device.Key;
  StoreState(std::move(state), command);
}

void SubcaptureLayer::Post(vkDestroyDeferredOperationKHRCommand& command) {
  m_StateTracking.RemoveState(command.m_operation.Key);
}

} // namespace vulkan
} // namespace gits
