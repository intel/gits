// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "commandSerializersFactory.h"
#include "commandCodersAuto.h"
#include "commandIdsAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "log.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>

namespace gits {
namespace vulkan {

namespace {

// Maximum queue families we query in one synthetic vkGetPhysicalDeviceQueueFamilyProperties
// during state restore.  Vulkan guarantees a small count (spec uses implementation-dependent
// upper bound; 64 matches legacy restore practice and all known drivers).
constexpr uint32_t kQueueFamilyQueryCapacity = 64;

// Mirrors Vulkan/recorder/vulkanStateRestore.cpp RestoreVkDevices: emit
// vkGetPhysicalDeviceQueueFamilyProperties before vkCreateDevice so validation and
// player-side remapping see up-to-date queue family counts on the replayed physical device.
void EmitGetPhysicalDeviceQueueFamilyProperties(SubcaptureRecorder& recorder,
                                                uint64_t physicalDeviceKey) {
  if (!physicalDeviceKey) {
    return;
  }
  static uint32_t s_queueFamilyCount;
  static VkQueueFamilyProperties s_queueFamilies[kQueueFamilyQueryCapacity];
  s_queueFamilyCount = kQueueFamilyQueryCapacity;
  std::memset(s_queueFamilies, 0, sizeof(s_queueFamilies));

  vkGetPhysicalDeviceQueueFamilyPropertiesCommand cmd{};
  cmd.m_physicalDevice.Key = physicalDeviceKey;
  cmd.m_pQueueFamilyPropertyCount.Value = &s_queueFamilyCount;
  cmd.m_pQueueFamilyProperties.Value = s_queueFamilies;
  cmd.m_pQueueFamilyProperties.Size = kQueueFamilyQueryCapacity;
  recorder.Record(vkGetPhysicalDeviceQueueFamilyPropertiesSerializer(cmd));
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

StateTrackingService::StateTrackingService(SubcaptureRecorder& recorder) : m_Recorder(recorder) {}

// ---------------------------------------------------------------------------
// StoreState / RemoveState / HasState
// ---------------------------------------------------------------------------

void StateTrackingService::StoreState(std::unique_ptr<ObjectState> state) {
  uint64_t key = state->Key;
  m_States[key] = std::move(state);
}

void StateTrackingService::RemoveState(uint64_t key) {
  m_States.erase(key);
}

bool StateTrackingService::HasState(uint64_t key) const {
  return m_States.count(key) != 0;
}

void StateTrackingService::EnsureRestored(uint64_t key) {
  if (!key) {
    return;
  }
  ObjectState* state = GetState(key);
  if (!state) {
    return;
  }
  RestoreOne(state);
}

// ---------------------------------------------------------------------------
// RestoreState -- public entry point
// ---------------------------------------------------------------------------

void StateTrackingService::RestoreState() {
  if (!m_Recorder.IsOpen()) {
    LOG_WARNING << "Vulkan2 subcapture: RestoreState called but recorder is not open";
    return;
  }

  LOG_INFO << "Vulkan2 subcapture: emitting state restore (" << m_States.size() << " objects)";

  m_RestoredThisPass.clear();
  m_DescriptorSetsAllocated.clear();
  m_CommandBuffersRecordingReplaySkipped.clear();
  m_TransientlyRestored.clear();

  // Emit StateRestoreBegin marker
  {
    StateRestoreBeginCommand cmd;
    m_Recorder.Record(StateRestoreBeginSerializer(cmd));
  }

  // Restore order mirrors legacy CState::Schedule (vulkanStateRestore.h): pipeline
  // objects are compiled *before* RestoreCommandBuffers. A single map-order pass
  // interleaves vkAllocateCommandBuffers (CB replay) with other objects; delaying
  // pipeline restore until after CB emits vkBegin/vkCmd* would write creates after
  // binds in the restore stream. Split into:
  //   (1) everything except vkAllocateCommandBuffers,
  //   (2) any unrestored graphics/compute pipeline (including Destroyed=True),
  //   (3) vkAllocateCommandBuffers only.
  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed) {
      continue;
    }
    if (state->CreationCommandId == CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
      continue;
    }
    RestoreOne(state);
  }

  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    const bool isVkPipelineCreate =
        state->CreationCommandId == CommandId::ID_VKCREATEGRAPHICSPIPELINES ||
        state->CreationCommandId == CommandId::ID_VKCREATECOMPUTEPIPELINES ||
        state->CreationCommandId == CommandId::ID_VKCREATERAYTRACINGPIPELINESKHR ||
        state->CreationCommandId == CommandId::ID_VKCREATERAYTRACINGPIPELINESNV;
    if (!isVkPipelineCreate) {
      continue;
    }
    if (m_RestoredThisPass.count(state->Key)) {
      continue;
    }
    RestoreOne(state);
  }

  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed) {
      continue;
    }
    if (state->CreationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
      continue;
    }
    RestoreOne(state);
  }

  // Destroy any objects that were transiently re-created as dependencies of
  // pipelines but had been Destroyed by the app before the subcapture point.
  // This mirrors the old Vulkan state-restore "temporaryShaderModules" logic.
  // VkRenderPass is omitted from m_TransientlyRestored (see RestoreOne): restored RP handles stay
  // live because vkCmdBeginRenderPass blobs reference them by key after pipelines compile.
  // VkPipeline is intentionally omitted: unlike shader modules, a pipeline may
  // remain referenced by restored vkCmd* blobs and subsequent submits; destroying
  // it here would break replay after StateRestoreEnd.
  for (uint64_t key : m_TransientlyRestored) {
    ObjectState* state = GetState(key);
    if (!state) {
      continue;
    }
    if (state->CreationCommandId == CommandId::ID_VKCREATESHADERMODULE) {
      vkDestroyShaderModuleCommand destroyCmd;
      destroyCmd.m_device.Key = state->ParentKey;
      destroyCmd.m_shaderModule.Key = state->Key;
      m_Recorder.Record(vkDestroyShaderModuleSerializer(destroyCmd));
    } else if (state->CreationCommandId == CommandId::ID_VKCREATEPIPELINELAYOUT) {
      vkDestroyPipelineLayoutCommand destroyCmd;
      destroyCmd.m_device.Key = state->ParentKey;
      destroyCmd.m_pipelineLayout.Key = state->Key;
      m_Recorder.Record(vkDestroyPipelineLayoutSerializer(destroyCmd));
    } else if (state->CreationCommandId == CommandId::ID_VKCREATEDESCRIPTORSETLAYOUT) {
      vkDestroyDescriptorSetLayoutCommand destroyCmd;
      destroyCmd.m_device.Key = state->ParentKey;
      destroyCmd.m_descriptorSetLayout.Key = state->Key;
      m_Recorder.Record(vkDestroyDescriptorSetLayoutSerializer(destroyCmd));
    }
  }
  m_TransientlyRestored.clear();

  // Signal binary semaphores that were in signaled state at the subcapture
  // point (signaled via a queue submit but not subsequently waited on).
  // Mirrors the old Vulkan state-restore RestoreSemaphores() logic.
  {
    // device key -> list of semaphore keys to signal
    std::unordered_map<uint64_t, std::vector<uint64_t>> semaphoresToSignal;
    for (auto& [_, statePtr] : m_States) {
      ObjectState* state = statePtr.get();
      if (state->Destroyed) {
        continue;
      }
      if (state->CreationCommandId != CommandId::ID_VKCREATESEMAPHORE) {
        continue;
      }
      auto* sem = static_cast<SemaphoreState*>(state);
      if (sem->IsBinary && sem->IsSignaled) {
        semaphoresToSignal[sem->ParentKey].push_back(sem->Key);
      }
    }

    for (auto& [deviceKey, semKeys] : semaphoresToSignal) {
      // Find a queue that belongs to this device.
      uint64_t queueKey = 0;
      for (auto& [_, statePtr2] : m_States) {
        ObjectState* state = statePtr2.get();
        if (state->Destroyed) {
          continue;
        }
        if ((state->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE ||
             state->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE2) &&
            state->ParentKey == deviceKey) {
          queueKey = state->Key;
          break;
        }
      }
      if (!queueKey) {
        LOG_WARNING << "Vulkan2 subcapture: cannot signal semaphores for device key=" << deviceKey
                    << " (no queue found)";
        continue;
      }

      // Build a VkSubmitInfo with pSignalSemaphores.
      // pSignalSemaphores must be non-null so that the player's handle
      // remapping code processes the HandleKeys entries.  The actual
      // semaphore handles are supplied via HandleKeys and remapped to live
      // handles by the player framework.
      static const VkSemaphore kDummySemaphoreSlot = VK_NULL_HANDLE;
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(semKeys.size());
      submitInfo.pSignalSemaphores = &kDummySemaphoreSlot; // non-null sentinel

      vkQueueSubmitCommand submitCmd;
      submitCmd.m_queue.Key = queueKey;
      submitCmd.m_fence.Key = 0;
      submitCmd.m_Return.Value = VK_SUCCESS;
      submitCmd.m_submitCount.Value = 1;
      submitCmd.m_pSubmits.Value = &submitInfo;
      submitCmd.m_pSubmits.Size = 1;
      // HandleKeys layout for VkSubmitInfo: [waitSem*], [cmdBuf*], [signalSem*]
      // We have no wait semaphores and no command buffers, so only signal keys.
      for (uint64_t semKey : semKeys) {
        submitCmd.m_pSubmits.HandleKeys.push_back(semKey);
      }
      m_Recorder.Record(vkQueueSubmitSerializer(submitCmd));
    }
  }

  // Re-set events that were signaled at the subcapture point.  The original
  // signal (host vkSetEvent or a device vkCmdSetEvent that executed before the
  // cut) is not part of the recording range, so without this a recording-range
  // vkGetEventStatus / vkCmdWaitEvents that polls for the set state would hang
  // forever.  Mirrors the legacy Vulkan RestoreEvents() logic.
  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed || state->CreationCommandId != CommandId::ID_VKCREATEEVENT) {
      continue;
    }
    if (!static_cast<EventState*>(state)->IsSignaled) {
      continue;
    }
    vkSetEventCommand setCmd;
    setCmd.m_device.Key = state->ParentKey;
    setCmd.m_event.Key = state->Key;
    setCmd.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkSetEventSerializer(setCmd));
  }

  // Restore GPU-local resource contents.  Must run after all objects are
  // created (m_RestoredThisPass is fully populated) and before
  // EmitImageLayoutTransitions (which skips images whose copy ends in the
  // correct layout already).
  if (m_GpuReadbackHelper) {
    RestoreBufferContents();
    RestoreImageContents();
  }

  EmitImageLayoutTransitions();

  // Restore query-pool contents (reset + fake-fill written queries) so the
  // recording range can read results for queries that were written before the
  // cut.  Emitted before the device-wait-idle loop below so its transient
  // submits are drained along with the layout-transition submits.
  RestoreQueryPools();

  // Drain all pre-recording-range GPU work before the recording range begins.
  // Mirrors legacy Vulkan::FinishStateRestore (Vulkan/recorder/vulkanStateRestore.cpp
  // ~4206 lines):
  //   for each device: drvVk.vkDeviceWaitIdle(...) + scheduler.Register(CvkDeviceWaitIdle(...));
  // Without this, helper submits emitted earlier in restore (binary-semaphore
  // re-signal submits with fence=0, queue submits for image-layout transitions
  // whose vkQueueWaitIdle is only per-queue, etc.) can still be in flight when
  // the first recorded command executes, producing a cascade of validation
  // errors around semaphore / fence / command-buffer / swapchain state:
  //   vkAcquireNextImageKHR-semaphore-01779, vkQueueSubmit-pWaitSemaphores-03238,
  //   vkQueueSubmit-pSignalSemaphores-00067, vkBeginCommandBuffer-commandBuffer-00049,
  //   vkResetFences-pFences-01123, vkQueueSubmit-pCommandBuffers-00071,
  //   VkPresentInfoKHR-pImageIndices-01430.
  // One vkDeviceWaitIdle per device covers every queue on that device, which
  // matches legacy's per-device loop.
  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed) {
      continue;
    }
    if (state->CreationCommandId != CommandId::ID_VKCREATEDEVICE) {
      continue;
    }
    vkDeviceWaitIdleCommand waitCmd;
    waitCmd.m_device.Key = state->Key;
    waitCmd.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkDeviceWaitIdleSerializer(waitCmd));
  }

  // Emit StateRestoreEnd marker
  {
    StateRestoreEndCommand cmd;
    m_Recorder.Record(StateRestoreEndSerializer(cmd));
  }

  // NOTE: do NOT call m_Recorder.FinishRecording() here.
  // The stream must remain open so that in-range commands (draw calls,
  // queue submissions, FrameEnd markers) can be written after state restore.
  LOG_INFO << "Vulkan2 subcapture: state restore complete";
}

// ---------------------------------------------------------------------------
// RestoreOne -- recursive, parent-first
// ---------------------------------------------------------------------------

void StateTrackingService::RestoreOne(ObjectState* state) {
  if (!state) {
    return;
  }
  // Skip if already fully restored (idempotency guard).
  if (m_RestoredThisPass.count(state->Key)) {
    return;
  }

  // If this is a Destroyed object being restored as a transient dependency
  // (e.g. a shader module needed only during pipeline creation), record it so
  // RestoreState can emit the matching destroy command afterwards.
  // VkRenderPass is excluded: vkCmdBeginRenderPass replay still needs the recreated handle; unlike
  // shader stages it remains referenced after pipeline compilation during command recording.
  if (state->Destroyed) {
    const CommandId cid = state->CreationCommandId;
    const bool isRenderPassCreate = cid == CommandId::ID_VKCREATERENDERPASS ||
                                    cid == CommandId::ID_VKCREATERENDERPASS2 ||
                                    cid == CommandId::ID_VKCREATERENDERPASS2KHR;
    if (!isRenderPassCreate) {
      m_TransientlyRestored.insert(state->Key);
    }
  }

  // Restore parent before child (device before buffer, pool before set, etc.)
  if (state->ParentKey) {
    RestoreOne(GetState(state->ParentKey));
  }

  // Restore sibling dependencies before emitting creation commands.  For
  // vkAllocateCommandBuffers we skip this: DependencyKeys list handles touched
  // during recording only; allocation depends on the pool (handled inside
  // RestoreCommandBuffers), while recording deps are restored immediately before
  // re-emitting vkBegin/vkCmd* there.
  if (state->CreationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
    for (uint64_t dep : state->DependencyKeys) {
      if (dep) {
        if (!HasState(dep)) {
          LOG_WARNING << "Vulkan2 subcapture: skipping object key=" << state->Key
                      << " (commandId=" << static_cast<uint32_t>(state->CreationCommandId)
                      << ") because dependency key=" << dep << " is no longer tracked";
          return;
        }
        RestoreOne(GetState(dep));
        if (!m_RestoredThisPass.count(dep)) {
          LOG_WARNING << "Vulkan2 subcapture: skipping object key=" << state->Key
                      << " (commandId=" << static_cast<uint32_t>(state->CreationCommandId)
                      << ") because dependency key=" << dep << " could not be restored.";
          return;
        }
      }
    }
  }

  // PhysicalDeviceState intentionally carries no recorded creation blob:
  // SubcaptureLayer doesn't encode the original vkEnumeratePhysicalDevices
  // because the same call would be re-emitted N times (once per PD) and
  // reference a stale instance key after a destroy/recreate sequence.
  // Instead, RestorePhysicalDevice synthesizes one enumerate per live parent
  // instance and marks every sibling PD restored.  Handled BEFORE the
  // empty-blob guard below.
  if (state->CreationCommandId == CommandId::ID_VKENUMERATEPHYSICALDEVICES) {
    if (!RestorePhysicalDevice(state)) {
      return;
    }
    // RestorePhysicalDevice already inserted the keys; the unconditional
    // insert at the end of this function would only re-insert idempotently,
    // but skip the pipeline-sibling block by returning early.
    m_RestoredThisPass.insert(state->Key);
    return;
  }

  if (state->CreationCommandBuffer.empty()) {
    // No creation command stored - nothing to emit and no handle to register.
    // Do NOT add to m_RestoredThisPass so that dependents correctly detect
    // the failure and skip themselves.
    return;
  }

  switch (state->CreationCommandId) {
  case CommandId::ID_VKCREATEBUFFER:
    if (!RestoreBuffer(state)) {
      return; // Do NOT insert into m_RestoredThisPass - dependents must skip this object.
    }
    break;
  case CommandId::ID_VKCREATEIMAGE:
    if (!RestoreImage(state)) {
      return; // Do NOT insert into m_RestoredThisPass - ImageViews for this image must be skipped.
    }
    break;
  case CommandId::ID_VKCREATEIMAGEVIEW:
    if (!RestoreImageView(state)) {
      return; // Do NOT insert into m_RestoredThisPass.
    }
    break;
  case CommandId::ID_VKCREATESWAPCHAINKHR:
    RestoreSwapchain(state);
    break;
  case CommandId::ID_VKALLOCATEDESCRIPTORSETS:
    if (!RestoreDescriptorSets(state)) {
      return; // Do NOT insert into m_RestoredThisPass - dependents must skip this object.
    }
    break;
  case CommandId::ID_VKALLOCATECOMMANDBUFFERS: {
    const CommandBufferRestoreOutcome cbOutcome = RestoreCommandBuffers(state);
    if (cbOutcome == CommandBufferRestoreOutcome::FailedNoAllocation) {
      return;
    }
    if (cbOutcome == CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped) {
      m_CommandBuffersRecordingReplaySkipped.insert(state->Key);
    }
    break;
  }
  case CommandId::ID_VKALLOCATEMEMORY:
    if (!EmitCreationCommand(state)) {
      return;
    }
    RestoreMappedMemory(state);
    break;
  case CommandId::ID_VKGETSWAPCHAINIMAGESKHR:
    // Swapchain images have no standalone creation command.
    // RestoreSwapchain inserts image keys into m_RestoredThisPass only when
    // vkGetSwapchainImagesKHR was actually emitted (emittedGetImages==true).
    // Use return (not break) to skip the unconditional insert below so that
    // a swapchain image key is never marked restored unless its handle was
    // actually registered in the player handle map.
    return;
  case CommandId::ID_VKCREATEWIN32SURFACEKHR:
    RestoreSurface(state);
    break;
#ifdef GITS_PLATFORM_X11
  case CommandId::ID_VKCREATEXCBSURFACEKHR:
    RestoreSurface(state);
    break;
  case CommandId::ID_VKCREATEXLIBSURFACEKHR:
    RestoreSurface(state);
    break;
#endif
  default:
    if (!EmitCreationCommand(state)) {
      return;
    }
    break;
  }

  // Object was successfully created (or is a swapchain image whose handle was
  // registered by RestoreSwapchain).  Mark as fully restored so dependents
  // can safely look up its handle in HandleMapService.
  m_RestoredThisPass.insert(state->Key);

  // For batch pipeline creation (createInfoCount > 1), all sibling handles
  // are produced by the same vkCreate*Pipelines command that was just emitted.
  // Mark every sibling as restored now so that subsequent RestoreOne calls for
  // other states in the same batch are no-ops and do not re-emit the full batch
  // command N times (once per pipeline), which would cause N×N pipeline objects
  // in the subcapture stream and corrupt handle-map entries for GPL library
  // pipelines that the link pipeline depends on.
  const bool isVkPipelineCreate =
      state->CreationCommandId == CommandId::ID_VKCREATEGRAPHICSPIPELINES ||
      state->CreationCommandId == CommandId::ID_VKCREATECOMPUTEPIPELINES ||
      state->CreationCommandId == CommandId::ID_VKCREATERAYTRACINGPIPELINESKHR ||
      state->CreationCommandId == CommandId::ID_VKCREATERAYTRACINGPIPELINESNV;
  if (isVkPipelineCreate) {
    auto* ps = static_cast<PipelineState*>(state);
    for (uint64_t sibKey : ps->BatchPipelineKeys) {
      m_RestoredThisPass.insert(sibKey);
    }
  }
}

// ---------------------------------------------------------------------------
// EmitImageLayoutTransitions
// ---------------------------------------------------------------------------

static VkImageAspectFlags AspectMaskFromFormat(VkFormat fmt) {
  switch (fmt) {
  case VK_FORMAT_D16_UNORM:
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D32_SFLOAT:
    return VK_IMAGE_ASPECT_DEPTH_BIT;
  case VK_FORMAT_S8_UINT:
    return VK_IMAGE_ASPECT_STENCIL_BIT;
  case VK_FORMAT_D16_UNORM_S8_UINT:
  case VK_FORMAT_D24_UNORM_S8_UINT:
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  default:
    return VK_IMAGE_ASPECT_COLOR_BIT;
  }
}

namespace {

// Find a queue key and command pool key that belong to deviceKey and share the
// same queue family index.  Submitting a CB allocated from pool family X to a
// queue from family Y is invalid; there is no fallback pairing when families
// cannot be matched.
static bool FindQueueAndPool(const std::map<uint64_t, std::unique_ptr<ObjectState>>& states,
                             uint64_t deviceKey,
                             uint64_t& outQueueKey,
                             uint64_t& outPoolKey) {
  outQueueKey = 0;
  outPoolKey = 0;

  std::unordered_map<uint32_t, uint64_t> familyToQueue;
  for (const auto& [k, sp] : states) {
    if (sp->Destroyed || sp->ParentKey != deviceKey) {
      continue;
    }
    if (sp->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE ||
        sp->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE2) {
      auto* qs = static_cast<QueueState*>(sp.get());
      if (qs->QueueFamilyIndex != UINT32_MAX) {
        familyToQueue.emplace(qs->QueueFamilyIndex, k);
      }
    }
  }

  if (familyToQueue.empty()) {
    return false;
  }

  for (const auto& [k, sp] : states) {
    if (sp->Destroyed || sp->ParentKey != deviceKey) {
      continue;
    }
    if (sp->CreationCommandId == CommandId::ID_VKCREATECOMMANDPOOL) {
      auto* ps = static_cast<CommandPoolState*>(sp.get());
      if (ps->QueueFamilyIndex == UINT32_MAX) {
        continue;
      }
      auto it = familyToQueue.find(ps->QueueFamilyIndex);
      if (it != familyToQueue.end()) {
        outQueueKey = it->second;
        outPoolKey = k;
        return true;
      }
    }
  }

  return false;
}

// Find a queue key and command pool key on deviceKey that both belong to the
// given queue family.  Used by query-pool restore, which must run on the same
// family the application used (and therefore a query-capable one), unlike the
// generic FindQueueAndPool above which returns the first family that pairs.
static bool FindQueueAndPoolForFamily(
    const std::map<uint64_t, std::unique_ptr<ObjectState>>& states,
    uint64_t deviceKey,
    uint32_t familyIndex,
    uint64_t& outQueueKey,
    uint64_t& outPoolKey) {
  outQueueKey = 0;
  outPoolKey = 0;
  if (familyIndex == UINT32_MAX) {
    return false;
  }

  for (const auto& [k, sp] : states) {
    if (sp->Destroyed || sp->ParentKey != deviceKey) {
      continue;
    }
    if ((sp->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE ||
         sp->CreationCommandId == CommandId::ID_VKGETDEVICEQUEUE2) &&
        outQueueKey == 0) {
      auto* qs = static_cast<QueueState*>(sp.get());
      if (qs->QueueFamilyIndex == familyIndex) {
        outQueueKey = k;
      }
    } else if (sp->CreationCommandId == CommandId::ID_VKCREATECOMMANDPOOL && outPoolKey == 0) {
      auto* ps = static_cast<CommandPoolState*>(sp.get());
      if (ps->QueueFamilyIndex == familyIndex) {
        outPoolKey = k;
      }
    }
  }

  return outQueueKey != 0 && outPoolKey != 0;
}

} // namespace

void StateTrackingService::EmitImageLayoutTransitions() {
  // Collect images per device that need a layout transition (i.e. their
  // layout at the subcapture point is neither UNDEFINED nor PREINITIALIZED).
  // The second player creates every image in UNDEFINED layout, so we must
  // transition them to the correct layout before the first recorded frame runs.
  std::unordered_map<uint64_t, std::vector<uint64_t>> imagesByDevice;

  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed) {
      continue;
    }
    if (state->CreationCommandId != CommandId::ID_VKCREATEIMAGE &&
        state->CreationCommandId != CommandId::ID_VKGETSWAPCHAINIMAGESKHR) {
      continue;
    }
    auto* img = static_cast<ImageState*>(state);

    if (img->ContentRestored) {
      continue;
    }

    // For regular images: skip UNDEFINED / PREINITIALIZED; the second player
    // creates them in UNDEFINED already so there is nothing to restore.
    // For swapchain images: always transition to PRESENT_SRC_KHR.  Their tracked
    // layout is reset to UNDEFINED by ImageLayoutService::OnQueuePresent (called
    // at vkQueuePresentKHR time), but the player may call vkQueuePresentKHR on
    // any acquired swapchain image during the index-rewind phase, so every
    // swapchain image must be in PRESENT_SRC_KHR when the subcapture starts.
    const bool isSwapchainImage =
        (state->CreationCommandId == CommandId::ID_VKGETSWAPCHAINIMAGESKHR);

    // Currently-acquired swapchain images are handled by the re-acquire calls
    // emitted in RestoreSwapchain; skip them here so we don't redundantly
    // transition an image the app already owns after the re-acquire.
    if (isSwapchainImage) {
      const auto* sc = GetState<SwapchainState>(img->ParentKey);
      if (sc) {
        bool isAcquired = false;
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(sc->ImageKeys.size()); ++idx) {
          if (sc->ImageKeys[idx] == state->Key) {
            isAcquired = sc->AcquiredImages.count(idx) != 0;
            break;
          }
        }
        if (isAcquired) {
          continue;
        }
      }
    }

    // For non-acquired swapchain images we will force PRESENT_SRC_KHR below in
    // the per-image barrier-building loop; for regular images we will reuse
    // the current tracked layout there.  We only need the skip decision here:
    // regular images already in UNDEFINED/PREINITIALIZED need no transition,
    // because the second player creates them in UNDEFINED.
    if (!isSwapchainImage && (img->CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
                              img->CurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)) {
      continue;
    }
    // Determine device key: direct parent for regular images; grandparent for
    // swapchain images whose parent is the swapchain key.
    uint64_t deviceKey = img->ParentKey;
    ObjectState* parent = GetState(deviceKey);
    if (parent && parent->CreationCommandId == CommandId::ID_VKCREATESWAPCHAINKHR) {
      deviceKey = parent->ParentKey;
    }
    if (deviceKey) {
      imagesByDevice[deviceKey].push_back(img->Key);
    }
  }

  if (imagesByDevice.empty()) {
    return;
  }

  // Synthetic high-value key for the temporary command buffer.  Must not
  // collide with any key assigned during the original recording.  Keys are
  // sequential integers starting from 1, so UINT64_MAX - 1 is safe.
  constexpr uint64_t kTempCBKey = static_cast<uint64_t>(-2);

  for (auto& [deviceKey, imgKeys] : imagesByDevice) {
    uint64_t queueKey = 0;
    uint64_t commandPoolKey = 0;
    if (!FindQueueAndPool(m_States, deviceKey, queueKey, commandPoolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: cannot emit image layout transitions for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(commandPoolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: queue or pool was not restored, skipping image "
                     "layout transitions for device key="
                  << deviceKey;
      continue;
    }

    // Allocate a temporary command buffer using the existing command pool.
    {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = 1;
      // allocInfo.commandPool is a handle (set to sentinel); the pool key
      // is in HandleKeys and will be remapped by the player.
      allocInfo.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL);

      static VkCommandBuffer kDummyCBSlot = VK_NULL_HANDLE;

      vkAllocateCommandBuffersCommand allocCmd;
      allocCmd.m_device.Key = deviceKey;
      allocCmd.m_pAllocateInfo.Value = &allocInfo;
      allocCmd.m_pAllocateInfo.HandleKeys = {commandPoolKey};
      allocCmd.m_pCommandBuffers.Value = &kDummyCBSlot;
      allocCmd.m_pCommandBuffers.Size = 1;
      allocCmd.m_pCommandBuffers.Keys = {kTempCBKey};
      allocCmd.m_Return.Value = VK_SUCCESS;
      m_Recorder.Record(vkAllocateCommandBuffersSerializer(allocCmd));
    }

    // Begin the temporary command buffer.
    {
      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      vkBeginCommandBufferCommand beginCmd;
      beginCmd.m_commandBuffer.Key = kTempCBKey;
      beginCmd.m_pBeginInfo.Value = &beginInfo;
      beginCmd.m_Return.Value = VK_SUCCESS;
      m_Recorder.Record(vkBeginCommandBufferSerializer(beginCmd));
    }

    // Build image memory barriers: UNDEFINED ? currentLayout for each image.
    std::vector<VkImageMemoryBarrier> barriers;
    std::vector<uint64_t> barrierImageKeys;

    static const VkImage kDummyImageSentinel = reinterpret_cast<VkImage>(0x1ULL);

    for (uint64_t imgKey : imgKeys) {
      auto* img = static_cast<ImageState*>(GetState(imgKey));
      if (!img) {
        continue;
      }
      const bool isSwapchain = (img->CreationCommandId == CommandId::ID_VKGETSWAPCHAINIMAGESKHR);
      VkImageLayout targetLayout;
      if (isSwapchain) {
        targetLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      } else {
        targetLayout = img->CurrentLayout;
      }
      VkImageMemoryBarrier b{};
      b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      b.srcAccessMask = 0;
      b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      b.newLayout = targetLayout;
      b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      b.image = kDummyImageSentinel; // non-null so the player's null-check passes
      b.subresourceRange.aspectMask = AspectMaskFromFormat(img->Format);
      b.subresourceRange.baseMipLevel = 0;
      b.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      b.subresourceRange.baseArrayLayer = 0;
      b.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      barriers.push_back(b);
      barrierImageKeys.push_back(imgKey);
    }

    if (!barriers.empty()) {
      vkCmdPipelineBarrierCommand barrierCmd;
      barrierCmd.m_commandBuffer.Key = kTempCBKey;
      barrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      barrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      barrierCmd.m_dependencyFlags.Value = 0;
      barrierCmd.m_memoryBarrierCount.Value = 0;
      barrierCmd.m_bufferMemoryBarrierCount.Value = 0;
      barrierCmd.m_imageMemoryBarrierCount.Value = static_cast<uint32_t>(barriers.size());
      barrierCmd.m_pImageMemoryBarriers.Value = barriers.data();
      barrierCmd.m_pImageMemoryBarriers.Size = static_cast<uint32_t>(barriers.size());
      barrierCmd.m_pImageMemoryBarriers.HandleKeys = barrierImageKeys;
      m_Recorder.Record(vkCmdPipelineBarrierSerializer(barrierCmd));
    }

    // End and submit the command buffer.
    {
      vkEndCommandBufferCommand endCmd;
      endCmd.m_commandBuffer.Key = kTempCBKey;
      endCmd.m_Return.Value = VK_SUCCESS;
      m_Recorder.Record(vkEndCommandBufferSerializer(endCmd));
    }

    {
      static VkCommandBuffer kDummyCBSlot2 = VK_NULL_HANDLE;
      kDummyCBSlot2 = reinterpret_cast<VkCommandBuffer>(kTempCBKey);

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &kDummyCBSlot2; // non-null sentinel

      vkQueueSubmitCommand submitCmd;
      submitCmd.m_queue.Key = queueKey;
      submitCmd.m_fence.Key = 0;
      submitCmd.m_Return.Value = VK_SUCCESS;
      submitCmd.m_submitCount.Value = 1;
      submitCmd.m_pSubmits.Value = &submitInfo;
      submitCmd.m_pSubmits.Size = 1;
      // HandleKeys layout: [waitSem*][cmdBuf*][signalSem*]); only one CB key.
      submitCmd.m_pSubmits.HandleKeys = {kTempCBKey};
      m_Recorder.Record(vkQueueSubmitSerializer(submitCmd));
    }

    // Wait for the GPU to finish before freeing the temporary command buffer.
    // Without this, the driver may still be executing the layout transitions
    // when vkFreeCommandBuffers is called, causing the validation layer to
    // lose track of the CB state and crash on the next vkWaitForFences.
    {
      vkQueueWaitIdleCommand waitCmd;
      waitCmd.m_queue.Key = queueKey;
      waitCmd.m_Return.Value = VK_SUCCESS;
      m_Recorder.Record(vkQueueWaitIdleSerializer(waitCmd));
    }

    // Free the temporary command buffer.
    {
      static VkCommandBuffer kDummyCBSlot3 = VK_NULL_HANDLE;

      vkFreeCommandBuffersCommand freeCmd;
      freeCmd.m_device.Key = deviceKey;
      freeCmd.m_commandPool.Key = commandPoolKey;
      freeCmd.m_commandBufferCount.Value = 1;
      freeCmd.m_pCommandBuffers.Value = &kDummyCBSlot3;
      freeCmd.m_pCommandBuffers.Size = 1;
      freeCmd.m_pCommandBuffers.Keys = {kTempCBKey};
      m_Recorder.Record(vkFreeCommandBuffersSerializer(freeCmd));
    }

    LOG_INFO << "Vulkan2 subcapture: emitted layout transitions for " << barriers.size()
             << " image(s) on device key=" << deviceKey;
  }
}

// ---------------------------------------------------------------------------
// RestoreQueryPools
// ---------------------------------------------------------------------------

void StateTrackingService::RestoreQueryPools() {
  // Collect, per device, the query pools that have at least one query written
  // before the subcapture cut (UsedQueries).  A pool whose queries are only
  // reset (never written) needs no restore: the recording range resets and
  // writes them itself before reading.  A pool with a *written* query, on the
  // other hand, leaves the recording range a result it never produced; on a
  // freshly created (uninitialized) pool the matching vkGetQueryPoolResults
  // returns VK_ERROR_DEVICE_LOST.
  // Grouped by device, then by the queue family the application used for the
  // pool's queries: each (device, family) pair gets one transient command
  // buffer allocated from a command pool of that family.
  std::unordered_map<uint64_t, std::map<uint32_t, std::vector<uint64_t>>> poolsByDeviceAndFamily;

  for (auto& [_, statePtr] : m_States) {
    ObjectState* state = statePtr.get();
    if (state->Destroyed) {
      continue;
    }
    if (state->CreationCommandId != CommandId::ID_VKCREATEQUERYPOOL) {
      continue;
    }
    auto* qp = static_cast<QueryPoolState*>(state);
    if (qp->QueryCount == 0) {
      continue;
    }
    bool anyUsed = false;
    for (bool used : qp->UsedQueries) {
      if (used) {
        anyUsed = true;
        break;
      }
    }
    if (!anyUsed) {
      continue;
    }
    // The pool itself must have been re-created this pass; otherwise the
    // player has no handle to remap our query commands onto.
    if (!m_RestoredThisPass.count(qp->Key)) {
      continue;
    }
    if (qp->RestoreQueueFamily == UINT32_MAX) {
      LOG_WARNING << "Vulkan2 subcapture: query pool key=" << qp->Key
                  << " has written queries but no recorded queue family; skipping its restore";
      continue;
    }
    if (qp->ParentKey) {
      poolsByDeviceAndFamily[qp->ParentKey][qp->RestoreQueueFamily].push_back(qp->Key);
    }
  }

  if (poolsByDeviceAndFamily.empty()) {
    return;
  }

  // Synthetic high-value key for the temporary command buffer; must not collide
  // with any recording key.  EmitImageLayoutTransitions already allocated and
  // freed its own (-2) CB before we run, but use a distinct value anyway.
  constexpr uint64_t kTempCBKey = static_cast<uint64_t>(-3);

  for (auto& [deviceKey, poolsByFamily] : poolsByDeviceAndFamily) {
    for (auto& [familyIndex, poolKeys] : poolsByFamily) {
      uint64_t queueKey = 0;
      uint64_t commandPoolKey = 0;
      if (!FindQueueAndPoolForFamily(m_States, deviceKey, familyIndex, queueKey, commandPoolKey)) {
        LOG_WARNING << "Vulkan2 subcapture: cannot restore query pools for device key=" << deviceKey
                    << " queue family=" << familyIndex
                    << " (no restored queue and command pool of that family)";
        continue;
      }
      if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(commandPoolKey)) {
        LOG_WARNING << "Vulkan2 subcapture: queue or pool was not restored, skipping query pool "
                       "restore for device key="
                    << deviceKey << " queue family=" << familyIndex;
        continue;
      }

      // Allocate a temporary command buffer from the existing command pool.
      {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL);

        static VkCommandBuffer kDummyCBSlot = VK_NULL_HANDLE;

        vkAllocateCommandBuffersCommand allocCmd;
        allocCmd.m_device.Key = deviceKey;
        allocCmd.m_pAllocateInfo.Value = &allocInfo;
        allocCmd.m_pAllocateInfo.HandleKeys = {commandPoolKey};
        allocCmd.m_pCommandBuffers.Value = &kDummyCBSlot;
        allocCmd.m_pCommandBuffers.Size = 1;
        allocCmd.m_pCommandBuffers.Keys = {kTempCBKey};
        allocCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkAllocateCommandBuffersSerializer(allocCmd));
      }

      // Begin the temporary command buffer.
      {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBufferCommand beginCmd;
        beginCmd.m_commandBuffer.Key = kTempCBKey;
        beginCmd.m_pBeginInfo.Value = &beginInfo;
        beginCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkBeginCommandBufferSerializer(beginCmd));
      }

      uint32_t fakeQueryCount = 0;
      for (uint64_t poolKey : poolKeys) {
        auto* qp = static_cast<QueryPoolState*>(GetState(poolKey));
        if (!qp || qp->QueryCount == 0) {
          continue;
        }

        // Reset the whole pool: a query must be in the post-reset state before it
        // can be written, and resetting unused queries is harmless (the recording
        // range resets again before its own use).
        {
          vkCmdResetQueryPoolCommand resetCmd;
          resetCmd.m_commandBuffer.Key = kTempCBKey;
          resetCmd.m_queryPool.Key = poolKey;
          resetCmd.m_firstQuery.Value = 0;
          resetCmd.m_queryCount.Value = qp->QueryCount;
          m_Recorder.Record(vkCmdResetQueryPoolSerializer(resetCmd));
        }

        // Issue a fake query for every query that was written before the cut so a
        // subsequent vkGetQueryPoolResults sees an available result.
        for (uint32_t i = 0; i < qp->QueryCount && i < qp->UsedQueries.size(); ++i) {
          if (!qp->UsedQueries[i]) {
            continue;
          }
          if (qp->QueryType == VK_QUERY_TYPE_TIMESTAMP) {
            vkCmdWriteTimestampCommand tsCmd;
            tsCmd.m_commandBuffer.Key = kTempCBKey;
            tsCmd.m_pipelineStage.Value = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            tsCmd.m_queryPool.Key = poolKey;
            tsCmd.m_query.Value = i;
            m_Recorder.Record(vkCmdWriteTimestampSerializer(tsCmd));
          } else {
            vkCmdBeginQueryCommand beginQueryCmd;
            beginQueryCmd.m_commandBuffer.Key = kTempCBKey;
            beginQueryCmd.m_queryPool.Key = poolKey;
            beginQueryCmd.m_query.Value = i;
            beginQueryCmd.m_flags.Value = 0;
            m_Recorder.Record(vkCmdBeginQuerySerializer(beginQueryCmd));

            vkCmdEndQueryCommand endQueryCmd;
            endQueryCmd.m_commandBuffer.Key = kTempCBKey;
            endQueryCmd.m_queryPool.Key = poolKey;
            endQueryCmd.m_query.Value = i;
            m_Recorder.Record(vkCmdEndQuerySerializer(endQueryCmd));
          }
          ++fakeQueryCount;
        }
      }

      // End and submit the command buffer.
      {
        vkEndCommandBufferCommand endCmd;
        endCmd.m_commandBuffer.Key = kTempCBKey;
        endCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkEndCommandBufferSerializer(endCmd));
      }

      {
        static VkCommandBuffer kDummyCBSlot2 = VK_NULL_HANDLE;
        kDummyCBSlot2 = reinterpret_cast<VkCommandBuffer>(kTempCBKey);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &kDummyCBSlot2;

        vkQueueSubmitCommand submitCmd;
        submitCmd.m_queue.Key = queueKey;
        submitCmd.m_fence.Key = 0;
        submitCmd.m_Return.Value = VK_SUCCESS;
        submitCmd.m_submitCount.Value = 1;
        submitCmd.m_pSubmits.Value = &submitInfo;
        submitCmd.m_pSubmits.Size = 1;
        submitCmd.m_pSubmits.HandleKeys = {kTempCBKey};
        m_Recorder.Record(vkQueueSubmitSerializer(submitCmd));
      }

      // Wait for the GPU before freeing the temporary command buffer.
      {
        vkQueueWaitIdleCommand waitCmd;
        waitCmd.m_queue.Key = queueKey;
        waitCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkQueueWaitIdleSerializer(waitCmd));
      }

      // Free the temporary command buffer.
      {
        static VkCommandBuffer kDummyCBSlot3 = VK_NULL_HANDLE;

        vkFreeCommandBuffersCommand freeCmd;
        freeCmd.m_device.Key = deviceKey;
        freeCmd.m_commandPool.Key = commandPoolKey;
        freeCmd.m_commandBufferCount.Value = 1;
        freeCmd.m_pCommandBuffers.Value = &kDummyCBSlot3;
        freeCmd.m_pCommandBuffers.Size = 1;
        freeCmd.m_pCommandBuffers.Keys = {kTempCBKey};
        m_Recorder.Record(vkFreeCommandBuffersSerializer(freeCmd));
      }

      LOG_INFO << "Vulkan2 subcapture: restored " << fakeQueryCount << " query result(s) across "
               << poolKeys.size() << " query pool(s) on device key=" << deviceKey
               << " queue family=" << familyIndex;
    }
  }
}

// ---------------------------------------------------------------------------
// EmitCreationCommand
// ---------------------------------------------------------------------------

bool StateTrackingService::EmitCreationCommand(ObjectState* state) {
  if (state->CreationCommandBuffer.empty()) {
    return false;
  }
  // Decode converts relative offsets to absolute pointers in-place via AddPtrs,
  // mutating the source buffer.  Work on a copy so CreationCommandBuffer remains
  // pristine and a second call (e.g. after a transient failure that left the key
  // out of m_RestoredThisPass) does not read already-rebased addresses as offsets.
  std::vector<char> scratch = state->CreationCommandBuffer;
  char* buf = scratch.data();

#define EMIT_DECODED(Prefix)                                                                       \
  {                                                                                                \
    Prefix##Command cmd;                                                                           \
    Decode(buf, cmd);                                                                              \
    m_Recorder.Record(Prefix##Serializer(cmd));                                                    \
    break;                                                                                         \
  }

  switch (state->CreationCommandId) {
  case CommandId::ID_VKCREATEINSTANCE:
    EMIT_DECODED(vkCreateInstance)
  case CommandId::ID_VKENUMERATEPHYSICALDEVICES:
    EMIT_DECODED(vkEnumeratePhysicalDevices)
  case CommandId::ID_VKENUMERATEPHYSICALDEVICEGROUPS:
    EMIT_DECODED(vkEnumeratePhysicalDeviceGroups)
  case CommandId::ID_VKENUMERATEPHYSICALDEVICEGROUPSKHR:
    EMIT_DECODED(vkEnumeratePhysicalDeviceGroupsKHR)
  case CommandId::ID_VKCREATEDEVICE:
    EmitGetPhysicalDeviceQueueFamilyProperties(m_Recorder, state->ParentKey);
    EMIT_DECODED(vkCreateDevice)
  case CommandId::ID_VKGETDEVICEQUEUE:
    EMIT_DECODED(vkGetDeviceQueue)
  case CommandId::ID_VKGETDEVICEQUEUE2:
    EMIT_DECODED(vkGetDeviceQueue2)
  case CommandId::ID_VKALLOCATEMEMORY:
    EMIT_DECODED(vkAllocateMemory)
  case CommandId::ID_VKCREATEBUFFER:
    EMIT_DECODED(vkCreateBuffer)
  case CommandId::ID_VKCREATEIMAGE:
    EMIT_DECODED(vkCreateImage)
  case CommandId::ID_VKCREATEBUFFERVIEW:
    EMIT_DECODED(vkCreateBufferView)
  case CommandId::ID_VKCREATEIMAGEVIEW:
    EMIT_DECODED(vkCreateImageView)
  case CommandId::ID_VKCREATERENDERPASS:
    EMIT_DECODED(vkCreateRenderPass)
  case CommandId::ID_VKCREATERENDERPASS2:
    EMIT_DECODED(vkCreateRenderPass2)
  case CommandId::ID_VKCREATERENDERPASS2KHR:
    EMIT_DECODED(vkCreateRenderPass2KHR)
  case CommandId::ID_VKCREATEFRAMEBUFFER:
    EMIT_DECODED(vkCreateFramebuffer)
  case CommandId::ID_VKCREATEPIPELINECACHE:
    EMIT_DECODED(vkCreatePipelineCache)
  case CommandId::ID_VKCREATEPIPELINELAYOUT:
    EMIT_DECODED(vkCreatePipelineLayout)
  case CommandId::ID_VKCREATESHADERMODULE:
    EMIT_DECODED(vkCreateShaderModule)
  case CommandId::ID_VKCREATEGRAPHICSPIPELINES: {
    vkCreateGraphicsPipelinesCommand cmd;
    Decode(buf, cmd);
    // pipelineCache is an optional optimization hint, not a dependency.  If the app
    // Destroyed it before the cut (its state was removed), or it otherwise failed to
    // restore, its key no longer resolves in the player handle map -- creating the
    // pipeline against it would crash vkCreateGraphicsPipelinesRunner.  A live cache
    // was already re-created in RestoreState's first pass and is kept; otherwise null
    // it so the pipeline is built without a cache (matching legacy RestorePipelines).
    if (cmd.m_pipelineCache.Key && !m_RestoredThisPass.count(cmd.m_pipelineCache.Key)) {
      cmd.m_pipelineCache.Key = 0;
      cmd.m_pipelineCache.Value = VK_NULL_HANDLE;
    }
    // Workaround for an Intel driver (igvk64) crash inside vkCreateGraphicsPipelines
    // when state-restoring GPL fast-path attempts.  The captured app uses the two-step
    // pattern: try a link with VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT
    // and fall back to a full link with VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT
    // on VK_PIPELINE_COMPILE_REQUIRED.  In the original capture (and in full-stream
    // replay) the driver cache is warmed by the surrounding command stream and the
    // fast-path succeeds.  During subcapture state restore all pipelines are emitted
    // back-to-back; the driver hits a code path inside its GPL fast-path lookup that
    // crashes at offset ~0xA6EE on some create infos (data is spec-valid; verified by
    // ReplayCustomizationLayer::Pre diagnostics).
    //
    // Legacy GITS already removed VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT
    // during state restore for the same reason.  We do the same here, and for GPL link
    // pipelines (link consumes libraries via VkPipelineLibraryCreateInfoKHR) we also
    // set VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT so the driver fully links
    // and properly propagates state (e.g. VkPipelineRenderingCreateInfo) from the
    // libraries rather than relying on the fast-path cache lookup.
    for (uint32_t i = 0; i < cmd.m_createInfoCount.Value; ++i) {
      auto& ci = const_cast<VkGraphicsPipelineCreateInfo&>(cmd.m_pCreateInfos.Value[i]);
      const bool hasFailOnCompile =
          (ci.flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT) != 0;
      if (!hasFailOnCompile) {
        continue;
      }
      // Detect GPL link pipeline: not itself a library (no LIBRARY_BIT_KHR) and
      // consumes one or more libraries via VkPipelineLibraryCreateInfoKHR in pNext.
      const bool isLibrary = (ci.flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR) != 0;
      bool consumesLibraries = false;
      if (!isLibrary) {
        const auto* node = reinterpret_cast<const VkBaseInStructure*>(ci.pNext);
        while (node) {
          if (node->sType == VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR) {
            const auto& lib = *reinterpret_cast<const VkPipelineLibraryCreateInfoKHR*>(node);
            if (lib.libraryCount > 0) {
              consumesLibraries = true;
              break;
            }
          }
          node = node->pNext;
        }
      }
      const VkPipelineCreateFlags originalFlags = ci.flags;
      ci.flags &= ~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
      if (consumesLibraries) {
        ci.flags |= VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT;
      }
      LOG_TRACE << "Vulkan2 subcapture: rewrote vkCreateGraphicsPipelines flags for state restore"
                << " stateKey=" << state->Key << " [" << i << "/" << cmd.m_createInfoCount.Value
                << "] 0x" << std::hex << originalFlags << " -> 0x" << ci.flags << std::dec
                << (consumesLibraries ? " (GPL link)" : " (standalone)");
    }
    m_Recorder.Record(vkCreateGraphicsPipelinesSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATECOMPUTEPIPELINES: {
    vkCreateComputePipelinesCommand cmd;
    Decode(buf, cmd);
    // See vkCreateGraphicsPipelines: null a pipeline cache that is no longer live.
    if (cmd.m_pipelineCache.Key && !m_RestoredThisPass.count(cmd.m_pipelineCache.Key)) {
      cmd.m_pipelineCache.Key = 0;
      cmd.m_pipelineCache.Value = VK_NULL_HANDLE;
    }
    // Same fast-path workaround as vkCreateGraphicsPipelines: strip
    // VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT during state restore
    // so the driver always produces a valid pipeline for the captured key.  Compute
    // pipelines have no GPL libraries, so no LINK_TIME_OPTIMIZATION bit is added.
    for (uint32_t i = 0; i < cmd.m_createInfoCount.Value; ++i) {
      auto& ci = const_cast<VkComputePipelineCreateInfo&>(cmd.m_pCreateInfos.Value[i]);
      if (ci.flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT) {
        const VkPipelineCreateFlags originalFlags = ci.flags;
        ci.flags &= ~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
        LOG_TRACE << "Vulkan2 subcapture: rewrote vkCreateComputePipelines flags for state restore"
                  << " stateKey=" << state->Key << " [" << i << "/" << cmd.m_createInfoCount.Value
                  << "] 0x" << std::hex << originalFlags << " -> 0x" << ci.flags << std::dec;
      }
    }
    m_Recorder.Record(vkCreateComputePipelinesSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATERAYTRACINGPIPELINESKHR: {
    vkCreateRayTracingPipelinesKHRCommand cmd;
    Decode(buf, cmd);
    // See vkCreateGraphicsPipelines: null a pipeline cache that is no longer live.
    if (cmd.m_pipelineCache.Key && !m_RestoredThisPass.count(cmd.m_pipelineCache.Key)) {
      cmd.m_pipelineCache.Key = 0;
      cmd.m_pipelineCache.Value = VK_NULL_HANDLE;
    }
    for (uint32_t i = 0; i < cmd.m_createInfoCount.Value; ++i) {
      auto& ci = const_cast<VkRayTracingPipelineCreateInfoKHR&>(cmd.m_pCreateInfos.Value[i]);
      if (ci.flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT) {
        const VkPipelineCreateFlags originalFlags = ci.flags;
        ci.flags &= ~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
        LOG_TRACE
            << "Vulkan2 subcapture: rewrote vkCreateRayTracingPipelinesKHR flags for state restore"
            << " stateKey=" << state->Key << " [" << i << "/" << cmd.m_createInfoCount.Value
            << "] 0x" << std::hex << originalFlags << " -> 0x" << ci.flags << std::dec;
      }
    }
    m_Recorder.Record(vkCreateRayTracingPipelinesKHRSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATERAYTRACINGPIPELINESNV: {
    vkCreateRayTracingPipelinesNVCommand cmd;
    Decode(buf, cmd);
    // See vkCreateGraphicsPipelines: null a pipeline cache that is no longer live.
    if (cmd.m_pipelineCache.Key && !m_RestoredThisPass.count(cmd.m_pipelineCache.Key)) {
      cmd.m_pipelineCache.Key = 0;
      cmd.m_pipelineCache.Value = VK_NULL_HANDLE;
    }
    for (uint32_t i = 0; i < cmd.m_createInfoCount.Value; ++i) {
      auto& ci = const_cast<VkRayTracingPipelineCreateInfoNV&>(cmd.m_pCreateInfos.Value[i]);
      if (ci.flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT) {
        const VkPipelineCreateFlags originalFlags = ci.flags;
        ci.flags &= ~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
        LOG_TRACE
            << "Vulkan2 subcapture: rewrote vkCreateRayTracingPipelinesNV flags for state restore"
            << " stateKey=" << state->Key << " [" << i << "/" << cmd.m_createInfoCount.Value
            << "] 0x" << std::hex << originalFlags << " -> 0x" << ci.flags << std::dec;
      }
    }
    m_Recorder.Record(vkCreateRayTracingPipelinesNVSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATEDESCRIPTORSETLAYOUT:
    EMIT_DECODED(vkCreateDescriptorSetLayout)
  case CommandId::ID_VKCREATEDESCRIPTORUPDATETEMPLATE:
    EMIT_DECODED(vkCreateDescriptorUpdateTemplate)
  case CommandId::ID_VKCREATEDESCRIPTORUPDATETEMPLATEKHR:
    EMIT_DECODED(vkCreateDescriptorUpdateTemplateKHR)
  case CommandId::ID_VKCREATEDESCRIPTORPOOL: {
    // Size the re-created pool from the observed peak demand, not a flat
    // multiplier.  The batched restore allocation (AllocateDescriptorSetBatchForPool)
    // packs the surviving sets coherently, but that alone is not enough for
    // heavily-churned FREE_DESCRIPTOR_SET pools: the seam between the restored
    // survivors and the recording-range alloc/free history still fragments the
    // driver's free list, so a later vkAllocateDescriptorSets can return
    // OUT_OF_POOL_MEMORY below maxSets.  The needed headroom scales with how hard
    // the pool is used (PeakLiveSets), which a flat percentage cannot express --
    // a small flat value starves busy pools, a large one bloats idle huge pools.
    // Formula: ~2x the peak simultaneously-live set count, never below the app's
    // own maxSets, capped at max(4x original, original + 256) so an idle huge
    // pool stays near original and a busy one cannot push vkCreateDescriptorPool
    // into OUT_OF_DEVICE_MEMORY.  (Core Vulkan defines no explicit device limit on
    // pool maxSets / sizes, so the cap is a bounded-growth + uint32 overflow guard.)
    vkCreateDescriptorPoolCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      auto* createInfo = const_cast<VkDescriptorPoolCreateInfo*>(cmd.m_pCreateInfo.Value);
      const uint32_t originalMaxSets = createInfo->maxSets;
      if (originalMaxSets > 0) {
        const uint32_t peak = static_cast<const DescriptorPoolState*>(state)->PeakLiveSets;
        uint64_t want = static_cast<uint64_t>(peak) + std::max<uint64_t>(peak, 64);
        want = std::max<uint64_t>(want, originalMaxSets);
        uint64_t ceiling = std::max<uint64_t>(static_cast<uint64_t>(originalMaxSets) * 4,
                                              static_cast<uint64_t>(originalMaxSets) + 256);
        ceiling = std::min<uint64_t>(ceiling, std::numeric_limits<uint32_t>::max());
        const uint32_t newMaxSets = static_cast<uint32_t>(std::min(want, ceiling));
        createInfo->maxSets = newMaxSets;

        // Scale per-type pool sizes by the same ratio so per-set descriptor
        // density is preserved (round up, clamp to uint32).
        auto* sizes = const_cast<VkDescriptorPoolSize*>(createInfo->pPoolSizes);
        for (uint32_t i = 0; i < createInfo->poolSizeCount && sizes; ++i) {
          const uint64_t scaled =
              (static_cast<uint64_t>(sizes[i].descriptorCount) * newMaxSets + originalMaxSets - 1) /
              originalMaxSets;
          sizes[i].descriptorCount = static_cast<uint32_t>(
              std::min<uint64_t>(scaled, std::numeric_limits<uint32_t>::max()));
        }
      }
    }
    m_Recorder.Record(vkCreateDescriptorPoolSerializer(cmd));
    break;
  }
  case CommandId::ID_VKALLOCATEDESCRIPTORSETS:
    EMIT_DECODED(vkAllocateDescriptorSets)
  case CommandId::ID_VKCREATESAMPLER:
    EMIT_DECODED(vkCreateSampler)
  case CommandId::ID_VKCREATECOMMANDPOOL:
    EMIT_DECODED(vkCreateCommandPool)
  case CommandId::ID_VKALLOCATECOMMANDBUFFERS:
    EMIT_DECODED(vkAllocateCommandBuffers)
#ifdef VK_USE_PLATFORM_WIN32_KHR
  case CommandId::ID_VKCREATEWIN32SURFACEKHR:
    EMIT_DECODED(vkCreateWin32SurfaceKHR)
#endif
#ifdef GITS_PLATFORM_X11
  case CommandId::ID_VKCREATEXCBSURFACEKHR:
    EMIT_DECODED(vkCreateXcbSurfaceKHR)
  case CommandId::ID_VKCREATEXLIBSURFACEKHR:
    EMIT_DECODED(vkCreateXlibSurfaceKHR)
#endif
  case CommandId::ID_VKCREATESWAPCHAINKHR:
    EMIT_DECODED(vkCreateSwapchainKHR)
  case CommandId::ID_VKGETSWAPCHAINIMAGESKHR:
    EMIT_DECODED(vkGetSwapchainImagesKHR)
  case CommandId::ID_VKCREATEQUERYPOOL:
    EMIT_DECODED(vkCreateQueryPool)
  case CommandId::ID_VKCREATEACCELERATIONSTRUCTUREKHR:
    EMIT_DECODED(vkCreateAccelerationStructureKHR)
  case CommandId::ID_VKCREATEACCELERATIONSTRUCTURENV:
    EMIT_DECODED(vkCreateAccelerationStructureNV)
  case CommandId::ID_VKCREATEDEFERREDOPERATIONKHR:
    EMIT_DECODED(vkCreateDeferredOperationKHR)
  case CommandId::ID_VKCREATEEVENT:
    EMIT_DECODED(vkCreateEvent)
  case CommandId::ID_VKCREATEFENCE: {
    vkCreateFenceCommand cmd;
    Decode(buf, cmd);
    // If the fence was signaled at the subcapture point (submitted to a queue
    // and not subsequently reset), recreate it in the signaled state so that
    // any vkWaitForFences in the first recorded frame does not hang.
    if (static_cast<FenceState*>(state)->IsSignaled) {
      if (cmd.m_pCreateInfo.Value) {
        cmd.m_pCreateInfo.Value->flags |= VK_FENCE_CREATE_SIGNALED_BIT;
      }
    }
    m_Recorder.Record(vkCreateFenceSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATESEMAPHORE:
    EMIT_DECODED(vkCreateSemaphore)
  default:
    LOG_WARNING << "Vulkan2 subcapture: unhandled CommandId "
                << static_cast<uint32_t>(state->CreationCommandId)
                << " for object key=" << state->Key
                << " (nothing emitted; RestoreOne must not mark this object restored)";
    return false;
  }
#undef EMIT_DECODED
  return true;
}

// ---------------------------------------------------------------------------
// Per-type special-case restores
// ---------------------------------------------------------------------------

bool StateTrackingService::RestorePhysicalDevice(ObjectState* state) {
  // The parent VkInstance must already be restored (RestoreOne walks parents
  // first).  If it could not be restored we cannot enumerate.
  if (!state->ParentKey || !m_RestoredThisPass.count(state->ParentKey)) {
    return false;
  }

  // Gather every live PhysicalDeviceState that shares this parent instance.
  // Sort the keys so the synthesized command's HandleKeys are deterministic
  // across runs (helps diff-debugging restore streams).
  std::vector<uint64_t> pdKeys;
  for (const auto& [key, statePtr] : m_States) {
    if (statePtr->CreationCommandId != CommandId::ID_VKENUMERATEPHYSICALDEVICES) {
      continue;
    }
    if (statePtr->Destroyed || statePtr->ParentKey != state->ParentKey) {
      continue;
    }
    pdKeys.push_back(key);
  }
  if (pdKeys.empty()) {
    // Defensive: state we were called for must have appeared in the loop.
    return false;
  }

  // Build a synthetic vkEnumeratePhysicalDevices.  The encoder needs non-null
  // Value pointers for HandleArrayOutputArgument / PointerArgument, but the
  // player allocates fresh storage based on the encoded Size/Keys when it
  // decodes - the values themselves do not flow through.
  uint32_t count = static_cast<uint32_t>(pdKeys.size());
  static VkPhysicalDevice kDummyPDSlot = VK_NULL_HANDLE;

  vkEnumeratePhysicalDevicesCommand cmd;
  cmd.m_Return.Value = VK_SUCCESS;
  cmd.m_instance.Key = state->ParentKey;
  cmd.m_pPhysicalDeviceCount.Value = &count;
  cmd.m_pPhysicalDevices.Value = &kDummyPDSlot;
  cmd.m_pPhysicalDevices.Size = count;
  cmd.m_pPhysicalDevices.Keys = pdKeys;
  m_Recorder.Record(vkEnumeratePhysicalDevicesSerializer(cmd));

  // Mark every sibling PD as restored so subsequent RestoreOne calls for
  // them short-circuit (idempotency guard at the top of RestoreOne).
  for (uint64_t pdKey : pdKeys) {
    m_RestoredThisPass.insert(pdKey);
  }
  return true;
}

void StateTrackingService::RestoreSurface(ObjectState* state) {
  auto* surf = static_cast<SurfaceState*>(state);
  if (surf->HwndKey != 0) {
    CreateWindowMetaCommand win;
    win.m_X.Value = surf->WindowX;
    win.m_Y.Value = surf->WindowY;
    win.m_Width.Value = surf->WindowWidth;
    win.m_Height.Value = surf->WindowHeight;
    win.m_Visible.Value = surf->WindowVisible;
    win.m_Hwnd.Value = surf->HwndKey;
    win.m_Hinstance.Value = surf->HinstanceKey;
    m_Recorder.Record(CreateWindowMetaSerializer(win));
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  // The CreationCommandBuffer was stored AFTER ReplayCustomizationLayer::Pre had
  // already substituted hwnd/hinstance with the first-player's runtime values.
  // CreateWindowMetaCommand above registers (HwndKey -> playbackHWND) and
  // (HinstanceKey -> playbackHINSTANCE) in WindowService.  For the second player's
  // ReplayCustomizationLayer::Pre to find those mappings, the hwnd/hinstance in the
  // surface create command must equal HwndKey/HinstanceKey, not the substituted
  // first-player values.  Decode the command, patch the fields, and re-emit.
  if (state->CreationCommandId == CommandId::ID_VKCREATEWIN32SURFACEKHR &&
      !state->CreationCommandBuffer.empty() && surf->HwndKey != 0) {
    // Decode rebases offsets in place; mirror EmitCreationCommand and decode
    // into a scratch copy so CreationCommandBuffer stays pristine.
    std::vector<char> scratch = state->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateWin32SurfaceKHRCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->hwnd = reinterpret_cast<HWND>(surf->HwndKey);
      cmd.m_pCreateInfo.Value->hinstance = reinterpret_cast<HINSTANCE>(surf->HinstanceKey);
    }
    m_Recorder.Record(vkCreateWin32SurfaceKHRSerializer(cmd));
    return;
  }
#endif

  if (!EmitCreationCommand(state)) {
    LOG_WARNING << "Vulkan2 subcapture: failed to emit creation command for surface key="
                << state->Key;
  }
}

bool StateTrackingService::RestoreBuffer(ObjectState* state) {
  auto* buf = static_cast<BufferState*>(state);

  // Emit vkCreateBuffer BEFORE restoring bound memory. For dedicated
  // allocations VkMemoryDedicatedAllocateInfo::buffer references this buffer,
  // so its handle must be registered in HandleMapService before vkAllocateMemory
  // is emitted; otherwise ResolvePNextHandleKeys crashes on the missing key.
  // For non-dedicated allocations the order makes no difference.
  if (!EmitCreationCommand(state)) {
    return false;
  }

  // Mark the buffer key as restored *now*, before recursing into bound memory.
  // For dedicated allocations DeviceMemoryState::DependencyKeys carries this
  // buffer's key (promoted from vkAllocateMemory's pNext HandleKeys), creating
  // a buffer<->memory back-edge.  Without this early insert RestoreOne(memory)
  // would re-enter RestoreOne(buffer) before the unconditional insert at the
  // end of RestoreOne fires, recursing until stack overflow.  The vkCreateBuffer
  // command above has already registered the handle in HandleMapService, so the
  // "restored = handle registered" contract still holds.
  m_RestoredThisPass.insert(state->Key);

  if (buf->BoundMemoryKey && buf->ParentKey) {
    RestoreOne(GetState(buf->BoundMemoryKey));
    if (!m_RestoredThisPass.count(buf->BoundMemoryKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping buffer memory bind for buffer key=" << buf->Key
                  << " because bound memory key=" << buf->BoundMemoryKey
                  << " could not be restored";
      // The buffer handle IS registered; return true so dependent buffer views
      // are not skipped unnecessarily.
      return true;
    }

    vkBindBufferMemoryCommand bind;
    bind.m_device.Key = buf->ParentKey;
    bind.m_buffer.Key = buf->Key;
    bind.m_memory.Key = buf->BoundMemoryKey;
    bind.m_memoryOffset.Value = buf->MemoryOffset;
    bind.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkBindBufferMemorySerializer(bind));
  }
  return true;
}

bool StateTrackingService::RestoreImage(ObjectState* state) {
  auto* img = static_cast<ImageState*>(state);

  if (img->CreationCommandBuffer.empty()) {
    return false;
  }

  // Emit vkCreateImage BEFORE restoring bound memory. For dedicated
  // allocations VkMemoryDedicatedAllocateInfo::image references this image,
  // so its handle must be registered in HandleMapService before vkAllocateMemory
  // is emitted; otherwise ResolvePNextHandleKeys crashes on the missing key.
  // For non-dedicated allocations the order makes no difference.
  //
  // We do NOT call the generic EmitCreationCommand here: the stored
  // vkCreateImage usage reflects what the application requested at recording
  // time, which is typically SAMPLED | COLOR_ATTACHMENT etc. without
  // TRANSFER_DST.  During the second player's state-restore phase,
  // RestoreImageContents emits vkCmdCopyBufferToImage to upload texel data
  // into this image; that copy requires the destination image to have been
  // created with VK_IMAGE_USAGE_TRANSFER_DST_BIT, otherwise validation fires
  // VUID-vkCmdCopyBufferToImage-dstImage-00177 and the copy may behave as a
  // no-op depending on the driver, leaving the image content uninitialised.
  //
  // Mirrors legacy Vulkan/recorder/vulkanStateRestore.cpp line ~734:
  //   imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  //
  // The fix must NOT touch the original (first-player) image creation -- only
  // the create command that the SECOND player will run during state restore.
  // Decode the stored blob into a scratch copy, OR in TRANSFER_DST, re-emit.
  //
  // KNOWN LIMITATION / FUTURE WORK:
  // Adding TRANSFER_DST here can legitimately change the image's
  // vkGetImageMemoryRequirements (size / alignment / memoryTypeBits), which
  // means the original vkAllocateMemory (sized for the app's requested usage
  // only) may no longer satisfy the new bind on some drivers.  The
  // spec-correct path is to promote TRANSFER_SRC | TRANSFER_DST in the legacy
  // interceptor (recExecWrap_vkCreateImage in
  // Vulkan/interceptor/include/vulkanExecWrap.h) so the captured
  // vkAllocateMemory is sized for the worst-case usage from day one.  Then
  // this state-restore-side OR becomes a no-op (usage already includes both
  // flags) and there is no requirements mismatch.  Tracking as follow-up.
  {
    std::vector<char> scratch = img->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateImageCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    m_Recorder.Record(vkCreateImageSerializer(cmd));
  }

  // Mark the image key as restored *now*, before recursing into bound memory.
  // For dedicated allocations DeviceMemoryState::DependencyKeys carries this
  // image's key (promoted from vkAllocateMemory's pNext HandleKeys), creating
  // an image<->memory back-edge.  Without this early insert RestoreOne(memory)
  // would re-enter RestoreOne(image) before the unconditional insert at the
  // end of RestoreOne fires, recursing until stack overflow.  The vkCreateImage
  // command above has already registered the handle in HandleMapService, so the
  // "restored = handle registered" contract still holds.
  m_RestoredThisPass.insert(state->Key);

  if (img->BoundMemoryKey && img->ParentKey) {
    RestoreOne(GetState(img->BoundMemoryKey));
    if (!m_RestoredThisPass.count(img->BoundMemoryKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping image memory bind for image key=" << img->Key
                  << " because bound memory key=" << img->BoundMemoryKey
                  << " could not be restored";
      // The image handle IS registered; return true so dependent image views
      // are not skipped unnecessarily.
      return true;
    }

    vkBindImageMemoryCommand bind;
    bind.m_device.Key = img->ParentKey;
    bind.m_image.Key = img->Key;
    bind.m_memory.Key = img->BoundMemoryKey;
    bind.m_memoryOffset.Value = img->MemoryOffset;
    bind.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkBindImageMemorySerializer(bind));
  }
  return true;
}

bool StateTrackingService::RestoreImageView(ObjectState* state) {
  auto* iv = static_cast<ImageViewState*>(state);

  if (iv->CreationCommandBuffer.empty()) {
    return false;
  }

  // Explicitly verify that the VkImage this view references has been
  // successfully restored before emitting vkCreateImageView.
  // This is a direct check on ImageViewState::imageKey rather than relying
  // solely on the generic DependencyKeys loop in RestoreOne, which can be
  // bypassed when imageKey is zero (HandleKeys was empty at tracking time).
  if (iv->ImageKey) {
    if (!HasState(iv->ImageKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping image view key=" << iv->Key
                  << " because image key=" << iv->ImageKey << " is no longer tracked";
      return false;
    }
    // Ensure the image itself is restored first (parent-first ordering).
    RestoreOne(GetState(iv->ImageKey));
    if (!m_RestoredThisPass.count(iv->ImageKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping image view key=" << iv->Key
                  << " because image key=" << iv->ImageKey << " could not be restored";
      return false;
    }
  }

  if (!EmitCreationCommand(state)) {
    return false;
  }
  return true;
}

void StateTrackingService::RestoreSwapchain(ObjectState* state) {
  // Same rationale as RestoreImage: the second player's content restore copies
  // texel data into swapchain images via vkCmdCopyBufferToImage, so the
  // swapchain images must be created with VK_IMAGE_USAGE_TRANSFER_DST_BIT.
  // The application typically requests only COLOR_ATTACHMENT, which makes the
  // restore copy spec-illegal (VUID-vkCmdCopyBufferToImage-dstImage-00177) and
  // a potential no-op depending on the driver.
  //
  // Mirrors legacy Vulkan/recorder/vulkanStateRestore.cpp line ~622:
  //   swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  //
  // Same KNOWN LIMITATION as RestoreImage above re: memory requirements
  // delta vs the original allocation.  Long-term: do the imageUsage promotion
  // at the legacy interceptor (recExecWrap_vkCreateSwapchainKHR).
  if (!state->CreationCommandBuffer.empty()) {
    std::vector<char> scratch = state->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateSwapchainKHRCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    // Handle the oldSwapchain reference.  HandleKeys layout for
    // VkSwapchainCreateInfoKHR is [surface, oldSwapchain].  The original create
    // call passed the swapchain it was retiring; mirror the legacy
    // RestoreSwapchainKHR behaviour (vulkanStateRestore.cpp ~617):
    //   * if that swapchain still exists, keep the reference -- but make sure it
    //     is restored (and thus registered in the player handle map) *before*
    //     this one, otherwise vkCreateSwapchainKHRRunner crashes resolving it;
    //   * if it was Destroyed before the cut, it was never restored, so null the
    //     reference (oldSwapchain is only an optimization hint).
    if (cmd.m_pCreateInfo.HandleKeys.size() > 1) {
      const uint64_t oldSwapchainKey = cmd.m_pCreateInfo.HandleKeys[1];
      if (oldSwapchainKey != 0) {
        if (HasState(oldSwapchainKey)) {
          RestoreOne(GetState(oldSwapchainKey));
        }
        if (!m_RestoredThisPass.count(oldSwapchainKey)) {
          cmd.m_pCreateInfo.HandleKeys[1] = 0;
          if (cmd.m_pCreateInfo.Value) {
            cmd.m_pCreateInfo.Value->oldSwapchain = VK_NULL_HANDLE;
          }
        }
      }
    }
    m_Recorder.Record(vkCreateSwapchainKHRSerializer(cmd));
  } else {
    LOG_WARNING << "Vulkan2 subcapture: failed to emit vkCreateSwapchainKHR for swapchain key="
                << state->Key;
    return;
  }

  auto* sc = static_cast<SwapchainState*>(state);
  bool emittedGetImages = false;

  // Always synthesise vkGetSwapchainImagesKHR using ALL keys from sc->ImageKeys.
  // Relying on the stored creation command of an individual image state is
  // unreliable: the app may have called vkGetSwapchainImagesKHR multiple times
  // (e.g. first to query the count, then to retrieve handles), so the command
  // stored in any single image state may only cover a partial subset of the
  // swapchain images.  The synthetic command always covers every key.
  //
  // NOTE: do NOT gate this on TryGetHandle().  At state-restore time the second
  // player has just created the swapchain but image handles are not yet in
  // HandleMapService -- that registration is exactly what this command performs.
  // The m_pSwapchainImages.Value array is an OUTPUT parameter; the runner
  // overwrites it with the real Vulkan results and uses Keys for the mapping, so
  // its initial content is irrelevant.
  if (!sc->ImageKeys.empty()) {
    uint32_t imageCount = static_cast<uint32_t>(sc->ImageKeys.size());
    std::vector<VkImage> dummyHandles(imageCount, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHRCommand cmd;
    cmd.m_Return.Value = VK_SUCCESS;
    cmd.m_device.Key = state->ParentKey;
    cmd.m_swapchain.Key = state->Key;
    cmd.m_pSwapchainImageCount.Value = &imageCount;
    cmd.m_pSwapchainImages.Value = dummyHandles.data();
    cmd.m_pSwapchainImages.Size = imageCount;
    for (uint64_t imgKey : sc->ImageKeys) {
      cmd.m_pSwapchainImages.Keys.push_back(imgKey);
    }
    m_Recorder.Record(vkGetSwapchainImagesKHRSerializer(cmd));
    emittedGetImages = true;
  }

  // Mark ALL swapchain image keys as restored now that vkGetSwapchainImagesKHR
  // has been emitted.  Do this after the loop (not inside it) so that images
  // whose state appeared before the emitting entry are also marked.
  if (emittedGetImages) {
    for (uint64_t imgKey : sc->ImageKeys) {
      m_RestoredThisPass.insert(imgKey);
    }
  }

  // Re-acquire any images that were acquired but not yet presented at the
  // subcapture boundary.  This mirrors the old-backend RestoreSwapchainKHR
  // which emits vkAcquireNextImageKHR for each index in AcquiredImages.
  // The image arrives in the correct state via the recorded frame's own
  // acquire+barrier path, so no separate layout barrier is needed for these.
  for (uint32_t imageIndex : sc->AcquiredImages) {
    vkAcquireNextImageKHRCommand acquireCmd;
    acquireCmd.m_device.Key = state->ParentKey;
    acquireCmd.m_swapchain.Key = state->Key;
    acquireCmd.m_timeout.Value = 3000000000ULL;
    acquireCmd.m_semaphore.Key = 0;
    acquireCmd.m_fence.Key = 0;
    acquireCmd.m_pImageIndex.Value = &imageIndex;
    acquireCmd.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkAcquireNextImageKHRSerializer(acquireCmd));
  }
}

bool StateTrackingService::RestoreDescriptorSets(ObjectState* state) {
  auto* ds = static_cast<DescriptorSetState*>(state);

  if (state->CreationCommandBuffer.empty()) {
    LOG_WARNING << "Vulkan2 subcapture: skipping descriptor set key=" << ds->Key
                << " because CreationCommandBuffer is empty (vkAllocateDescriptorSets blob was "
                   "never stored)";
    return false;
  }

  if (ds->PoolKey) {
    RestoreOne(GetState(ds->PoolKey));
    if (!m_RestoredThisPass.count(ds->PoolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping descriptor set key=" << ds->Key
                  << " because descriptor pool key=" << ds->PoolKey << " could not be restored";
      return false;
    }
  }

  // Allocation step (emitted once per set).  pNext-free sets are allocated in a
  // single batched vkAllocateDescriptorSets per pool; sets that carried a pNext
  // chain are allocated individually from their stored single-set blob.
  if (!m_DescriptorSetsAllocated.count(ds->Key)) {
    if (ds->HasAllocPNext || ds->PoolKey == 0) {
      if (!EmitCreationCommand(state)) {
        LOG_WARNING << "Vulkan2 subcapture: skipping descriptor set key=" << ds->Key
                    << " because EmitCreationCommand failed (likely a missing dependency such as "
                       "VkDescriptorSetLayout key="
                    << (ds->DependencyKeys.empty() ? 0 : ds->DependencyKeys.front()) << ")";
        return false;
      }
      m_DescriptorSetsAllocated.insert(ds->Key);
    } else {
      AllocateDescriptorSetBatchForPool(ds->PoolKey);
      if (!m_DescriptorSetsAllocated.count(ds->Key)) {
        LOG_WARNING << "Vulkan2 subcapture: skipping descriptor set key=" << ds->Key
                    << " because its batched allocation failed (likely a missing "
                       "VkDescriptorSetLayout key="
                    << ds->LayoutKey << ")";
        return false;
      }
    }
  }

  // Re-emit all tracked descriptor writes / copies / template updates for
  // this set so the second player ends up with the same binding state.  Done
  // per set in normal object order so the writes appear after the buffers /
  // images they reference have been re-created in the restore stream.
  m_DescriptorSetUpdateService.RestoreUpdates(ds->Key, m_Recorder, *this);
  return true;
}

void StateTrackingService::AllocateDescriptorSetBatchForPool(uint64_t poolKey) {
  ObjectState* poolState = GetState(poolKey);
  if (!poolState) {
    return;
  }
  const uint64_t deviceKey = poolState->ParentKey;

  // Collect every live, pNext-free, not-yet-allocated set of this pool whose
  // layout can be restored.  Sets whose layout is gone are skipped (left out of
  // m_DescriptorSetsAllocated so the caller reports the failure for that set).
  std::vector<uint64_t> setKeys;
  std::vector<uint64_t> layoutKeys;
  for (auto& [_, statePtr] : m_States) {
    ObjectState* s = statePtr.get();
    if (s->Destroyed || s->CreationCommandId != CommandId::ID_VKALLOCATEDESCRIPTORSETS) {
      continue;
    }
    auto* ds = static_cast<DescriptorSetState*>(s);
    if (ds->PoolKey != poolKey || ds->HasAllocPNext) {
      continue;
    }
    if (m_DescriptorSetsAllocated.count(ds->Key) || ds->CreationCommandBuffer.empty()) {
      continue;
    }
    if (ds->LayoutKey) {
      RestoreOne(GetState(ds->LayoutKey));
      if (!m_RestoredThisPass.count(ds->LayoutKey)) {
        LOG_WARNING << "Vulkan2 subcapture: omitting descriptor set key=" << ds->Key
                    << " from batch because VkDescriptorSetLayout key=" << ds->LayoutKey
                    << " could not be restored";
        continue;
      }
    }
    setKeys.push_back(ds->Key);
    layoutKeys.push_back(ds->LayoutKey);
  }
  if (setKeys.empty()) {
    return;
  }

  // Build one vkAllocateDescriptorSets for the whole group.  Handle resolution
  // is driven entirely by HandleKeys ([0]=pool, [1+i]=pSetLayouts[i]) and the
  // output Keys; the Value handles are placeholders overwritten by the player.
  const uint32_t count = static_cast<uint32_t>(setKeys.size());
  std::vector<VkDescriptorSetLayout> layouts(count, VK_NULL_HANDLE);
  std::vector<VkDescriptorSet> outSets(count, VK_NULL_HANDLE);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.descriptorPool = VK_NULL_HANDLE; // remapped from HandleKeys[0]
  allocInfo.descriptorSetCount = count;
  allocInfo.pSetLayouts = layouts.data();

  vkAllocateDescriptorSetsCommand cmd;
  cmd.m_device.Key = deviceKey;
  cmd.m_pAllocateInfo.Value = &allocInfo;
  cmd.m_pAllocateInfo.HandleKeys.reserve(count + 1);
  cmd.m_pAllocateInfo.HandleKeys.push_back(poolKey);
  for (uint64_t layoutKey : layoutKeys) {
    cmd.m_pAllocateInfo.HandleKeys.push_back(layoutKey);
  }
  cmd.m_pDescriptorSets.Value = outSets.data();
  cmd.m_pDescriptorSets.Size = count;
  cmd.m_pDescriptorSets.Keys = setKeys;
  cmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkAllocateDescriptorSetsSerializer(cmd));

  for (uint64_t setKey : setKeys) {
    m_DescriptorSetsAllocated.insert(setKey);
  }
}

StateTrackingService::CommandBufferRestoreOutcome StateTrackingService::RestoreCommandBuffers(
    ObjectState* state) {
  auto* cb = static_cast<CommandBufferState*>(state);

  // Matches legacy RestoreCommandBuffers pre-checks (vulkanStateRestore.cpp): omit
  // this CB entirely when its pool is gone or unrestorable; no allocation emitted.
  if (cb->PoolKey) {
    ObjectState* poolState = GetState(cb->PoolKey);
    if (!poolState) {
      LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because VkCommandPool key=" << cb->PoolKey << " is no longer tracked";
      return CommandBufferRestoreOutcome::FailedNoAllocation;
    }
    RestoreOne(poolState);
    if (!m_RestoredThisPass.count(cb->PoolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because VkCommandPool key=" << cb->PoolKey << " could not be restored";
      return CommandBufferRestoreOutcome::FailedNoAllocation;
    }
  }

  if (!EmitCreationCommand(state)) {
    LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                << " because vkAllocateCommandBuffers could not be emitted";
    return CommandBufferRestoreOutcome::FailedNoAllocation;
  }

  // Legacy backend omits vkBegin + recorded tokens when bound buffers / images /
  // descriptor sets / pipelines / framebuffers / secondary CBs are missing.
  // DependencyKeys aggregates vkCmd* references from SubcaptureLayer; if any
  // dep cannot be restored, skip recorded-command replay only (allocation stands).
  //
  // vkCmdExecuteCommands only registers secondary VkCommandBuffer handles; if a
  // secondary skipped replay due to unrestorable deps, this CB cannot replay either.
  for (uint64_t dep : cb->DependencyKeys) {
    if (!dep) {
      continue;
    }
    if (!HasState(dep)) {
      LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because referenced object key=" << dep << " is no longer tracked";
      return CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped;
    }
    RestoreOne(GetState(dep));
    if (!m_RestoredThisPass.count(dep)) {
      LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because referenced object key=" << dep << " could not be restored";
      return CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped;
    }
    if (m_CommandBuffersRecordingReplaySkipped.count(dep)) {
      LOG_WARNING << "Vulkan2 subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because referenced VkCommandBuffer key=" << dep
                  << " did not replay its recording";
      return CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped;
    }
  }

  if (!cb->BeginCommandBuffer.empty()) {
    // Re-emit vkBeginCommandBuffer and every recorded vkCmd*.
    EmitRawCommand(CommandId::ID_VKBEGINCOMMANDBUFFER, cb->BeginCommandBuffer);
    for (size_t i = 0; i < cb->RecordedCommands.size(); ++i) {
      EmitRawCommand(cb->RecordedCommandIds[i], cb->RecordedCommands[i]);
    }
    // If the CB was in executable state (ended but not reset), close it again
    // so the second player has it in the same executable state.
    if (cb->IsExecutable && !cb->EndCommandBuffer.empty()) {
      EmitRawCommand(CommandId::ID_VKENDCOMMANDBUFFER, cb->EndCommandBuffer);
    }
  }

  return CommandBufferRestoreOutcome::AllocationOkFullRecordingReplay;
}

void StateTrackingService::RestoreMappedMemory(ObjectState* state) {
  auto* mem = static_cast<DeviceMemoryState*>(state);

  const bool hasDirtyData =
      !mem->ShadowBuffer.empty() && mem->ShadowDirtyEnd > mem->ShadowDirtyBegin;

  if (!hasDirtyData && !mem->IsMapped) {
    return;
  }

  // Decide which mapping parameters to use for the data-write phase.
  //
  // region.Offset in MappedDataMetaCommand is RELATIVE to the mapped-range
  // start (i.e. region.Offset = allocationOffset - MappingOffset).  The
  // player writes at:  mappedPtr + region.Offset  ?  allocation byte
  //                     MappingOffset + region.Offset.
  //
  // To land at the correct allocation byte we therefore need:
  //   region.Offset = ShadowDirtyBegin - mapOffset.
  //
  // The current app mapping [MappingOffset, MappingOffset+MappingSize) can be
  // reused when it fully covers [ShadowDirtyBegin, ShadowDirtyEnd), which
  // avoids an extra unmap/remap cycle in the common case.
  // When it doesn't cover the dirty range (e.g. dirty bytes were written
  // during an earlier mapping that has since been replaced), we fall back to
  // offset=0 / VK_WHOLE_SIZE which always covers everything.

  const bool currentMappingCoversData =
      mem->IsMapped && hasDirtyData && mem->MappingOffset <= mem->ShadowDirtyBegin &&
      (mem->MappingSize == VK_WHOLE_SIZE ||
       mem->MappingOffset + mem->MappingSize >= mem->ShadowDirtyEnd);

  const bool useBroaderMapping = hasDirtyData && !currentMappingCoversData;

  const VkDeviceSize mapOffset = useBroaderMapping ? 0 : mem->MappingOffset;
  const VkDeviceSize mapSize = useBroaderMapping ? VK_WHOLE_SIZE : mem->MappingSize;
  const VkMemoryMapFlags mapFlags = useBroaderMapping ? 0 : mem->MappingFlags;

  if (hasDirtyData) {
    {
      vkMapMemoryCommand map;
      map.m_device.Key = mem->ParentKey;
      map.m_memory.Key = mem->Key;
      map.m_offset.Value = mapOffset;
      map.m_size.Value = mapSize;
      map.m_flags.Value = mapFlags;
      map.m_Return.Value = VK_SUCCESS;
      m_Recorder.Record(vkMapMemorySerializer(map));
    }

    {
      MappedDataMetaCommand mdc;
      mdc.m_Device.Key = mem->ParentKey;
      mdc.m_Memory.Key = mem->Key;
      MemoryRegions::Region region;
      // region.Offset must be relative to the mapped-range start so the player
      // writes to the correct allocation byte: mapOffset + region.Offset = ShadowDirtyBegin.
      region.Offset = mem->ShadowDirtyBegin - mapOffset;
      region.Size = mem->ShadowDirtyEnd - mem->ShadowDirtyBegin;
      region.Data = const_cast<char*>(
          reinterpret_cast<const char*>(mem->ShadowBuffer.data() + mem->ShadowDirtyBegin));
      mdc.m_Regions.Regions.push_back(region);
      mdc.m_Regions.Size = 1;
      m_Recorder.Record(MappedDataMetaSerializer(mdc));
    }

    // Unmap when: memory should be unmapped at the capture boundary,
    // OR we used a broader mapping that differs from the original.
    if (!mem->IsMapped || useBroaderMapping) {
      vkUnmapMemoryCommand unmap;
      unmap.m_device.Key = mem->ParentKey;
      unmap.m_memory.Key = mem->Key;
      m_Recorder.Record(vkUnmapMemorySerializer(unmap));
    }
  }

  // Restore the original mapping state when:
  // (a) memory is mapped AND we had to use a broader mapping for writing, OR
  // (b) memory is mapped AND there was no dirty data (mapping state still needs restoring).
  if (mem->IsMapped && (useBroaderMapping || !hasDirtyData)) {
    vkMapMemoryCommand map;
    map.m_device.Key = mem->ParentKey;
    map.m_memory.Key = mem->Key;
    map.m_offset.Value = mem->MappingOffset;
    map.m_size.Value = mem->MappingSize;
    map.m_flags.Value = mem->MappingFlags;
    map.m_Return.Value = VK_SUCCESS;
    m_Recorder.Record(vkMapMemorySerializer(map));
  }
}

// ---------------------------------------------------------------------------
// RestoreBufferContents / RestoreImageContents
// ---------------------------------------------------------------------------

// Synthetic GITSKeys for temporary staging resources created in the stream.
// Must not collide with real keys (which are sequential starting from 1).
// kTempCBKey = UINT64_MAX-1 is used by EmitImageLayoutTransitions.
static constexpr uint64_t kStagingBufKey = static_cast<uint64_t>(-3);
static constexpr uint64_t kStagingMemKey = static_cast<uint64_t>(-4);
static constexpr uint64_t kContentCBKey = static_cast<uint64_t>(-5);

// ---------------------------------------------------------------------------
// Emit stream commands that create a staging buffer, upload data, copy it to
// the destination resource, and then tear the staging buffer down.
// The content CB (kContentCBKey) is allocated from commandPoolKey.
//
// For buffers: the stream emits  vkCmdCopyBuffer (staging → dstBufKey).
// For images:  the stream emits  vkCmdCopyBufferToImage (staging → dstImageKey)
//              followed by a pipeline barrier to the correct finalLayout.
// ---------------------------------------------------------------------------

static void EmitStagingUploadAndCopyBuffer(SubcaptureRecorder& recorder,
                                           uint64_t deviceKey,
                                           uint64_t queueKey,
                                           uint64_t commandPoolKey,
                                           uint64_t dstBufKey,
                                           VkDeviceSize bufSize,
                                           VkDeviceSize stagingAllocationSize,
                                           uint32_t stagingMemTypeIndex,
                                           const std::vector<uint8_t>& data) {

  // --- Create staging buffer ---
  // bci.size is the buffer's logical length (used for vkCmdCopyBuffer); the
  // *memory* we allocate must be >= the requirements-reported size
  // (alignment-rounded), passed in as stagingAllocationSize.  Without the
  // separation we hit VUID-vkBindBufferMemory-None-10741 ("allocationSize ...
  // must be at least as large as VkMemoryRequirements::size").
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = bufSize;
  bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vkCreateBufferCommand createBufCmd;
  createBufCmd.m_device.Key = deviceKey;
  createBufCmd.m_pCreateInfo.Value = &bci;
  createBufCmd.m_pBuffer.Key = kStagingBufKey;
  createBufCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkCreateBufferSerializer(createBufCmd));

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = stagingAllocationSize;
  mai.memoryTypeIndex = stagingMemTypeIndex;

  vkAllocateMemoryCommand allocMemCmd;
  allocMemCmd.m_device.Key = deviceKey;
  allocMemCmd.m_pAllocateInfo.Value = &mai;
  allocMemCmd.m_pMemory.Key = kStagingMemKey;
  allocMemCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkAllocateMemorySerializer(allocMemCmd));

  vkBindBufferMemoryCommand bindCmd;
  bindCmd.m_device.Key = deviceKey;
  bindCmd.m_buffer.Key = kStagingBufKey;
  bindCmd.m_memory.Key = kStagingMemKey;
  bindCmd.m_memoryOffset.Value = 0;
  bindCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkBindBufferMemorySerializer(bindCmd));

  // --- Upload data into staging via map + MappedDataMetaCommand ---
  vkMapMemoryCommand mapCmd;
  mapCmd.m_device.Key = deviceKey;
  mapCmd.m_memory.Key = kStagingMemKey;
  mapCmd.m_offset.Value = 0;
  mapCmd.m_size.Value = VK_WHOLE_SIZE;
  mapCmd.m_flags.Value = 0;
  mapCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkMapMemorySerializer(mapCmd));

  MappedDataMetaCommand mdc;
  mdc.m_Device.Key = deviceKey;
  mdc.m_Memory.Key = kStagingMemKey;
  MemoryRegions::Region region;
  region.Offset = 0;
  region.Size = bufSize;
  region.Data = const_cast<char*>(reinterpret_cast<const char*>(data.data()));
  mdc.m_Regions.Regions.push_back(region);
  mdc.m_Regions.Size = 1;
  recorder.Record(MappedDataMetaSerializer(mdc));

  vkUnmapMemoryCommand unmapCmd;
  unmapCmd.m_device.Key = deviceKey;
  unmapCmd.m_memory.Key = kStagingMemKey;
  recorder.Record(vkUnmapMemorySerializer(unmapCmd));

  // --- Allocate one-shot CB and issue copy ---
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  cbai.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL); // sentinel

  static VkCommandBuffer kDummyCB = VK_NULL_HANDLE;
  vkAllocateCommandBuffersCommand allocCBCmd;
  allocCBCmd.m_device.Key = deviceKey;
  allocCBCmd.m_pAllocateInfo.Value = &cbai;
  allocCBCmd.m_pAllocateInfo.HandleKeys = {commandPoolKey};
  allocCBCmd.m_pCommandBuffers.Value = &kDummyCB;
  allocCBCmd.m_pCommandBuffers.Size = 1;
  allocCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  allocCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkAllocateCommandBuffersSerializer(allocCBCmd));

  VkCommandBufferBeginInfo cbbi{};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBufferCommand beginCBCmd;
  beginCBCmd.m_commandBuffer.Key = kContentCBKey;
  beginCBCmd.m_pBeginInfo.Value = &cbbi;
  beginCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkBeginCommandBufferSerializer(beginCBCmd));

  // Barrier: staging TRANSFER_SRC → dstBuf TRANSFER_DST
  VkBufferMemoryBarrier barriers[2]{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barriers[0].srcAccessMask = 0;
  barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = reinterpret_cast<VkBuffer>(0x1ULL); // sentinel
  barriers[0].size = VK_WHOLE_SIZE;
  barriers[1] = barriers[0];
  barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barriers[1].buffer = reinterpret_cast<VkBuffer>(0x2ULL); // sentinel

  vkCmdPipelineBarrierCommand preBarrierCmd;
  preBarrierCmd.m_commandBuffer.Key = kContentCBKey;
  preBarrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  preBarrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_TRANSFER_BIT;
  preBarrierCmd.m_dependencyFlags.Value = 0;
  preBarrierCmd.m_memoryBarrierCount.Value = 0;
  preBarrierCmd.m_bufferMemoryBarrierCount.Value = 2;
  preBarrierCmd.m_pBufferMemoryBarriers.Value = barriers;
  preBarrierCmd.m_pBufferMemoryBarriers.Size = 2;
  preBarrierCmd.m_pBufferMemoryBarriers.HandleKeys = {dstBufKey, kStagingBufKey};
  preBarrierCmd.m_imageMemoryBarrierCount.Value = 0;
  recorder.Record(vkCmdPipelineBarrierSerializer(preBarrierCmd));

  VkBufferCopy copyRegion{0, 0, bufSize};

  vkCmdCopyBufferCommand copyCmd;
  copyCmd.m_commandBuffer.Key = kContentCBKey;
  copyCmd.m_srcBuffer.Key = kStagingBufKey;
  copyCmd.m_dstBuffer.Key = dstBufKey;
  copyCmd.m_regionCount.Value = 1;
  copyCmd.m_pRegions.Value = &copyRegion;
  copyCmd.m_pRegions.Size = 1;
  recorder.Record(vkCmdCopyBufferSerializer(copyCmd));

  // Post-barrier: TRANSFER_DST → MEMORY_READ|WRITE (generic)
  barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  preBarrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TRANSFER_BIT;
  preBarrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  preBarrierCmd.m_bufferMemoryBarrierCount.Value = 1;
  preBarrierCmd.m_pBufferMemoryBarriers.Size = 1;
  preBarrierCmd.m_pBufferMemoryBarriers.HandleKeys = {dstBufKey};
  recorder.Record(vkCmdPipelineBarrierSerializer(preBarrierCmd));

  vkEndCommandBufferCommand endCBCmd;
  endCBCmd.m_commandBuffer.Key = kContentCBKey;
  endCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkEndCommandBufferSerializer(endCBCmd));

  static VkCommandBuffer kDummyCBSlot = VK_NULL_HANDLE;
  kDummyCBSlot = reinterpret_cast<VkCommandBuffer>(kContentCBKey);
  VkSubmitInfo si{
      VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &kDummyCBSlot, 0, nullptr};
  vkQueueSubmitCommand submitCmd;
  submitCmd.m_queue.Key = queueKey;
  submitCmd.m_fence.Key = 0;
  submitCmd.m_Return.Value = VK_SUCCESS;
  submitCmd.m_submitCount.Value = 1;
  submitCmd.m_pSubmits.Value = &si;
  submitCmd.m_pSubmits.Size = 1;
  submitCmd.m_pSubmits.HandleKeys = {kContentCBKey};
  recorder.Record(vkQueueSubmitSerializer(submitCmd));

  vkQueueWaitIdleCommand waitCmd;
  waitCmd.m_queue.Key = queueKey;
  waitCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkQueueWaitIdleSerializer(waitCmd));

  static VkCommandBuffer kDummyCBFree = VK_NULL_HANDLE;
  vkFreeCommandBuffersCommand freeCBCmd;
  freeCBCmd.m_device.Key = deviceKey;
  freeCBCmd.m_commandPool.Key = commandPoolKey;
  freeCBCmd.m_commandBufferCount.Value = 1;
  freeCBCmd.m_pCommandBuffers.Value = &kDummyCBFree;
  freeCBCmd.m_pCommandBuffers.Size = 1;
  freeCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  recorder.Record(vkFreeCommandBuffersSerializer(freeCBCmd));

  vkDestroyBufferCommand destroyBufCmd;
  destroyBufCmd.m_device.Key = deviceKey;
  destroyBufCmd.m_buffer.Key = kStagingBufKey;
  recorder.Record(vkDestroyBufferSerializer(destroyBufCmd));

  vkFreeMemoryCommand freeMemCmd;
  freeMemCmd.m_device.Key = deviceKey;
  freeMemCmd.m_memory.Key = kStagingMemKey;
  recorder.Record(vkFreeMemorySerializer(freeMemCmd));
}

static void EmitStagingUploadAndCopyImage(SubcaptureRecorder& recorder,
                                          uint64_t deviceKey,
                                          uint64_t queueKey,
                                          uint64_t commandPoolKey,
                                          uint64_t dstImageKey,
                                          VkFormat format,
                                          const VkExtent3D& /*extent*/,
                                          VkImageLayout finalLayout,
                                          VkImageAspectFlags aspectMask,
                                          VkDeviceSize stagingSize,
                                          VkDeviceSize stagingAllocationSize,
                                          uint32_t stagingMemTypeIndex,
                                          const std::vector<uint8_t>& data,
                                          const std::vector<VkBufferImageCopy>& regions) {

  // Create staging buffer - see EmitStagingUploadAndCopyBuffer for the
  // rationale behind the bci.size vs mai.allocationSize split.
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = stagingSize;
  bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vkCreateBufferCommand createBufCmd;
  createBufCmd.m_device.Key = deviceKey;
  createBufCmd.m_pCreateInfo.Value = &bci;
  createBufCmd.m_pBuffer.Key = kStagingBufKey;
  createBufCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkCreateBufferSerializer(createBufCmd));

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = stagingAllocationSize;
  mai.memoryTypeIndex = stagingMemTypeIndex;

  vkAllocateMemoryCommand allocMemCmd;
  allocMemCmd.m_device.Key = deviceKey;
  allocMemCmd.m_pAllocateInfo.Value = &mai;
  allocMemCmd.m_pMemory.Key = kStagingMemKey;
  allocMemCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkAllocateMemorySerializer(allocMemCmd));

  vkBindBufferMemoryCommand bindCmd;
  bindCmd.m_device.Key = deviceKey;
  bindCmd.m_buffer.Key = kStagingBufKey;
  bindCmd.m_memory.Key = kStagingMemKey;
  bindCmd.m_memoryOffset.Value = 0;
  bindCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkBindBufferMemorySerializer(bindCmd));

  // Upload data
  vkMapMemoryCommand mapCmd;
  mapCmd.m_device.Key = deviceKey;
  mapCmd.m_memory.Key = kStagingMemKey;
  mapCmd.m_offset.Value = 0;
  mapCmd.m_size.Value = VK_WHOLE_SIZE;
  mapCmd.m_flags.Value = 0;
  mapCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkMapMemorySerializer(mapCmd));

  MappedDataMetaCommand mdc;
  mdc.m_Device.Key = deviceKey;
  mdc.m_Memory.Key = kStagingMemKey;
  MemoryRegions::Region region;
  region.Offset = 0;
  region.Size = stagingSize;
  region.Data = const_cast<char*>(reinterpret_cast<const char*>(data.data()));
  mdc.m_Regions.Regions.push_back(region);
  mdc.m_Regions.Size = 1;
  recorder.Record(MappedDataMetaSerializer(mdc));

  vkUnmapMemoryCommand unmapCmd;
  unmapCmd.m_device.Key = deviceKey;
  unmapCmd.m_memory.Key = kStagingMemKey;
  recorder.Record(vkUnmapMemorySerializer(unmapCmd));

  // Allocate content CB
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  cbai.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL);

  static VkCommandBuffer kDummyCBImg = VK_NULL_HANDLE;
  vkAllocateCommandBuffersCommand allocCBCmd;
  allocCBCmd.m_device.Key = deviceKey;
  allocCBCmd.m_pAllocateInfo.Value = &cbai;
  allocCBCmd.m_pAllocateInfo.HandleKeys = {commandPoolKey};
  allocCBCmd.m_pCommandBuffers.Value = &kDummyCBImg;
  allocCBCmd.m_pCommandBuffers.Size = 1;
  allocCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  allocCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkAllocateCommandBuffersSerializer(allocCBCmd));

  VkCommandBufferBeginInfo cbbi{};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBufferCommand beginCBCmd;
  beginCBCmd.m_commandBuffer.Key = kContentCBKey;
  beginCBCmd.m_pBeginInfo.Value = &cbbi;
  beginCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkBeginCommandBufferSerializer(beginCBCmd));

  // Barrier: UNDEFINED → TRANSFER_DST_OPTIMAL
  VkImageMemoryBarrier toDst{};
  toDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toDst.srcAccessMask = 0;
  toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toDst.image = reinterpret_cast<VkImage>(0x1ULL);
  toDst.subresourceRange = {aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};

  vkCmdPipelineBarrierCommand preBarrierCmd;
  preBarrierCmd.m_commandBuffer.Key = kContentCBKey;
  preBarrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  preBarrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_TRANSFER_BIT;
  preBarrierCmd.m_dependencyFlags.Value = 0;
  preBarrierCmd.m_memoryBarrierCount.Value = 0;
  preBarrierCmd.m_bufferMemoryBarrierCount.Value = 0;
  preBarrierCmd.m_imageMemoryBarrierCount.Value = 1;
  preBarrierCmd.m_pImageMemoryBarriers.Value = &toDst;
  preBarrierCmd.m_pImageMemoryBarriers.Size = 1;
  preBarrierCmd.m_pImageMemoryBarriers.HandleKeys = {dstImageKey};
  recorder.Record(vkCmdPipelineBarrierSerializer(preBarrierCmd));

  // vkCmdCopyBufferToImage
  vkCmdCopyBufferToImageCommand copyCmd;
  copyCmd.m_commandBuffer.Key = kContentCBKey;
  copyCmd.m_srcBuffer.Key = kStagingBufKey;
  copyCmd.m_dstImage.Key = dstImageKey;
  copyCmd.m_dstImageLayout.Value = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  copyCmd.m_regionCount.Value = static_cast<uint32_t>(regions.size());
  copyCmd.m_pRegions.Value = const_cast<VkBufferImageCopy*>(regions.data());
  copyCmd.m_pRegions.Size = static_cast<uint32_t>(regions.size());
  recorder.Record(vkCmdCopyBufferToImageSerializer(copyCmd));

  // Barrier: TRANSFER_DST_OPTIMAL → finalLayout
  VkImageMemoryBarrier toFinal{};
  toFinal.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  toFinal.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  toFinal.newLayout = finalLayout;
  toFinal.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toFinal.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toFinal.image = reinterpret_cast<VkImage>(0x1ULL);
  toFinal.subresourceRange = {aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};

  vkCmdPipelineBarrierCommand postBarrierCmd;
  postBarrierCmd.m_commandBuffer.Key = kContentCBKey;
  postBarrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TRANSFER_BIT;
  postBarrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  postBarrierCmd.m_dependencyFlags.Value = 0;
  postBarrierCmd.m_memoryBarrierCount.Value = 0;
  postBarrierCmd.m_bufferMemoryBarrierCount.Value = 0;
  postBarrierCmd.m_imageMemoryBarrierCount.Value = 1;
  postBarrierCmd.m_pImageMemoryBarriers.Value = &toFinal;
  postBarrierCmd.m_pImageMemoryBarriers.Size = 1;
  postBarrierCmd.m_pImageMemoryBarriers.HandleKeys = {dstImageKey};
  recorder.Record(vkCmdPipelineBarrierSerializer(postBarrierCmd));

  vkEndCommandBufferCommand endCBCmd;
  endCBCmd.m_commandBuffer.Key = kContentCBKey;
  endCBCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkEndCommandBufferSerializer(endCBCmd));

  static VkCommandBuffer kDummyCBSlotImg = VK_NULL_HANDLE;
  kDummyCBSlotImg = reinterpret_cast<VkCommandBuffer>(kContentCBKey);
  VkSubmitInfo si{
      VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &kDummyCBSlotImg, 0, nullptr};
  vkQueueSubmitCommand submitCmd;
  submitCmd.m_queue.Key = queueKey;
  submitCmd.m_fence.Key = 0;
  submitCmd.m_Return.Value = VK_SUCCESS;
  submitCmd.m_submitCount.Value = 1;
  submitCmd.m_pSubmits.Value = &si;
  submitCmd.m_pSubmits.Size = 1;
  submitCmd.m_pSubmits.HandleKeys = {kContentCBKey};
  recorder.Record(vkQueueSubmitSerializer(submitCmd));

  vkQueueWaitIdleCommand waitCmd;
  waitCmd.m_queue.Key = queueKey;
  waitCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkQueueWaitIdleSerializer(waitCmd));

  static VkCommandBuffer kDummyCBFreeImg = VK_NULL_HANDLE;
  vkFreeCommandBuffersCommand freeCBCmd;
  freeCBCmd.m_device.Key = deviceKey;
  freeCBCmd.m_commandPool.Key = commandPoolKey;
  freeCBCmd.m_commandBufferCount.Value = 1;
  freeCBCmd.m_pCommandBuffers.Value = &kDummyCBFreeImg;
  freeCBCmd.m_pCommandBuffers.Size = 1;
  freeCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  recorder.Record(vkFreeCommandBuffersSerializer(freeCBCmd));

  vkDestroyBufferCommand destroyBufCmd;
  destroyBufCmd.m_device.Key = deviceKey;
  destroyBufCmd.m_buffer.Key = kStagingBufKey;
  recorder.Record(vkDestroyBufferSerializer(destroyBufCmd));

  vkFreeMemoryCommand freeMemCmd;
  freeMemCmd.m_device.Key = deviceKey;
  freeMemCmd.m_memory.Key = kStagingMemKey;
  recorder.Record(vkFreeMemorySerializer(freeMemCmd));
}

// ---------------------------------------------------------------------------

void StateTrackingService::RestoreBufferContents() {
  // Group buffers by device.
  std::unordered_map<uint64_t, std::vector<uint64_t>> buffersByDevice;
  for (const auto& [key, sp] : m_States) {
    if (sp->Destroyed) {
      continue;
    }
    if (sp->CreationCommandId != CommandId::ID_VKCREATEBUFFER) {
      continue;
    }
    if (!m_RestoredThisPass.count(key)) {
      continue;
    }

    auto* buf = static_cast<BufferState*>(sp.get());
    if (buf->BufferSize == 0 || buf->BoundMemoryKey == 0) {
      continue;
    }
    //if (!(buf->UsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
    //  continue;
    //}

    // Skip if the bound memory is host-visible — RestoreMappedMemory already
    // handled it (or it will be written via the app's own map calls).
    auto* mem = GetState<DeviceMemoryState>(buf->BoundMemoryKey);
    if (!mem) {
      continue;
    }
    // ParentKey of buf = deviceKey; ParentKey of deviceState = physDevKey
    auto* devState = GetState<ObjectState>(buf->ParentKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (physDevKey && m_GpuReadbackHelper->IsHostVisible(physDevKey, mem->MemoryTypeIndex)) {
      continue;
    }

    buffersByDevice[buf->ParentKey].push_back(key);
  }

  for (auto& [deviceKey, bufKeys] : buffersByDevice) {
    uint64_t queueKey = 0, poolKey = 0;
    if (!FindQueueAndPool(m_States, deviceKey, queueKey, poolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping buffer content restore for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(poolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: queue or pool was not restored, skipping buffer "
                     "content restore for device key="
                  << deviceKey;
      continue;
    }

    auto* devState = GetState<ObjectState>(deviceKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (!physDevKey) {
      continue;
    }

    for (uint64_t bufKey : bufKeys) {
      auto* buf = static_cast<BufferState*>(GetState(bufKey));
      if (!buf) {
        continue;
      }

      std::vector<uint8_t> data;
      if (!m_GpuReadbackHelper->ReadBuffer(deviceKey, physDevKey, queueKey, poolKey, bufKey,
                                           buf->BufferSize, data)) {
        LOG_WARNING << "Vulkan2 subcapture: GPU readback failed for buffer key=" << bufKey;
        continue;
      }

      // Query the actual memory requirements the second player's driver will
      // report for a TRANSFER_SRC buffer of this size, so we can pick a memory
      // type the buffer is allowed to use (req.memoryTypeBits, fixes
      // VUID-vkBindBufferMemory-memory-01035: previously we passed 0xFFFFFFFF
      // and could land on a HOST_VISIBLE type with no bit in common with the
      // buffer's allowed types) and allocate enough memory for it (req.size,
      // fixes VUID-vkBindBufferMemory-None-10741: previously we used the raw
      // data length, missing alignment overhead).  Both bugs corrupted the
      // staging upload and propagated to every restored buffer's contents.
      VkMemoryRequirements stagingReq{};
      if (!m_GpuReadbackHelper->QueryStagingBufferRequirements(
              deviceKey, buf->BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingReq)) {
        LOG_WARNING << "Vulkan2 subcapture: failed to query staging buffer requirements for key="
                    << bufKey;
        continue;
      }
      uint32_t stagingMemType =
          m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, stagingReq.memoryTypeBits);
      if (stagingMemType == UINT32_MAX) {
        LOG_WARNING << "Vulkan2 subcapture: no HOST_VISIBLE memory type satisfying buffer "
                       "memoryTypeBits=0x"
                    << std::hex << stagingReq.memoryTypeBits << std::dec
                    << " for buffer key=" << bufKey;
        continue;
      }

      EmitStagingUploadAndCopyBuffer(m_Recorder, deviceKey, queueKey, poolKey, bufKey,
                                     buf->BufferSize, stagingReq.size, stagingMemType, data);
      LOG_TRACE << "Vulkan2 subcapture: restored buffer content, key=" << bufKey
                << " size=" << buf->BufferSize << " allocSize=" << stagingReq.size;
    }
  }
}

void StateTrackingService::RestoreImageContents() {
  std::unordered_map<uint64_t, std::vector<uint64_t>> imagesByDevice;

  for (const auto& [key, sp] : m_States) {
    if (sp->Destroyed) {
      continue;
    }
    if (sp->CreationCommandId != CommandId::ID_VKCREATEIMAGE) {
      continue;
    }
    if (!m_RestoredThisPass.count(key)) {
      continue;
    }

    auto* img = static_cast<ImageState*>(sp.get());
    // Skip undefined/preinitialized — nothing to copy.
    if (img->CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
        img->CurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
      continue;
    }
    // Skip unbound images.
    if (img->BoundMemoryKey == 0) {
      continue;
    }
    // Multisampled images cannot be copied with vkCmdCopyImageToBuffer.
    if (img->Samples != VK_SAMPLE_COUNT_1_BIT) {
      continue;
    }
    //// Skip images without VK_IMAGE_USAGE_TRANSFER_SRC_BIT -- cannot use as copy source.
    //if (!(img->UsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
    //  continue;
    //}
    // Skip zero-size images.
    if (img->Extent.width == 0 || img->Extent.height == 0) {
      continue;
    }

    auto* mem = GetState<DeviceMemoryState>(img->BoundMemoryKey);
    if (!mem) {
      continue;
    }
    auto* devState = GetState<ObjectState>(img->ParentKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (physDevKey && m_GpuReadbackHelper->IsHostVisible(physDevKey, mem->MemoryTypeIndex)) {
      continue;
    }

    imagesByDevice[img->ParentKey].push_back(key);
  }

  for (auto& [deviceKey, imgKeys] : imagesByDevice) {
    uint64_t queueKey = 0, poolKey = 0;
    if (!FindQueueAndPool(m_States, deviceKey, queueKey, poolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: skipping image content restore for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(poolKey)) {
      LOG_WARNING << "Vulkan2 subcapture: queue or pool was not restored, skipping image "
                     "content restore for device key="
                  << deviceKey;
      continue;
    }

    auto* devState = GetState<ObjectState>(deviceKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (!physDevKey) {
      continue;
    }

    for (uint64_t imgKey : imgKeys) {
      auto* img = static_cast<ImageState*>(GetState(imgKey));
      if (!img) {
        continue;
      }

      std::vector<uint8_t> data;
      std::vector<VkBufferImageCopy> regions;

      if (!m_GpuReadbackHelper->ReadImage(
              deviceKey, physDevKey, queueKey, poolKey, imgKey, img->Format, img->Extent,
              img->MipLevels, img->ArrayLayers, img->Samples, img->CurrentLayout, data, regions)) {
        LOG_WARNING << "Vulkan2 subcapture: GPU readback failed for image key=" << imgKey;
        continue;
      }

      if (data.empty() || regions.empty()) {
        continue;
      }

      // Per-image staging memory query (same rationale as RestoreBufferContents
      // above): the previous code hoisted FindStagingMemoryType(_, 0xFFFFFFFF)
      // out of the loop so every image's staging buffer used the SAME
      // memoryTypeIndex, which need not be in the buffer's allowed
      // memoryTypeBits, and used data.size() as both bci.size AND
      // mai.allocationSize, missing the driver's alignment-rounded
      // requirement.  Querying per-image is one extra create/destroy per image
      // but is cheap compared to ReadImage and produces a spec-valid stream.
      const VkDeviceSize stagingDataSize = static_cast<VkDeviceSize>(data.size());
      VkMemoryRequirements stagingReq{};
      if (!m_GpuReadbackHelper->QueryStagingBufferRequirements(
              deviceKey, stagingDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingReq)) {
        LOG_WARNING << "Vulkan2 subcapture: failed to query staging buffer requirements for image"
                       " key="
                    << imgKey;
        continue;
      }
      uint32_t stagingMemType =
          m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, stagingReq.memoryTypeBits);
      if (stagingMemType == UINT32_MAX) {
        LOG_WARNING << "Vulkan2 subcapture: no HOST_VISIBLE memory type satisfying buffer "
                       "memoryTypeBits=0x"
                    << std::hex << stagingReq.memoryTypeBits << std::dec
                    << " for image key=" << imgKey;
        continue;
      }

      VkImageAspectFlags aspectMask = AspectMaskFromFormat(img->Format);
      EmitStagingUploadAndCopyImage(m_Recorder, deviceKey, queueKey, poolKey, imgKey, img->Format,
                                    img->Extent, img->CurrentLayout, aspectMask, stagingDataSize,
                                    stagingReq.size, stagingMemType, data, regions);

      img->ContentRestored = true;
      LOG_TRACE << "Vulkan2 subcapture: restored image content, key=" << imgKey << " "
                << img->Extent.width << "x" << img->Extent.height << " mips=" << img->MipLevels
                << " layers=" << img->ArrayLayers;
    }
  }
}

// ---------------------------------------------------------------------------
// EmitRawCommand
// ---------------------------------------------------------------------------

void StateTrackingService::EmitRawCommand(CommandId id, const std::vector<char>& encoded) {
  if (encoded.empty()) {
    return;
  }
  class RawSerializer : public stream::CommandSerializer {
  public:
    RawSerializer(CommandId cmdId, const std::vector<char>& data) : m_CmdId(cmdId) {
      // Base class m_DataSize is uint64_t; preserve full size to avoid
      // silently truncating very large encoded blobs.
      m_DataSize = static_cast<uint64_t>(data.size());
      m_Data.reset(new char[data.size()]);
      std::memcpy(m_Data.get(), data.data(), data.size());
    }
    uint32_t Id() const override {
      return static_cast<uint32_t>(m_CmdId);
    }

  private:
    CommandId m_CmdId;
  };
  m_Recorder.Record(RawSerializer(id, encoded));
}

} // namespace vulkan
} // namespace gits
