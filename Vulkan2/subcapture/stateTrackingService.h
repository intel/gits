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

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

// ---------------------------------------------------------------------------
// IGpuReadbackHelper: interface injected from the player module.
//
// The subcapture library (Vulkan_subcapture_2) cannot link against
// Vulkan_services (HandleMapService) or Vulkan_player_2 (PlayerManager).
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

  // Reads the contents of a GPU-local VkBuffer into outData.
  // queueKey / commandPoolKey identify Vulkan objects to use for the transfer.
  // Returns false when readback is not possible.
  virtual bool ReadBuffer(uint64_t deviceKey,
                          uint64_t physDevKey,
                          uint64_t queueKey,
                          uint64_t commandPoolKey,
                          uint64_t bufferKey,
                          VkDeviceSize size,
                          std::vector<uint8_t>& outData) = 0;

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

  // Remove a destroyed object (if it exists).
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

  // Returns true if key is currently tracked (not yet destroyed).
  bool HasState(uint64_t key) const;

  // Returns true if key was successfully restored during the current pass
  // (i.e. its creation command was emitted and its handle registered in
  // HandleMapService).  Use this instead of HasState() when checking
  // whether a resource is safe to reference in a descriptor write: an object
  // may still be in m_States but have failed to restore (e.g. an image view
  // whose backing image was destroyed before the subcapture point).
  bool IsRestored(uint64_t key) const {
    return m_RestoredThisPass.count(key) != 0;
  }

  // Force-restore the live state at key now (idempotent).  Used by
  // collaborators (e.g. DescriptorSetUpdateService::RestoreUpdates) to pull
  // in resources referenced indirectly via tracked state that are not on
  // the standard dependencyKeys chain, so that the referenced object lands
  // in m_RestoredThisPass before its enclosing object is emitted.
  //
  // No-op when the key is unknown (e.g. the resource was destroyed by the
  // app and its state was removed): destroyed resources are NOT resurrected
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

  // Inject the GPU readback helper (provided by the player module).
  void SetGpuReadbackHelper(IGpuReadbackHelper* helper) {
    m_GpuReadbackHelper = helper;
  }

private:
  enum class CommandBufferRestoreOutcome {
    FailedNoAllocation,
    // vkAllocateCommandBuffers ran but vkBegin/vkCmd* blobs were not emitted (missing dep).
    AllocationOkRecordingReplaySkipped,
    AllocationOkFullRecordingReplay,
  };

  // Recursively restore a single object (parent-first).
  void RestoreOne(ObjectState* state);

  // Per-type special cases that need more than one command.
  bool RestoreBuffer(ObjectState* state);
  bool RestoreImage(ObjectState* state);
  bool RestoreImageView(ObjectState* state);
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

  // Emit the stored creation command bytes directly as a serializer.
  // Returns false if CommandId is not handled (no bytes emitted).
  bool EmitCreationCommand(ObjectState* state);

  // Emit image layout transitions for all images whose currentLayout is neither
  // UNDEFINED nor PREINITIALIZED.  Called from RestoreState() after all objects
  // have been re-created, before the StateRestoreEnd marker.
  void EmitImageLayoutTransitions();

  // Emit a pre-encoded command directly from a raw byte buffer (used for
  // replaying in-flight command buffer commands during state restore).
  void EmitRawCommand(CommandId id, const std::vector<char>& encoded);

  SubcaptureRecorder& m_Recorder;
  IGpuReadbackHelper* m_GpuReadbackHelper{nullptr};
  DescriptorSetUpdateService m_DescriptorSetUpdateService;
  // Single ordered container: key (sequential integer) -> owned state.
  // std::map keeps entries sorted by key, which equals creation order because
  // Vulkan2 keys are sequential integers assigned by the coder, exactly the
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
  // keys of destroyed objects that were transiently re-created as pipeline
  // dependencies; destroy commands for these are emitted after all pipelines
  // have been created, mirroring the old Vulkan state-restore approach.
  std::unordered_set<uint64_t> m_TransientlyRestored;
};

} // namespace vulkan
} // namespace gits
