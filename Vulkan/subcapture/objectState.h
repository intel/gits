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

struct DeviceState : ObjectState {
  // True if VK_KHR_timeline_semaphore was listed in ppEnabledExtensionNames at
  // vkCreateDevice time.  When set, state-restore timeline semaphore signals
  // must use vkSignalSemaphoreKHR instead of the Vulkan 1.2 core function.
  bool HasTimelineSemaphoreKHR{false};
  // True if VK_KHR_buffer_device_address / VK_EXT_buffer_device_address was listed in
  // ppEnabledExtensionNames at vkCreateDevice time.  Vulkan 1.1 devices expose only the
  // extension entry point, so synthetic state-restore address queries must preserve that
  // command variant.
  bool HasBufferDeviceAddressKHR{false};
  bool HasBufferDeviceAddressEXT{false};
};

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
  // VkMemoryOpaqueCaptureAddressAllocateInfo::opaqueCaptureAddress (0 if absent).
  // Needed to recreate the memory at the same capture/replay device address. The
  // buffer-side opaque address alone is not always enough.
  uint64_t OpaqueCaptureAddress{};
  // True if this allocation backs an acceleration-structure storage buffer. Such a
  // state survives vkFreeMemory (flagged Destroyed) because RestoreBlasChain may need
  // to re-create it transiently. See BufferState::AsBacking.
  bool AsBacking{};
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
  // For binary semaphores only: true if the semaphore was signaled (via
  // pSignalSemaphores in a queue submit, or by a vkAcquireNextImageKHR /
  // vkAcquireNextImage2KHR acquire) and not subsequently waited on.  Used
  // during state restore to re-signal via a dummy queue submission.
  bool IsSignaled{false};
  // For timeline semaphores only: the highest counter value observed since
  // creation (from pSignalSemaphoreValues in a vkQueueSubmit pNext chain,
  // VkSemaphoreSubmitInfo::value in vkQueueSubmit2, or a host-side
  // vkSignalSemaphore / vkSignalSemaphoreKHR call).  Zero means no signals
  // have been observed beyond the create-time VkSemaphoreTypeCreateInfo::
  // initialValue and no extra restore command is needed.
  uint64_t LastSignaledValue{};
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
  // VkBufferOpaqueCaptureAddressCreateInfo::opaqueCaptureAddress, if present. Cached
  // here so it can be looked up without re-decoding the creation command.
  uint64_t OpaqueCaptureAddress{};
  // Populated by Post(vkGetBufferDeviceAddress[KHR/EXT]) and fed into
  // DeviceAddressTrackingService.
  VkDeviceAddress DeviceAddress{};
  // True once the content has been copied into the subcapture stream (an input buffer
  // shared across builds must not be uploaded twice).
  bool ContentRestored{false};
  // True if this buffer is the storage buffer of an acceleration structure. Such a
  // state survives vkDestroyBuffer (flagged Destroyed) because RestoreBlasChain may
  // need to re-create it - with its acceleration structure - for one chain op.
  bool AsBacking{};
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
  // Sharing mode and, for VK_SHARING_MODE_CONCURRENT, the queue families the
  // image was created over (VkImageCreateInfo::pQueueFamilyIndices).  Content
  // restore needs them to tell whether its readback may be submitted on a queue
  // family other than the one that pairing happened to pick
  // (VUID-vkQueueSubmit-pSubmits-04626).
  VkSharingMode SharingMode{VK_SHARING_MODE_EXCLUSIVE};
  std::vector<uint32_t> ConcurrentFamilies{};
  // For VK_SHARING_MODE_EXCLUSIVE, the queue family that currently owns the image
  // (updated from ownership-transfer barriers and from submit-time use). UINT32_MAX
  // means no family has been observed as owner yet.
  uint32_t ExclusiveOwnerFamily{UINT32_MAX};
  // True from the point a queue-family ownership *release* barrier is submitted
  // until the matching *acquire* barrier is submitted on the destination
  // family.  While pending, no family may legally read the image (a one-shot
  // content-restore readback cannot perform the release+acquire handshake
  // itself), so ExclusiveOwnerFamily must be ignored and the image excluded
  // from content restore - see ImageLayoutService::NoteExclusiveQueueFamilyTransfer.
  bool ExclusiveOwnershipPending{false};
  // True once a queue-family ownership transfer barrier has been observed
  // whose subresourceRange does not cover the whole image: ownership is
  // tracked per whole image here, not per mip/layer/aspect, so such a
  // barrier may leave some subresources on a different family than
  // ExclusiveOwnerFamily records.  Sticky for simplicity (never cleared)
  // rather than attempting to reconcile partial ranges - content restore
  // always copies the whole image, so it must exclude any image that was
  // ever partially transferred instead of guessing which parts are safe.
  bool ExclusiveOwnershipMixed{false};
  // From VkImageCreateInfo::flags & VK_IMAGE_CREATE_DISJOINT_BIT. A disjoint
  // multi-planar image's barriers must use plane aspect bits rather than
  // COLOR (VUID-VkImageMemoryBarrier-image-01672 / -image-09242) - see
  // AspectMaskForFormat.
  bool Disjoint{false};
  // Set true by RestoreImageContents once pixel data has been copied into the
  // subcapture stream.  EmitImageLayoutTransitions skips these images because
  // the buffer-to-image copy already ends in the correct layout.
  bool ContentRestored{false};
};

struct BufferViewState : ObjectState {};

struct ImageViewState : ObjectState {
  // Needed for dependency-order restore: image must be restored before its views.
  uint64_t ImageKey{};
  // Non-zero when VkImageViewCreateInfo::pNext carries VkSamplerYcbcrConversionInfo.
  // The conversion must be restored before this view.
  uint64_t YcbcrConversionKey{};
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
  // Keys of every descriptor set currently allocated (live) from this pool.
  // Maintained in lockstep with LiveSets: inserted on vkAllocateDescriptorSets,
  // erased on vkFreeDescriptorSets, cleared on vkResetDescriptorPool /
  // vkDestroyDescriptorPool.  Lets a pool reset reclaim its sets in
  // O(sets in this pool) instead of scanning every tracked object -- critical
  // for descriptor-churn-heavy titles that reset pools millions of times.  An
  // unordered_set keeps individual vkFreeDescriptorSets erase O(1) average for
  // titles that free sets one at a time (a std::vector erase would be O(n)).
  std::unordered_set<uint64_t> AllocatedSetKeys;
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
  // Keys of every command buffer currently allocated (live) from this pool.
  // Maintained in lockstep with allocation: inserted on vkAllocateCommandBuffers,
  // erased on vkFreeCommandBuffers, cleared on vkDestroyCommandPool.  Lets a
  // command-pool reset / destroy walk only this pool's buffers instead of
  // scanning every tracked object (mirrors DescriptorPoolState::AllocatedSetKeys).
  // An unordered_set keeps individual vkFreeCommandBuffers erase O(1) average.
  std::unordered_set<uint64_t> AllocatedCommandBufferKeys;
};

// One referenced byte range of an acceleration-structure build input buffer.
struct CapturedBuildInputRegion {
  VkDeviceSize SrcOffset{}; // byte offset within the owning buffer
  VkDeviceSize RangeSize{}; // number of referenced bytes
  uint64_t Hash{};          // key into StateTrackingService content store
};

// The referenced sub-ranges of one acceleration-structure build input buffer, plus the
// capture/replay metadata needed to recreate the buffer at its original device address
// during restore. The bytes themselves live in the content store, keyed by hash.
struct CapturedBuildInputBuffer {
  uint64_t BufferKey{};                  // original buffer key (to detect if still restored)
  VkDeviceSize Size{};                   // full buffer size in bytes (for recreate)
  uint64_t BufferOpaqueCaptureAddress{}; // VkBufferOpaqueCaptureAddressCreateInfo
  uint64_t MemoryOpaqueCaptureAddress{}; // VkMemoryOpaqueCaptureAddressAllocateInfo
  uint32_t MemoryTypeIndex{UINT32_MAX};  // original backing-memory type index
  // vkBindBufferMemory offset within that allocation. A non-zero offset (or an
  // allocation shared with another input of the same build) makes the captured address
  // unreproducible by a dedicated recreate, so the restore relocates such an input.
  VkDeviceSize MemoryOffset{};
  // Base device address at build time, used to relocate the build command's baked
  // geometry addresses when the captured opaque address cannot be reproduced.
  VkDeviceAddress BaseDeviceAddress{};
  std::vector<CapturedBuildInputRegion> Regions; // merged, sorted by SrcOffset
};

// Live staging resources for an acceleration-structure build-input readback. These are
// raw driver handles, not GITS keys. They live as long as the command buffer may be
// resubmitted and are released when its staged list is cleared.
struct StagedInputReadback {
  VkDevice Device{VK_NULL_HANDLE};
  VkBuffer Buffer{VK_NULL_HANDLE};
  VkDeviceMemory Memory{VK_NULL_HANDLE};
  void* Mapped{nullptr};
  VkDeviceSize Size{}; // packed size = sum of the buffer's merged region sizes
};

// An acceleration-structure build whose input buffers are copied into staging at
// build-record time and read back after the command buffer is submitted (the inputs
// are only guaranteed valid at build execution, not at record time). Consumed by
// StateTrackingService::ApplyAsInputReadbacksAfterSubmit.
struct PendingAsInputReadback {
  uint64_t AsKey{};
  // Key of the vkCmdBuildAccelerationStructuresKHR command this dst belongs to. A
  // multi-info build produces one PendingAsInputReadback per dst, all sharing it.
  uint64_t CommandKey{};
  std::vector<CapturedBuildInputBuffer> Buffers;
  std::vector<StagedInputReadback> Staging; // parallel to Buffers
};

// Buffered per-CB owner update for one EXCLUSIVE image, applied to
// ImageState::ExclusiveOwnerFamily / ExclusiveOwnershipPending at submit time.
// See CommandBufferState::ExclusiveOwnerAfterSubmit and
// ImageLayoutService::NoteExclusiveQueueFamilyTransfer.
struct ExclusiveOwnerUpdate {
  uint32_t Family{UINT32_MAX};
  bool Pending{false};
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
  // Net layout each image (key) ends up in after this command buffer is
  // submitted and executed.  Populated at record time by the pipeline-barrier,
  // event-wait and render-pass Post handlers (last write wins per image);
  // applied to ImageState::CurrentLayout when the CB is submitted.  Image
  // layout transitions take effect on the GPU at submit time, not at vkCmd*
  // record time, so applying them at record time captures stale mid-frame
  // layouts (e.g. the next frame's command buffers being recorded ahead).
  // Mirrors legacy CCommandBufferState::imageLayoutAfterSubmit.
  std::unordered_map<uint64_t, VkImageLayout> ImageLayoutAfterSubmit;
  // For VK_SHARING_MODE_EXCLUSIVE images, the owner-family update each image
  // gets once this CB is submitted (last write wins).  Populated from
  // queue-family ownership barriers and from use on the recording pool's
  // family; applied to ImageState::ExclusiveOwnerFamily /
  // ExclusiveOwnershipPending at submit time, not at vkCmd* record time.
  std::unordered_map<uint64_t, ExclusiveOwnerUpdate> ExclusiveOwnerAfterSubmit;
  // EXCLUSIVE images this CB observed a partial-range ownership-transfer
  // barrier for (see ImageLayoutService::RecordExclusiveMixed).  Kept
  // separate from ExclusiveOwnerAfterSubmit's per-image "last write wins"
  // map so a later whole-image owner update recorded on the same image in
  // this CB cannot erase the taint before it is applied at submit time.
  std::unordered_set<uint64_t> ExclusiveOwnerMixedAfterSubmit;
  // Acceleration-structure builds whose input buffers must be read back after this CB is
  // submitted. Folded onto the executing primary at vkCmdExecuteCommands. Cleared on CB
  // reset and one-time-submit invalidation.
  std::vector<PendingAsInputReadback> AsInputReadbacksAfterSubmit;
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

struct AccelerationStructureState : ObjectState {
  // From VkAccelerationStructureCreateInfoKHR.
  VkAccelerationStructureTypeKHR Type{};
  uint64_t BufferKey{}; // backing VkBuffer (createInfo.buffer)
  VkDeviceSize Offset{};
  VkDeviceSize Size{};
  // VkAccelerationStructureCreateInfoKHR::deviceAddress: unlike VkBuffer's
  // opaque capture address, this is a direct create-info field, not a pNext.
  uint64_t OpaqueCaptureAddress{};
  // Populated from a non-zero capture/replay deviceAddress at creation or by
  // Post(vkGetAccelerationStructureDeviceAddressKHR), and fed into
  // DeviceAddressTrackingService.
  VkDeviceAddress DeviceAddress{};
  // Encoded bytes of the last vkCmdBuildAccelerationStructuresKHR that built this AS.
  // The rebuild content-restore path replays this build against re-uploaded inputs
  // instead of serializing/deserializing the AS itself.
  CommandId LastBuildCommandId{static_cast<CommandId>(0)};
  std::vector<char> LastBuildCommandBytes;
  // Every destination AS key touched by the same build call, including this one, so a
  // multi-info build is emitted once rather than per destination AS.
  std::vector<uint64_t> LastBuildSiblingAsKeys;
  // True if the last build's instances geometry used arrayOfPointers layout. Such a TLAS
  // is not rebuilt at state restore (its scattered instance structs cannot be captured at
  // record time) but in-range by the application. Its referenced BLASes are retained via
  // analysis-pass discovery.
  bool ArrayOfPointersInstances{false};

  // Referenced sub-ranges of each build input buffer (vertex/index/transform/instances/
  // aabbs). The rebuild content-restore recreates each input at its original
  // capture/replay device address and re-uploads only these bytes, even if the
  // application already destroyed the buffer, so the replayed build resolves the same
  // baked addresses with no patching. Scratch is regenerated fresh instead.
  std::vector<CapturedBuildInputBuffer> CapturedBuildInputs;
};
struct DeferredOperationState : ObjectState {};

// ---- Video session objects ---------------------------------------------

struct VideoSessionState : ObjectState {
  // Encoded vkBindVideoSessionMemoryKHR command bytes (stored from the Post
  // handler so RestoreVideoSession can re-emit it after memory is allocated).
  std::vector<char> BindCommandBuffer;
  // GITSKeys for every VkDeviceMemory bound via vkBindVideoSessionMemoryKHR,
  // collected for dependency ordering in RestoreVideoSession.
  std::vector<uint64_t> MemoryKeys;
};

struct VideoSessionParametersState : ObjectState {};

} // namespace vulkan
} // namespace gits
