// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "command.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

namespace gits {
namespace vulkan {

// Base state kept for every Vulkan object that has a create/destroy lifecycle.
// The key is the recorder-side handle key stored in the GITS stream (not the
// replayed VkXxx value).
//
// creationCommandBuffer holds the encoded bytes of the creation command so
// that the command can be re-decoded and re-issued during state restore without
// holding pointers into the original (now-gone) stream buffer.
struct ObjectState {
  ObjectState() = default;
  virtual ~ObjectState() = default;

  uint64_t Key{};       // recorder-side handle key
  uint64_t ParentKey{}; // e.g. device key that owns this object
  // Additional keys that must be restored before this object.  Unlike
  // ParentKey (which is a single required ancestor), DependencyKeys holds
  // sibling dependencies such as the VkImage a VkImageView references.
  std::vector<uint64_t> DependencyKeys;
  bool Destroyed{};
  bool Restored{};

  // The CommandId that identifies which command type the bytes in
  // CreationCommandBuffer represent.  Required at restore time to dispatch
  // to the correct Decode<T> overload, because one Vulkan object type can be
  // created by several different commands (e.g. RenderPass by
  // vkCreateRenderPass / vkCreateRenderPass2 / vkCreateRenderPass2KHR).
  CommandId CreationCommandId{static_cast<CommandId>(0)};

  // Encoded creation command blob.  Re-decoded during state restore to
  // replay the creation using the command type identified by CreationCommandId.
  std::vector<char> CreationCommandBuffer;
};

// ---- Instance / device -------------------------------------------------

struct InstanceState : ObjectState {};

// VkPhysicalDevice is enumerated, not created.  The creation command buffer
// holds a snapshot of the vkEnumeratePhysicalDevices call that produced this
// physical device, so it can be re-emitted during state restore.
struct PhysicalDeviceState : ObjectState {};

struct DeviceState : ObjectState {};

// ---- Memory ------------------------------------------------------------

struct DeviceMemoryState : ObjectState {
  bool IsMapped{};
  VkDeviceSize MappingOffset{};
  VkDeviceSize MappingSize{};
  VkMemoryMapFlags MappingFlags{};
  // Total allocation size, stored at vkAllocateMemory time to size the shadow buffer.
  VkDeviceSize AllocationSize{};
  // Flat shadow copy of host-visible memory content, matching DX12's shadowMemory_
  // approach.  Patched in-place by each MappedDataMetaCommand so only the latest
  // write to any byte range survives - no stale duplicates accumulate even when the
  // same pages are dirtied every frame.  Empty if no write has ever been seen.
  std::vector<uint8_t> ShadowBuffer;
  // Half-open interval [ShadowDirtyBegin, ShadowDirtyEnd) within the allocation
  // that has been written at least once.  Lets RestoreMappedMemory emit only the
  // touched portion rather than the full (possibly large) allocation.
  VkDeviceSize ShadowDirtyBegin{};
  VkDeviceSize ShadowDirtyEnd{};
  // Index into VkPhysicalDeviceMemoryProperties::memoryTypes[].  Used to check
  // whether this allocation is HOST_VISIBLE (already covered by RestoreMappedMemory)
  // so GPU-readback content restore can skip it.
  uint32_t MemoryTypeIndex{UINT32_MAX};
};

// ---- Synchronization primitives ----------------------------------------

struct FenceState : ObjectState {
  // True if the fence was submitted to a queue and not subsequently reset.
  // Used during state restore to recreate it with VK_FENCE_CREATE_SIGNALED_BIT.
  bool IsSignaled{false};
};

// Queues are not explicitly created; they are retrieved via vkGetDeviceQueue
// or vkGetDeviceQueue2.  The creation command buffer stores that retrieval
// call verbatim so it can be replayed during state restore.
struct QueueState : ObjectState {
  uint32_t QueueFamilyIndex{UINT32_MAX};
};

struct SemaphoreState : ObjectState {
  // True if this is a binary semaphore (as opposed to a timeline semaphore).
  bool IsBinary{true};
  // True if the semaphore was signaled (via pSignalSemaphores in a queue
  // submit, or by a vkAcquireNextImageKHR / vkAcquireNextImage2KHR acquire)
  // and not subsequently waited on.  Used during state restore to signal the
  // semaphore via a dummy queue submission.
  bool IsSignaled{false};
};

struct EventState : ObjectState {
  // True if the event was in the signaled (set) state at the subcapture point,
  // either via a host vkSetEvent or a device vkCmdSetEvent that has executed.
  // Used during state restore to re-set the event (vkSetEvent) so that a
  // recording-range vkGetEventStatus / vkCmdWaitEvents poll does not hang
  // waiting for a signal that was produced before the cut.  Mirrors the legacy
  // CEventState::eventUsed flag + RestoreEvents logic.
  bool IsSignaled{false};
};

// ---- Buffers / images --------------------------------------------------

// size, usage and sharingMode are re-readable from creationCommandBuffer,
// but kept here for cheap access by memory-restore and layout-tracking logic
// without re-decoding the full command.
struct BufferState : ObjectState {
  // Populated by vkBindBufferMemory* - not part of the creation command.
  uint64_t BoundMemoryKey{};
  VkDeviceSize MemoryOffset{};
  // Stored at vkCreateBuffer time for GPU-readback content restore.
  VkDeviceSize BufferSize{};
  VkBufferUsageFlags UsageFlags{};
};

// format, extent, mipLevels etc. are kept for resource-content restore helpers
// that need them without re-decoding the creation command each time.
// currentLayout is mutable state updated by barrier tracking - never in the
// creation command.
struct ImageState : ObjectState {
  // Format is read by AspectMaskFromFormat in EmitImageLayoutTransitions.
  VkFormat Format{};
  // CurrentLayout is mutable runtime state updated by barrier and render-pass
  // tracking - it is never stored in the creation command.
  VkImageLayout CurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  // Populated by vkBindImageMemory* - not part of the creation command.
  uint64_t BoundMemoryKey{};
  VkDeviceSize MemoryOffset{};
  // Stored at vkCreateImage time for GPU-readback content restore.
  VkExtent3D Extent{};
  uint32_t MipLevels{1};
  uint32_t ArrayLayers{1};
  VkSampleCountFlagBits Samples{VK_SAMPLE_COUNT_1_BIT};
  VkImageUsageFlags UsageFlags{};
  // Set true by RestoreImageContents once pixel data has been copied into the
  // subcapture stream.  EmitImageLayoutTransitions skips these images because
  // the buffer-to-image copy already ends in the correct layout.
  bool ContentRestored{false};
};

struct BufferViewState : ObjectState {};

struct ImageViewState : ObjectState {
  // Needed for dependency-order restore: image must be restored before its views.
  uint64_t ImageKey{};
};

// ---- Render pass / framebuffer -----------------------------------------

struct RenderPassState : ObjectState {
  // finalLayout per attachment, in pAttachments order from VkRenderPassCreateInfo*.
  // Populated at vkCreateRenderPass* time so ImageLayoutService can apply
  // the implicit final-layout transitions when a render pass ends.
  std::vector<VkImageLayout> AttachmentFinalLayouts;
};

struct FramebufferState : ObjectState {
  // Image view keys for each pAttachments[i] entry in VkFramebufferCreateInfo,
  // stored in creation order (0 for a null/imageless slot).
  // Used by ImageLayoutService to map attachment index ? image key.
  std::vector<uint64_t> AttachmentImageViewKeys;
};

// ---- Pipelines ---------------------------------------------------------

struct PipelineCacheState : ObjectState {};
struct PipelineLayoutState : ObjectState {};

struct PipelineState : ObjectState {
  // Keys of every VkPipeline handle produced by the same vkCreate*Pipelines
  // batch call (including this state's own key).  Populated in
  // SubcaptureLayer::Post so that RestoreOne can mark all sibling handles
  // as restored after emitting the batch command once, preventing N redundant
  // full-batch emissions when a batch of N pipelines is state-restored.
  std::vector<uint64_t> BatchPipelineKeys;
};

struct ShaderModuleState : ObjectState {};

// ---- Descriptors -------------------------------------------------------

struct DescriptorSetLayoutState : ObjectState {};

struct DescriptorPoolState : ObjectState {
  // High-water mark of simultaneously-live descriptor sets allocated from this
  // pool over the whole observed stream.  Used at state-restore time to size the
  // re-created pool from actual demand: a heavily-churned pool gets proportional
  // fragmentation headroom while a lightly-used (possibly huge) pool gets almost
  // none, which a flat multiplier cannot express -- too small starves the busy
  // pools, too large wastes driver memory on the big ones.
  uint32_t LiveSets{0};
  uint32_t PeakLiveSets{0};
};

struct DescriptorUpdateTemplateState : ObjectState {};

struct DescriptorSetState : ObjectState {
  // Needed for dependency-order restore: pool must exist before allocating sets.
  uint64_t PoolKey{};
  // The single VkDescriptorSetLayout this set was allocated with.  Cached so the
  // batched restore path can build one vkAllocateDescriptorSets for many sets of
  // the same pool without re-decoding each set's stored allocation blob.
  uint64_t LayoutKey{};
  // True when the original vkAllocateDescriptorSets carried a pNext chain (e.g.
  // VkDescriptorSetVariableDescriptorCountAllocateInfo).  Such sets carry
  // per-set pNext arrays that cannot be merged into a single batched call, so
  // they are restored one at a time from their stored blob (mirrors legacy).
  bool HasAllocPNext{false};
};

// ---- Sampler -----------------------------------------------------------

struct SamplerState : ObjectState {};
struct SamplerYcbcrConversionState : ObjectState {};

// ---- Command pool / buffers --------------------------------------------

struct CommandPoolState : ObjectState {
  uint32_t QueueFamilyIndex{UINT32_MAX};
};

struct CommandBufferState : ObjectState {
  // Needed for dependency-order restore: pool must exist before allocating buffers.
  uint64_t PoolKey{};
  // True while the command buffer is in the recording state (between
  // vkBeginCommandBuffer and vkEndCommandBuffer).
  bool IsRecording{false};
  // True after vkEndCommandBuffer until the CB is reset or invalidated by a
  // one-time-submit.  An executable CB must be re-opened and re-closed during
  // state restore so the second player has it in executable state too.
  bool IsExecutable{false};
  // VkCommandBufferUsageFlags from the last vkBeginCommandBuffer call.
  // Needed to detect VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.
  uint32_t BeginFlags{0};
  // Encoded bytes for the vkBeginCommandBuffer call that opened the current
  // recording session.
  std::vector<char> BeginCommandBuffer;
  // Encoded bytes for the vkEndCommandBuffer call that closed the session.
  // Non-empty only while IsExecutable == true.
  std::vector<char> EndCommandBuffer;
  // Encoded bytes (one entry per call) and matching command IDs for every
  // vkCmd* issued while IsRecording == true.  Re-emitted verbatim during state
  // restore in submission order.
  std::vector<std::vector<char>> RecordedCommands;
  std::vector<CommandId> RecordedCommandIds;
  // Net signaled state each event ends up in after this command buffer is
  // submitted and executed (event key -> set/reset).  Populated by
  // vkCmdSetEvent / vkCmdResetEvent (and the 2/2KHR variants); applied to the
  // corresponding EventState::IsSignaled when the CB is submitted.  Mirrors the
  // legacy CCommandBufferState::eventStatesAfterSubmit.
  std::unordered_map<uint64_t, bool> EventStatesAfterSubmit;
  // Per query pool (key) -> set of query indices reset / used by the vkCmd*
  // calls recorded into this command buffer.  Applied to the QueryPoolState
  // when the CB is submitted (queries take effect on the GPU at submit time,
  // not at record time).  Mirrors legacy CCommandBufferState::
  // resetQueriesAfterSubmit / usedQueriesAfterSubmit.
  std::unordered_map<uint64_t, std::unordered_set<uint32_t>> ResetQueriesAfterSubmit;
  std::unordered_map<uint64_t, std::unordered_set<uint32_t>> UsedQueriesAfterSubmit;
};

// ---- Swapchain / surface -----------------------------------------------

struct SurfaceState : ObjectState {
  // Window geometry captured from the CreateWindowMetaCommand that preceded
  // the surface creation.  Re-emitted as a CreateWindowMetaCommand before the
  // surface creation command during state restore.
  uint32_t Protocol{};
  int32_t WindowX{};
  int32_t WindowY{};
  int32_t WindowWidth{};
  int32_t WindowHeight{};
  bool WindowVisible{true};
  uint64_t HwndKey{};
  uint64_t HinstanceKey{};
};

struct SwapchainState : ObjectState {
  // Swapchain images have no explicit creation command; tracked via
  // vkGetSwapchainImagesKHR so they can be restored in order.
  // Stored as a vector to allow O(1) lookup by presentation image index.
  std::vector<uint64_t> ImageKeys{};
  // Indices of images currently acquired by the application (i.e. returned by
  // vkAcquireNextImageKHR but not yet passed back via vkQueuePresentKHR).
  // Mirrors old-backend acquiredImages in CSwapchainKHRState.
  // Used during state restore to re-acquire those images so the first recorded
  // frame sees them in the correct state without needing layout barriers.
  std::unordered_set<uint32_t> AcquiredImages{};
};

// ---- Query pool --------------------------------------------------------

struct QueryPoolState : ObjectState {
  // Captured at vkCreateQueryPool.  queryType selects how a query is made
  // "available" again during state restore (timestamp write vs. begin/end);
  // queryCount sizes the per-query bitmaps below.
  uint32_t QueryType{};
  uint32_t QueryCount{0};
  // Queue family index of the command pool the application used to reset/write
  // this pool's queries.  That family is, by construction, capable of the query
  // operations (the app issued them successfully), so the state-restore pass
  // must replay its reset / fake-query commands on a queue of the same family.
  // Picking an arbitrary family (e.g. a transfer-only one) violates
  // VUID-vkCmdResetQueryPool-commandBuffer-cmdpool and can lose the device.
  uint32_t RestoreQueueFamily{UINT32_MAX};
  // Per-query state at the subcapture cut, accumulated as command buffers that
  // touch this pool are submitted.  ResetQueries[i] == true: query i was reset
  // (vkCmd/vkResetQueryPool) and is in the post-reset (writable) state.
  // UsedQueries[i] == true: query i was written before the cut and is therefore
  // *available* for reading.  Such queries must be re-created with a fake
  // result during state restore, otherwise the recording range's
  // vkGetQueryPoolResults reads an uninitialized query and the device is lost.
  // Mirrors the legacy CQueryPoolState resetQueries / usedQueries.
  std::vector<bool> ResetQueries;
  std::vector<bool> UsedQueries;
};

// ---- Misc extension objects --------------------------------------------

struct AccelerationStructureState : ObjectState {};
struct DeferredOperationState : ObjectState {};

} // namespace vulkan
} // namespace gits
