// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "subcaptureRecorder.h"
#include "descriptorSetUpdateService.h"
#include "queryPoolStateService.h"
#include "deviceAddressTrackingService.h"

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits {
namespace vulkan {

class AnalyzerResults;

// ---------------------------------------------------------------------------
// IGpuReadbackHelper: interface injected from the player module.
//
// The subcapture library (Vulkan_subcapture) cannot link against
// Vulkan_services (HandleMapService) or Vulkan_player (PlayerManager).
// This pure-virtual interface lets the player module provide GPU readback
// without creating a circular dependency.
// ---------------------------------------------------------------------------
class IGpuReadbackHelper {
public:
  virtual ~IGpuReadbackHelper() = default;

  // Returns true if the memory type at memoryTypeIndex for the given physical
  // device is HOST_VISIBLE (already covered by RestoreMappedMemory).
  virtual bool IsHostVisible(uint64_t physDevKey, uint32_t memoryTypeIndex) = 0;

  // Finds a HOST_VISIBLE | HOST_COHERENT memory type index suitable for a
  // staging buffer of 'memoryTypeBits' requirements.  Returns UINT32_MAX on failure.
  virtual uint32_t FindStagingMemoryType(uint64_t physDevKey, uint32_t memoryTypeBits) = 0;

  // Query memory requirements for a hypothetical staging buffer of the given
  // size+usage on the given device.  The recorder uses this when emitting
  // staging-buffer creation commands during state-restore content upload, to
  // ensure mai.allocationSize >= req.size (VUID-vkBindBufferMemory-None-10741)
  // and that FindStagingMemoryType is filtered by the buffer's allowed
  // memoryTypeBits (VUID-vkBindBufferMemory-memory-01035).  Without this we
  // were under-allocating (e.g. 65520 < required 65536) and picking a memory
  // type whose bit was not in the buffer's memoryTypeBits (e.g. type 2 / 0x6
  // when buffer required 0x11), corrupting every restored buffer/image.
  // Returns false when the temporary buffer creation or the requirements
  // query fails (e.g. device handle no longer valid).
  virtual bool QueryStagingBufferRequirements(uint64_t deviceKey,
                                              VkDeviceSize size,
                                              VkBufferUsageFlags usage,
                                              VkMemoryRequirements& outReq) = 0;

  // Always probe with the same create-info the buffer will actually be created with:
  // flags and pNext change what the driver requires (notably
  // VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT can coarsen the reported
  // alignment), so the size+usage shorthand above would under-report req.size and the
  // emitted allocation would be too small to back the buffer.
  virtual bool QueryBufferRequirements(uint64_t deviceKey,
                                       const VkBufferCreateInfo& createInfo,
                                       VkMemoryRequirements& outReq) = 0;

  // Reads 'size' bytes starting at 'srcOffset' of a GPU-local VkBuffer into outData.
  // The caller must keep srcOffset+size within the buffer. Pass the queue on whose
  // timeline the producing work has completed, so the copy observes the intended
  // contents. Returns false when readback is not possible.
  virtual bool ReadBuffer(uint64_t deviceKey,
                          uint64_t physDevKey,
                          uint64_t queueKey,
                          uint64_t commandPoolKey,
                          uint64_t bufferKey,
                          VkDeviceSize srcOffset,
                          VkDeviceSize size,
                          std::vector<uint8_t>& outData) = 0;

  // Compute the tightly-packed staging layout for an image (one
  // VkBufferImageCopy per subresource, bufferOffset relative to the image's
  // data start) and return the total byte size, WITHOUT reading pixel data.
  // The recorder uses this to build the content-restore manifest up front so
  // it can stream per-image bytes afterwards while keeping peak host memory
  // bounded to a single image.  Returns 0 when the image cannot be laid out.
  virtual VkDeviceSize GetImageStagingLayout(VkFormat format,
                                             const VkExtent3D& extent,
                                             uint32_t mipLevels,
                                             uint32_t arrayLayers,
                                             std::vector<VkBufferImageCopy>& outRegions) = 0;

  // Record into the application's command buffer appCbKey a barrier + vkCmdCopyBuffer
  // that copies the given (merged, sorted) regions of srcBufferKey into a fresh
  // host-visible staging buffer, packed in region order. Because the copy executes
  // with the application's own submission, it snapshots the inputs at the build's point
  // in the timeline. outStaging is then read via ReadStaged and released via
  // FreeStaged. Returns false on failure.
  virtual bool StageBufferRegions(uint64_t deviceKey,
                                  uint64_t physDevKey,
                                  uint64_t appCbKey,
                                  uint64_t srcBufferKey,
                                  const std::vector<CapturedBuildInputRegion>& regions,
                                  StagedInputReadback& outStaging) = 0;

  // Copy the whole staging buffer's contents into outData, once the GPU copy staged by
  // StageBufferRegions has completed. Returns false if staging is invalid.
  virtual bool ReadStaged(const StagedInputReadback& staging, std::vector<uint8_t>& outData) = 0;

  // Release the staging resources created by StageBufferRegions.
  virtual void FreeStaged(const StagedInputReadback& staging) = 0;

  // vkQueueWaitIdle, so a staged copy is finished before ReadStaged maps it.
  virtual bool WaitQueueIdle(uint64_t deviceKey, uint64_t queueKey) = 0;

  // Reads the contents of a GPU-local VkImage into outData.
  // outRegions is populated with one VkBufferImageCopy per subresource
  // (layer, mip, aspect), matching outData's layout.
  // Returns false when readback is not possible (multisampled, etc.).
  virtual bool ReadImage(uint64_t deviceKey,
                         uint64_t physDevKey,
                         uint64_t queueKey,
                         uint64_t commandPoolKey,
                         uint64_t imageKey,
                         VkFormat format,
                         const VkExtent3D& extent,
                         uint32_t mipLevels,
                         uint32_t arrayLayers,
                         VkSampleCountFlagBits samples,
                         VkImageLayout currentLayout,
                         std::vector<uint8_t>& outData,
                         std::vector<VkBufferImageCopy>& outRegions) = 0;

  // Reads the serialized bytes of a VkAccelerationStructureKHR into outData, via a
  // throwaway capture/replay buffer. outDeviceAddress is that buffer's device address.
  // outOpaqueCaptureAddress / outMemoryOpaqueCaptureAddress are its buffer- and
  // memory-side opaque addresses, both of which the caller must re-supply when it
  // recreates an equivalent buffer for the deserialize-restore commands. Doing so
  // reproduces outDeviceAddress, which is why it can be hardcoded into those commands.
  // Returns false when readback is not possible.
  virtual bool ReadAccelerationStructureSerialized(uint64_t deviceKey,
                                                   uint64_t physDevKey,
                                                   uint64_t queueKey,
                                                   uint64_t commandPoolKey,
                                                   uint64_t accelerationStructureKey,
                                                   std::vector<uint8_t>& outData,
                                                   VkDeviceAddress& outDeviceAddress,
                                                   uint64_t& outOpaqueCaptureAddress,
                                                   uint64_t& outMemoryOpaqueCaptureAddress) = 0;

  // Live query of the real accelerationStructureSize / updateScratchSize /
  // buildScratchSize for buildInfo. The captured scratch buffer cannot be trusted to be
  // large enough for a replayed build. pMaxPrimitiveCounts must have
  // buildInfo.geometryCount entries.
  virtual bool QueryAccelerationStructureBuildSizes(
      uint64_t deviceKey,
      const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo,
      const uint32_t* pMaxPrimitiveCounts,
      VkAccelerationStructureBuildSizesInfoKHR& outSizes) = 0;

  // Reserves a capture/replay-stable VkDeviceAddress for a not-yet-created scratch
  // buffer of 'size' bytes. The caller emits creation commands supplying these same
  // opaque addresses, so the driver reproduces outDeviceAddress and it can be hardcoded
  // into the replayed build's scratchData.deviceAddress.
  virtual bool ReserveScratchBufferAddress(uint64_t deviceKey,
                                           uint64_t physDevKey,
                                           VkDeviceSize size,
                                           VkDeviceAddress& outDeviceAddress,
                                           uint64_t& outOpaqueCaptureAddress,
                                           uint64_t& outMemoryOpaqueCaptureAddress) = 0;

  // Destroys every throwaway buffer that ReserveScratchBufferAddress kept alive. Call
  // it once a rebuild's transient creation commands have all been authored: until then
  // the addresses must stay reserved so two coexisting transients cannot alias.
  virtual void ReleaseReservedAddresses() = 0;
};

// Owns and manages the per-object state tables populated by SubcaptureLayer.
// All public methods are called from the player thread only (no locking needed).
class StateTrackingService {
public:
  explicit StateTrackingService(SubcaptureRecorder& recorder);
  ~StateTrackingService() = default;
  StateTrackingService(const StateTrackingService&) = delete;
  StateTrackingService& operator=(const StateTrackingService&) = delete;

  // Store ownership of a newly created object state.
  void StoreState(std::unique_ptr<ObjectState> state);

  // Remove a Destroyed object (if it exists).
  void RemoveState(uint64_t key);

  // Retrieve a typed state pointer; returns nullptr if not found or wrong type.
  template <typename T>
  T* GetState(uint64_t key) {
    auto it = m_States.find(key);
    if (it == m_States.end()) {
      return nullptr;
    }
    return dynamic_cast<T*>(it->second.get());
  }

  // Returns the base state pointer; returns nullptr if not found.
  ObjectState* GetState(uint64_t key) {
    auto it = m_States.find(key);
    if (it == m_States.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  // Returns true if key is currently tracked (not yet Destroyed).
  bool HasState(uint64_t key) const;

  // Returns true if key was successfully restored during the current pass
  // (i.e. its creation command was emitted and its handle registered in
  // HandleMapService).  Use this instead of HasState() when checking
  // whether a resource is safe to reference in a descriptor write: an object
  // may still be in m_States but have failed to restore (e.g. an image view
  // whose backing image was Destroyed before the subcapture point).
  bool IsRestored(uint64_t key) const {
    return m_RestoredThisPass.count(key) != 0;
  }

  // Force-restore the live state at key now (idempotent).  Used by
  // collaborators (e.g. DescriptorSetUpdateService::RestoreUpdates) to pull
  // in resources referenced indirectly via tracked state that are not on
  // the standard DependencyKeys chain, so that the referenced object lands
  // in m_RestoredThisPass before its enclosing object is emitted.
  //
  // No-op when the key is unknown (e.g. the resource was Destroyed by the
  // app and its state was removed): Destroyed resources are NOT resurrected
  // and the caller's subsequent IsRestored() check returns false, matching
  // legacy "omit on missing" semantics.
  void EnsureRestored(uint64_t key);

  // Returns all states in key order (== creation order, since keys are
  // sequential integers).  Prefer iterating this map over a separate ordered
  // container; the map is the single source of truth.
  const std::map<uint64_t, std::unique_ptr<ObjectState>>& GetStates() const {
    return m_States;
  }

  // Emit state-restore commands into the subcapture stream for every live
  // object, in dependency order.  Called once at the subcapture boundary.
  void RestoreState();

  // Expose the descriptor-set update tracker so SubcaptureLayer can feed it.
  DescriptorSetUpdateService& GetDescriptorSetUpdateService() {
    return m_DescriptorSetUpdateService;
  }

  // Expose the query-pool state tracker so SubcaptureLayer (record-time
  // tracking) and SyncStateService (submit-time application) can feed it.
  QueryPoolStateService& GetQueryPoolStateService() {
    return m_QueryPoolState;
  }

  // Fed by SubcaptureLayer from the vkGet*DeviceAddress Post hooks.
  DeviceAddressTrackingService& GetDeviceAddressTracking() {
    return m_DeviceAddressTracking;
  }

  // Inject the GPU readback helper (provided by the player module).
  void SetGpuReadbackHelper(IGpuReadbackHelper* helper) {
    m_GpuReadbackHelper = helper;
  }

  // Inject the analysis results consumed during the recording pass.  When set
  // to a non-null pointer with optimization enabled and a non-empty restore
  // set, RestoreState() only restores objects in that set (plus the
  // dependencies pulled transitively by RestoreOne).  When null (or the
  // results say "restore everything") behavior is identical to the legacy
  // single-pass flow.
  void SetAnalyzerResults(const AnalyzerResults* results) {
    m_AnalyzerResults = results;
  }

  // Find a live queue key and command pool key of deviceKey that share the same queue
  // family index, for one-shot GPU readback. False if no such pair exists.
  bool FindQueueAndPool(uint64_t deviceKey, uint64_t& outQueueKey, uint64_t& outPoolKey) const;

  // Read back the build inputs staged on cbKey now that its build has executed: reads
  // each referenced sub-range, hashes/stores the bytes and moves the finalized inputs
  // onto the destination AccelerationStructureState (last submit before the cut wins).
  void ApplyAsInputReadbacksAfterSubmit(uint64_t cbKey, uint64_t submitQueueKey);

  // Fold a secondary command buffer's staged AS build-input readbacks into the primary
  // so they run when the primary is submitted.
  void MergeSecondaryAsInputReadbacks(uint64_t primaryKey, uint64_t secondaryKey);

  // Record a retained BLAS chain command's encoded bytes for RestoreBlasChain (recording
  // pass only). No-op unless this command key is part of a loaded BlasChain. A
  // multi-info build is stored once under its key.
  void StoreRetainedAsCommandBytes(uint64_t commandKey,
                                   const std::vector<char>& bytes,
                                   bool isCopy);

  // Release the live staging buffers behind a command buffer's staged AS build-input
  // readbacks. Call before its staged list is cleared, so they are not leaked.
  void FreeCommandBufferStagedReadbacks(CommandBufferState& cb);

  // Extra per-command-buffer callback fired at submit time, taking the submitted command
  // buffer and the queue. Set by the analysis pass for its TLAS instance readback. The
  // recording pass leaves it unset.
  void SetSubmittedCommandBufferCallback(std::function<void(uint64_t, uint64_t)> cb) {
    m_OnCommandBufferSubmitted = std::move(cb);
  }

private:
  enum class CommandBufferRestoreOutcome {
    FailedNoAllocation,
    // vkAllocateCommandBuffers ran but vkBegin/vkCmd* blobs were not emitted (missing dep).
    AllocationOkRecordingReplaySkipped,
    AllocationOkFullRecordingReplay,
  };

  // True if the object identified by key should be restored.  Returns true
  // (restore everything) when no analysis results are attached, mirroring the
  // legacy behavior.
  bool ShouldRestore(uint64_t key) const;

  // Emit the buffer-device-address command variant supported by deviceKey.
  void EmitGetBufferDeviceAddress(uint64_t deviceKey, uint64_t bufferKey, VkDeviceAddress address);

  // Recursively restore a single object (parent-first).
  void RestoreOne(ObjectState* state);

  // Per-type special cases that need more than one command.
  bool RestoreBuffer(ObjectState* state);
  bool RestoreImage(ObjectState* state);
  bool RestoreImageView(ObjectState* state);
  bool RestoreAccelerationStructure(ObjectState* state);
  bool RestoreVideoSession(ObjectState* state);
  void RestoreSurface(ObjectState* state);
  void RestoreSwapchain(ObjectState* state);
  bool RestoreDescriptorSets(ObjectState* state);
  // Emit a single vkAllocateDescriptorSets that allocates every live, pNext-free
  // descriptor set of the given pool that has not yet been allocated this pass.
  // Batching reproduces the application's own packing far better than 1877
  // single-set calls, which is what actually defeats the descriptor-pool
  // fragmentation that caused recording-range VK_ERROR_OUT_OF_POOL_MEMORY.
  void AllocateDescriptorSetBatchForPool(uint64_t poolKey);
  // Lazily synthesize one vkEnumeratePhysicalDevices for the parent instance
  // of `state`, covering every live PhysicalDeviceState that shares that
  // parent.  Marks all sibling PD keys as restored so subsequent RestoreOne
  // calls for siblings short-circuit.  Returns false if the parent instance
  // could not be restored (the physical device cannot be enumerated then).
  bool RestorePhysicalDevice(ObjectState* state);
  CommandBufferRestoreOutcome RestoreCommandBuffers(ObjectState* state);
  void RestoreMappedMemory(ObjectState* state);
  void RestoreBufferContents();
  void RestoreImageContents();
  void RestoreAccelerationStructureContents();

  void EmitAccelerationStructureDeserialize(uint64_t deviceKey,
                                            uint64_t queueKey,
                                            uint64_t commandPoolKey,
                                            uint64_t dstAsKey,
                                            VkDeviceSize dataSize,
                                            VkDeviceSize stagingAllocationSize,
                                            uint32_t stagingMemTypeIndex,
                                            VkDeviceAddress capturedDeviceAddress,
                                            uint64_t capturedOpaqueCaptureAddress,
                                            uint64_t capturedMemoryOpaqueCaptureAddress,
                                            const std::vector<uint8_t>& data);

  void EmitCaptureReplayBufferCreate(uint64_t deviceKey,
                                     uint64_t bufKey,
                                     uint64_t memKey,
                                     VkDeviceSize bufferSize,
                                     VkDeviceSize allocationSize,
                                     uint32_t memTypeIndex,
                                     VkBufferUsageFlags usage,
                                     uint64_t opaqueCaptureAddress,
                                     uint64_t memoryOpaqueCaptureAddress,
                                     VkDeviceAddress deviceAddress);

  // Flag asKey's backing buffer as content-restored so RestoreBufferContents leaves it
  // alone. Must be called for every acceleration structure whose content a restore path
  // regenerates. See the definition for why a raw byte copy over its storage is invalid.
  void MarkAccelerationStructureBackingContentRestored(uint64_t asKey);

  // Rebuild-from-inputs content restore: replays asKey's last captured
  // vkCmdBuildAccelerationStructuresKHR (re-uploading its input buffers first), and its update
  // source's before that. Reads no BlasChain, so it only runs for TLASes the chain does not cover.
  // Aborts the run if asKey cannot be rebuilt.
  void RestoreAccelerationStructureByRebuild(
      uint64_t asKey, uint64_t deviceKey, uint64_t physDevKey, uint64_t queueKey, uint64_t poolKey);

  // Replays asState's stored build command bytes in a one-shot command buffer, patching
  // their CB key (the stored one is the original app CB's, dead by restore time).
  void EmitAccelerationStructureRebuild(uint64_t deviceKey,
                                        uint64_t physDevKey,
                                        uint64_t queueKey,
                                        uint64_t poolKey,
                                        const AccelerationStructureState& asState);

  // Core replay used by both the per-AS wrapper above and the chain replay. Replays the
  // build command bytes verbatim - each info keeps the mode the application recorded -
  // against re-uploaded captured inputs and a freshly reserved scratch buffer. logAsKey
  // is used only for logging.
  //
  // updateSourceByDstAs maps a destination AS key to the source an UPDATE-mode info
  // targeting it must refit from, replacing the recorded source that the chain reduction
  // may have dropped. Only the chain replay supplies it, since the per-AS path retains no
  // predecessor. An UPDATE-mode info with no source available aborts the run rather
  // than being rewritten into a BUILD, which would emit a different operation.
  void EmitAccelerationStructureRebuildBytes(
      uint64_t deviceKey,
      uint64_t physDevKey,
      uint64_t queueKey,
      uint64_t poolKey,
      const std::vector<char>& commandBytes,
      const std::vector<CapturedBuildInputBuffer>& capturedInputs,
      uint64_t logAsKey,
      const std::unordered_map<uint64_t, uint64_t>* updateSourceByDstAs = nullptr);

  // Probed with the exact create-info EmitCaptureReplayBufferCreate uses. Never probe
  // such a buffer with IGpuReadbackHelper::QueryStagingBufferRequirements: it omits the
  // capture/replay flag and would under-report req.size.
  bool QueryCaptureReplayBufferRequirements(uint64_t deviceKey,
                                            VkDeviceSize size,
                                            VkBufferUsageFlags usage,
                                            VkMemoryRequirements& outReq);

  // True for a Destroyed object kept in the state map solely so RestoreBlasChain can
  // transiently re-create it. Every other restore path must treat such an object as
  // untracked. See the definition for the rationale.
  bool IsChainRetainedOnly(const ObjectState* state) const;

  // Replay the analyzer's reduced BLAS chain in order: each build/update as a
  // build-from-inputs, each copy verbatim (its source AS was produced by an earlier op).
  // Used instead of the per-AS rebuild for BLASes when a BlasChain was loaded.
  void RestoreBlasChain();

  // Emit a stored copy command (vkCmdCopyAccelerationStructureKHR) in a one-shot
  // command buffer, patching its CB key. Used by RestoreBlasChain for CLONE/COMPACT.
  void EmitAccelerationStructureCopyReplay(uint64_t deviceKey,
                                           uint64_t queueKey,
                                           uint64_t poolKey,
                                           const std::vector<char>& commandBytes);

  // Re-create a Destroyed acceleration structure on a freshly reserved capture/replay
  // address instead of its captured one, backed by a dedicated storage buffer and
  // allocation under the caller-supplied synthetic keys. A verbatim re-create would fail
  // with VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, since the driver routinely hands the
  // freed structure's address out again. Relocating is safe because the chain ops that
  // read an intermediate BLAS reference it by handle, not by address.
  // Returns false if no address could be reserved (nothing emitted).
  bool EmitRelocatedAccelerationStructureCreate(uint64_t deviceKey,
                                                uint64_t physDevKey,
                                                const AccelerationStructureState& asState,
                                                uint64_t bufKey,
                                                uint64_t memKey);

  // Insert bytes into the AS build-input content store and return their hash key. On a
  // hash collision the content is compared and probed to a fresh key, never aliased.
  uint64_t StoreAsBuildInputContent(std::vector<uint8_t> bytes);
  // Look up previously stored AS build-input bytes by hash, or nullptr.
  const std::vector<uint8_t>* GetAsBuildInputContent(uint64_t hash) const;

  // Emit the stored creation command bytes directly as a serializer.
  // Returns false if CommandId is not handled (no bytes emitted).
  bool EmitCreationCommand(ObjectState* state);

  // Emit image layout transitions for all images whose currentLayout is neither
  // UNDEFINED nor PREINITIALIZED.  Called from RestoreState() after all objects
  // have been re-created, before the StateRestoreEnd marker.
  void EmitImageLayoutTransitions();

  // True if `img` is a swapchain image that was acquired (and not yet presented)
  // at the subcapture cut.  Such images are owned by the application: their
  // tracked layout must be restored like a regular image, whereas non-acquired
  // swapchain images are forced to PRESENT_SRC_KHR for the present-index rewind.
  bool IsAcquiredSwapchainImage(const ImageState* img);

  // Emit, per device, the commands needed to restore VkQueryPool contents:
  // reset the touched queries and issue a fake query (vkCmdWriteTimestamp, or
  // vkCmdBeginQuery + vkCmdEndQuery) for every query that was written before
  // the subcapture cut, so the recording range's vkGetQueryPoolResults reads an
  // available result instead of losing the device.  Uses the same transient
  // command-buffer + submit + wait-idle pattern as EmitImageLayoutTransitions.
  void RestoreQueryPools();

  // Emit a pre-encoded command directly from a raw byte buffer (used for
  // replaying in-flight command buffer commands during state restore).
  void EmitRawCommand(CommandId id, const std::vector<char>& encoded);

  SubcaptureRecorder& m_Recorder;
  IGpuReadbackHelper* m_GpuReadbackHelper{nullptr};
  const AnalyzerResults* m_AnalyzerResults{nullptr};
  DescriptorSetUpdateService m_DescriptorSetUpdateService;
  QueryPoolStateService m_QueryPoolState{*this};
  DeviceAddressTrackingService m_DeviceAddressTracking;
  // Single ordered container: key (sequential integer) -> owned state.
  // std::map keeps entries sorted by key, which equals creation order because
  // Vulkan keys are sequential integers assigned by the coder, exactly the
  // same as the DX subcapture which also uses std::map for this reason.
  std::map<uint64_t, std::unique_ptr<ObjectState>> m_States;
  // Keys for which RestoreOne has fully completed (object created + handle
  // registered).  Inserted only after successful creation so dependents can
  // rely on the presence of a key here as proof the object actually exists.
  std::unordered_set<uint64_t> m_RestoredThisPass;
  // Descriptor set keys whose vkAllocateDescriptorSets has already been emitted
  // this pass (via the per-pool batch or, for pNext sets, individually).  Kept
  // separate from m_RestoredThisPass because a set is "allocated" before its
  // descriptor writes are restored, and the writes must still be emitted in the
  // normal object order (after the buffers/images they reference are created).
  std::unordered_set<uint64_t> m_DescriptorSetsAllocated;
  // VkCommandBuffer keys where allocation was replayed but recorded commands were
  // skipped.  Primaries that call vkCmdExecuteCommands must not replay their own
  // recording when a secondary is in this set (transitive failure propagation).
  std::unordered_set<uint64_t> m_CommandBuffersRecordingReplaySkipped;
  // keys of Destroyed objects that were transiently re-created as pipeline
  // dependencies; destroy commands for these are emitted after all pipelines
  // have been created, mirroring the old Vulkan state-restore approach.
  std::unordered_set<uint64_t> m_TransientlyRestored;
  // AS keys already covered by a rebuild emitted this pass, directly or as a sibling
  // destination of the same multi-info build command.
  std::unordered_set<uint64_t> m_RebuiltAsKeys;
  // Retained BLAS chain commands (recording pass), keyed by command key. Populated by
  // StoreRetainedAsCommandBytes (bytes) and ApplyAsInputReadbacksAfterSubmit (inputs).
  struct RetainedAsCommand {
    std::vector<char> CommandBytes;
    std::vector<CapturedBuildInputBuffer> Inputs; // build/update only
    bool IsCopy{};
  };
  std::unordered_map<uint64_t, RetainedAsCommand> m_RetainedAsCommands;
  // AS build-input byte ranges keyed by content hash, deduplicating identical input
  // bytes across builds, buffers and frames.
  std::map<uint64_t, std::vector<uint8_t>> m_AsBuildInputContent;
  // Per-restore-pass dedup of emitted input uploads: (buffer key, dst offset) -> last
  // uploaded content hash. Lets a re-referenced range of a reused input buffer skip a
  // redundant upload. Reset at the start of RestoreState.
  std::map<std::pair<uint64_t, uint64_t>, uint64_t> m_RestoredInputRegionHashes;
  // See SetSubmittedCommandBufferCallback.
  std::function<void(uint64_t, uint64_t)> m_OnCommandBufferSubmitted;
};

} // namespace vulkan
} // namespace gits
