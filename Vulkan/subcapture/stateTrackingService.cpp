// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingService.h"
#include "analyzerResults.h"
#include "subcaptureFatal.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "commandSerializersFactory.h"
#include "commandCodersAuto.h"
#include "commandIdsAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "configurator.h"
#include "log.h"
#include "tools.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

namespace gits {
namespace vulkan {

namespace {

// Maximum queue families we query in one synthetic vkGetPhysicalDeviceQueueFamilyProperties
// during state restore.  Vulkan guarantees a small count (spec uses implementation-dependent
// upper bound; 64 matches legacy restore practice and all known drivers).
constexpr uint32_t kQueueFamilyQueryCapacity = 64;

// Mirrors VulkanLegacy/recorder/vulkanStateRestore.cpp RestoreVkDevices: emit
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

bool StateTrackingService::ShouldRestore(uint64_t key) const {
  if (!m_AnalyzerResults) {
    return true;
  }
  // Queues and command pools are always restored regardless of the analysis
  // results. They are few per device and cheap to recreate, and the restore
  // helpers (FindQueueAndPool / FindQueueAndPoolForFamily) need a live queue and
  // command pool of whatever family they pick. Pruning them would leave an
  // in-range vkQueueSubmit's VkQueue (or a helper's command pool) unmapped,
  // crashing vkQueueSubmitRunner::Run() on HandleMapService::GetHandle. Their
  // owning VkDevice is pulled in automatically by RestoreOne (parent-first).
  auto it = m_States.find(key);
  if (it != m_States.end()) {
    switch (it->second->CreationCommandId) {
    case CommandId::ID_VKGETDEVICEQUEUE:
    case CommandId::ID_VKGETDEVICEQUEUE2:
    case CommandId::ID_VKCREATECOMMANDPOOL:
      return true;
    default:
      break;
    }
  }
  return m_AnalyzerResults->RestoreObject(key);
}

// ---------------------------------------------------------------------------
// StoreState / RemoveState / HasState
// ---------------------------------------------------------------------------

void StateTrackingService::StoreState(std::unique_ptr<ObjectState> state) {
  uint64_t key = state->Key;
  // Key 0 denotes VK_NULL_HANDLE; no real object ever has key 0.  Inserting such
  // an entry would place it at m_States.begin() (the map is ordered by key), so
  // it would be the FIRST object handed to RestoreOne and emit a bogus creation
  // command with a null handle before any genuine object is restored.
  if (!key) {
    return;
  }
  m_States[key] = std::move(state);
}

void StateTrackingService::RemoveState(uint64_t key) {
  m_States.erase(key);
}

bool StateTrackingService::HasState(uint64_t key) const {
  return m_States.count(key) != 0;
}

// True for a Destroyed object whose state is retained *only* so RestoreBlasChain can
// transiently re-create it while replaying a chain op. Unlike the other
// Destroyed-but-kept object kinds (shader modules, pipeline layouts, ...), these must
// not be resurrected as an incidental dependency, which would leave a live handle
// parked on a capture/replay device address for the rest of the stream. Callers
// therefore treat them as no longer tracked.
bool StateTrackingService::IsChainRetainedOnly(const ObjectState* state) const {
  if (!state || !state->Destroyed) {
    return false;
  }
  switch (state->CreationCommandId) {
  case CommandId::ID_VKCREATEACCELERATIONSTRUCTUREKHR:
  case CommandId::ID_VKCREATEBUFFER:
  case CommandId::ID_VKALLOCATEMEMORY:
    return true;
  default:
    return false;
  }
}

void StateTrackingService::EnsureRestored(uint64_t key) {
  if (!key) {
    return;
  }
  ObjectState* state = GetState(key);
  if (!state || IsChainRetainedOnly(state)) {
    return;
  }
  RestoreOne(state);
}

// ---------------------------------------------------------------------------
// RestoreState -- public entry point
// ---------------------------------------------------------------------------

void StateTrackingService::RestoreState() {
  if (!m_Recorder.IsOpen()) {
    LOG_WARNING << "Vulkan subcapture: RestoreState called but recorder is not open";
    return;
  }

  LOG_INFO << "Vulkan subcapture: emitting state restore (" << m_States.size() << " objects)";

  m_RestoredThisPass.clear();
  m_DescriptorSetsAllocated.clear();
  m_CommandBuffersRecordingReplaySkipped.clear();
  m_TransientlyRestored.clear();
  m_RestoredInputRegionHashes.clear();

  // Emit StateRestoreBegin marker
  {
    StateRestoreBeginCommand cmd;
    m_Recorder.Record(StateRestoreBeginSerializer(cmd));
  }

  auto recordStatus = [this](MarkerUInt64Command::Value state) {
    m_Recorder.Record(MarkerUInt64Serializer(MarkerUInt64Command(state)));
  };

  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_BEGIN);

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
    if (!ShouldRestore(state->Key)) {
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
    if (!ShouldRestore(state->Key)) {
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
    if (!ShouldRestore(state->Key)) {
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
      if (!ShouldRestore(state->Key)) {
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
        LOG_WARNING << "Vulkan subcapture: cannot signal semaphores for device key=" << deviceKey
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
    if (!ShouldRestore(state->Key)) {
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

  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_END);

  // Restore GPU-local resource contents.  Must run after all objects are
  // created (m_RestoredThisPass is fully populated) and before
  // EmitImageLayoutTransitions (which skips images whose copy ends in the
  // correct layout already).
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_BEGIN);
  if (m_GpuReadbackHelper) {
    RestoreAccelerationStructureContents();
    RestoreBufferContents();
    RestoreImageContents();
  }
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_END);

  EmitImageLayoutTransitions();

  // Restore query-pool contents (reset + fake-fill written queries) so the
  // recording range can read results for queries that were written before the
  // cut.  Emitted before the device-wait-idle loop below so its transient
  // submits are drained along with the layout-transition submits.
  RestoreQueryPools();

  // Drain all pre-recording-range GPU work before the recording range begins.
  // Mirrors legacy Vulkan::FinishStateRestore (VulkanLegacy/recorder/vulkanStateRestore.cpp
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
  LOG_INFO << "Vulkan subcapture: state restore complete";
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
        if (!HasState(dep) || IsChainRetainedOnly(GetState(dep))) {
          LOG_WARNING << "Vulkan subcapture: skipping object key=" << state->Key
                      << " (commandId=" << static_cast<uint32_t>(state->CreationCommandId)
                      << ") because dependency key=" << dep << " is no longer tracked";
          return;
        }
        RestoreOne(GetState(dep));
        if (!m_RestoredThisPass.count(dep)) {
          LOG_WARNING << "Vulkan subcapture: skipping object key=" << state->Key
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
  case CommandId::ID_VKCREATEVIDEOSESSIONKHR:
    if (!RestoreVideoSession(state)) {
      return; // Do NOT insert into m_RestoredThisPass - dependents must skip this object.
    }
    break;
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
#ifdef VK_USE_PLATFORM_WIN32_KHR
  case CommandId::ID_VKCREATEWIN32SURFACEKHR:
    RestoreSurface(state);
    break;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  case CommandId::ID_VKCREATEXLIBSURFACEKHR:
    RestoreSurface(state);
    break;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  case CommandId::ID_VKCREATEXCBSURFACEKHR:
    RestoreSurface(state);
    break;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  case CommandId::ID_VKCREATEWAYLANDSURFACEKHR:
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

bool StateTrackingService::FindQueueAndPool(uint64_t deviceKey,
                                            uint64_t& outQueueKey,
                                            uint64_t& outPoolKey) const {
  return ::gits::vulkan::FindQueueAndPool(m_States, deviceKey, outQueueKey, outPoolKey);
}

uint64_t StateTrackingService::StoreAsBuildInputContent(std::vector<uint8_t> bytes) {
  // Widen the 32-bit XX hash with the size, then verify on collision so a genuine clash
  // never aliases two different byte ranges (which would upload the wrong geometry).
  uint64_t key = gits::ComputeHash(bytes.data(), bytes.size(), gits::THashType::XX);
  key ^= static_cast<uint64_t>(bytes.size()) * 0x9E3779B97F4A7C15ull;
  if (key == 0) {
    key = 0x9E3779B97F4A7C15ull; // reserve 0 to mean "no content"
  }
  for (;;) {
    auto it = m_AsBuildInputContent.find(key);
    if (it == m_AsBuildInputContent.end()) {
      m_AsBuildInputContent.emplace(key, std::move(bytes));
      return key;
    }
    if (it->second.size() == bytes.size() &&
        std::equal(it->second.begin(), it->second.end(), bytes.begin())) {
      return key; // identical content already stored - dedup
    }
    key = key * 0x100000001B3ull + 0x9E3779B1ull; // probe past a rare true collision
    if (key == 0) {
      key = 1;
    }
  }
}

const std::vector<uint8_t>* StateTrackingService::GetAsBuildInputContent(uint64_t hash) const {
  auto it = m_AsBuildInputContent.find(hash);
  return it == m_AsBuildInputContent.end() ? nullptr : &it->second;
}

void StateTrackingService::ApplyAsInputReadbacksAfterSubmit(uint64_t cbKey,
                                                            uint64_t submitQueueKey) {
  // Fire the analysis-pass hook first (queue-execution-synced TLAS readback).
  // Unset in the recording pass.
  if (m_OnCommandBufferSubmitted) {
    m_OnCommandBufferSubmitted(cbKey, submitQueueKey);
  }

  auto* cbState = GetState<CommandBufferState>(cbKey);
  if (!cbState || cbState->AsInputReadbacksAfterSubmit.empty() || !m_GpuReadbackHelper ||
      submitQueueKey == 0) {
    return;
  }
  const uint64_t deviceKey = cbState->ParentKey;

  // The copies were recorded into this CB and have now been submitted on submitQueueKey.
  // Drain the queue so they are complete before we map staging.
  m_GpuReadbackHelper->WaitQueueIdle(deviceKey, submitQueueKey);

  // On a resubmit we re-read fresh content, so a retained command's accumulated inputs
  // are cleared the first time we see it here - last submit wins.
  std::unordered_set<uint64_t> clearedRetainedThisCall;

  for (auto& pending : cbState->AsInputReadbacksAfterSubmit) {
    auto* asState = GetState<AccelerationStructureState>(pending.AsKey);
    if (!asState) {
      continue;
    }
    // A retained op is only replayable if its inputs were captured. A missing range
    // would be rebuilt over whatever the recreated input buffer happens to hold.
    // Strict membership, since RestoreBlasCommand also answers yes with no chain.
    const bool retainedByChain =
        m_AnalyzerResults && m_AnalyzerResults->IsRetainedBlasCommand(pending.CommandKey);
    std::vector<CapturedBuildInputBuffer> finalized;
    finalized.reserve(pending.Buffers.size());
    for (size_t b = 0; b < pending.Buffers.size(); ++b) {
      CapturedBuildInputBuffer buf = pending.Buffers[b];
      std::vector<uint8_t> allBytes;
      if (b < pending.Staging.size() &&
          m_GpuReadbackHelper->ReadStaged(pending.Staging[b], allBytes)) {
        // Regions were packed into staging in this exact order (see
        // GpuReadbackHelper::StageBufferRegions), so slice by running offset.
        VkDeviceSize offset = 0;
        for (auto& region : buf.Regions) {
          if (region.RangeSize == 0) {
            continue;
          }
          if (offset + region.RangeSize <= allBytes.size()) {
            std::vector<uint8_t> bytes(allBytes.begin() + static_cast<ptrdiff_t>(offset),
                                       allBytes.begin() +
                                           static_cast<ptrdiff_t>(offset + region.RangeSize));
            region.Hash = StoreAsBuildInputContent(std::move(bytes));
          } else if (retainedByChain) {
            FatalSubcaptureError("staged build input of acceleration structure key=" +
                                 std::to_string(pending.AsKey) +
                                 " (input buffer key=" + std::to_string(buf.BufferKey) +
                                 ", command key=" + std::to_string(pending.CommandKey) +
                                 ") read back short: " + std::to_string(allBytes.size()) +
                                 " bytes for a range ending at " +
                                 std::to_string(offset + region.RangeSize) +
                                 ", so this retained op's inputs cannot be reproduced");
          } else {
            region.Hash = 0;
          }
          offset += region.RangeSize;
        }
      } else {
        if (retainedByChain) {
          FatalSubcaptureError("failed to read back a staged build input of acceleration structure "
                               "key=" +
                               std::to_string(pending.AsKey) +
                               " (input buffer key=" + std::to_string(buf.BufferKey) +
                               ", command key=" + std::to_string(pending.CommandKey) +
                               "), so this retained op's inputs cannot be reproduced");
        }
        LOG_WARNING << "Vulkan subcapture: failed to read staged acceleration structure build "
                       "input (buffer key="
                    << buf.BufferKey << "); rebuild may be incomplete";
        for (auto& region : buf.Regions) {
          region.Hash = 0;
        }
      }
      finalized.push_back(std::move(buf));
    }
    // Route to the per-command chain-replay store (recording pass, retained BLAS
    // commands only): a multi-info build's dsts accumulate under one command key.
    if (m_AnalyzerResults && m_AnalyzerResults->UseAsChainRestore() &&
        m_AnalyzerResults->RestoreBlasCommand(pending.CommandKey)) {
      auto& rc = m_RetainedAsCommands[pending.CommandKey];
      if (clearedRetainedThisCall.insert(pending.CommandKey).second) {
        rc.Inputs.clear();
      }
      for (const auto& b : finalized) {
        rc.Inputs.push_back(b);
      }
    }
    // Last submit before the cut wins (matches "only the latest build per AS is
    // retained").
    asState->CapturedBuildInputs = std::move(finalized);
  }
  // Staging is not freed here: a reused CB re-executes the recorded copies on every
  // resubmit. FreeCommandBufferStagedReadbacks does it when the staged list is cleared.
}

void StateTrackingService::FreeCommandBufferStagedReadbacks(CommandBufferState& cb) {
  if (!m_GpuReadbackHelper) {
    return;
  }
  for (auto& pending : cb.AsInputReadbacksAfterSubmit) {
    for (const auto& staging : pending.Staging) {
      m_GpuReadbackHelper->FreeStaged(staging);
    }
    pending.Staging.clear();
  }
}

void StateTrackingService::MergeSecondaryAsInputReadbacks(uint64_t primaryKey,
                                                          uint64_t secondaryKey) {
  auto* prim = GetState<CommandBufferState>(primaryKey);
  auto* sec = GetState<CommandBufferState>(secondaryKey);
  if (!prim || !sec || sec->AsInputReadbacksAfterSubmit.empty()) {
    return;
  }
  for (const auto& pending : sec->AsInputReadbacksAfterSubmit) {
    prim->AsInputReadbacksAfterSubmit.push_back(pending);
  }
}

void StateTrackingService::StoreRetainedAsCommandBytes(uint64_t commandKey,
                                                       const std::vector<char>& bytes,
                                                       bool isCopy) {
  // Only relevant when the chain restore will run.
  // Not worth holding when the serialized-blob path is in use.
  if (!m_AnalyzerResults || !m_AnalyzerResults->UseAsChainRestore() ||
      !m_AnalyzerResults->RestoreBlasCommand(commandKey)) {
    return;
  }
  auto& rc = m_RetainedAsCommands[commandKey];
  rc.CommandBytes = bytes;
  rc.IsCopy = isCopy;
}

bool StateTrackingService::IsAcquiredSwapchainImage(const ImageState* img) {
  if (!img || img->CreationCommandId != CommandId::ID_VKGETSWAPCHAINIMAGESKHR) {
    return false;
  }
  const auto* sc = GetState<SwapchainState>(img->ParentKey);
  if (!sc) {
    return false;
  }
  for (uint32_t idx = 0; idx < static_cast<uint32_t>(sc->ImageKeys.size()); ++idx) {
    if (sc->ImageKeys[idx] == img->Key) {
      return sc->AcquiredImages.count(idx) != 0;
    }
  }
  return false;
}

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
    if (!ShouldRestore(state->Key)) {
      continue;
    }
    auto* img = static_cast<ImageState*>(state);

    if (img->ContentRestored) {
      continue;
    }

    // Layout-restore policy at the cut:
    //   * Regular image: transition to its tracked CurrentLayout; skip
    //     UNDEFINED / PREINITIALIZED (the second player creates it UNDEFINED).
    //   * Non-acquired swapchain image: always transition to PRESENT_SRC_KHR.
    //     Its tracked layout is reset to UNDEFINED by
    //     ImageLayoutService::OnQueuePresent, but the player may present any
    //     swapchain image during the index-rewind phase, so all of them must be
    //     in PRESENT_SRC_KHR when the subcapture starts.
    //   * Acquired swapchain image: owned by the application at the cut.  The
    //     vkAcquireNextImageKHR that RestoreSwapchain re-emits returns the image
    //     in UNDEFINED, so it needs the same tracked-layout restore as a regular
    //     image; otherwise the first recorded use mismatches (e.g. expecting
    //     PRESENT_SRC_KHR while UNDEFINED, VUID-vkCmdDraw-None-09600).
    const bool isSwapchainImage =
        (state->CreationCommandId == CommandId::ID_VKGETSWAPCHAINIMAGESKHR);
    const bool forcePresentSrc = isSwapchainImage && !IsAcquiredSwapchainImage(img);

    // Images that resolve to their tracked CurrentLayout (regular images and
    // acquired swapchain images) need no transition if that layout is
    // UNDEFINED / PREINITIALIZED.  Non-acquired swapchain images are forced to
    // PRESENT_SRC_KHR regardless of tracked layout.
    if (!forcePresentSrc && (img->CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
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
    if (!::gits::vulkan::FindQueueAndPool(m_States, deviceKey, queueKey, commandPoolKey)) {
      LOG_WARNING << "Vulkan subcapture: cannot emit image layout transitions for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(commandPoolKey)) {
      LOG_WARNING << "Vulkan subcapture: queue or pool was not restored, skipping image "
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
      if (isSwapchain && !IsAcquiredSwapchainImage(img)) {
        // Non-acquired swapchain image: force PRESENT_SRC_KHR for present rewind.
        targetLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      } else {
        // Regular image or acquired swapchain image: restore its tracked layout.
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

    LOG_INFO << "Vulkan subcapture: emitted layout transitions for " << barriers.size()
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
    bool anyReset = false;
    for (bool reset : qp->ResetQueries) {
      if (reset) {
        anyReset = true;
        break;
      }
    }
    bool anyUsed = false;
    for (bool used : qp->UsedQueries) {
      if (used) {
        anyUsed = true;
        break;
      }
    }
    // Restore every pool that was touched before the cut, exactly as legacy
    // gits::Vulkan::RestoreQueryPool (vulkanStateRestore.cpp) does: it iterates
    // all pools and, per pool, re-emits a vkCmdResetQueryPool for the queries
    // that were reset before the cut (resetQueries[i]) and a fake query for the
    // ones written before the cut (usedQueries[i]).  We reproduce that exactly
    // below; the only difference is structural -- pool *creation* happens in the
    // generic object-restore pass here, not in this function.
    //
    // A pool with neither reset nor written queries before the cut is left
    // untouched.  This is NOT an assumption about what the recording range does
    // with it: such a pool has no recorded queue family (RecordQueueFamily only
    // runs on a reset/use), so we cannot pick a transient command buffer for it,
    // and -- matching legacy -- its resetQueries/usedQueries are all false, so
    // legacy would emit no reset and no fake query for it either.  Whatever the
    // recording range later does (including resetting it before first use) is
    // self-contained and needs no pre-cut state to be reconstructed.
    if (!anyReset && !anyUsed) {
      continue;
    }
    // The pool itself must have been re-created this pass; otherwise the
    // player has no handle to remap our query commands onto.
    if (!m_RestoredThisPass.count(qp->Key)) {
      continue;
    }
    if (qp->RestoreQueueFamily == UINT32_MAX) {
      LOG_WARNING << "Vulkan subcapture: query pool key=" << qp->Key
                  << " needs query restore but has no recorded queue family; skipping its restore";
      continue;
    }
    if (qp->QueryType == static_cast<uint32_t>(VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR)) {
      // Result-status queries can only be reset and written inside a video coding
      // scope (VUID-vkCmdBeginQuery-queryType-09438).  Submitting even a bare
      // vkCmdResetQueryPool for such a pool to the video decode queue causes a
      // GPU hang on some drivers when no prior video coding scope has been opened
      // on that queue.  Skip these pools entirely; the captured video decode
      // commands will reset and repopulate them naturally.
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
        LOG_WARNING << "Vulkan subcapture: cannot restore query pools for device key=" << deviceKey
                    << " queue family=" << familyIndex
                    << " (no restored queue and command pool of that family)";
        continue;
      }
      if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(commandPoolKey)) {
        LOG_WARNING << "Vulkan subcapture: queue or pool was not restored, skipping query pool "
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

        // Reset every query that was in the post-reset state at the cut.  The
        // application's vkCmdResetQueryPool calls happened before the recording
        // range, so without re-emitting them a recording-range vkCmdBeginQuery /
        // vkCmdWriteTimestamp would hit an unreset query
        // (VUID-vkCmdBeginQuery-None-00807).  Contiguous reset runs are coalesced
        // into a single vkCmdResetQueryPool to minimise command count, mirroring
        // legacy gits::Vulkan::RestoreQueryPool (vulkanStateRestore.cpp).
        {
          const uint32_t resetCountTotal =
              std::min<uint32_t>(qp->QueryCount, static_cast<uint32_t>(qp->ResetQueries.size()));
          uint32_t start = 0;
          uint32_t count = 0;
          auto emitReset = [&](uint32_t firstQuery, uint32_t queryCount) {
            vkCmdResetQueryPoolCommand resetCmd;
            resetCmd.m_commandBuffer.Key = kTempCBKey;
            resetCmd.m_queryPool.Key = poolKey;
            resetCmd.m_firstQuery.Value = firstQuery;
            resetCmd.m_queryCount.Value = queryCount;
            m_Recorder.Record(vkCmdResetQueryPoolSerializer(resetCmd));
          };
          for (uint32_t i = 0; i < resetCountTotal; ++i) {
            if (qp->ResetQueries[i]) {
              ++count;
            } else {
              if (count > 0) {
                emitReset(start, count);
              }
              count = 0;
              start = i + 1;
            }
          }
          if (count > 0) {
            emitReset(start, count);
          }
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
          } else if (qp->QueryType == static_cast<uint32_t>(VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR)) {
            // vkCmdBeginQuery for result-status queries requires an active video
            // coding scope (VUID-vkCmdBeginQuery-queryType-09438). We cannot open
            // one here without a valid VkVideoSessionKHR. Skip the fake query;
            // the captured video decode commands will repopulate the status.
            continue;
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

      LOG_INFO << "Vulkan subcapture: restored " << fakeQueryCount << " query result(s) across "
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
      LOG_TRACE << "Vulkan subcapture: rewrote vkCreateGraphicsPipelines flags for state restore"
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
        LOG_TRACE << "Vulkan subcapture: rewrote vkCreateComputePipelines flags for state restore"
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
            << "Vulkan subcapture: rewrote vkCreateRayTracingPipelinesKHR flags for state restore"
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
            << "Vulkan subcapture: rewrote vkCreateRayTracingPipelinesNV flags for state restore"
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
  case CommandId::ID_VKCREATESAMPLERYCBCRCONVERSION:
    EMIT_DECODED(vkCreateSamplerYcbcrConversion)
  case CommandId::ID_VKCREATESAMPLERYCBCRCONVERSIONKHR:
    EMIT_DECODED(vkCreateSamplerYcbcrConversionKHR)
  case CommandId::ID_VKCREATECOMMANDPOOL:
    EMIT_DECODED(vkCreateCommandPool)
  case CommandId::ID_VKALLOCATECOMMANDBUFFERS:
    EMIT_DECODED(vkAllocateCommandBuffers)
#ifdef VK_USE_PLATFORM_WIN32_KHR
  case CommandId::ID_VKCREATEWIN32SURFACEKHR:
    EMIT_DECODED(vkCreateWin32SurfaceKHR)
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  case CommandId::ID_VKCREATEXLIBSURFACEKHR:
    EMIT_DECODED(vkCreateXlibSurfaceKHR)
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  case CommandId::ID_VKCREATEXCBSURFACEKHR:
    EMIT_DECODED(vkCreateXcbSurfaceKHR)
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  case CommandId::ID_VKCREATEWAYLANDSURFACEKHR:
    EMIT_DECODED(vkCreateWaylandSurfaceKHR)
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
  case CommandId::ID_VKCREATEVIDEOSESSIONKHR:
    EMIT_DECODED(vkCreateVideoSessionKHR)
  case CommandId::ID_VKCREATEVIDEOSESSIONPARAMETERSKHR:
    EMIT_DECODED(vkCreateVideoSessionParametersKHR)
  case CommandId::ID_VKCREATEEVENT:
    EMIT_DECODED(vkCreateEvent)
  case CommandId::ID_VKCREATEFENCE: {
    vkCreateFenceCommand cmd;
    Decode(buf, cmd);
    // Recreate the fence with the exact signaled state tracked at the cut so
    // that a first recorded vkWaitForFences / vkGetFenceStatus poll behaves as
    // it did originally.  Force the VK_FENCE_CREATE_SIGNALED_BIT to match
    // IsSignaled in BOTH directions: set it when signaled (else the poll hangs),
    // and CLEAR it when not signaled (else a fence whose recorded create-info
    // carried the signaled bit but was reset before the cut would be wrongly
    // restored signaled).  Mirrors legacy vulkanStateRestore.cpp:2164-2169.
    if (cmd.m_pCreateInfo.Value) {
      if (static_cast<FenceState*>(state)->IsSignaled) {
        cmd.m_pCreateInfo.Value->flags |= VK_FENCE_CREATE_SIGNALED_BIT;
      } else {
        cmd.m_pCreateInfo.Value->flags &= ~VK_FENCE_CREATE_SIGNALED_BIT;
      }
    }
    m_Recorder.Record(vkCreateFenceSerializer(cmd));
    break;
  }
  case CommandId::ID_VKCREATESEMAPHORE: {
    vkCreateSemaphoreCommand cmd;
    Decode(buf, cmd);
    m_Recorder.Record(vkCreateSemaphoreSerializer(cmd));
    // For timeline semaphores that were signaled beyond their create-time
    // initialValue, emit a host-side vkSignalSemaphore to advance the
    // counter to the value observed at the subcapture point.  Without this,
    // any recording-range vkWaitSemaphores(value=N) where N was produced by
    // a pre-subcapture signal would block forever.
    auto* sem = static_cast<SemaphoreState*>(state);
    if (!sem->IsBinary && sem->LastSignaledValue > 0) {
      static VkSemaphore kDummySemaphoreSlot = VK_NULL_HANDLE;
      VkSemaphoreSignalInfo signalInfo{};
      signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
      signalInfo.semaphore = kDummySemaphoreSlot; // remapped by player from HandleKeys[0]
      signalInfo.value = sem->LastSignaledValue;

      const auto* devState = GetState<DeviceState>(sem->ParentKey);
      if (devState && devState->HasTimelineSemaphoreKHR) {
        vkSignalSemaphoreKHRCommand signalCmd;
        signalCmd.m_device.Key = sem->ParentKey;
        signalCmd.m_pSignalInfo.Value = &signalInfo;
        signalCmd.m_pSignalInfo.HandleKeys = {sem->Key};
        signalCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkSignalSemaphoreKHRSerializer(signalCmd));
      } else {
        vkSignalSemaphoreCommand signalCmd;
        signalCmd.m_device.Key = sem->ParentKey;
        signalCmd.m_pSignalInfo.Value = &signalInfo;
        signalCmd.m_pSignalInfo.HandleKeys = {sem->Key};
        signalCmd.m_Return.Value = VK_SUCCESS;
        m_Recorder.Record(vkSignalSemaphoreSerializer(signalCmd));
      }
    }
    break;
  }
  default:
    LOG_WARNING << "Vulkan subcapture: unhandled CommandId "
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
    win.m_DisplayProtocol.Value = surf->Protocol;
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

#ifdef VK_USE_PLATFORM_XLIB_KHR
  if (state->CreationCommandId == CommandId::ID_VKCREATEXLIBSURFACEKHR &&
      !state->CreationCommandBuffer.empty() && surf->HwndKey != 0) {
    std::vector<char> scratch = state->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateXlibSurfaceKHRCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->dpy = reinterpret_cast<Display*>(surf->HwndKey);
      cmd.m_pCreateInfo.Value->window = reinterpret_cast<Window>(surf->HinstanceKey);
    }
    m_Recorder.Record(vkCreateXlibSurfaceKHRSerializer(cmd));
    return;
  }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
  if (state->CreationCommandId == CommandId::ID_VKCREATEXCBSURFACEKHR &&
      !state->CreationCommandBuffer.empty() && surf->HwndKey != 0) {
    std::vector<char> scratch = state->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateXcbSurfaceKHRCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->connection = reinterpret_cast<xcb_connection_t*>(surf->HwndKey);
      cmd.m_pCreateInfo.Value->window = static_cast<xcb_window_t>(surf->HinstanceKey);
    }
    m_Recorder.Record(vkCreateXcbSurfaceKHRSerializer(cmd));
    return;
  }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  if (state->CreationCommandId == CommandId::ID_VKCREATEWAYLANDSURFACEKHR &&
      !state->CreationCommandBuffer.empty() && surf->HwndKey != 0) {
    std::vector<char> scratch = state->CreationCommandBuffer;
    char* buf = scratch.data();
    vkCreateWaylandSurfaceKHRCommand cmd;
    Decode(buf, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->display = reinterpret_cast<wl_display*>(surf->HwndKey);
      cmd.m_pCreateInfo.Value->surface = reinterpret_cast<wl_surface*>(surf->HinstanceKey);
    }
    m_Recorder.Record(vkCreateWaylandSurfaceKHRSerializer(cmd));
    return;
  }
#endif

  if (!EmitCreationCommand(state)) {
    LOG_WARNING << "Vulkan subcapture: failed to emit creation command for surface key="
                << state->Key;
  }
}

bool StateTrackingService::RestoreBuffer(ObjectState* state) {
  auto* buf = static_cast<BufferState*>(state);

  if (buf->CreationCommandBuffer.empty()) {
    return false;
  }

  // Emit vkCreateBuffer BEFORE restoring bound memory. For dedicated
  // allocations VkMemoryDedicatedAllocateInfo::buffer references this buffer,
  // so its handle must be registered in HandleMapService before vkAllocateMemory
  // is emitted; otherwise ResolvePNextHandleKeys crashes on the missing key.
  // For non-dedicated allocations the order makes no difference.
  //
  // EmitCreationCommand is not used here: the stored usage is what the application
  // requested, which may not include TRANSFER_DST, and the content-restore paths upload
  // through vkCmdCopyBuffer. Mirrors RestoreImage's OR-in of TRANSFER_DST below,
  // including its KNOWN LIMITATION about memory requirements shifting.
  {
    std::vector<char> scratch = buf->CreationCommandBuffer;
    char* bufPtr = scratch.data();
    vkCreateBufferCommand cmd;
    Decode(bufPtr, cmd);
    if (cmd.m_pCreateInfo.Value) {
      cmd.m_pCreateInfo.Value->usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    m_Recorder.Record(vkCreateBufferSerializer(cmd));
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
      LOG_WARNING << "Vulkan subcapture: skipping buffer memory bind for buffer key=" << buf->Key
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

bool StateTrackingService::RestoreVideoSession(ObjectState* stateBase) {
  auto* state = static_cast<VideoSessionState*>(stateBase);

  // Emit vkCreateVideoSessionKHR before restoring bound memory. For dedicated
  // allocations VkMemoryDedicatedAllocateInfo may reference this session, so
  // its handle must be registered in HandleMapService first.
  if (!EmitCreationCommand(stateBase)) {
    return false;
  }

  // Mark as restored early to break the potential circular dependency:
  // video-session memory is typically dedicated, so its VkMemoryDedicatedAllocateInfo
  // carries the session key, creating a session<->memory back-edge.
  m_RestoredThisPass.insert(stateBase->Key);

  if (!state->BindCommandBuffer.empty()) {
    for (uint64_t memKey : state->MemoryKeys) {
      if (!memKey) {
        continue;
      }
      auto* memState = GetState(memKey);
      if (memState) {
        RestoreOne(memState);
      }
    }

    // Re-emit the encoded vkBindVideoSessionMemoryKHR. Decode modifies the
    // buffer in-place (AddPtrs), so work on a copy to keep BindCommandBuffer
    // pristine for any future re-emission.
    std::vector<char> scratch = state->BindCommandBuffer;
    char* buf = scratch.data();
    vkBindVideoSessionMemoryKHRCommand bindCmd;
    Decode(buf, bindCmd);
    m_Recorder.Record(vkBindVideoSessionMemoryKHRSerializer(bindCmd));
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
  // Mirrors legacy VulkanLegacy/recorder/vulkanStateRestore.cpp line ~734:
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
  // VulkanLegacy/interceptor/include/vulkanExecWrap.h) so the captured
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
      LOG_WARNING << "Vulkan subcapture: skipping image memory bind for image key=" << img->Key
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
      LOG_WARNING << "Vulkan subcapture: skipping image view key=" << iv->Key
                  << " because image key=" << iv->ImageKey << " is no longer tracked";
      return false;
    }
    // Ensure the image itself is restored first (parent-first ordering).
    RestoreOne(GetState(iv->ImageKey));
    if (!m_RestoredThisPass.count(iv->ImageKey)) {
      LOG_WARNING << "Vulkan subcapture: skipping image view key=" << iv->Key
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
  // Mirrors legacy VulkanLegacy/recorder/vulkanStateRestore.cpp line ~622:
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
    LOG_WARNING << "Vulkan subcapture: failed to emit vkCreateSwapchainKHR for swapchain key="
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
  // (vulkanStateRestore.cpp), which emits one vkAcquireNextImageKHR per index in
  // acquiredImages with a VK_NULL_HANDLE semaphore and fence.  Passing both as
  // null is technically a spec violation (VUID-vkAcquireNextImageKHR-semaphore-
  // 01780), but it is a benign one: the call only rewinds the swapchain's
  // internal acquire index so the recording range's own acquire returns the
  // expected image, and matching legacy keeps the two backends identical.  The
  // re-acquired image arrives in UNDEFINED layout; EmitImageLayoutTransitions
  // (run later in RestoreState) transitions it to the layout tracked at the cut.
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
    LOG_WARNING << "Vulkan subcapture: skipping descriptor set key=" << ds->Key
                << " because CreationCommandBuffer is empty (vkAllocateDescriptorSets blob was "
                   "never stored)";
    return false;
  }

  if (ds->PoolKey) {
    RestoreOne(GetState(ds->PoolKey));
    if (!m_RestoredThisPass.count(ds->PoolKey)) {
      LOG_WARNING << "Vulkan subcapture: skipping descriptor set key=" << ds->Key
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
        LOG_WARNING << "Vulkan subcapture: skipping descriptor set key=" << ds->Key
                    << " because EmitCreationCommand failed (likely a missing dependency such as "
                       "VkDescriptorSetLayout key="
                    << (ds->DependencyKeys.empty() ? 0 : ds->DependencyKeys.front()) << ")";
        return false;
      }
      m_DescriptorSetsAllocated.insert(ds->Key);
    } else {
      AllocateDescriptorSetBatchForPool(ds->PoolKey);
      if (!m_DescriptorSetsAllocated.count(ds->Key)) {
        LOG_WARNING << "Vulkan subcapture: skipping descriptor set key=" << ds->Key
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
        LOG_WARNING << "Vulkan subcapture: omitting descriptor set key=" << ds->Key
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
      LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because VkCommandPool key=" << cb->PoolKey << " is no longer tracked";
      return CommandBufferRestoreOutcome::FailedNoAllocation;
    }
    RestoreOne(poolState);
    if (!m_RestoredThisPass.count(cb->PoolKey)) {
      LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because VkCommandPool key=" << cb->PoolKey << " could not be restored";
      return CommandBufferRestoreOutcome::FailedNoAllocation;
    }
  }

  if (!EmitCreationCommand(state)) {
    LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
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
    if (!HasState(dep) || IsChainRetainedOnly(GetState(dep))) {
      LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because referenced object key=" << dep << " is no longer tracked";
      return CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped;
    }
    RestoreOne(GetState(dep));
    if (!m_RestoredThisPass.count(dep)) {
      LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
                  << " because referenced object key=" << dep << " could not be restored";
      return CommandBufferRestoreOutcome::AllocationOkRecordingReplaySkipped;
    }
    if (m_CommandBuffersRecordingReplaySkipped.count(dep)) {
      LOG_WARNING << "Vulkan subcapture: omitting restore of VkCommandBuffer key=" << cb->Key
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

// ---------------------------------------------------------------------------
// Content restore is streamed to the player as a compact manifest
// (RestoreContentManifest) plus exactly one RestoreContentData token per
// manifest entry (a zero-length region when a resource's readback failed).
// All transient staging objects (buffers, memory, command buffers, fences)
// live on the player side (RestoreContentService), which sizes them from live
// device memory and batches the uploads, flushing and tearing down once it has
// consumed manifest-many data tokens.  Nothing staging-related is written into
// the stream, and the recorder streams the readback bytes one resource at a
// time so peak host memory stays bounded to a single resource.
// ---------------------------------------------------------------------------

// Synthetic GITSKeys for temporary staging resources created in the stream.
// Must not collide with real keys (which are sequential starting from 1).
// kTempCBKey = UINT64_MAX-1 is used by EmitImageLayoutTransitions.
static constexpr uint64_t kStagingBufKey = static_cast<uint64_t>(-3);
static constexpr uint64_t kStagingMemKey = static_cast<uint64_t>(-4);
static constexpr uint64_t kContentCBKey = static_cast<uint64_t>(-5);
// Base for the (buffer,memory) key pairs synthesized during a single
// EmitAccelerationStructureRebuild: its scratch buffer(s) and any transient build-input
// buffers. Allocated downward within one rebuild call and torn down at its end, so the
// range is reused across acceleration structures.
static constexpr uint64_t kRebuildTransientKeyBase = static_cast<uint64_t>(-64);
// Base for the (buffer,memory) key pairs synthesized by RestoreBlasChain for a relocated
// acceleration structure. Allocated downward across the whole chain replay, so it must
// stay disjoint from kRebuildTransientKeyBase, which every rebuild in the chain reuses.
static constexpr uint64_t kChainTransientKeyBase = static_cast<uint64_t>(-4096);

// Defined below, next to EmitCaptureReplayBufferCreate. Declared here so the
// serialize/deserialize path can build its staging buffer the same way.
static VkBufferCreateInfo MakeCaptureReplayBufferCreateInfo(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VkBufferOpaqueCaptureAddressCreateInfo* opaqueAddrCI);

// ---------------------------------------------------------------------------
// Content-restore policy
//
// Images/textures: ALWAYS restored via GPU readback + staging upload,
// regardless of the bound memory's host-visibility.  An OPTIMAL-tiled image has
// an opaque, vendor-specific memory layout, so raw host bytes (the CPU shadow /
// mapped-memory path) cannot reconstruct it -- only vkCmdCopyBufferToImage can.
// GPU-written content is also invisible to the CPU-write tracker, so a GPU copy
// is the only mechanism that restores image contents correctly.
//
// Buffers: ALWAYS restored via GPU readback + staging upload, PLUS a guarded
// "compare-and-skip" optimization for HOST_VISIBLE buffers.  The CPU shadow
// (RestoreMappedMemory) restores host-visible memory on replay with a cheap host
// memcpy and NO GPU copy.  When that shadow already holds the exact live content
// of a buffer, streaming the readback bytes and issuing a player-side
// vkCmdCopyBuffer is pure waste, so we skip it (emit a zero-length content token)
// and let the mapped-memory restore cover the buffer.
//
// Compare-and-skip is only correct when BOTH hold (any uncertainty -> copy):
//   (1) COVERAGE: the buffer's entire bound range
//       [MemoryOffset, MemoryOffset+BufferSize) lies inside the single
//       contiguous dirty interval [ShadowDirtyBegin, ShadowDirtyEnd) that
//       RestoreMappedMemory emits.  Bytes outside that interval are NOT written
//       by the mapped-memory restore (freshly allocated memory is undefined), so
//       equality alone is insufficient.
//   (2) EQUALITY: the shadow bytes for that range match the live GPU readback
//       byte-for-byte.
// If the GPU wrote content the CPU tracker never saw (e.g. ReBAR
// DEVICE_LOCAL|HOST_VISIBLE render targets), equality fails and the buffer is
// copied.  Device-local buffers are never host-visible/shadowed, so they always
// copy.
// ---------------------------------------------------------------------------

namespace {

// Returns true when the CPU shadow of `mem` fully covers `buf`'s entire bound
// range and its bytes match the live GPU readback `data` exactly.  See the
// coverage + equality rules documented above; callers MUST fall back to a GPU
// copy whenever this returns false (conservative, always-correct default).
bool ShadowFullyCoversAndMatches(const BufferState& buf,
                                 const DeviceMemoryState& mem,
                                 const std::vector<uint8_t>& data) {
  // A shadow must exist to rely on the mapped-memory restore.
  if (mem.ShadowBuffer.empty()) {
    return false;
  }
  // The readback must have produced exactly the buffer's bytes.
  if (data.size() != static_cast<size_t>(buf.BufferSize)) {
    return false;
  }
  const VkDeviceSize begin = buf.MemoryOffset;
  const VkDeviceSize end = buf.MemoryOffset + buf.BufferSize;
  // Overflow / out-of-range guard against the shadow storage.
  if (end < begin || end > static_cast<VkDeviceSize>(mem.ShadowBuffer.size())) {
    return false;
  }
  // (1) COVERAGE: whole bound range inside [ShadowDirtyBegin, ShadowDirtyEnd).
  if (mem.ShadowDirtyEnd <= mem.ShadowDirtyBegin || begin < mem.ShadowDirtyBegin ||
      end > mem.ShadowDirtyEnd) {
    return false;
  }
  // (2) EQUALITY: shadow bytes for the range match the live readback.
  return std::memcmp(data.data(), mem.ShadowBuffer.data() + static_cast<size_t>(begin),
                     static_cast<size_t>(buf.BufferSize)) == 0;
}

} // namespace

static void EmitStagingUploadAndCopyBuffer(SubcaptureRecorder& recorder,
                                           uint64_t deviceKey,
                                           uint64_t queueKey,
                                           uint64_t commandPoolKey,
                                           uint64_t dstBufKey,
                                           VkDeviceSize dstOffset,
                                           VkDeviceSize bufSize,
                                           VkDeviceSize stagingAllocationSize,
                                           uint32_t stagingMemTypeIndex,
                                           const std::vector<uint8_t>& data) {

  // --- Create staging buffer ---
  // bci.size is the buffer's logical length. The memory allocated must be >= the
  // requirements-reported size (alignment-rounded), passed in as stagingAllocationSize.
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

  VkBufferCopy copyRegion{0, dstOffset, bufSize};

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

// ---------------------------------------------------------------------------
// Emit stream commands that create a staging buffer holding a serialized acceleration
// structure blob, then deserialize it into dstAsKey via
// vkCmdCopyMemoryToAccelerationStructureKHR.
//
// Both the buffer- and memory-side captured opaque addresses are re-supplied, so the
// driver assigns the buffer the same device address it had at analysis time. That is
// what lets capturedDeviceAddress be hardcoded into the copy command's src.deviceAddress
// even though the buffer does not exist yet when the stream is authored.
// ---------------------------------------------------------------------------

static void EmitAccelerationStructureDeserialize(SubcaptureRecorder& recorder,
                                                 uint64_t deviceKey,
                                                 uint64_t queueKey,
                                                 uint64_t commandPoolKey,
                                                 uint64_t dstAsKey,
                                                 VkDeviceSize dataSize,
                                                 VkDeviceSize stagingAllocationSize,
                                                 uint32_t stagingMemTypeIndex,
                                                 VkDeviceAddress capturedDeviceAddress,
                                                 uint64_t capturedOpaqueCaptureAddress,
                                                 uint64_t capturedMemoryOpaqueCaptureAddress,
                                                 const std::vector<uint8_t>& data) {

  // --- Create staging buffer (capture/replay-stable address) ---
  VkBufferOpaqueCaptureAddressCreateInfo opaqueAddrCI{};
  opaqueAddrCI.sType = VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
  opaqueAddrCI.opaqueCaptureAddress = capturedOpaqueCaptureAddress;

  VkBufferCreateInfo bci = MakeCaptureReplayBufferCreateInfo(
      dataSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, &opaqueAddrCI);

  vkCreateBufferCommand createBufCmd;
  createBufCmd.m_device.Key = deviceKey;
  createBufCmd.m_pCreateInfo.Value = &bci;
  createBufCmd.m_pBuffer.Key = kStagingBufKey;
  createBufCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkCreateBufferSerializer(createBufCmd));

  VkMemoryOpaqueCaptureAddressAllocateInfo memOpaqueAddrCI{};
  memOpaqueAddrCI.sType = VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
  memOpaqueAddrCI.opaqueCaptureAddress = capturedMemoryOpaqueCaptureAddress;

  VkMemoryAllocateFlagsInfo allocFlagsInfo{};
  allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  allocFlagsInfo.pNext = &memOpaqueAddrCI;
  allocFlagsInfo.flags =
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.pNext = &allocFlagsInfo;
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

  // --- Upload serialized bytes into staging via map + MappedDataMetaCommand ---
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
  region.Size = dataSize;
  region.Data = const_cast<char*>(reinterpret_cast<const char*>(data.data()));
  mdc.m_Regions.Regions.push_back(region);
  mdc.m_Regions.Size = 1;
  recorder.Record(MappedDataMetaSerializer(mdc));

  vkUnmapMemoryCommand unmapCmd;
  unmapCmd.m_device.Key = deviceKey;
  unmapCmd.m_memory.Key = kStagingMemKey;
  recorder.Record(vkUnmapMemorySerializer(unmapCmd));

  // --- Allocate one-shot CB and issue the deserialize copy ---
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

  // Barrier: staging buffer host-write -> the AS-build stage that performs the
  // deserialize copy, whose source reads use VK_ACCESS_TRANSFER_READ_BIT.
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = reinterpret_cast<VkBuffer>(0x1ULL); // sentinel
  barrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrierCommand barrierCmd;
  barrierCmd.m_commandBuffer.Key = kContentCBKey;
  barrierCmd.m_srcStageMask.Value = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  barrierCmd.m_dstStageMask.Value = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
  barrierCmd.m_dependencyFlags.Value = 0;
  barrierCmd.m_memoryBarrierCount.Value = 0;
  barrierCmd.m_bufferMemoryBarrierCount.Value = 1;
  barrierCmd.m_pBufferMemoryBarriers.Value = &barrier;
  barrierCmd.m_pBufferMemoryBarriers.Size = 1;
  barrierCmd.m_pBufferMemoryBarriers.HandleKeys = {kStagingBufKey};
  barrierCmd.m_imageMemoryBarrierCount.Value = 0;
  recorder.Record(vkCmdPipelineBarrierSerializer(barrierCmd));

  VkCopyMemoryToAccelerationStructureInfoKHR copyInfo{};
  copyInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
  copyInfo.src.deviceAddress = capturedDeviceAddress;
  copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR;

  vkCmdCopyMemoryToAccelerationStructureKHRCommand copyCmd;
  copyCmd.m_commandBuffer.Key = kContentCBKey;
  copyCmd.m_pInfo.Value = &copyInfo;
  copyCmd.m_pInfo.HandleKeys = {dstAsKey};
  recorder.Record(vkCmdCopyMemoryToAccelerationStructureKHRSerializer(copyCmd));

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

  vkFreeMemoryCommand freeMemCmd2;
  freeMemCmd2.m_device.Key = deviceKey;
  freeMemCmd2.m_memory.Key = kStagingMemKey;
  recorder.Record(vkFreeMemorySerializer(freeMemCmd2));
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

// The single definition of what a capture/replay transient buffer looks like. Both
// EmitCaptureReplayBufferCreate and QueryCaptureReplayBufferRequirements must go through
// here, because the driver's requirements depend on these fields: the capture/replay flag
// can raise the reported alignment, so probing without it under-reports req.size and the
// buffer's tail ends up unbacked.
static VkBufferCreateInfo MakeCaptureReplayBufferCreateInfo(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VkBufferOpaqueCaptureAddressCreateInfo* opaqueAddrCI) {
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = opaqueAddrCI;
  bci.flags = VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  bci.size = size;
  bci.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return bci;
}

// Create a buffer at a specific capture/replay address (buffer- and memory-side
// opaque capture addresses), under caller-supplied synthetic keys, so a replayed build
// resolves the same device addresses.
static void EmitCaptureReplayBufferCreate(SubcaptureRecorder& recorder,
                                          uint64_t deviceKey,
                                          uint64_t bufKey,
                                          uint64_t memKey,
                                          VkDeviceSize bufferSize,
                                          VkDeviceSize allocationSize,
                                          uint32_t memTypeIndex,
                                          VkBufferUsageFlags usage,
                                          uint64_t opaqueCaptureAddress,
                                          uint64_t memoryOpaqueCaptureAddress) {
  VkBufferOpaqueCaptureAddressCreateInfo opaqueAddrCI{};
  opaqueAddrCI.sType = VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
  opaqueAddrCI.opaqueCaptureAddress = opaqueCaptureAddress;

  VkBufferCreateInfo bci = MakeCaptureReplayBufferCreateInfo(bufferSize, usage, &opaqueAddrCI);

  vkCreateBufferCommand createBufCmd;
  createBufCmd.m_device.Key = deviceKey;
  createBufCmd.m_pCreateInfo.Value = &bci;
  createBufCmd.m_pBuffer.Key = bufKey;
  createBufCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkCreateBufferSerializer(createBufCmd));

  VkMemoryOpaqueCaptureAddressAllocateInfo memOpaqueAddrCI{};
  memOpaqueAddrCI.sType = VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
  memOpaqueAddrCI.opaqueCaptureAddress = memoryOpaqueCaptureAddress;

  VkMemoryAllocateFlagsInfo allocFlagsInfo{};
  allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  allocFlagsInfo.pNext = &memOpaqueAddrCI;
  allocFlagsInfo.flags =
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.pNext = &allocFlagsInfo;
  mai.allocationSize = allocationSize;
  mai.memoryTypeIndex = memTypeIndex;

  vkAllocateMemoryCommand allocMemCmd;
  allocMemCmd.m_device.Key = deviceKey;
  allocMemCmd.m_pAllocateInfo.Value = &mai;
  allocMemCmd.m_pMemory.Key = memKey;
  allocMemCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkAllocateMemorySerializer(allocMemCmd));

  vkBindBufferMemoryCommand bindCmd;
  bindCmd.m_device.Key = deviceKey;
  bindCmd.m_buffer.Key = bufKey;
  bindCmd.m_memory.Key = memKey;
  bindCmd.m_memoryOffset.Value = 0;
  bindCmd.m_Return.Value = VK_SUCCESS;
  recorder.Record(vkBindBufferMemorySerializer(bindCmd));
}

static void EmitCaptureReplayBufferDestroy(SubcaptureRecorder& recorder,
                                           uint64_t deviceKey,
                                           uint64_t bufKey,
                                           uint64_t memKey) {
  vkDestroyBufferCommand destroyBufCmd;
  destroyBufCmd.m_device.Key = deviceKey;
  destroyBufCmd.m_buffer.Key = bufKey;
  recorder.Record(vkDestroyBufferSerializer(destroyBufCmd));

  vkFreeMemoryCommand freeMemCmd;
  freeMemCmd.m_device.Key = deviceKey;
  freeMemCmd.m_memory.Key = memKey;
  recorder.Record(vkFreeMemorySerializer(freeMemCmd));
}

void StateTrackingService::EmitAccelerationStructureRebuild(
    uint64_t deviceKey,
    uint64_t physDevKey,
    uint64_t queueKey,
    uint64_t poolKey,
    const AccelerationStructureState& asState) {
  // The per-AS last-build path replays the AS's single stored build command. No source
  // map: only that one op is retained, so nothing produced the contents an update would
  // refit from, and an update-mode info aborts the run instead of becoming a build.
  EmitAccelerationStructureRebuildBytes(deviceKey, physDevKey, queueKey, poolKey,
                                        asState.LastBuildCommandBytes, asState.CapturedBuildInputs,
                                        asState.Key);
}

bool StateTrackingService::QueryCaptureReplayBufferRequirements(uint64_t deviceKey,
                                                                VkDeviceSize size,
                                                                VkBufferUsageFlags usage,
                                                                VkMemoryRequirements& outReq) {
  // Probe without the opaque-address pNext: the requirements do not depend on *which*
  // address is requested, and naming one would transiently claim an address we are about
  // to hand to the real buffer. Everything that does affect them is identical.
  VkBufferCreateInfo bci = MakeCaptureReplayBufferCreateInfo(size, usage, nullptr);
  return m_GpuReadbackHelper->QueryBufferRequirements(deviceKey, bci, outReq);
}

void StateTrackingService::EmitAccelerationStructureRebuildBytes(
    uint64_t deviceKey,
    uint64_t physDevKey,
    uint64_t queueKey,
    uint64_t poolKey,
    const std::vector<char>& commandBytes,
    const std::vector<CapturedBuildInputBuffer>& capturedInputs,
    uint64_t logAsKey,
    const std::unordered_map<uint64_t, uint64_t>* updateSourceByDstAs) {
  if (commandBytes.empty()) {
    return;
  }
  // Decode mutates the source buffer (AddPtrs), so work on a copy. Its m_commandBuffer
  // holds the original app CB's key, which refers to nothing at restore-emission time and
  // is patched to the one-shot CB allocated below - after the input uploads, which each
  // run their own one-shot submit and must not nest inside this recording.
  std::vector<char> scratch = commandBytes;
  vkCmdBuildAccelerationStructuresKHRCommand cmd;
  Decode(scratch.data(), cmd);
  cmd.m_commandBuffer.Key = kContentCBKey;

  // Synthetic (buffer,memory) keys for this rebuild's transients - recreated inputs and
  // fresh scratch - allocated downward and destroyed after the build.
  uint64_t nextTransientKey = kRebuildTransientKeyBase;
  auto newTransientKey = [&nextTransientKey]() { return nextTransientKey--; };
  std::vector<std::pair<uint64_t, uint64_t>> transientBufs; // (bufKey, memKey)

  // A build input recreated at a freshly reserved address needs the build command's baked
  // geometry addresses that point into it relocated to the new base.
  struct InputRemap {
    VkDeviceAddress OldBase;
    VkDeviceSize Size;
    VkDeviceAddress NewBase;
  };
  std::vector<InputRemap> inputRemaps;

  // Capture/replay address ranges already owned by objects restored this pass. An app may
  // free an AS build input and the driver later hand that same range to a different object
  // that is live at the cut, so recreating the input at its captured address would be
  // rejected with VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS - detect the recycling and
  // relocate the input instead.
  //
  // Buffer- and memory-side opaque addresses come out of one shared linear address space,
  // so they go into a single set and either kind is tested against all of it. Ranges are
  // compared by overlap, not equal base: a recycled input typically lands in the middle of
  // a larger live allocation.
  struct ClaimedRange {
    uint64_t Begin;
    uint64_t End; // exclusive
  };
  std::vector<ClaimedRange> claimedRanges;
  auto rangeEnd = [](uint64_t base, VkDeviceSize size) {
    // A zero/unknown size still reserves its base address.
    const VkDeviceSize span = size ? size : 1;
    return (span > UINT64_MAX - base) ? UINT64_MAX : base + span;
  };
  auto addClaimedRange = [&claimedRanges, &rangeEnd](uint64_t base, VkDeviceSize size) {
    if (base == 0) {
      return;
    }
    claimedRanges.push_back({base, rangeEnd(base, size)});
  };
  auto collidesWithClaimed = [&claimedRanges, &rangeEnd](uint64_t base, VkDeviceSize size) {
    if (base == 0) {
      return false;
    }
    const uint64_t end = rangeEnd(base, size);
    for (const auto& r : claimedRanges) {
      if (base < r.End && r.Begin < end) {
        return true;
      }
    }
    return false;
  };
  for (const auto& [key, sp] : m_States) {
    if (sp->Destroyed || !m_RestoredThisPass.count(key)) {
      continue;
    }
    if (auto* b = dynamic_cast<BufferState*>(sp.get())) {
      addClaimedRange(b->OpaqueCaptureAddress, b->BufferSize);
    } else if (auto* m = dynamic_cast<DeviceMemoryState*>(sp.get())) {
      addClaimedRange(m->OpaqueCaptureAddress, m->AllocationSize);
    }
  }

  // Inputs that shared one backing allocation cannot each be recreated as a dedicated
  // allocation at its captured address - the second would request an address the first
  // already took. Count the uses so those are relocated instead.
  std::unordered_map<uint64_t, uint32_t> capturedInputsPerAllocation;
  for (const auto& in : capturedInputs) {
    if (in.Size && in.MemoryOpaqueCaptureAddress) {
      ++capturedInputsPerAllocation[in.MemoryOpaqueCaptureAddress];
    }
  }
  auto sharesCapturedAllocation = [&capturedInputsPerAllocation](uint64_t memOpaque) {
    auto it = capturedInputsPerAllocation.find(memOpaque);
    return it != capturedInputsPerAllocation.end() && it->second > 1;
  };

  // --- Recreate/refill build-input buffers from captured content ---
  // The app may have destroyed the vertex/index/transform/instances buffers right after the
  // build. Recreate each at its original capture/replay device address (so the build's baked
  // geometry addresses resolve unchanged) and upload the snapshotted bytes. An input still
  // restored as a normal object this pass is reused instead - its address is already claimed
  // - and only refilled, since RestoreBufferContents runs later than the build.
  for (const auto& in : capturedInputs) {
    if (in.Size == 0) {
      continue;
    }
    uint64_t dstBufKey = in.BufferKey;
    const bool reuseExisting = m_RestoredThisPass.count(in.BufferKey) != 0;
    if (!reuseExisting) {
      const VkBufferUsageFlags usage =
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      VkMemoryRequirements req{};
      if (!QueryCaptureReplayBufferRequirements(deviceKey, in.Size, usage, req)) {
        // The input buffer would be missing entirely from the replay, leaving the
        // op's baked geometry addresses pointing at nothing.
        FatalSubcaptureError(
            "failed to query requirements for a recreated build input of acceleration structure "
            "key=" +
            std::to_string(logAsKey) + " (input buffer key=" + std::to_string(in.BufferKey) + ")");
      }

      // Can this input be recreated verbatim, at its captured address? Not if
      //  - its captured range (req.size, the span the driver reserves) collides with one a
      //    live restored object already claimed, or
      //  - it was suballocated at a non-zero offset of its backing allocation, or shared
      //    that allocation with another input of this build, so a dedicated allocation
      //    based at the captured allocation address reproduces neither, or
      //  - nothing pins its address at all (neither opaque address was captured).
      // Each case is relocated to a freshly reserved address instead, with the build's
      // baked geometry addresses patched to match.
      const bool addressRecycled = collidesWithClaimed(in.BufferOpaqueCaptureAddress, req.size) ||
                                   collidesWithClaimed(in.MemoryOpaqueCaptureAddress, req.size);
      const bool allocationNotReproducible =
          in.MemoryOffset != 0 || sharesCapturedAllocation(in.MemoryOpaqueCaptureAddress);
      const bool addressUnpinned =
          in.BufferOpaqueCaptureAddress == 0 && in.MemoryOpaqueCaptureAddress == 0;
      const bool mustRelocate = addressRecycled || allocationNotReproducible || addressUnpinned;

      uint64_t bufOpaque = in.BufferOpaqueCaptureAddress;
      uint64_t memOpaque = in.MemoryOpaqueCaptureAddress;
      uint32_t memType = UINT32_MAX;
      VkDeviceSize allocSize = req.size;

      if (mustRelocate) {
        // Reserve a brand-new address and record a remap so the build command's baked
        // geometry addresses into this buffer are relocated below. Relocation needs the
        // original base address. Without it the build cannot be patched.
        VkDeviceAddress freshDeviceAddress = 0;
        VkMemoryRequirements freshReq{};
        if (in.BaseDeviceAddress != 0 &&
            m_GpuReadbackHelper->ReserveScratchBufferAddress(
                deviceKey, physDevKey, in.Size, freshDeviceAddress, bufOpaque, memOpaque) &&
            QueryCaptureReplayBufferRequirements(deviceKey, in.Size, usage, freshReq)) {
          memType = m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, freshReq.memoryTypeBits);
          allocSize = freshReq.size;
        }
        if (memType == UINT32_MAX) {
          LOG_WARNING << "Vulkan subcapture: could not relocate acceleration structure build input "
                         "buffer that cannot be recreated at its captured address (orig key="
                      << in.BufferKey << ", AS key=" << logAsKey << ", recycled=" << addressRecycled
                      << ", suballocated=" << allocationNotReproducible
                      << ", unpinned=" << addressUnpinned
                      << "); the rebuild is emitted without this input";
          continue;
        }
        inputRemaps.push_back({in.BaseDeviceAddress, in.Size, freshDeviceAddress});
      } else {
        memType = in.MemoryTypeIndex;
        if (memType == UINT32_MAX || !((req.memoryTypeBits >> memType) & 1u)) {
          memType = UINT32_MAX;
          for (uint32_t t = 0; t < 32; ++t) {
            if ((req.memoryTypeBits >> t) & 1u) {
              memType = t;
              break;
            }
          }
        }
        if (memType == UINT32_MAX) {
          LOG_WARNING << "Vulkan subcapture: no memory type for recreated acceleration structure "
                         "input buffer (orig key="
                      << in.BufferKey << ")";
          continue;
        }
      }
      const uint64_t bufKey = newTransientKey();
      const uint64_t memKey = newTransientKey();
      EmitCaptureReplayBufferCreate(m_Recorder, deviceKey, bufKey, memKey, in.Size, allocSize,
                                    memType, usage, bufOpaque, memOpaque);
      // The addresses this input just took are claimed for the rest of the
      // rebuild, so a later input of the same build is tested against them too.
      addClaimedRange(bufOpaque, allocSize);
      addClaimedRange(memOpaque, allocSize);
      transientBufs.emplace_back(bufKey, memKey);
      dstBufKey = bufKey;
    }

    // Upload each referenced sub-range from the content store via a host-visible staging
    // buffer. For a reused buffer, skip a range whose identical bytes an earlier rebuild
    // this pass already uploaded to the same (buffer, offset) slot. A freshly recreated
    // transient buffer is empty, so it is never deduped.
    for (const auto& region : in.Regions) {
      if (region.RangeSize == 0) {
        continue;
      }
      const std::vector<uint8_t>* bytes = GetAsBuildInputContent(region.Hash);
      if (!bytes) {
        // Replaying the op without this range builds the structure over whatever the
        // recreated input buffer happens to contain - for instance data, addresses that
        // point nowhere.
        FatalSubcaptureError(
            "no captured content for a build input of acceleration structure key=" +
            std::to_string(logAsKey) + " (input buffer key=" + std::to_string(in.BufferKey) +
            ", offset=" + std::to_string(region.SrcOffset) + ", " +
            std::to_string(region.RangeSize) + " bytes), so its build cannot be replayed");
      }
      if (reuseExisting) {
        const auto slot = std::make_pair(in.BufferKey, region.SrcOffset);
        auto it = m_RestoredInputRegionHashes.find(slot);
        if (it != m_RestoredInputRegionHashes.end() && it->second == region.Hash) {
          continue; // identical bytes already uploaded to this stable buffer slot
        }
        m_RestoredInputRegionHashes[slot] = region.Hash;
      }
      // As above: skipping the upload would leave this range of the input buffer
      // holding whatever it holds, and the replayed op would build over it.
      VkMemoryRequirements stagingReq{};
      if (!m_GpuReadbackHelper->QueryStagingBufferRequirements(
              deviceKey, region.RangeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingReq)) {
        FatalSubcaptureError("failed to query staging requirements for a build input of "
                             "acceleration structure key=" +
                             std::to_string(logAsKey) +
                             " (input buffer key=" + std::to_string(in.BufferKey) + ")");
      }
      const uint32_t stagingMemType =
          m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, stagingReq.memoryTypeBits);
      if (stagingMemType == UINT32_MAX) {
        FatalSubcaptureError(
            "no HOST_VISIBLE memory type for a build input of acceleration structure key=" +
            std::to_string(logAsKey) + " (input buffer key=" + std::to_string(in.BufferKey) +
            ", memoryTypeBits=" + std::to_string(stagingReq.memoryTypeBits) + ")");
      }
      EmitStagingUploadAndCopyBuffer(m_Recorder, deviceKey, queueKey, poolKey, dstBufKey,
                                     region.SrcOffset, region.RangeSize, stagingReq.size,
                                     stagingMemType, *bytes);
    }
  }

  // Relocate the build command's baked geometry device addresses for any relocated input.
  // Mirrors CollectGeometryInputBufferKeys' field walk. TLAS-instance BLAS references live
  // in the buffer content, not these fields, and are unaffected.
  if (!inputRemaps.empty()) {
    auto relocate = [&inputRemaps](VkDeviceAddress& addr) {
      if (addr == 0) {
        return;
      }
      for (const auto& r : inputRemaps) {
        if (addr >= r.OldBase && addr < r.OldBase + r.Size) {
          addr = r.NewBase + (addr - r.OldBase);
          return;
        }
      }
    };
    for (uint32_t i = 0; i < cmd.m_infoCount.Value && cmd.m_pInfos.Value; ++i) {
      VkAccelerationStructureBuildGeometryInfoKHR& info = cmd.m_pInfos.Value[i];
      for (uint32_t g = 0; g < info.geometryCount; ++g) {
        VkAccelerationStructureGeometryKHR* geom = nullptr;
        if (info.pGeometries) {
          geom = const_cast<VkAccelerationStructureGeometryKHR*>(&info.pGeometries[g]);
        } else if (info.ppGeometries && info.ppGeometries[g]) {
          geom = const_cast<VkAccelerationStructureGeometryKHR*>(info.ppGeometries[g]);
        }
        if (!geom) {
          continue;
        }
        switch (geom->geometryType) {
        case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
          auto& tri = geom->geometry.triangles;
          relocate(tri.vertexData.deviceAddress);
          if (tri.indexType != VK_INDEX_TYPE_NONE_KHR) {
            relocate(tri.indexData.deviceAddress);
          }
          relocate(tri.transformData.deviceAddress);
          break;
        }
        case VK_GEOMETRY_TYPE_AABBS_KHR:
          relocate(geom->geometry.aabbs.data.deviceAddress);
          break;
        case VK_GEOMETRY_TYPE_INSTANCES_KHR:
          if (!geom->geometry.instances.arrayOfPointers) {
            relocate(geom->geometry.instances.data.deviceAddress);
          }
          break;
        default:
          break;
        }
      }
    }
  }

  // --- Per info: keep the recorded mode, repoint the source, reserve fresh scratch ---
  // Scratch is pure working memory (never captured), so a new one is always allocated and
  // scratchData repointed at it. The mode is left exactly as recorded, so the emitted
  // stream performs the same operations the application did. Rewriting an update into a
  // build would also be invalid on a compacted destination, which
  // VUID-vkCmdBuildAccelerationStructuresKHR-pInfos-10126 allows to be as small as its
  // COMPACTED_SIZE query result while a build must satisfy the full build size.
  for (uint32_t i = 0; i < cmd.m_infoCount.Value && cmd.m_pInfos.Value &&
                       2 * static_cast<size_t>(i) + 1 < cmd.m_pInfos.HandleKeys.size();
       ++i) {
    VkAccelerationStructureBuildGeometryInfoKHR& info = cmd.m_pInfos.Value[i];
    const bool isUpdate = info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    const uint64_t dstAsKey = cmd.m_pInfos.HandleKeys[2 * static_cast<size_t>(i) + 1];

    // Repoint an update at the source the reduced chain actually produced. The recorded
    // one may be an intermediate the reduction dropped. Handles travel only via
    // HandleKeys (see generator_coders.py), so that slot is the one to write.
    if (isUpdate) {
      uint64_t chainSrcAsKey = 0;
      if (updateSourceByDstAs) {
        auto srcIt = updateSourceByDstAs->find(dstAsKey);
        if (srcIt != updateSourceByDstAs->end()) {
          chainSrcAsKey = srcIt->second;
        }
      }
      if (!chainSrcAsKey) {
        const std::string what =
            "acceleration structure key=" + std::to_string(dstAsKey) +
            " must be restored by replaying an update (command key=" + std::to_string(cmd.m_Key) +
            "), but the structure that update refits from is not known, so it cannot be replayed";
        if (!updateSourceByDstAs) {
          // The per-AS path: no operation chain is tracked for this structure, so
          // there is no predecessor to refit from. Top-level structures live here.
          FatalSubcaptureError(
              what +
              ". No operation chain is tracked for it - TLAS update (refit) "
              "by the application are not supported yet. Set "
              "Common.Player.Subcapture.Vulkan.CaptureASBuildInputs=false to restore all "
              "acceleration structures from serialized blobs instead (replayable only on this GPU "
              "and driver)");
        }
        FatalSubcaptureError(what + ". Delete '" + AnalyzerResults::GetAnalysisFileName() +
                             "' and re-run so the analysis pass regenerates it");
      }
      cmd.m_pInfos.HandleKeys[2 * static_cast<size_t>(i)] = chainSrcAsKey;
    }

    std::vector<uint32_t> maxPrimitiveCounts(info.geometryCount, 0);
    if (i < cmd.m_ppBuildRangeInfos.Data.size()) {
      const auto& ranges = cmd.m_ppBuildRangeInfos.Data[i];
      for (uint32_t g = 0; g < info.geometryCount && g < ranges.size(); ++g) {
        maxPrimitiveCounts[g] = ranges[g].primitiveCount;
      }
    }

    // vkGetAccelerationStructureBuildSizesKHR ignores mode, srcAccelerationStructure and
    // dstAccelerationStructure, so one call yields the sizes for either mode. They are
    // still neutralized in a local copy: the decoded handles belong to the recorded
    // process and must not reach the driver.
    VkAccelerationStructureBuildGeometryInfoKHR sizeQueryInfo = info;
    sizeQueryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    sizeQueryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    sizeQueryInfo.dstAccelerationStructure = VK_NULL_HANDLE;

    VkAccelerationStructureBuildSizesInfoKHR sizes{};
    if (!m_GpuReadbackHelper->QueryAccelerationStructureBuildSizes(
            deviceKey, sizeQueryInfo, maxPrimitiveCounts.data(), sizes) ||
        sizes.buildScratchSize == 0) {
      // Without the sizes there is no scratch to give the op. Emitting it against the
      // application's long-freed scratch address would fault on replay.
      FatalSubcaptureError(
          "failed to query build sizes for acceleration structure key=" + std::to_string(dstAsKey) +
          " (command key=" + std::to_string(cmd.m_Key) + "), so its build cannot be replayed");
    }

    // Sanity check, not a policy decision: the op replays in its recorded mode, so the size
    // the application satisfied is the size needed. A smaller destination means the size
    // query or the tracked create size is wrong.
    auto* dstAsState = GetState<AccelerationStructureState>(dstAsKey);
    if (!isUpdate && dstAsState && dstAsState->Size &&
        sizes.accelerationStructureSize > dstAsState->Size) {
      FatalSubcaptureError("acceleration structure key=" + std::to_string(dstAsKey) +
                           " was created with size=" + std::to_string(dstAsState->Size) +
                           " but replaying its build (command key=" + std::to_string(cmd.m_Key) +
                           ") needs " + std::to_string(sizes.accelerationStructureSize));
    }

    // An update needs only updateScratchSize. Reserve whatever the replayed mode
    // requires, never less than the driver asked for.
    const VkDeviceSize scratchSize =
        isUpdate && sizes.updateScratchSize ? sizes.updateScratchSize : sizes.buildScratchSize;

    VkDeviceAddress scratchAddress = 0;
    uint64_t scratchOpaqueAddress = 0;
    uint64_t scratchMemOpaqueAddress = 0;
    VkMemoryRequirements scratchReq{};
    uint32_t scratchMemType = UINT32_MAX;
    if (m_GpuReadbackHelper->ReserveScratchBufferAddress(deviceKey, physDevKey, scratchSize,
                                                         scratchAddress, scratchOpaqueAddress,
                                                         scratchMemOpaqueAddress) &&
        QueryCaptureReplayBufferRequirements(deviceKey, scratchSize,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, scratchReq)) {
      scratchMemType =
          m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, scratchReq.memoryTypeBits);
    }
    if (scratchMemType == UINT32_MAX) {
      // Same reasoning as the size query above: the alternative is an op pointed at
      // the application's freed scratch address.
      FatalSubcaptureError("could not reserve a scratch buffer for acceleration structure key=" +
                           std::to_string(dstAsKey) + " (command key=" + std::to_string(cmd.m_Key) +
                           "), so its build cannot be replayed");
    }
    const uint64_t scratchBufKey = newTransientKey();
    const uint64_t scratchMemKey = newTransientKey();
    EmitCaptureReplayBufferCreate(m_Recorder, deviceKey, scratchBufKey, scratchMemKey, scratchSize,
                                  scratchReq.size, scratchMemType,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, scratchOpaqueAddress,
                                  scratchMemOpaqueAddress);
    transientBufs.emplace_back(scratchBufKey, scratchMemKey);
    info.scratchData.deviceAddress = scratchAddress;
  }

  // Allocate and begin the build's one-shot command buffer only now - after all
  // input uploads have run and freed their own use of kContentCBKey.
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  cbai.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL); // sentinel

  static VkCommandBuffer kDummyCB = VK_NULL_HANDLE;
  vkAllocateCommandBuffersCommand allocCBCmd;
  allocCBCmd.m_device.Key = deviceKey;
  allocCBCmd.m_pAllocateInfo.Value = &cbai;
  allocCBCmd.m_pAllocateInfo.HandleKeys = {poolKey};
  allocCBCmd.m_pCommandBuffers.Value = &kDummyCB;
  allocCBCmd.m_pCommandBuffers.Size = 1;
  allocCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  allocCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkAllocateCommandBuffersSerializer(allocCBCmd));

  VkCommandBufferBeginInfo cbbi{};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBufferCommand beginCBCmd;
  beginCBCmd.m_commandBuffer.Key = kContentCBKey;
  beginCBCmd.m_pBeginInfo.Value = &cbbi;
  beginCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkBeginCommandBufferSerializer(beginCBCmd));

  m_Recorder.Record(vkCmdBuildAccelerationStructuresKHRSerializer(cmd));

  vkEndCommandBufferCommand endCBCmd;
  endCBCmd.m_commandBuffer.Key = kContentCBKey;
  endCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkEndCommandBufferSerializer(endCBCmd));

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
  m_Recorder.Record(vkQueueSubmitSerializer(submitCmd));

  vkQueueWaitIdleCommand waitCmd;
  waitCmd.m_queue.Key = queueKey;
  waitCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkQueueWaitIdleSerializer(waitCmd));

  static VkCommandBuffer kDummyCBFree = VK_NULL_HANDLE;
  vkFreeCommandBuffersCommand freeCBCmd;
  freeCBCmd.m_device.Key = deviceKey;
  freeCBCmd.m_commandPool.Key = poolKey;
  freeCBCmd.m_commandBufferCount.Value = 1;
  freeCBCmd.m_pCommandBuffers.Value = &kDummyCBFree;
  freeCBCmd.m_pCommandBuffers.Size = 1;
  freeCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  m_Recorder.Record(vkFreeCommandBuffersSerializer(freeCBCmd));

  // Tear down every transient (recreated input + fresh scratch) now the build
  // has completed and its results live in the AS backing buffer.
  for (const auto& [bufKey, memKey] : transientBufs) {
    EmitCaptureReplayBufferDestroy(m_Recorder, deviceKey, bufKey, memKey);
  }

  // All transient addresses for this rebuild have been authored. A later rebuild may
  // safely reuse the freed addresses: its transients live in a separate command stream.
  m_GpuReadbackHelper->ReleaseReservedAddresses();
}

// Restore TLAS asKey's content by replaying the last build command against re-uploaded inputs.
void StateTrackingService::RestoreAccelerationStructureByRebuild(
    uint64_t asKey, uint64_t deviceKey, uint64_t physDevKey, uint64_t queueKey, uint64_t poolKey) {
  if (m_RebuiltAsKeys.count(asKey)) {
    return;
  }
  const std::string useBlobs =
      ". Set Common.Player.Subcapture.Vulkan.CaptureASBuildInputs=false to restore all "
      "acceleration structures from serialized blobs instead (replayable only on this GPU and "
      "driver)";

  auto* asState = GetState<AccelerationStructureState>(asKey);
  if (!asState || asState->LastBuildCommandBytes.empty()) {
    FatalSubcaptureError("acceleration structure key=" + std::to_string(asKey) +
                         " must be restored, but no acceleration structure build that writes "
                         "it was recorded before the subcapture range, so it cannot be rebuilt" +
                         useBlobs);
  }

  // An array-of-pointers TLAS cannot be rebuilt from inputs: its instance structs are
  // scattered behind a pointer table whose values are only known at build execution, so the
  // recording pass captures no instance content for it.
  if (asState->ArrayOfPointersInstances) {
    FatalSubcaptureError(
        "acceleration structure key=" + std::to_string(asKey) +
        " is built from array-of-pointers instances, whose instance structs are only reachable "
        "at build execution, so the recording pass captured no content to rebuild it from" +
        useBlobs);
  }

  // An update-mode build's source AS (src != dst) is carried in DependencyKeys. Rebuild it
  // first so this AS does not refit from uninitialized source content.
  //
  // EmitAccelerationStructureRebuild replays the *entire* captured build command, which may
  // update several destinations at once (LastBuildSiblingAsKeys), each refitting from its
  // own source. So every sibling's sources are walked here, not just this AS's.
  std::vector<uint64_t> siblingKeys = asState->LastBuildSiblingAsKeys;
  if (siblingKeys.empty()) {
    siblingKeys.push_back(asKey);
  }
  for (uint64_t sibKey : siblingKeys) {
    auto* sibState = GetState<AccelerationStructureState>(sibKey);
    if (!sibState) {
      continue;
    }
    for (uint64_t dep : sibState->DependencyKeys) {
      if (GetState<AccelerationStructureState>(dep)) {
        RestoreAccelerationStructureByRebuild(dep, deviceKey, physDevKey, queueKey, poolKey);
      }
    }
  }

  // The build (emitted below) regenerates the AS backing-buffer content.
  MarkAccelerationStructureBackingContentRestored(asKey);

  // The rebuild is self-contained: it recreates the build inputs from the captured content
  // and reserves a fresh scratch, so no live input buffers are required here.
  EmitAccelerationStructureRebuild(deviceKey, physDevKey, queueKey, poolKey, *asState);
  for (uint64_t sibling : asState->LastBuildSiblingAsKeys) {
    m_RebuiltAsKeys.insert(sibling);
  }
  m_RebuiltAsKeys.insert(asKey);
  LOG_TRACE << "Vulkan subcapture: restored acceleration structure content via rebuild, key="
            << asKey;
}

void StateTrackingService::EmitAccelerationStructureCopyReplay(
    uint64_t deviceKey,
    uint64_t queueKey,
    uint64_t poolKey,
    const std::vector<char>& commandBytes) {
  if (commandBytes.empty()) {
    return;
  }
  // Decode mutates the buffer in place (AddPtrs), so work on a copy. The stored
  // bytes carry the original app CB key, which is patched to the one-shot CB.
  std::vector<char> scratch = commandBytes;
  vkCmdCopyAccelerationStructureKHRCommand cmd;
  Decode(scratch.data(), cmd);
  cmd.m_commandBuffer.Key = kContentCBKey;

  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  cbai.commandPool = reinterpret_cast<VkCommandPool>(0x1ULL); // sentinel
  static VkCommandBuffer kDummyCB = VK_NULL_HANDLE;
  vkAllocateCommandBuffersCommand allocCBCmd;
  allocCBCmd.m_device.Key = deviceKey;
  allocCBCmd.m_pAllocateInfo.Value = &cbai;
  allocCBCmd.m_pAllocateInfo.HandleKeys = {poolKey};
  allocCBCmd.m_pCommandBuffers.Value = &kDummyCB;
  allocCBCmd.m_pCommandBuffers.Size = 1;
  allocCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  allocCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkAllocateCommandBuffersSerializer(allocCBCmd));

  VkCommandBufferBeginInfo cbbi{};
  cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBufferCommand beginCBCmd;
  beginCBCmd.m_commandBuffer.Key = kContentCBKey;
  beginCBCmd.m_pBeginInfo.Value = &cbbi;
  beginCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkBeginCommandBufferSerializer(beginCBCmd));

  m_Recorder.Record(vkCmdCopyAccelerationStructureKHRSerializer(cmd));

  vkEndCommandBufferCommand endCBCmd;
  endCBCmd.m_commandBuffer.Key = kContentCBKey;
  endCBCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkEndCommandBufferSerializer(endCBCmd));

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
  m_Recorder.Record(vkQueueSubmitSerializer(submitCmd));

  vkQueueWaitIdleCommand waitCmd;
  waitCmd.m_queue.Key = queueKey;
  waitCmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkQueueWaitIdleSerializer(waitCmd));

  static VkCommandBuffer kDummyCBFree = VK_NULL_HANDLE;
  vkFreeCommandBuffersCommand freeCBCmd;
  freeCBCmd.m_device.Key = deviceKey;
  freeCBCmd.m_commandPool.Key = poolKey;
  freeCBCmd.m_commandBufferCount.Value = 1;
  freeCBCmd.m_pCommandBuffers.Value = &kDummyCBFree;
  freeCBCmd.m_pCommandBuffers.Size = 1;
  freeCBCmd.m_pCommandBuffers.Keys = {kContentCBKey};
  m_Recorder.Record(vkFreeCommandBuffersSerializer(freeCBCmd));
}

bool StateTrackingService::EmitRelocatedAccelerationStructureCreate(
    uint64_t deviceKey,
    uint64_t physDevKey,
    const AccelerationStructureState& asState,
    uint64_t bufKey,
    uint64_t memKey) {
  if (!m_GpuReadbackHelper || asState.CreationCommandBuffer.empty() || asState.Size == 0 ||
      asState.CreationCommandId != CommandId::ID_VKCREATEACCELERATIONSTRUCTUREKHR) {
    return false;
  }

  // A dedicated storage buffer sized exactly to the acceleration structure, on a
  // capture/replay address the driver has just told us is free.
  const VkBufferUsageFlags usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  VkDeviceAddress freshDeviceAddress = 0;
  uint64_t bufOpaque = 0;
  uint64_t memOpaque = 0;
  VkMemoryRequirements req{};
  if (!m_GpuReadbackHelper->ReserveScratchBufferAddress(deviceKey, physDevKey, asState.Size,
                                                        freshDeviceAddress, bufOpaque, memOpaque) ||
      !QueryCaptureReplayBufferRequirements(deviceKey, asState.Size, usage, req)) {
    return false;
  }
  const uint32_t memType =
      m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, req.memoryTypeBits);
  if (memType == UINT32_MAX) {
    return false;
  }

  // Re-emit the original create against the relocated buffer. The captured deviceAddress is
  // dropped along with the create flag that gives it meaning - that address is taken, which
  // is the whole reason for relocating.
  std::vector<char> scratch = asState.CreationCommandBuffer;
  vkCreateAccelerationStructureKHRCommand cmd;
  Decode(scratch.data(), cmd);
  if (!cmd.m_pCreateInfo.Value || cmd.m_pCreateInfo.HandleKeys.empty()) {
    return false;
  }

  EmitCaptureReplayBufferCreate(m_Recorder, deviceKey, bufKey, memKey, asState.Size, req.size,
                                memType, usage, bufOpaque, memOpaque);

  // Handle members travel exclusively via HandleKeys (see generator_coders.py).
  cmd.m_pCreateInfo.HandleKeys[0] = bufKey;
  cmd.m_pCreateInfo.Value->offset = 0;
  cmd.m_pCreateInfo.Value->deviceAddress = 0;
  cmd.m_pCreateInfo.Value->createFlags &= ~static_cast<VkAccelerationStructureCreateFlagsKHR>(
      VK_ACCELERATION_STRUCTURE_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR);
  cmd.m_Return.Value = VK_SUCCESS;
  m_Recorder.Record(vkCreateAccelerationStructureKHRSerializer(cmd));
  return true;
}

void StateTrackingService::RestoreBlasChain() {
  if (!m_AnalyzerResults) {
    return;
  }
  const std::vector<BlasChainOp>& chain = m_AnalyzerResults->GetBlasChain();
  if (chain.empty()) {
    return;
  }

  // Per-device queue/pool/physDev context, resolved lazily and cached.
  struct DevCtx {
    uint64_t PhysDev{};
    uint64_t Queue{};
    uint64_t Pool{};
    bool Ok{};
  };
  std::unordered_map<uint64_t, DevCtx> devCtxByDevice;
  auto resolveDev = [&](uint64_t deviceKey) -> const DevCtx* {
    auto it = devCtxByDevice.find(deviceKey);
    if (it != devCtxByDevice.end()) {
      return it->second.Ok ? &it->second : nullptr;
    }
    DevCtx ctx{};
    uint64_t queueKey = 0, poolKey = 0;
    if (::gits::vulkan::FindQueueAndPool(m_States, deviceKey, queueKey, poolKey) &&
        m_RestoredThisPass.count(queueKey) && m_RestoredThisPass.count(poolKey)) {
      auto* devState = GetState<ObjectState>(deviceKey);
      if (devState && devState->ParentKey) {
        ctx = {devState->ParentKey, queueKey, poolKey, true};
      }
    }
    const DevCtx& stored = (devCtxByDevice[deviceKey] = ctx);
    return stored.Ok ? &stored : nullptr;
  };

  // Reduced-chain source AS per (command key, destination AS), handed to
  // EmitAccelerationStructureRebuildBytes so it can repoint each retained update at the
  // structure this chain really produces. Multi-info commands contribute one entry per
  // destination.
  std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>> updateSourceByCmd;
  for (const BlasChainOp& op : chain) {
    if (!op.IsCopy && op.SrcAsKey) {
      updateSourceByCmd[op.CommandKey][op.DstAsKey] = op.SrcAsKey;
    }
  }

  // Acceleration-structure handles a retained command references when it replays, i.e. the
  // set the player resolves through HandleMapService. Destinations come straight out of the
  // encoded HandleKeys, since a multi-info command replays wholesale, including destinations
  // that are not themselves retained. Sources come from the reduced chain rather than the
  // bytes, because that is what the emit patches the source slot to. A copy is replayed
  // verbatim and reads both [src, dst] by handle.
  auto referencedAsKeys = [&updateSourceByCmd](const RetainedAsCommand& rc, uint64_t commandKey) {
    std::vector<uint64_t> keys;
    std::vector<char> scratch = rc.CommandBytes;
    if (rc.IsCopy) {
      vkCmdCopyAccelerationStructureKHRCommand cmd;
      Decode(scratch.data(), cmd);
      for (uint64_t key : cmd.m_pInfo.HandleKeys) {
        if (key) {
          keys.push_back(key);
        }
      }
    } else {
      vkCmdBuildAccelerationStructuresKHRCommand cmd;
      Decode(scratch.data(), cmd);
      const auto& handleKeys = cmd.m_pInfos.HandleKeys; // [src, dst] per info
      for (size_t i = 1; i < handleKeys.size(); i += 2) {
        if (handleKeys[i]) {
          keys.push_back(handleKeys[i]);
        }
      }
      auto cmdIt = updateSourceByCmd.find(commandKey);
      if (cmdIt != updateSourceByCmd.end()) {
        for (const auto& [dstAsKey, srcAsKey] : cmdIt->second) {
          if (srcAsKey && srcAsKey != dstAsKey) {
            keys.push_back(srcAsKey);
          }
        }
      }
    }
    return keys;
  };

  // Resolve the required handles per chain entry once, then note the last entry that needs
  // each one. A transiently re-created acceleration structure is torn down as soon as the
  // chain stops reading it, reproducing the lifetime the application gave it: it freed each
  // intermediate BLAS before allocating the next, so keeping them all alive at once would
  // collide on a recycled capture/replay address.
  std::vector<std::vector<uint64_t>> requiredByOp(chain.size());
  std::unordered_map<uint64_t, size_t> lastUse;
  for (size_t i = 0; i < chain.size(); ++i) {
    auto rcIt = m_RetainedAsCommands.find(chain[i].CommandKey);
    if (rcIt == m_RetainedAsCommands.end() || rcIt->second.CommandBytes.empty()) {
      // The analysis says this op is needed but the recording pass never captured it.
      // Usually a stale analysis file whose command keys no longer match this run.
      FatalSubcaptureError(
          "no captured command bytes for retained acceleration structure chain op (command key=" +
          std::to_string(chain[i].CommandKey) +
          ", destination AS key=" + std::to_string(chain[i].DstAsKey) + "); delete '" +
          AnalyzerResults::GetAnalysisFileName() +
          "' and re-run so the analysis pass regenerates it");
    }
    requiredByOp[i] = referencedAsKeys(rcIt->second, chain[i].CommandKey);
    for (uint64_t key : requiredByOp[i]) {
      lastUse[key] = i;
    }
  }

  // Resources we created for an acceleration structure the application had
  // already destroyed, to be released again after its last chain use.
  struct TransientAs {
    uint64_t DeviceKey{};
    uint64_t BufferKey{}; // 0 when the storage buffer was already live
    uint64_t MemoryKey{}; // 0 when the allocation was already live
  };
  std::unordered_map<uint64_t, TransientAs> transientAs;
  uint64_t nextChainTransientKey = kChainTransientKeyBase;

  // Destroy whichever of a transient acceleration structure's storage resources
  // we created ourselves (either may be 0 when it was already live).
  auto releaseTransientStorage = [&](uint64_t deviceKey, uint64_t bufKey, uint64_t memKey) {
    if (bufKey) {
      vkDestroyBufferCommand destroyBuf;
      destroyBuf.m_device.Key = deviceKey;
      destroyBuf.m_buffer.Key = bufKey;
      m_Recorder.Record(vkDestroyBufferSerializer(destroyBuf));
      m_RestoredThisPass.erase(bufKey);
    }
    if (memKey) {
      vkFreeMemoryCommand freeMem;
      freeMem.m_device.Key = deviceKey;
      freeMem.m_memory.Key = memKey;
      m_Recorder.Record(vkFreeMemorySerializer(freeMem));
      m_RestoredThisPass.erase(memKey);
    }
  };

  // Give a Destroyed acceleration structure a live handle again for the duration of the chain
  // ops that read it, on a freshly reserved capture/replay address. Re-using the captured
  // one fails with VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, because the driver hands the
  // freed intermediate's address out again - observed both against objects live at the cut
  // and between two intermediates of the same chain. Relocating is safe because every chain
  // op that reads an intermediate BLAS names it by handle, never by address.
  auto resurrect = [&](uint64_t asKey) -> bool {
    auto* as = GetState<AccelerationStructureState>(asKey);
    if (!as) {
      LOG_WARNING << "Vulkan subcapture: acceleration structure key=" << asKey
                  << " needed by the BLAS chain is not tracked";
      return false;
    }
    if (!as->Destroyed) {
      // Still live at the cut, just not restored yet. Bring it up, but never enrol it for
      // teardown: only structures the application itself destroyed may be destroyed again.
      RestoreOne(as);
      return m_RestoredThisPass.count(asKey) != 0;
    }
    const uint64_t bufKey = nextChainTransientKey--;
    const uint64_t memKey = nextChainTransientKey--;
    const DevCtx* dev = resolveDev(as->ParentKey);
    if (!dev || !EmitRelocatedAccelerationStructureCreate(as->ParentKey, dev->PhysDev, *as, bufKey,
                                                          memKey)) {
      LOG_WARNING << "Vulkan subcapture: failed to re-create destroyed acceleration structure key="
                  << asKey
                  << " needed by the BLAS chain; its content will be missing from the subcapture";
      return false;
    }
    TransientAs transient{};
    transient.DeviceKey = as->ParentKey;
    transient.BufferKey = bufKey;
    transient.MemoryKey = memKey;
    // EmitRelocatedAccelerationStructureCreate emitted the create directly rather
    // than going through RestoreOne, so register the handle as restored by hand.
    m_RestoredThisPass.insert(asKey);
    transientAs[asKey] = transient;
    return true;
  };

  // Release a transiently re-created acceleration structure, and the storage buffer and
  // allocation created with it, once the chain no longer reads it. Each op ends in a
  // vkQueueWaitIdle, so the work that used it has completed.
  auto teardown = [&](uint64_t asKey) {
    auto it = transientAs.find(asKey);
    if (it == transientAs.end()) {
      return;
    }
    const TransientAs transient = it->second;
    transientAs.erase(it);

    vkDestroyAccelerationStructureKHRCommand destroyAs;
    destroyAs.m_device.Key = transient.DeviceKey;
    destroyAs.m_accelerationStructure.Key = asKey;
    m_Recorder.Record(vkDestroyAccelerationStructureKHRSerializer(destroyAs));
    m_RestoredThisPass.erase(asKey);

    // The storage buffer / allocation can be shared with another transient
    // acceleration structure that is still needed further down the chain.
    auto stillUsed = [&](uint64_t resourceKey) {
      for (const auto& [otherAs, other] : transientAs) {
        if (other.BufferKey == resourceKey || other.MemoryKey == resourceKey) {
          return true;
        }
      }
      return false;
    };
    releaseTransientStorage(transient.DeviceKey,
                            stillUsed(transient.BufferKey) ? 0 : transient.BufferKey,
                            stillUsed(transient.MemoryKey) ? 0 : transient.MemoryKey);
  };

  // Replay the reduced chain in the analyzer's execution (Id) order. A multi-info build is
  // replayed once (dedup by command key) since it rebuilds all its destinations. Copies
  // replay verbatim, their source AS already produced by an earlier op in this order.
  std::unordered_set<uint64_t> replayedBuildCmds;

  for (size_t i = 0; i < chain.size(); ++i) {
    const BlasChainOp& op = chain[i];
    // Wrapped so the emit block still falls through to the teardown below. Every failure in
    // here is fatal: dropping an op the analysis determined is needed leaves its destination
    // holding uninitialized memory.
    [&]() {
      auto rcIt = m_RetainedAsCommands.find(op.CommandKey);
      GITS_ASSERT(rcIt != m_RetainedAsCommands.end()); // checked when building requiredByOp
      auto* dstState = GetState<AccelerationStructureState>(op.DstAsKey);
      if (!dstState) {
        FatalSubcaptureError(
            "destination acceleration structure key=" + std::to_string(op.DstAsKey) +
            " of retained chain op (command key=" + std::to_string(op.CommandKey) +
            ") is not tracked, so the op cannot be replayed");
      }
      const DevCtx* dev = resolveDev(dstState->ParentKey);
      if (!dev) {
        FatalSubcaptureError(
            "no restored queue and command pool with matching queue family indices on device key=" +
            std::to_string(dstState->ParentKey) +
            ", so retained acceleration structure chain op (command key=" +
            std::to_string(op.CommandKey) + ") cannot be replayed");
      }

      // Give every acceleration structure this command references a live handle, re-creating
      // the ones the application already destroyed (build-then-compact destroys the
      // uncompacted intermediate right after the copy that reads it).
      for (uint64_t key : requiredByOp[i]) {
        if (m_RestoredThisPass.count(key)) {
          continue;
        }
        if (!resurrect(key)) {
          FatalSubcaptureError(
              "acceleration structure key=" + std::to_string(key) +
              " is required by retained chain op (command key=" + std::to_string(op.CommandKey) +
              ") but cannot be given a live handle, so the op cannot be replayed");
        }
      }

      if (op.IsCopy) {
        EmitAccelerationStructureCopyReplay(dstState->ParentKey, dev->Queue, dev->Pool,
                                            rcIt->second.CommandBytes);
      } else if (replayedBuildCmds.insert(op.CommandKey).second) {
        // Replayed in its recorded mode. updateSourceByCmd repoints each update info at the
        // structure this chain produces, which the loop above has already made live.
        auto srcMapIt = updateSourceByCmd.find(op.CommandKey);
        EmitAccelerationStructureRebuildBytes(
            dstState->ParentKey, dev->PhysDev, dev->Queue, dev->Pool, rcIt->second.CommandBytes,
            rcIt->second.Inputs, op.DstAsKey,
            srcMapIt != updateSourceByCmd.end() ? &srcMapIt->second : nullptr);
      }
    }();

    for (uint64_t key : requiredByOp[i]) {
      auto lastIt = lastUse.find(key);
      if (lastIt != lastUse.end() && lastIt->second == i) {
        teardown(key);
      }
    }
  }

  // Every key has a last use, so nothing should be left. Belt and braces so no
  // transient handle stays parked on a capture/replay address after the chain.
  while (!transientAs.empty()) {
    teardown(transientAs.begin()->first);
  }
}

// An acceleration structure's storage is opaque: the only spec-sanctioned way to populate it
// from bytes is vkCmdCopyMemoryToAccelerationStructureKHR after a
// vkGetDeviceAccelerationStructureCompatibilityKHR check. The plain vkCmdCopyBuffer that
// RestoreBufferContents emits would leave the buffer out of sync with the structure the
// driver just built on the player, its captured internal references still pointing at the
// original process's input addresses. The first trace then traverses a malformed BVH and
// hangs the GPU.
void StateTrackingService::MarkAccelerationStructureBackingContentRestored(uint64_t asKey) {
  auto* asState = GetState<AccelerationStructureState>(asKey);
  if (!asState) {
    return;
  }
  if (auto* backing = GetState<BufferState>(asState->BufferKey)) {
    backing->ContentRestored = true;
  }
}

void StateTrackingService::RestoreAccelerationStructureContents() {
  // Two mutually exclusive paths: chain (Optimize + Vulkan.CaptureASBuildInputs), serialized blobs.
  const bool useChain = m_AnalyzerResults && m_AnalyzerResults->UseAsChainRestore();
  LOG_INFO << "Vulkan subcapture: restoring acceleration structure content by "
           << (useChain ? "replaying captured build operations (portable)"
                        : "deserializing content at the state restore dump (not portable "
                          "and stream playback may fail on different GPU or driver)");

  std::unordered_set<uint64_t> chainDestinationAsKeys;
  if (useChain) {
    RestoreBlasChain();
    for (const BlasChainOp& op : m_AnalyzerResults->GetBlasChain()) {
      chainDestinationAsKeys.insert(op.DstAsKey);
    }
  }

  // Group acceleration structures by device.
  std::unordered_map<uint64_t, std::vector<uint64_t>> asByDevice;
  for (const auto& [key, sp] : m_States) {
    if (sp->Destroyed) {
      continue;
    }
    if (sp->CreationCommandId != CommandId::ID_VKCREATEACCELERATIONSTRUCTUREKHR) {
      continue;
    }
    if (!m_RestoredThisPass.count(key)) {
      continue;
    }
    asByDevice[sp->ParentKey].push_back(key);
    // Claim the backing buffer for the acceleration-structure restore paths before
    // RestoreBufferContents runs. Done here rather than in each path so none can forget it,
    // and unconditional on the restore succeeding - a raw byte copy is not a valid fallback
    // for an acceleration structure either.
    MarkAccelerationStructureBackingContentRestored(key);
  }

  for (auto& [deviceKey, asKeys] : asByDevice) {
    // Every restore mechanism below needs a queue and pool to submit through, so
    // without one the structures on this device would all stay uninitialized.
    uint64_t queueKey = 0, poolKey = 0;
    if (!::gits::vulkan::FindQueueAndPool(m_States, deviceKey, queueKey, poolKey)) {
      FatalSubcaptureError(
          "no queue and command pool with matching queue family indices on device key=" +
          std::to_string(deviceKey) +
          ", so acceleration structure content cannot be restored there");
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(poolKey)) {
      FatalSubcaptureError("queue key=" + std::to_string(queueKey) +
                           " or command pool key=" + std::to_string(poolKey) +
                           " was not restored on device key=" + std::to_string(deviceKey) +
                           ", so acceleration structure content cannot be restored there");
    }

    auto* devState = GetState<ObjectState>(deviceKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (!physDevKey) {
      FatalSubcaptureError("device key=" + std::to_string(deviceKey) +
                           " has no physical device, so acceleration structure content cannot be "
                           "restored there");
    }

    for (uint64_t asKey : asKeys) {
      // The chain path (RestoreBlasChain) has already reconstructed every BLAS,
      // so the per-AS rebuild below handles only TLAS.
      if (useChain) {
        auto* as = GetState<AccelerationStructureState>(asKey);
        if (as && as->Type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR) {
          // No chain op writes this structure, so it is restored with no contents. That means
          // no pre-range submitted build or copy targeted it, which is legitimate and common:
          // its builds may all be in range, it may not have been built yet, or its build may
          // sit in a command buffer that was never submitted - in each case uninitialized *is*
          // its state at the cut. Trace level, since only the analysis pass can tell whether
          // something fills it before the first trace.
          if (!chainDestinationAsKeys.count(asKey)) {
            LOG_TRACE << "Vulkan subcapture: bottom-level acceleration structure key=" << asKey
                      << " is restored with no contents - no retained chain operation writes it";
          }
          continue;
        }
      }
      if (useChain) {
        if (auto* as = GetState<AccelerationStructureState>(asKey);
            as && as->Type != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR) {
          FatalSubcaptureError(
              "acceleration structure key=" + std::to_string(asKey) + " has type " +
              std::to_string(static_cast<int>(as->Type)) +
              ", which is neither top- nor bottom-level, so subcapture cannot restore it");
        }
        // Aborts the run if it cannot be rebuilt.
        RestoreAccelerationStructureByRebuild(asKey, deviceKey, physDevKey, queueKey, poolKey);
        continue;
      }

      std::vector<uint8_t> data;
      VkDeviceAddress stagingDeviceAddress = 0;
      uint64_t stagingOpaqueCaptureAddress = 0;
      uint64_t stagingMemoryOpaqueCaptureAddress = 0;
      if (!m_GpuReadbackHelper->ReadAccelerationStructureSerialized(
              deviceKey, physDevKey, queueKey, poolKey, asKey, data, stagingDeviceAddress,
              stagingOpaqueCaptureAddress, stagingMemoryOpaqueCaptureAddress)) {
        // Skipping leaves the structure uninitialized - its backing buffer is claimed, so
        // nothing else fills it either - and the first trace over it loses the device.
        FatalSubcaptureError("GPU readback failed for acceleration structure key=" +
                             std::to_string(asKey) + ", so its content cannot be restored");
      }

      // EmitAccelerationStructureDeserialize creates this staging buffer at a capture/replay
      // address, so it must be probed as such.
      VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
      VkMemoryRequirements stagingReq{};
      if (!QueryCaptureReplayBufferRequirements(deviceKey, data.size(), stagingUsage, stagingReq)) {
        FatalSubcaptureError(
            "failed to query staging buffer requirements for acceleration structure key=" +
            std::to_string(asKey) + ", so its content cannot be restored");
      }
      uint32_t stagingMemType =
          m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, stagingReq.memoryTypeBits);
      if (stagingMemType == UINT32_MAX) {
        FatalSubcaptureError("no HOST_VISIBLE memory type satisfying staging memoryTypeBits=" +
                             std::to_string(stagingReq.memoryTypeBits) +
                             " for acceleration structure key=" + std::to_string(asKey) +
                             ", so its content cannot be restored");
      }

      EmitAccelerationStructureDeserialize(m_Recorder, deviceKey, queueKey, poolKey, asKey,
                                           data.size(), stagingReq.size, stagingMemType,
                                           stagingDeviceAddress, stagingOpaqueCaptureAddress,
                                           stagingMemoryOpaqueCaptureAddress, data);
      LOG_TRACE << "Vulkan subcapture: restored acceleration structure content, key=" << asKey
                << " size=" << data.size() << " allocSize=" << stagingReq.size;
    }
  }
}

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
    if (buf->ContentRestored) {
      continue;
    }
    if (buf->BufferSize == 0 || buf->BoundMemoryKey == 0) {
      continue;
    }
    if (!(buf->UsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
      continue;
    }
    // Diagnostic only. If this buffer really does back an acceleration structure the copy
    // below will corrupt it, but the usage bit alone does not prove that: applications
    // over-declare usage, and may sub-allocate structures out of a buffer that also holds
    // ordinary data which must still be restored. So warn and continue rather than skip.
    if (buf->UsageFlags & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR) {
      LOG_WARNING << "Vulkan subcapture: restoring raw content of buffer key=" << key
                  << " which declares ACCELERATION_STRUCTURE_STORAGE usage; if it backs an "
                     "acceleration structure, its content restore path failed to claim it";
    }

    // Host-visible buffers are NOT skipped here: the CPU shadow only captures
    // CPU writes, so GPU-written host-visible content (e.g. ReBAR render
    // targets) must be restored via the live readback path too.  The
    // compare-and-skip optimization in the data loop below still avoids the
    // player-side GPU copy when the shadow already holds the exact content.
    buffersByDevice[buf->ParentKey].push_back(key);
  }

  for (auto& [deviceKey, bufKeys] : buffersByDevice) {
    uint64_t queueKey = 0, poolKey = 0;
    if (!::gits::vulkan::FindQueueAndPool(m_States, deviceKey, queueKey, poolKey)) {
      LOG_WARNING << "Vulkan subcapture: skipping buffer content restore for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(poolKey)) {
      LOG_WARNING << "Vulkan subcapture: queue or pool was not restored, skipping buffer "
                     "content restore for device key="
                  << deviceKey;
      continue;
    }

    auto* devState = GetState<ObjectState>(deviceKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (!physDevKey) {
      continue;
    }

    // Build the manifest: every candidate buffer that actually needs a GPU
    // copy, in a dense index order the data tokens will reference.  The player
    // sizes and batches its own staging from these sizes at replay time.
    //
    // Compare-and-skip is decided HERE (phase 1) rather than in the data pass so
    // that a skipped buffer is fully omitted: it never enters the manifest,
    // never gets a data token, and never inflates the player's staging size or
    // batch count.  A HOST_VISIBLE buffer whose live content is already fully
    // covered by, and byte-for-byte matches, the CPU shadow is excluded because
    // RestoreMappedMemory restores it on replay with a cheap host memcpy;
    // including it would only schedule a redundant player-side vkCmdCopyBuffer.
    // Any uncertainty (device-local, missing memory, partial coverage,
    // mismatch, or a probe readback failure) falls back to inclusion (copy).
    RestoreContentManifestCommand manifest;
    manifest.m_DeviceKey = deviceKey;
    manifest.m_PhysDevKey = physDevKey;
    manifest.m_QueueKey = queueKey;
    manifest.m_CommandPoolKey = poolKey;

    std::vector<uint64_t> orderedKeys;
    for (uint64_t bufKey : bufKeys) {
      auto* buf = static_cast<BufferState*>(GetState(bufKey));
      if (!buf) {
        continue;
      }

      // Only host-visible buffers are skip-eligible; probe their live content
      // now and exclude them when the CPU shadow already covers and matches it.
      // The probe bytes are discarded; the data pass below re-reads the live
      // bytes of the buffers that survive (recorder-side readback cost is
      // acceptable and peak host memory stays bounded to one resource).
      auto* mem = GetState<DeviceMemoryState>(buf->BoundMemoryKey);
      const bool hostVisible =
          mem != nullptr && m_GpuReadbackHelper->IsHostVisible(physDevKey, mem->MemoryTypeIndex);
      if (hostVisible) {
        std::vector<uint8_t> probe;
        if (m_GpuReadbackHelper->ReadBuffer(deviceKey, physDevKey, queueKey, poolKey, bufKey,
                                            /*srcOffset=*/0, buf->BufferSize, probe) &&
            ShadowFullyCoversAndMatches(*buf, *mem, probe)) {
          LOG_TRACE << "Vulkan subcapture: buffer key=" << bufKey
                    << " matches CPU shadow - excluded from content manifest "
                       "(mapped-memory restore covers it)";
          continue; // omit entirely: no manifest entry, no token, no staging
        }
      }

      RestoreContentManifestCommand::BufferEntry entry;
      entry.DstBufferKey = bufKey;
      entry.Size = buf->BufferSize;
      manifest.m_Buffers.push_back(entry);
      manifest.m_TotalBytes += buf->BufferSize;
      orderedKeys.push_back(bufKey);
    }
    if (manifest.m_Buffers.empty()) {
      continue;
    }
    m_Recorder.Record(RestoreContentManifestSerializer(manifest));

    // Stream each surviving buffer's bytes one at a time so peak host RAM stays
    // bounded.  The manifest and the data tokens are 1:1 (compare-and-skip
    // already excluded the redundant buffers above), so emit exactly one token
    // per manifest entry.  A zero-length region is emitted only if a phase-2
    // readback unexpectedly fails after the phase-1 probe succeeded: the token
    // is still emitted so the player's token count matches the manifest and the
    // stream terminates cleanly without a separate end token.
    static char sEmptyByte = 0;
    for (size_t i = 0; i < orderedKeys.size(); ++i) {
      const uint64_t bufKey = orderedKeys[i];
      std::vector<uint8_t> data;
      auto* buf = static_cast<BufferState*>(GetState(bufKey));
      if (buf) {
        if (!m_GpuReadbackHelper->ReadBuffer(deviceKey, physDevKey, queueKey, poolKey, bufKey,
                                             /*srcOffset=*/0, buf->BufferSize, data)) {
          LOG_WARNING << "Vulkan subcapture: GPU readback failed for buffer key=" << bufKey;
          data.clear();
        }
      }

      RestoreContentDataCommand dataCmd;
      dataCmd.m_DeviceKey = deviceKey;
      MemoryRegions::Region region;
      region.Offset = static_cast<uint64_t>(i); // resource index, not a byte offset
      region.Size = static_cast<uint64_t>(data.size());
      region.Data = data.empty() ? &sEmptyByte : reinterpret_cast<char*>(data.data());
      dataCmd.m_Regions.Regions.push_back(region);
      dataCmd.m_Regions.Size = 1;
      m_Recorder.Record(RestoreContentDataSerializer(dataCmd));

      LOG_TRACE << "Vulkan subcapture: streamed buffer content, key=" << bufKey
                << " size=" << data.size();
    }
  }
}

void StateTrackingService::RestoreImageContents() {
  // Images are ALWAYS restored via GPU copy (see policy note above): there is no
  // configuration switch; only the technical skips below apply.
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
    // Skip images without VK_IMAGE_USAGE_TRANSFER_SRC_BIT -- cannot use as copy source.
    if (!(img->UsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
      continue;
    }
    // Skip zero-size images.
    if (img->Extent.width == 0 || img->Extent.height == 0) {
      continue;
    }

    // Images are ALWAYS GPU-copied regardless of host-visibility: an
    // OPTIMAL-tiled image cannot be reconstructed from raw host bytes (the
    // shadow / mapped-memory path), and GPU-written content is invisible to the
    // CPU-write tracker.  This matches the legacy backend's always-copy policy.
    imagesByDevice[img->ParentKey].push_back(key);
  }

  for (auto& [deviceKey, imgKeys] : imagesByDevice) {
    uint64_t queueKey = 0, poolKey = 0;
    if (!::gits::vulkan::FindQueueAndPool(m_States, deviceKey, queueKey, poolKey)) {
      LOG_WARNING << "Vulkan subcapture: skipping image content restore for device key="
                  << deviceKey << " (no queue and command pool with matching queue family indices)";
      continue;
    }
    if (!m_RestoredThisPass.count(queueKey) || !m_RestoredThisPass.count(poolKey)) {
      LOG_WARNING << "Vulkan subcapture: queue or pool was not restored, skipping image "
                     "content restore for device key="
                  << deviceKey;
      continue;
    }

    auto* devState = GetState<ObjectState>(deviceKey);
    uint64_t physDevKey = devState ? devState->ParentKey : 0;
    if (!physDevKey) {
      continue;
    }

    // Build the manifest first (layout + size per image, no pixel data) so the
    // player knows the full footprint before any bytes stream in.  The layout
    // is computed the same way ReadImage lays out its readback, so the region
    // offsets match the bytes streamed below.
    RestoreContentManifestCommand manifest;
    manifest.m_DeviceKey = deviceKey;
    manifest.m_PhysDevKey = physDevKey;
    manifest.m_QueueKey = queueKey;
    manifest.m_CommandPoolKey = poolKey;

    std::vector<uint64_t> orderedKeys;
    for (uint64_t imgKey : imgKeys) {
      auto* img = static_cast<ImageState*>(GetState(imgKey));
      if (!img) {
        continue;
      }
      RestoreContentManifestCommand::ImageEntry entry;
      entry.Size = m_GpuReadbackHelper->GetImageStagingLayout(
          img->Format, img->Extent, img->MipLevels, img->ArrayLayers, entry.Regions);
      if (entry.Size == 0 || entry.Regions.empty()) {
        continue;
      }
      entry.DstImageKey = imgKey;
      entry.Format = static_cast<uint32_t>(img->Format);
      entry.FinalLayout = static_cast<uint32_t>(img->CurrentLayout);
      entry.AspectMask = static_cast<uint32_t>(AspectMaskFromFormat(img->Format));
      manifest.m_Images.push_back(std::move(entry));
      manifest.m_TotalBytes += manifest.m_Images.back().Size;
      orderedKeys.push_back(imgKey);
    }
    if (manifest.m_Images.empty()) {
      continue;
    }
    m_Recorder.Record(RestoreContentManifestSerializer(manifest));

    // Stream each image's bytes one at a time.  Emit exactly one data token per
    // manifest entry (zero-length on failure) so the player can detect stream
    // completion by counting tokens, without a separate end token.
    static char sEmptyByte = 0;
    for (size_t i = 0; i < orderedKeys.size(); ++i) {
      const uint64_t imgKey = orderedKeys[i];
      std::vector<uint8_t> data;
      std::vector<VkBufferImageCopy> regions;
      auto* img = static_cast<ImageState*>(GetState(imgKey));
      if (img) {
        if (!m_GpuReadbackHelper->ReadImage(deviceKey, physDevKey, queueKey, poolKey, imgKey,
                                            img->Format, img->Extent, img->MipLevels,
                                            img->ArrayLayers, img->Samples, img->CurrentLayout,
                                            data, regions)) {
          LOG_WARNING << "Vulkan subcapture: GPU readback failed for image key=" << imgKey;
          data.clear();
        } else {
          // The player's upload leaves the image in its tracked layout, so
          // EmitImageLayoutTransitions must skip it.
          img->ContentRestored = true;
        }
      }

      RestoreContentDataCommand dataCmd;
      dataCmd.m_DeviceKey = deviceKey;
      MemoryRegions::Region region;
      region.Offset = static_cast<uint64_t>(i); // resource index, not a byte offset
      region.Size = static_cast<uint64_t>(data.size());
      region.Data = data.empty() ? &sEmptyByte : reinterpret_cast<char*>(data.data());
      dataCmd.m_Regions.Regions.push_back(region);
      dataCmd.m_Regions.Size = 1;
      m_Recorder.Record(RestoreContentDataSerializer(dataCmd));

      LOG_TRACE << "Vulkan subcapture: streamed image content, key=" << imgKey
                << " size=" << data.size();
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
