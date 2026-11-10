// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourcesDump.h"
#include "imageLayouts.h"
#include "queueFamilyOwnership.h"
#include "arguments.h"
#include "enumsAuto.h"
#include "vulkanHeader2.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

class DispatchTablesHolder;

// Dumps the resources listed in Vulkan.Player.ResourcesDump.resourceKeys after the commands listed
// in Vulkan.Player.ResourcesDump.commandKeys. A command recorded into a command buffer dumps the
// resources when that command buffer executes, right after the command; a queue submission dumps
// them after the work it submits.
class ResourcesDumpService {
public:
  explicit ResourcesDumpService(DispatchTablesHolder& dispatchTablesHolder);
  ~ResourcesDumpService();
  ResourcesDumpService(const ResourcesDumpService&) = delete;
  ResourcesDumpService& operator=(const ResourcesDumpService&) = delete;

  // The layouts state restore leaves the images in, with the barriers it records itself.
  void RestoredImageLayouts(VkQueue queue,
                            const std::vector<std::pair<uint64_t, VkImageLayout>>& layouts);

  void CreateDevice(VkPhysicalDevice physicalDevice,
                    VkDevice device,
                    const VkDeviceCreateInfo& info);
  void DestroyDevice(VkDevice device);
  void GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, VkQueue queue);
  void TrackSemaphore(VkDevice device, VkSemaphore semaphore, const VkSemaphoreCreateInfo& info);
  void DestroySemaphore(VkSemaphore semaphore);

  void CreateBuffer(GITSKey key, VkDevice device, VkBuffer buffer, const VkBufferCreateInfo& info);
  void BindBuffer(VkBuffer buffer);
  void DestroyBuffer(VkBuffer buffer);
  void CreateImage(GITSKey key, VkDevice device, VkImage image, const VkImageCreateInfo& info);
  void BindImage(VkImage image);
  void DestroyImage(VkImage image);
  // Adds the transfer source usage that copying swapchain images requires, if the surface
  // supports it.
  void PrepareSwapchain(VkDevice device, VkSwapchainCreateInfoKHR& info);
  void CreateSwapchain(VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR& info);
  void GetSwapchainImages(VkDevice device,
                          VkSwapchainKHR swapchain,
                          const VkImage* images,
                          const GITSKey* keys,
                          uint32_t count);
  void DestroySwapchain(VkSwapchainKHR swapchain);
  void AcquireSwapchainImage(VkSwapchainKHR swapchain, uint32_t imageIndex, VkSemaphore semaphore);
  // The swapchain images presented or released to the presentation engine.
  void ReleaseSwapchainImages(VkSwapchainKHR swapchain,
                              uint32_t imageIndexCount,
                              const uint32_t* imageIndices);
  void CreateImageView(VkImageView view, const VkImageViewCreateInfo& info);
  void DestroyImageView(VkImageView view);
  // The layouts a render pass leaves an attachment in.
  struct AttachmentFinalLayouts {
    VkImageLayout Layout{};
    VkImageLayout StencilLayout{};
  };
  // viewMask is the union of the view masks of the subpasses of a multiview render pass.
  void CreateRenderPass(VkRenderPass renderPass,
                        std::vector<AttachmentFinalLayouts> finalLayouts,
                        uint32_t viewMask);
  void DestroyRenderPass(VkRenderPass renderPass);
  void CreateFramebuffer(VkFramebuffer framebuffer, const VkFramebufferCreateInfo& info);
  void DestroyFramebuffer(VkFramebuffer framebuffer);

  void CreateCommandPool(VkDevice device,
                         VkCommandPool commandPool,
                         const VkCommandPoolCreateInfo& info);
  void DestroyCommandPool(VkCommandPool commandPool);
  void ResetCommandPool(VkCommandPool commandPool);
  void AllocateCommandBuffers(VkDevice device,
                              const VkCommandBufferAllocateInfo& info,
                              const VkCommandBuffer* commandBuffers);
  void FreeCommandBuffers(const VkCommandBuffer* commandBuffers, uint32_t count);
  void BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo& info);
  void ResetCommandBuffer(VkCommandBuffer commandBuffer);

  void Barriers(VkCommandBuffer commandBuffer,
                uint32_t bufferBarrierCount,
                const VkBufferMemoryBarrier* bufferBarriers,
                uint32_t imageBarrierCount,
                const VkImageMemoryBarrier* imageBarriers);
  void Barriers(VkCommandBuffer commandBuffer, const VkDependencyInfo& dependencyInfo);
  void BeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo& info);
  void EndRenderPass(VkCommandBuffer commandBuffer);
  void BeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo& info);
  void EndRendering(VkCommandBuffer commandBuffer);
  void ExecuteCommands(VkCommandBuffer commandBuffer,
                       const VkCommandBuffer* secondaries,
                       uint32_t count);
  void BeginVideoCoding(GITSKey key, VkCommandBuffer commandBuffer);
  void EndVideoCoding(GITSKey key, VkCommandBuffer commandBuffer);

  // A command recorded into a command buffer, after which the resources may be dumped.
  void CommandBufferCall(GITSKey key, VkCommandBuffer commandBuffer);
  void EndCommandBuffer(GITSKey key, VkCommandBuffer commandBuffer);

  // Before the command buffers are submitted: checks that the copies recorded into them transition
  // the images from the layouts the images have when the copies execute, and that they do not
  // execute between a render pass instance that suspends and the one that resumes it.
  void PreSubmit(const std::vector<VkCommandBuffer>& commandBuffers);
  // Before a submission: tracks the swapchain image acquisitions it waits for, and checks that the
  // copies recorded into its command buffers can execute. They cannot read swapchain images the
  // presentation engine may use, and the semaphore signals of the batches executing them, and of
  // later ones, must wait for them without waiting for values that may depend on those signals.
  // Neither may those signals nor the fence wait for such values through the semaphore waits of
  // earlier submissions to the queue.
  void CheckSubmission(
      GITSKey key, VkQueue queue, uint32_t submitCount, const VkSubmitInfo* submits, VkFence fence);
  void CheckSubmission(GITSKey key,
                       VkQueue queue,
                       uint32_t submitCount,
                       const VkSubmitInfo2* submits,
                       VkFence fence);
  // After a successful submission or sparse binding: tracks the semaphore signal operations it
  // submits, and the timeline semaphore values they and the later work of the queue may wait for.
  void TrackSignals(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* submits);
  void TrackSignals(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2* submits);
  void TrackSignals(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* bindInfos);
  // Whether resources are dumped after the submission, so its signal operations are deferred.
  // Logs why not if the queue cannot copy them.
  bool DumpsAfterSubmission(GITSKey key, VkQueue queue);
  // The signal operations of a submission that dumps resources after it cannot be deferred.
  void CannotDeferSignals(GITSKey key, const char* reason);
  // Whether waiting for the value of the semaphore may depend on signal operations submitted after
  // the wait. A timeline semaphore value does not if it is reached, or if a submitted signal
  // operation signals it without waiting for values that may. A binary semaphore wait waits for a
  // signal operation submitted before it, which may itself wait for such values.
  bool WaitMayDependOnLaterSignals(VkSemaphore semaphore, uint64_t value) const;
  // Whether the work submitted to the queue may wait for signal operations not submitted yet.
  bool QueueMayWaitForLaterSignals(VkQueue queue) const;
  // After a submission. Dumps after it if its signals are given, and submits them, even if dumping
  // fails.
  void Submit(GITSKey key,
              VkQueue queue,
              const std::vector<VkCommandBuffer>& commandBuffers,
              const SubmitSignals* signals);

private:
  // A layout transition, queue family ownership transfer or use of a resource on a queue family
  // recorded into a command buffer, which takes effect when the command buffer executes.
  struct StateChange {
    VkBuffer Buffer{};
    VkImage Image{};
    VkImageSubresourceRange Range{};
    std::optional<VkImageLayout> Layout;
    std::optional<OwnershipChange> Owner;
  };

  struct Resource {
    GITSKey Key{};
    VkDevice Device{};
    bool TransferSrc{};
    bool Sparse{};
    bool Protected{};
    bool Bound{};
    bool Exclusive{};
    // The queue families that may access a resource created with VK_SHARING_MODE_CONCURRENT.
    std::vector<uint32_t> SharingQueueFamilies;
    Ownership Owner;
  };

  struct Buffer : Resource {
    VkDeviceSize Size{};
  };

  struct Image : Resource {
    VkFormat Format{};
    VkImageTiling Tiling{};
    VkImageType Type{};
    VkExtent3D Extent{};
    VkSampleCountFlagBits Samples{};
    bool Disjoint{};
    // The aspect of a depth/stencil image created without the transfer source usage, which
    // VkImageStencilUsageCreateInfo sets apart for the stencil, while the other aspect has it.
    VkImageAspectFlags NoTransferSrcAspect{};
    VkSwapchainKHR Swapchain{};
    // A swapchain image acquired and not presented or released since.
    bool Acquired{};
    // The queues that waited for the semaphore of the acquisition of the swapchain image.
    std::unordered_set<VkQueue> AcquisitionWaits;
    // Acquired with a fence only, which the application waits for before using the image.
    bool AcquiredWithFence{};
    ImageLayouts Layouts;
  };

  struct View {
    VkImage Image{};
    // The subresources of the image. A 2D view of a 3D image selects depth slices instead of
    // array layers, so its range has the one array layer of the image.
    VkImageSubresourceRange Range{};
  };

  struct Framebuffer {
    // Empty for an imageless framebuffer.
    std::vector<VkImageView> Views;
    uint32_t Layers{};
  };

  // The copies recorded at a dump point, which execute after the first ChangeIndex changes of the
  // command buffer.
  struct DumpPoint {
    size_t ChangeIndex{};
    std::shared_ptr<DumpBatch> Batch;
    // Recorded into or executed by a command buffer begun with
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, so the copies may execute again before their
    // earlier execution completes.
    bool MayOverlap{};
  };

  struct CommandPoolState {
    uint32_t QueueFamilyIndex{};
    bool Protected{};
  };

  struct CommandBufferState {
    VkDevice Device{};
    VkCommandPool CommandPool{};
    uint32_t QueueFamilyIndex{};
    // Allocated from a protected command pool, so it cannot write the staging memory.
    bool Protected{};
    bool Secondary{};
    // Begun with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT.
    bool SimultaneousUse{};
    // Recorded and not submitted since. Its changes are expected to execute before the command
    // buffers recorded after it.
    bool Unsubmitted{};
    // Orders the command buffers by the end of their recording.
    uint64_t RecordingEnd{};
    // Inside a video coding scope, where resources cannot be copied.
    bool InVideoCoding{};
    // Inside a render pass instance, or after one that suspends.
    bool InRenderPass{};
    // The latest render pass instance ends by suspending, so the first render pass instance after
    // it in submission order resumes it.
    bool Suspending{};
    // The number of dump points recorded before the first render pass instance began.
    std::optional<size_t> DumpsBeforeRenderPass;
    // The layers of the attachments that the render pass instance renders: the views of its view
    // mask, or without multiview the first LayerCount layers.
    uint32_t LayerCount{};
    uint32_t ViewMask{};
    std::vector<VkImageView> Attachments;
    std::vector<AttachmentFinalLayouts> FinalLayouts;
    std::vector<StateChange> Changes;
    std::vector<DumpPoint> Dumps;
  };

  struct Device {
    VkPhysicalDevice PhysicalDevice{};
    bool SeparateDepthStencilLayouts{};
    // maintenance10 is enabled and VkFormatProperties3 reports the format features of depth/stencil
    // copies on queue families without graphics support.
    bool NonGraphicsDepthStencilCopies{};
    // Indexed by queue family.
    std::vector<VkQueueFlags> QueueFamilyFlags;
    // The queue families of the command pools of the device.
    std::set<uint32_t> QueueFamilies;
    std::unique_ptr<ResourcesDump> Dump;
  };

  // The changes executing before a dump point, in execution order.
  using ChangeLists = std::vector<const std::vector<StateChange>*>;

  // A batch of a submission. The signals of vkQueueSubmit wait for every command.
  struct SubmitBatch {
    std::vector<VkSemaphoreSubmitInfo> Waits;
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphoreSubmitInfo> Signals;
  };
  static std::vector<SubmitBatch> Batches(uint32_t submitCount, const VkSubmitInfo* submits);
  static std::vector<SubmitBatch> Batches(uint32_t submitCount, const VkSubmitInfo2* submits);
  static std::vector<SubmitBatch> Batches(uint32_t bindInfoCount,
                                          const VkBindSparseInfo* bindInfos);

  // A timeline semaphore value that submitted work may wait for.
  using TimelineWait = std::pair<VkSemaphore, uint64_t>;
  struct TimelineSignal {
    uint64_t Value{};
    // The values the signal operation may wait for.
    std::vector<TimelineWait> Waits;
  };

  ResourcesDump& GetDump(VkDevice device);
  // Waits until the dumps of the device are written, unless none is written for a while, and stops
  // its worker. Returns the failure of the worker, or reports the dumped resources not written, or
  // returns an empty string.
  std::string FinishDump(ResourcesDump& dump);
  // Finishes the dumps of every device, and returns the first failure, or an empty string.
  std::string FinishDumps();
  CommandBufferState* GetCommandBuffer(VkCommandBuffer commandBuffer);
  void ClearCommandBuffer(CommandBufferState& state);
  void TrackOwnershipChange(const CommandBufferState& state,
                            StateChange& change,
                            uint32_t srcQueueFamilyIndex,
                            uint32_t dstQueueFamilyIndex,
                            bool whole);
  void TrackBufferBarrier(CommandBufferState& state,
                          VkBuffer buffer,
                          VkDeviceSize offset,
                          VkDeviceSize size,
                          uint32_t srcQueueFamilyIndex,
                          uint32_t dstQueueFamilyIndex);
  void TrackImageBarrier(CommandBufferState& state,
                         VkImage image,
                         const VkImageSubresourceRange& range,
                         VkImageLayout oldLayout,
                         VkImageLayout newLayout,
                         uint32_t srcQueueFamilyIndex,
                         uint32_t dstQueueFamilyIndex);
  // The attachment views of a render pass instance are used on the queue family of the command
  // buffer.
  void TrackAttachmentUse(CommandBufferState& state, VkImageView view);
  // The subresources of the attachment view that the render pass instance of the command buffer
  // renders.
  std::vector<VkImageSubresourceRange> RenderedRanges(const CommandBufferState& state,
                                                      const View& view,
                                                      const Image& image) const;
  void Apply(const StateChange& change);
  // The submitted changes are in the tracked state already. Before a dump point of a command
  // buffer execute the changes of the command buffers recorded but not submitted yet, in the order
  // their recording ended, followed by those of the command buffer so far.
  ChangeLists ChangesBefore(const CommandBufferState* state) const;
  ImageLayouts ResolveLayouts(VkImage handle, const Image& image, const ChangeLists& before) const;
  Ownership ResolveOwner(VkBuffer buffer,
                         VkImage image,
                         const Resource& resource,
                         const ChangeLists& before) const;
  void RecordDumps(VkCommandBuffer commandBuffer,
                   CommandBufferState& state,
                   std::vector<DumpTarget> targets);
  // The listed resources of a dump point in the command buffer, or after a submission if there is
  // no command buffer.
  std::vector<DumpTarget> CollectTargets(GITSKey commandKey,
                                         VkDevice device,
                                         uint32_t queueFamilyIndex,
                                         const CommandBufferState* state);
  std::optional<DumpTarget> BufferTarget(GITSKey commandKey,
                                         VkBuffer handle,
                                         const Buffer& buffer,
                                         uint32_t queueFamilyIndex,
                                         const ChangeLists& before);
  std::optional<DumpTarget> ImageTarget(GITSKey commandKey,
                                        VkImage handle,
                                        const Image& image,
                                        uint32_t queueFamilyIndex,
                                        const ChangeLists& before);
  // Completes a target of an image whose layouts are defined. The context starts the messages of
  // skipped images.
  std::optional<DumpTarget> MakeImageTarget(const std::string& context,
                                            DumpTarget target,
                                            const Image& image,
                                            const Ownership& owner,
                                            uint32_t queueFamilyIndex);
  bool CanDump(const std::string& context,
               const char* kind,
               const Resource& resource,
               const Ownership& owner,
               uint32_t queueFamilyIndex);
  // Why the queue family cannot copy the exclusive resource, or empty if it can.
  std::string OwnershipProblem(const Resource& resource,
                               const Ownership& owner,
                               uint32_t queueFamilyIndex) const;
  void CheckSubmission(GITSKey key,
                       VkQueue queue,
                       const std::vector<SubmitBatch>& batches,
                       VkFence fence);
  void TrackSignals(VkQueue queue, const std::vector<SubmitBatch>& batches);
  bool TimelineReached(VkSemaphore semaphore, uint64_t value) const;
  // Whether the value is reached, or signaled by a submitted signal operation that waits only for
  // such values. depth limits the signal operations followed.
  bool Signaled(const TimelineWait& wait, unsigned depth) const;
  bool AllSignaled(const std::vector<TimelineWait>& waits, unsigned depth) const;
  // Why copies on the queue cannot read the image, as the presentation engine may use it, or empty
  // if they can.
  std::string SwapchainImageProblem(const Image& image, VkQueue queue) const;
  // Whether the queue family supports the transfer commands that copy resources.
  bool SupportsCopies(VkDevice device, uint32_t queueFamilyIndex) const;
  // The depth/stencil aspects of the image that the queue family cannot copy, as it has no
  // graphics support.
  VkImageAspectFlags AspectsWithoutCopies(const Image& image, uint32_t queueFamilyIndex) const;
  std::string CommandContext(GITSKey commandKey) const;
  std::filesystem::path DumpName(GITSKey commandKey, const char* kind, GITSKey resourceKey) const;
  // Finishes the dumps once the replay ends, and returns the first failure, or an empty string.
  std::string Close();

  DispatchTablesHolder& m_DispatchTablesHolder;
  std::unordered_set<GITSKey> m_ResourceKeys;
  std::unordered_set<GITSKey> m_CommandKeys;
  std::unordered_set<GITSKey> m_ReachedCommandKeys;
  std::unordered_set<GITSKey> m_CreatedResourceKeys;
  ImageFormat m_ImageFormat{};
  std::filesystem::path m_DumpPath;
  unsigned m_EndSubscription{};
  bool m_Closed{};
  uint64_t m_RecordingEnds{};

  std::unordered_map<VkDevice, Device> m_Devices;
  std::unordered_map<VkQueue, std::pair<VkDevice, uint32_t>> m_Queues;
  // The device of each semaphore, and whether it is a timeline semaphore.
  std::unordered_map<VkSemaphore, std::pair<VkDevice, bool>> m_Semaphores;
  // The timeline semaphore values that the work submitted to each queue may wait for, directly or
  // through the binary semaphore signal operations it waits for, unless they are signaled.
  std::unordered_map<VkQueue, std::vector<TimelineWait>> m_QueueWaits;
  // The values that the pending signal operation of each binary semaphore may wait for.
  std::unordered_map<VkSemaphore, std::vector<TimelineWait>> m_BinarySignalWaits;
  // The submitted signal operations of each timeline semaphore.
  std::unordered_map<VkSemaphore, std::vector<TimelineSignal>> m_TimelineSignals;
  std::unordered_map<VkBuffer, Buffer> m_Buffers;
  std::unordered_map<VkImage, Image> m_Images;
  // The create infos of the swapchains, without their pNext chains and queue family indices.
  std::unordered_map<VkSwapchainKHR, VkSwapchainCreateInfoKHR> m_Swapchains;
  std::unordered_map<VkSwapchainKHR, std::vector<uint32_t>> m_SwapchainSharingQueueFamilies;
  // The images of each swapchain, by index.
  std::unordered_map<VkSwapchainKHR, std::vector<VkImage>> m_SwapchainImages;
  // The semaphores of swapchain image acquisitions that no submission waited for yet.
  std::unordered_map<VkSemaphore, VkImage> m_AcquireSemaphores;
  std::unordered_map<VkImageView, View> m_Views;
  std::unordered_map<VkRenderPass, std::vector<AttachmentFinalLayouts>> m_RenderPasses;
  // The union of the view masks of the subpasses of each multiview render pass.
  std::unordered_map<VkRenderPass, uint32_t> m_RenderPassViewMasks;
  std::unordered_map<VkFramebuffer, Framebuffer> m_Framebuffers;
  std::unordered_map<VkCommandPool, CommandPoolState> m_CommandPools;
  std::unordered_map<VkCommandBuffer, CommandBufferState> m_CommandBuffers;
};

} // namespace vulkan
} // namespace gits
