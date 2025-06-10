// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanTools_lite.h"
#include "texture_converter.h"
#ifndef BUILD_FOR_CCODE
#include "vulkanStateDynamic.h"
#include "vulkanArgumentsAuto.h"
#include "vulkanStructStorageAuto.h"
#endif

namespace gits {
#ifndef BUILD_FOR_CCODE
bool operator==(const CGits::CCounter& counter, const VulkanObjectRange& vulkanObjRange);
#endif
namespace Vulkan {

struct CBufferState;
struct CDeviceMemoryState;
struct CCommandBufferState;

struct CWindowParameters {
  HINSTANCE hInstance;
  HWND hWnd;
  VkExtent2D extent;
};

typedef enum _VulkanWriteScreenshotMode {
  VULKAN_MODE_FRAMES,
  VULKAN_MODE_RENDER_PASS_ATTACHMENTS,
  VULKAN_MODE_RESOURCES
} VulkanWriteScreenshotMode;

typedef enum _VulkanDumpingMode {
  VULKAN_NONE,
  VULKAN_PER_COMMAND_BUFFER,
  VULKAN_PER_RENDER_PASS,
  VULKAN_PER_DRAW,
  VULKAN_PER_BLIT,
  VULKAN_PER_DISPATCH
} VulkanDumpingMode;

bool writeScreenshotUtil(std::string fileName,
                         VkQueue& queue,
                         VkImage& sourceImage,
                         VulkanWriteScreenshotMode mode = VULKAN_MODE_FRAMES,
                         VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE);
void writeScreenshot(VkQueue queue,
                     VkCommandBuffer cmdbuffer,
                     uint32_t commandBufferBatchNumber,
                     uint32_t cmdBufferNumber);
void writeScreenshot(VkQueue queue, const VkPresentInfoKHR& presentInfo);
void writeCCodeScreenshot(
    VkQueue queue,
    const VkPresentInfoKHR& presentInfo,
    const std::function<VkImage(VkSwapchainKHR, unsigned int)>& getImageFromSwapchain);
void writeResources(VkQueue queue,
                    VkCommandBuffer cmdbuffer,
                    uint32_t commandBufferBatchNumber,
                    uint32_t cmdBufferNumber);
void writeBufferUtil(std::string fileName, VkQueue& queue, VkBuffer& sourceBuffer);
texel_type getTexelToConvertFromImageFormat(VkFormat format);
void suppressPhysicalDeviceFeatures(std::vector<std::string> const& suppressFeatures,
                                    VkPhysicalDeviceFeatures* features);
void suppressRequestedNames(std::vector<const char*>& requestedNames,
                            std::vector<std::string> const& suppressedNames,
                            uint32_t& updatedCount,
                            const char* const*& updatedPtr);
bool checkForSupportForInstanceExtensions(uint32_t extensionsCount, char const* const* extensions);
bool checkForSupportForInstanceLayers(uint32_t requestedLayersCount,
                                      char const* const* requestedLayers);
bool checkForSupportForPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                              VkPhysicalDeviceFeatures* enabledFeatures);
#ifndef BUILD_FOR_CCODE
bool checkForSupportForQueues(VkPhysicalDevice physicalDevice,
                              uint32_t requestedQueueCreateInfoCount,
                              VkDeviceQueueCreateInfo const* requestedQueueCreateInfos);
bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                  uint32_t extensionsCount,
                                  char const* const* extensions,
                                  bool printErrorLog = false);
bool areDeviceExtensionsEnabled(VkDevice device,
                                uint32_t requestedExtensionsCount,
                                char const* const* requestedExtensions);
bool isSynchronization2FeatureEnabled(VkDevice device);
void printShaderHashes(VkPipeline pipeline);
void waitForAllDevices();
void destroyDeviceLevelResources(VkDevice device = VK_NULL_HANDLE);
void destroyInstanceLevelResources(VkInstance instance = VK_NULL_HANDLE);
void getRangesForMemoryUpdate(VkDeviceMemory memory,
                              std::vector<VkBufferCopy>& updatedRanges,
                              bool unmap);
void flushShadowMemory(VkDeviceMemory memory, bool unmap);
std::set<uint64_t> getRelatedPointers(std::set<uint64_t>& originalSet);
struct CVkSubmitInfoArrayWrap {
  CVkSubmitInfoDataArray submitInfoData;
  CVkSubmitInfo2DataArray submitInfo2Data;
  CVkSubmitInfoArrayWrap();
  CVkSubmitInfoArrayWrap(uint32_t submitCount, VkSubmitInfo* submitInfo);
  CVkSubmitInfoArrayWrap(uint32_t submitCount, VkSubmitInfo2* submit2Info);
};
std::set<uint64_t> getPointersUsedInQueueSubmit(CVkSubmitInfoArrayWrap& submitInfoData,
                                                const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                gits::VulkanObjectMode objMode);
CVkSubmitInfoArrayWrap getSubmitInfoForPrepare(const std::vector<uint32_t>& countersTable,
                                               const BitRange& objRange,
                                               gits::VulkanObjectMode objMode);
VkCommandBuffer GetLastCommandBuffer(CVkSubmitInfoArrayWrap& submitInfoData);
void restoreCommandBufferSettings(const BitRange& objRange,
                                  CVkSubmitInfoArrayWrap& submitInfoData,
                                  gits::VulkanObjectMode objMode,
                                  uint64_t renderPassNumber = 0);

CVkSubmitInfoArrayWrap getSubmitInfoForSchedule(const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                gits::VulkanObjectMode objMode);
bool checkMemoryMappingFeasibility(VkDevice device,
                                   VkDeviceMemory memory,
                                   bool throwException = true);
bool checkMemoryMappingFeasibility(VkDevice device,
                                   uint32_t memoryTypeIndex,
                                   bool throwException = true);
std::unordered_map<uint32_t, uint32_t> matchCorrespondingMemoryTypeIndexes(
    VkPhysicalDevice physicalDevice);
uint32_t getMappedMemoryTypeIndex(VkDevice device, uint32_t memoryTypeIndexOriginal);
uint32_t findCompatibleMemoryTypeIndex(VkMemoryPropertyFlags originalMemoryPropertyFlags,
                                       VkPhysicalDeviceMemoryProperties& currentMemoryProperties,
                                       uint32_t requirementsMemoryTypeBits);
uint32_t getRayTracingShaderGroupCaptureReplayHandleSize(VkDevice device);

using TemporaryBufferPairType =
    std::pair<std::shared_ptr<CDeviceMemoryState>, std::shared_ptr<CBufferState>>;

TemporaryBufferPairType createTemporaryBuffer(VkDevice device,
                                              VkDeviceSize size,
                                              VkBufferUsageFlags bufferUsage,
                                              CCommandBufferState* commandBufferState = nullptr,
                                              VkMemoryPropertyFlags requiredMemoryPropertyFlags = 0,
                                              bool ignoreInRecorder = true);
VkDescriptorSet getTemporaryDescriptorSet(VkDevice device, CCommandBufferState& commandBufferState);
void injectCopyCommand(VkCommandBuffer commandBuffer,
                       VkPipelineStageFlags srcPipelineStageFlags,
                       VkPipelineStageFlags dstPipelineStageFlags,
                       VkDeviceSize dataSize,
                       VkBuffer srcBuffer,
                       VkDeviceSize srcOffset,
                       VkAccessFlags srcAccessMaskPre,
                       VkAccessFlags srcAccessMaskPost,
                       VkBuffer dstBuffer,
                       VkDeviceSize dstOffset,
                       VkAccessFlags dstAccessMaskPre,
                       VkAccessFlags dstAccessMaskPost);
void mapMemoryAndCopyData(VkDevice device,
                          VkDeviceMemory destination,
                          VkDeviceSize offset,
                          const void* source,
                          VkDeviceSize dataSize);
void mapMemoryAndCopyData(void* destination,
                          VkDevice device,
                          VkDeviceMemory source,
                          VkDeviceSize offset,
                          VkDeviceSize dataSize);
VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer);
VkBuffer findBufferFromDeviceAddress(VkDeviceAddress deviceAddress);
VkDeviceAddress getAccelerationStructureDeviceAddress(
    VkDevice device, VkAccelerationStructureKHR accelerationStructure);
VkAccelerationStructureBuildControlDataGITS prepareAccelerationStructureControlData(
    VkCommandBuffer commandBuffer);
std::shared_ptr<CDescriptorSetLayoutState> createInternalDescriptorSetLayout(VkDevice device);
VkPipelineLayout createInternalPipelineLayout(VkDevice device,
                                              uint32_t descriptorSetLayoutCount,
                                              VkDescriptorSetLayout* pDescriptorSetLayouts);
VkPipeline createInternalPipeline(VkDevice device,
                                  VkPipelineLayout layout,
                                  const std::vector<uint32_t>& code);
bool isVulkanAPIVersionSupported(uint32_t major, uint32_t minor, VkPhysicalDevice physicalDevice);
void checkReturnValue(VkResult playerSideReturnValue,
                      CVkResult& recorderSideReturnValue,
                      const char* functionName);
uint32_t GetHash(uint32_t size, const void* data);
bool IsObjectToSkip(uint64_t vulkanObject);
bool vulkanCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer bufferHandle, std::string fileName);
bool vulkanCopyImage(VkCommandBuffer commandBuffer,
                     VkImage imageHandle,
                     std::string fileName,
                     bool isResource,
                     VkAttachmentStoreOp imageStoreOption,
                     VulkanDumpingMode dumpingMode);
void vulkanScheduleCopyResources(VkCommandBuffer cmdBuffer,
                                 uint64_t queueSubmitNumber,
                                 uint32_t cmdBuffBatchNumber,
                                 uint32_t cmdBuffNumber,
                                 uint64_t renderPassNumber,
                                 VulkanDumpingMode dumpingMode,
                                 uint64_t drawNumber = 0);
void vulkanScheduleCopyRenderPasses(VkCommandBuffer cmdBuffer,
                                    uint64_t queueSubmitNumber,
                                    uint32_t cmdBuffBatchNumber,
                                    uint32_t cmdBuffNumber,
                                    uint64_t renderPassNumber,
                                    uint64_t drawNumber = 0);

void vulkanDumpRenderPasses(VkCommandBuffer commandBuffer);
void vulkanDumpRenderPassResources(VkCommandBuffer cmdBuffer);
VkResult _vkCreateRenderPass_Helper(VkDevice device,
                                    const VkRenderPassCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkRenderPass& pRenderPass,
                                    CreationFunction createdWith);
VkResult _vkCreateRenderPass_Helper(VkDevice device,
                                    const VkRenderPassCreateInfo2* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkRenderPass& pRenderPass,
                                    CreationFunction createdWith);
template <typename T, typename attDesc>
void CreateRenderPasses_helper(VkDevice device,
                               VkRenderPass renderPass,
                               T pCreateInfo,
                               CreationFunction createdWith) {
  if ((Configurator::IsPlayer() &&
       Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer) ||
      (Configurator::IsRecorder() &&
       gits::CGits::Instance().apis.Iface3D().CfgRec_IsDrawsRangeMode())) {
    // For executing each Vulkan draw in separate VkCommandBuffer we need some additional VkRenderPasses:
    // - storeNoLoadRenderPassHandle - for modyfying original VkRenderPass - changing storeOp to STORE, loadOp is original
    // - loadAndStoreRenderPassHandle - after draw execution subsequent draw will load result from previous and store it (loadOp = LOAD, storeOp = STORE)
    // - restoreRenderPassHandle - for restoration of original settings and the end of RenderPass - loadOp = LOAD, storeOp is original
    T renderPassCreateInfo = pCreateInfo;
    std::vector<attDesc> attDescLoadAndStoreVector;
    std::vector<attDesc> attDescRestoreVector;
    std::vector<attDesc> attDescStoreNoLoadVector;
    bool loadAndStoreChanged = false;
    bool restoreChanged = false;
    bool storeNoLoadChanged = false;

    for (uint32_t i = 0; i < renderPassCreateInfo.attachmentCount; i++) {
      VkImageAspectFlags aspect = getFormatAspectFlags(renderPassCreateInfo.pAttachments[i].format);
      attDesc attDescLoadAndStore = renderPassCreateInfo.pAttachments[i];
      attDesc attDescRestore = renderPassCreateInfo.pAttachments[i];
      attDesc attDescStoreNoLoad = renderPassCreateInfo.pAttachments[i];
      if (aspect & VK_IMAGE_ASPECT_COLOR_BIT || aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        if (renderPassCreateInfo.pAttachments[i].loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
          attDescLoadAndStore.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          attDescRestore.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          if (renderPassCreateInfo.pAttachments[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            // We can't set loadOp to VK_ATTACHMENT_LOAD_OP_LOAD when initial layout is VK_IMAGE_LAYOUT_UNDEFINED
            // As we are in the middle (or at the end) of original VkRenderPass execution we are setting initialLayout to finalLayout
            attDescLoadAndStore.initialLayout = attDescLoadAndStore.finalLayout;
            attDescRestore.initialLayout = attDescRestore.finalLayout;
          }
          loadAndStoreChanged = true;
          restoreChanged = true;
        }
        if (renderPassCreateInfo.pAttachments[i].storeOp != VK_ATTACHMENT_STORE_OP_STORE) {
          attDescLoadAndStore.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          attDescStoreNoLoad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          loadAndStoreChanged = true;
          storeNoLoadChanged = true;
        }
      }
      if (aspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
        if (renderPassCreateInfo.pAttachments[i].stencilLoadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
          attDescLoadAndStore.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          attDescRestore.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          if (renderPassCreateInfo.pAttachments[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            // We can't set loadOp to VK_ATTACHMENT_LOAD_OP_LOAD when initial layout is VK_IMAGE_LAYOUT_UNDEFINED
            // As we are in the middle (or at the end) of original VkRenderPass execution we are setting initialLayout to finalLayout
            attDescLoadAndStore.initialLayout = attDescLoadAndStore.finalLayout;
            attDescRestore.initialLayout = attDescRestore.finalLayout;
          }
          loadAndStoreChanged = true;
          restoreChanged = true;
        }
        if (renderPassCreateInfo.pAttachments[i].stencilStoreOp != VK_ATTACHMENT_STORE_OP_STORE) {
          attDescLoadAndStore.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
          attDescStoreNoLoad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
          loadAndStoreChanged = true;
          storeNoLoadChanged = true;
        }
      }
      attDescLoadAndStoreVector.push_back(attDescLoadAndStore);
      attDescRestoreVector.push_back(attDescRestore);
      attDescStoreNoLoadVector.push_back(attDescStoreNoLoad);
    }
    if (loadAndStoreChanged) {
      renderPassCreateInfo.pAttachments = attDescLoadAndStoreVector.data();
      _vkCreateRenderPass_Helper(device, &renderPassCreateInfo, nullptr,
                                 SD()._renderpassstates[renderPass]->loadAndStoreRenderPassHandle,
                                 createdWith);

    } else {
      SD()._renderpassstates[renderPass]->loadAndStoreRenderPassHandle = renderPass;
    }
    if (restoreChanged) {
      renderPassCreateInfo.pAttachments = attDescRestoreVector.data();
      _vkCreateRenderPass_Helper(device, &renderPassCreateInfo, nullptr,
                                 SD()._renderpassstates[renderPass]->restoreRenderPassHandle,
                                 createdWith);
      if (Configurator::IsRecorder()) {
        auto restoreRenderPassState = std::make_shared<CRenderPassState>(
            &SD()._renderpassstates[renderPass]->restoreRenderPassHandle, &renderPassCreateInfo,
            createdWith, SD()._devicestates[device]);

        SD()._renderpassstates[renderPass]->restoreRenderPassStateStore =
            std::move(restoreRenderPassState);
      }
    } else {
      SD()._renderpassstates[renderPass]->restoreRenderPassHandle = renderPass;
    }
    if (storeNoLoadChanged) {
      renderPassCreateInfo.pAttachments = attDescStoreNoLoadVector.data();
      _vkCreateRenderPass_Helper(device, &renderPassCreateInfo, nullptr,
                                 SD()._renderpassstates[renderPass]->storeNoLoadRenderPassHandle,
                                 createdWith);
    } else {
      SD()._renderpassstates[renderPass]->storeNoLoadRenderPassHandle = renderPass;
    }
  }
}
unsigned int getBeginRenderFunctionID(unsigned int endFuncID);
void callvkCmdBeginRenderingByID(unsigned int ID,
                                 const VkCommandBuffer& commandBuffer,
                                 const VkRenderingInfo* pRenderingInfo);
void callvkCmdEndRenderingByID(unsigned int ID, const VkCommandBuffer& commandBuffer);
void callvkCmdBeginRenderPassByID(unsigned int ID,
                                  const VkCommandBuffer& commandBuffer,
                                  const VkRenderPassBeginInfo* pRenderPassBegin,
                                  const VkSubpassContents& contents);
void callvkCmdEndRenderPassByID(unsigned int ID, const VkCommandBuffer& commandBuffer);
void schedulevkCmdBeginRenderPassByID(unsigned int ID,
                                      void (*schedulerFunc)(Vulkan::CFunction*),
                                      const VkCommandBuffer& commandBuffer,
                                      const VkRenderPassBeginInfo* pRenderPassBegin,
                                      const VkSubpassContents& contents);
void schedulevkCmdBeginRenderingByID(unsigned int ID,
                                     void (*schedulerFunc)(Vulkan::CFunction*),
                                     const VkCommandBuffer& commandBuffer,
                                     const VkRenderingInfo* pRenderingInfo);
#endif

} // namespace Vulkan
} // namespace gits
