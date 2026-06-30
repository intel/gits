// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "objectState.h"
#include "stateTrackingService.h"
#include "gpuReadbackHelper.h"
#include "syncStateService.h"
#include "imageLayoutService.h"
#include "commandBufferLifecycleService.h"
#include "mappedMemoryService.h"
#include "subcaptureRange.h"
#include "subcaptureRecorder.h"
#include "analyzerResults.h"
#include "analyzerService.h"
#include "commandCodersAuto.h"

#include <memory>
#include <string>
#include <vector>

namespace gits {
namespace vulkan {

class PlayerManager;

// SubcaptureLayer sits in the player layer stack and tracks the live state of
// every Vulkan object that passes through.  During normal playback it keeps
// the state tables up-to-date so that a future state-restore pass (triggered
// at the frame boundary defined by SubcaptureRange) can replay creation
// commands in the right order.  Mirrors the DX12 StateTrackingLayer design.
class SubcaptureLayer : public Layer {
public:
  // framesStr: frame range string from config, e.g. "5" or "3-6".
  // An empty/"-" string disables subcapture.
  //
  // analysisMode == true selects the analysis pass: the recorder is kept closed
  // (no output stream), no state restore is emitted, and an AnalyzerService is
  // created so the AnalyzerLayer can collect in-range object usage and dump the
  // analysis file.  analysisMode == false is the regular recording pass.
  explicit SubcaptureLayer(PlayerManager& playerManager,
                           const std::string& framesStr,
                           bool analysisMode = false);

  StateTrackingService& GetStateTrackingService() {
    return m_StateTracking;
  }

  // Non-null only in analysis mode.  Passed to the AnalyzerLayer.
  AnalyzerService* GetAnalyzerService() {
    return m_AnalyzerService.get();
  }

  SubcaptureRange& GetSubcaptureRange() {
    return m_SubcaptureRange;
  }

  SubcaptureRange& GetRange() {
    return m_SubcaptureRange;
  }

  SubcaptureRecorder& GetRecorder() {
    return m_Recorder;
  }

  // ---- Frame boundary --------------------------------------------------
  // Triggered by vkQueuePresentKHR: advances frame counter, fires state
  // restore at the configured boundary, emits FrameEndCommand while recording.
  void Post(vkQueuePresentKHRCommand& command) override;

  // ---- Window / surface meta commands ----------------------------------
  // Track window geometry so it can be re-emitted before the surface during
  // state restore.  The CreateWindowMetaCommand is emitted by the Vulkan2
  // interceptor/recorder and replayed by the player before the matching
  // vkCreate*SurfaceKHR.
  void Post(CreateWindowMetaCommand& command) override;

  // ---- Instance / device -----------------------------------------------
  void Post(vkCreateInstanceCommand& command) override;
  void Post(vkDestroyInstanceCommand& command) override;
  void Post(vkEnumeratePhysicalDevicesCommand& command) override;
  void Post(vkEnumeratePhysicalDeviceGroupsCommand& command) override;
  void Post(vkEnumeratePhysicalDeviceGroupsKHRCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
  void Post(vkDestroyDeviceCommand& command) override;

  // ---- Memory ----------------------------------------------------------
  void Post(vkAllocateMemoryCommand& command) override;
  void Post(vkFreeMemoryCommand& command) override;
  void Post(vkMapMemoryCommand& command) override;
  void Post(vkUnmapMemoryCommand& command) override;
  void Post(vkMapMemory2Command& command) override;
  void Post(vkMapMemory2KHRCommand& command) override;
  void Post(vkUnmapMemory2Command& command) override;
  void Post(vkUnmapMemory2KHRCommand& command) override;
  // Track mapped data snapshots so memory content can be restored.
  void Post(MappedDataMetaCommand& command) override;

  // ---- Synchronization -------------------------------------------------
  void Post(vkCreateFenceCommand& command) override;
  void Post(vkDestroyFenceCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;
  void Post(vkQueueSubmit2Command& command) override;
  void Post(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueueBindSparseCommand& command) override;
  void Post(vkResetFencesCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkCreateSemaphoreCommand& command) override;
  void Post(vkDestroySemaphoreCommand& command) override;
  void Post(vkSignalSemaphoreCommand& command) override;
  void Post(vkSignalSemaphoreKHRCommand& command) override;
  void Post(vkCreateEventCommand& command) override;
  void Post(vkDestroyEventCommand& command) override;
  void Post(vkSetEventCommand& command) override;
  void Post(vkResetEventCommand& command) override;

  // ---- Buffers / images ------------------------------------------------
  void Post(vkCreateBufferCommand& command) override;
  void Post(vkDestroyBufferCommand& command) override;
  void Post(vkBindBufferMemoryCommand& command) override;
  void Post(vkBindBufferMemory2Command& command) override;
  void Post(vkBindBufferMemory2KHRCommand& command) override;

  void Post(vkCreateImageCommand& command) override;
  void Post(vkDestroyImageCommand& command) override;
  void Post(vkBindImageMemoryCommand& command) override;
  void Post(vkBindImageMemory2Command& command) override;
  void Post(vkBindImageMemory2KHRCommand& command) override;

  void Post(vkCreateBufferViewCommand& command) override;
  void Post(vkDestroyBufferViewCommand& command) override;
  void Post(vkCreateImageViewCommand& command) override;
  void Post(vkDestroyImageViewCommand& command) override;

  // ---- Render pass / framebuffer ---------------------------------------
  void Post(vkCreateRenderPassCommand& command) override;
  void Post(vkCreateRenderPass2Command& command) override;
  void Post(vkCreateRenderPass2KHRCommand& command) override;
  void Post(vkDestroyRenderPassCommand& command) override;
  void Post(vkCreateFramebufferCommand& command) override;
  void Post(vkDestroyFramebufferCommand& command) override;

  // ---- Pipelines -------------------------------------------------------
  void Post(vkCreatePipelineCacheCommand& command) override;
  void Post(vkDestroyPipelineCacheCommand& command) override;
  void Post(vkCreatePipelineLayoutCommand& command) override;
  void Post(vkDestroyPipelineLayoutCommand& command) override;
  void Post(vkCreateShaderModuleCommand& command) override;
  void Post(vkDestroyShaderModuleCommand& command) override;
  void Post(vkCreateGraphicsPipelinesCommand& command) override;
  void Post(vkCreateComputePipelinesCommand& command) override;
  void Post(vkCreateRayTracingPipelinesKHRCommand& command) override;
  void Post(vkCreateRayTracingPipelinesNVCommand& command) override;
  void Post(vkDestroyPipelineCommand& command) override;

  // ---- Descriptors -----------------------------------------------------
  void Post(vkCreateDescriptorSetLayoutCommand& command) override;
  void Post(vkDestroyDescriptorSetLayoutCommand& command) override;
  void Post(vkCreateDescriptorPoolCommand& command) override;
  void Post(vkDestroyDescriptorPoolCommand& command) override;
  void Post(vkResetDescriptorPoolCommand& command) override;
  void Post(vkAllocateDescriptorSetsCommand& command) override;
  void Post(vkFreeDescriptorSetsCommand& command) override;
  void Post(vkUpdateDescriptorSetsCommand& command) override;
  void Post(vkCreateDescriptorUpdateTemplateCommand& command) override;
  void Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) override;
  void Post(vkDestroyDescriptorUpdateTemplateCommand& command) override;
  void Post(vkDestroyDescriptorUpdateTemplateKHRCommand& command) override;
  void Post(vkUpdateDescriptorSetWithTemplateCommand& command) override;
  void Post(vkUpdateDescriptorSetWithTemplateKHRCommand& command) override;

  // ---- Sampler ---------------------------------------------------------
  void Post(vkCreateSamplerCommand& command) override;
  void Post(vkDestroySamplerCommand& command) override;
  void Post(vkCreateSamplerYcbcrConversionCommand& command) override;
  void Post(vkCreateSamplerYcbcrConversionKHRCommand& command) override;
  void Post(vkDestroySamplerYcbcrConversionCommand& command) override;
  void Post(vkDestroySamplerYcbcrConversionKHRCommand& command) override;

  // ---- Command pool / buffers ------------------------------------------
  void Post(vkCreateCommandPoolCommand& command) override;
  void Post(vkDestroyCommandPoolCommand& command) override;
  void Post(vkResetCommandPoolCommand& command) override;
  void Post(vkAllocateCommandBuffersCommand& command) override;
  void Post(vkFreeCommandBuffersCommand& command) override;

  // Track command buffers that are in a recording state at the subcapture
  // boundary so they can be re-opened and replayed during state restore.
  void Post(vkBeginCommandBufferCommand& command) override;
  void Post(vkEndCommandBufferCommand& command) override;
  void Post(vkResetCommandBufferCommand& command) override;

  // ---- vkCmd* dependency tracking -------------------------------------
  // When RecordingLayer stores a pre-range vkCmd* command into
  // CommandBufferState::RecordedCommands, the objects it references are not
  // automatically added to DependencyKeys.  These Post hooks fill that gap so
  // RestoreOne restores every referenced object before emitting the CB.
  void Post(vkCmdBeginRenderPassCommand& command) override;
  void Post(vkCmdBeginRenderPass2Command& command) override;
  void Post(vkCmdBeginRenderPass2KHRCommand& command) override;
  void Post(vkCmdBeginRenderingCommand& command) override;
  void Post(vkCmdBeginRenderingKHRCommand& command) override;
  void Post(vkCmdBindPipelineCommand& command) override;
  void Post(vkCmdBindPipelineShaderGroupNVCommand& command) override;
  void Post(vkCmdBindDescriptorSetsCommand& command) override;
  void Post(vkCmdBindVertexBuffersCommand& command) override;
  void Post(vkCmdBindIndexBufferCommand& command) override;
  void Post(vkCmdBindVertexBuffers2Command& command) override;
  void Post(vkCmdBindVertexBuffers2EXTCommand& command) override;
  void Post(vkCmdBindIndexBuffer2Command& command) override;
  void Post(vkCmdBindIndexBuffer2KHRCommand& command) override;
  // Transfer: buffer
  void Post(vkCmdCopyBufferCommand& command) override;
  void Post(vkCmdCopyBuffer2Command& command) override;
  void Post(vkCmdCopyBuffer2KHRCommand& command) override;
  void Post(vkCmdFillBufferCommand& command) override;
  void Post(vkCmdUpdateBufferCommand& command) override;
  // Transfer: image
  void Post(vkCmdCopyImageCommand& command) override;
  void Post(vkCmdCopyImage2Command& command) override;
  void Post(vkCmdCopyImage2KHRCommand& command) override;
  void Post(vkCmdBlitImageCommand& command) override;
  void Post(vkCmdBlitImage2Command& command) override;
  void Post(vkCmdBlitImage2KHRCommand& command) override;
  void Post(vkCmdClearColorImageCommand& command) override;
  void Post(vkCmdClearDepthStencilImageCommand& command) override;
  void Post(vkCmdResolveImageCommand& command) override;
  void Post(vkCmdResolveImage2Command& command) override;
  void Post(vkCmdResolveImage2KHRCommand& command) override;
  // Transfer: buffer/image
  void Post(vkCmdCopyBufferToImageCommand& command) override;
  void Post(vkCmdCopyBufferToImage2Command& command) override;
  void Post(vkCmdCopyBufferToImage2KHRCommand& command) override;
  void Post(vkCmdCopyImageToBufferCommand& command) override;
  void Post(vkCmdCopyImageToBuffer2Command& command) override;
  void Post(vkCmdCopyImageToBuffer2KHRCommand& command) override;
  // Events
  void Post(vkCmdSetEventCommand& command) override;
  void Post(vkCmdSetEvent2Command& command) override;
  void Post(vkCmdSetEvent2KHRCommand& command) override;
  void Post(vkCmdResetEventCommand& command) override;
  void Post(vkCmdResetEvent2Command& command) override;
  void Post(vkCmdResetEvent2KHRCommand& command) override;
  void Post(vkCmdWaitEventsCommand& command) override;
  void Post(vkCmdWaitEvents2Command& command) override;
  void Post(vkCmdWaitEvents2KHRCommand& command) override;
  // Indirect draw/dispatch
  void Post(vkCmdDrawIndirectCommand& command) override;
  void Post(vkCmdDrawIndexedIndirectCommand& command) override;
  void Post(vkCmdDispatchIndirectCommand& command) override;
  // Query
  void Post(vkCmdBeginQueryCommand& command) override;
  void Post(vkCmdEndQueryCommand& command) override;
  void Post(vkCmdResetQueryPoolCommand& command) override;
  void Post(vkCmdWriteTimestampCommand& command) override;
  void Post(vkCmdWriteTimestamp2Command& command) override;
  void Post(vkCmdWriteTimestamp2KHRCommand& command) override;
  void Post(vkCmdCopyQueryPoolResultsCommand& command) override;
  // Execute secondary command buffers
  void Post(vkCmdExecuteCommandsCommand& command) override;

  // ---- Image layout tracking ------------------------------------------
  // Update ImageState::currentLayout as pipeline barriers execute so that
  // RestoreState can transition images back to their correct layout.
  void Post(vkCmdPipelineBarrierCommand& command) override;
  void Post(vkCmdPipelineBarrier2Command& command) override;
  void Post(vkCmdPipelineBarrier2KHRCommand& command) override;

  // Track implicit render-pass final-layout transitions.
  // BeginRenderPass variants record which images are bound; EndRenderPass
  // applies the finalLayouts declared in the VkRenderPass create info.
  void Post(vkCmdEndRenderPassCommand& command) override;
  void Post(vkCmdEndRenderPass2Command& command) override;
  void Post(vkCmdEndRenderPass2KHRCommand& command) override;

  // ---- Swapchain / surface ---------------------------------------------
#ifdef VK_USE_PLATFORM_WIN32_KHR
  void Post(vkCreateWin32SurfaceKHRCommand& command) override;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  void Post(vkCreateXcbSurfaceKHRCommand& command) override;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  void Post(vkCreateXlibSurfaceKHRCommand& command) override;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  void Post(vkCreateWaylandSurfaceKHRCommand& command) override;
#endif
  void Post(vkDestroySurfaceKHRCommand& command) override;
  void Post(vkCreateSwapchainKHRCommand& command) override;
  void Post(vkDestroySwapchainKHRCommand& command) override;
  void Post(vkGetSwapchainImagesKHRCommand& command) override;
  void Post(vkAcquireNextImageKHRCommand& command) override;
  void Post(vkAcquireNextImage2KHRCommand& command) override;

  // ---- Query pool ------------------------------------------------------
  void Post(vkCreateQueryPoolCommand& command) override;
  void Post(vkDestroyQueryPoolCommand& command) override;

  // ---- Acceleration structures -----------------------------------------
  void Post(vkCreateAccelerationStructureKHRCommand& command) override;
  void Post(vkDestroyAccelerationStructureKHRCommand& command) override;
  void Post(vkCreateAccelerationStructureNVCommand& command) override;
  void Post(vkDestroyAccelerationStructureNVCommand& command) override;

  // ---- Deferred operations ---------------------------------------------
  void Post(vkCreateDeferredOperationKHRCommand& command) override;
  void Post(vkDestroyDeferredOperationKHRCommand& command) override;

  // ---- Video sessions --------------------------------------------------
  void Post(vkCreateVideoSessionKHRCommand& command) override;
  void Post(vkDestroyVideoSessionKHRCommand& command) override;
  void Post(vkBindVideoSessionMemoryKHRCommand& command) override;
  void Post(vkCreateVideoSessionParametersKHRCommand& command) override;
  void Post(vkDestroyVideoSessionParametersKHRCommand& command) override;

  void SetGpuReadbackHelper(IGpuReadbackHelper* helper) {
    m_StateTracking.SetGpuReadbackHelper(helper);
  }

private:
  // Feed the per-event VkDependencyInfo image barriers carried by a sync2
  // event wait (vkCmdWaitEvents2/KHR) into image-layout tracking, mirroring the
  // sync2 pipeline-barrier path.  handleKeys is the flat, array-order
  // concatenation of every dependency info's handle keys ([buffers][images]
  // per element); it is sliced back into per-element ranges here.
  void TrackWaitEvents2ImageLayouts(uint64_t cbKey,
                                    const VkDependencyInfo* dependencyInfos,
                                    uint32_t infoCount,
                                    const std::vector<uint64_t>& handleKeys);

  // Records the net signaled state an event ends up in after the given
  // (still recording) command buffer executes.  No-op for zero keys or a
  // command buffer that is not currently recording.
  void RecordCmdEventState(uint64_t cbKey, uint64_t eventKey, bool signaled);

  // Encode the creation command into state->CreationCommandBuffer and transfer
  // ownership of state into the persistent tracking table.
  //
  // Only the encoded bytes are stored, not a decoded TCommand, because all
  // pointer arguments in a decoded command (PointerArgument, ArrayArgument,
  // pNext chains) point into the stream buffer which will be overwritten by
  // the next token.  Storing the encoded bytes lets us re-decode the command
  // fresh at restore time into a self-contained, valid structure.
  //
  // This is the store-half of the encode/decode round-trip; the decode-half
  // happens in StateTrackingService::RestoreState() when the subcapture file
  // is being written.
  template <typename TCommand>
  void StoreState(std::unique_ptr<ObjectState> state, const TCommand& command) {
    state->CreationCommandId = command.GetId();
    uint32_t size = GetSize(command);
    state->CreationCommandBuffer.resize(size);
    Encode(command, state->CreationCommandBuffer.data());
    m_StateTracking.StoreState(std::move(state));
  }

  // When m_Recording is true (i.e. we are within the active subcapture range),
  // serialize the command and write it directly to the output stream so that
  // the subcapture includes the live API calls, not just the state restore.
  // TSerializer must be a CommandSerializer subclass whose constructor accepts
  // a const TCommand&.
  template <typename TCommand, typename TSerializer>
  void RecordInRange(const TCommand& command) {
    if (!m_Recording) {
      return;
    }
    TSerializer serializer(command);
    m_Recorder.Record(serializer);
  }

  // Append an encoded snapshot of a command buffer command to the in-flight
  // recording log for the given command buffer key.  Kept for use by
  // RecordingLayer via StateTrackingService; see recordingLayerAuto.h.
  // This private helper is no longer called directly; tracking is done by
  // RecordingLayer::TrackCmdBuffer.

  void TriggerRestoreState();

  void RemoveDescriptorSetsByPool(uint64_t poolKey);

  // Analysis pass only: mark every object handle embedded in a
  // vkUpdateDescriptorSetWithTemplate[KHR] data blob as used so the analyzer
  // keeps it in the restore set.  No-op outside analysis mode.
  void NotifyTemplateUpdateHandles(uint64_t templateKey, const std::vector<char>& dataBytes);

  // True for the analysis pass.  Declared first so it can be used in the
  // member initializer list (e.g. to keep the recorder closed).
  bool m_AnalysisMode{false};
  SubcaptureRange m_SubcaptureRange;
  SubcaptureRecorder m_Recorder;
  GpuReadbackHelper m_GpuReadbackHelper;
  StateTrackingService m_StateTracking;
  SyncStateService m_SyncState;
  ImageLayoutService m_ImageLayout;
  CommandBufferLifecycleService m_CommandBufferLifecycle;
  MappedMemoryService m_MappedMemory;
  // Recording pass: consumed by StateTrackingService to gate restore.
  AnalyzerResults m_AnalyzerResults;
  // Analysis pass only: collects in-range object usage and dumps the analysis
  // file.  Null in recording mode.
  std::unique_ptr<AnalyzerService> m_AnalyzerService;

  // Pending window geometry: set when a CreateWindowMetaCommand is observed,
  // consumed when the next surface creation command is processed.
  struct PendingWindowInfo {
    uint32_t Protocol{};
    int32_t X{}, Y{}, Width{}, Height{};
    bool Visible{true};
    uint64_t HwndKey{}, HinstanceKey{};
    bool Valid{false};
  };
  PendingWindowInfo m_PendingWindow;

  // True while we are within the active subcapture frame range and are writing
  // all API calls verbatim to the output stream.
  bool m_Recording{false};
};

} // namespace vulkan
} // namespace gits
