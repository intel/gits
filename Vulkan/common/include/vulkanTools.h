// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanTools_lite.h"
#include "texture_converter.h"
#ifndef BUILD_FOR_CCODE
#include "vulkanArgumentsAuto.h"
#include "vulkanStructStorageAuto.h"
#include "config.h"
#include <set>
#endif
#include <functional>
#include <string>
#include <vector>

namespace gits {

namespace Vulkan {

struct CBufferState;

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
typedef enum _VulkanResourceType {
  VULKAN_NONE_RESOURCE,
  //READ_WRITE
  VULKAN_STORAGE_IMAGE,
  VULKAN_STORAGE_BUFFER,
  VULKAN_STORAGE_TEXEL_BUFFER,
  VULKAN_STORAGE_BUFFER_DYNAMIC,
  //READ_ONLY
  VULKAN_SAMPLED_IMAGE,
  VULKAN_UNIFORM_TEXEL_BUFFER,
  VULKAN_UNIFORM_BUFFER,
  VULKAN_UNIFORM_BUFFER_DYNAMIC,
  VULKAN_INLINE_UNIFORM_BLOCK_EXT,
  VULKAN_INPUT_ATTACHMENT,
  VULKAN_ACCELARATION_STRUCTURE_KHR,
  VULKAN_ACCELARATION_STRUCTURE_NV,
  VULKAN_SAMPLER,
  VULKAN_COMBINED_IMAGE_SAMPLER,
  //WRITE
  VULKAN_BLIT_DESTINATION_BUFFER,
  VULKAN_BLIT_DESTINATION_IMAGE,
  VULKAN_RESOLVE_IMAGE
} VulkanResourceType;
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
bool checkForSupportForQueues(VkPhysicalDevice physicalDevice,
                              uint32_t requestedQueueCreateInfoCount,
                              VkDeviceQueueCreateInfo const* requestedQueueCreateInfos);
#ifndef BUILD_FOR_CCODE
bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                  uint32_t extensionsCount,
                                  char const* const* extensions,
                                  bool printErrorLog = false);
bool areDeviceExtensionsEnabled(VkDevice device,
                                uint32_t requestedExtensionsCount,
                                char const* const* requestedExtensions);
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
  CVkSubmitInfoArrayWrap(const CVkSubmitInfoArrayWrap& wrap);
};
std::set<uint64_t> getPointersUsedInQueueSubmit(CVkSubmitInfoArrayWrap& submitInfoData,
                                                const BitRange& objRange,
                                                gits::Config::VulkanObjectMode objMode);
CVkSubmitInfoArrayWrap getSubmitInfoForPrepare(const std::vector<uint32_t>& countersTable,
                                               const BitRange& objRange,
                                               gits::Config::VulkanObjectMode objMode);
void restoreToSpecifiedRenderPass(const BitRange& objRange, CVkSubmitInfoArrayWrap& submitInfoData);

CVkSubmitInfoArrayWrap getSubmitInfoForSchedule(const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                gits::Config::VulkanObjectMode objMode);
bool checkMemoryMappingFeasibility(VkDevice device,
                                   VkDeviceMemory memory,
                                   bool throwException = true);
bool checkMemoryMappingFeasibility(VkDevice device,
                                   uint32_t memoryTypeIndex,
                                   bool throwException = true);
uint32_t findCompatibleMemoryTypeIndex(VkPhysicalDevice physicalDevice,
                                       uint32_t originalMemoryTypeIndex,
                                       uint32_t currentCompatibleMemoryTypes);
std::shared_ptr<CBufferState> findBufferStateFromDeviceAddress(VkDeviceAddress deviceAddress);
bool isVulkanAPIVersionSupported(uint32_t major, uint32_t minor, VkPhysicalDevice physicalDevice);
void checkReturnValue(VkResult playerSideReturnValue,
                      CVkResult& recorderSideReturnValue,
                      const char* functionName);
bool IsObjectToSkip(uint64_t vulkanObject);
bool operator==(const CGits::CCounter& counter, const Config::VulkanObjectRange& vulkanObjRange);
bool vulkanCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer bufferHandle, std::string fileName);
bool vulkanCopyImage(VkCommandBuffer commandBuffer,
                     VkImage imageHandle,
                     std::string fileName,
                     bool isResource = false,
                     VkAttachmentStoreOp imageStoreOption = VK_ATTACHMENT_STORE_OP_STORE);
void vulkanScheduleCopyResources(VkCommandBuffer cmdBuffer,
                                 uint64_t queueSubmitNumber,
                                 uint32_t cmdBuffBatchNumber,
                                 uint32_t cmdBuffNumber,
                                 uint64_t renderPassNumber);
void vulkanScheduleCopyRenderPasses(VkCommandBuffer cmdBuffer,
                                    uint64_t queueSubmitNumber,
                                    uint32_t cmdBuffBatchNumber,
                                    uint32_t cmdBuffNumber,
                                    uint64_t renderPassNumber);

void vulkanDumpRenderPasses(VkCommandBuffer commandBuffer);
void vulkanDumpRenderPassResources(VkCommandBuffer cmdBuffer);

// Kudos to Piotr Horodecki
class MemoryAliasingTracker {
  struct Range {
    uint64_t offset;
    uint64_t size;

    mutable std::set<std::pair<uint64_t, bool>> resources;

    // We make this struct a functor, so it can be used as a comparator.
    bool operator()(Range const& lRange, Range const& rRange) const;
  };

  // The set contains Ranges and uses Range->operator() to compare them.
  using RangeSetType = std::set<Range, Range>;
  RangeSetType MemoryRanges;

  RangeSetType::iterator GetRange(uint64_t offset);
  void SplitRange(uint64_t offset);
  void AddResource(uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);
  void RemoveResource(uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForResource(
      uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);

public:
  MemoryAliasingTracker(uint64_t size);
  void AddImage(uint64_t offset, uint64_t size, VkImage image);
  void AddBuffer(uint64_t offset, uint64_t size, VkBuffer buffer);
  void RemoveImage(uint64_t offset, uint64_t size, VkImage image);
  void RemoveBuffer(uint64_t offset, uint64_t size, VkBuffer buffer);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForImage(uint64_t offset,
                                                                  uint64_t size,
                                                                  VkImage image);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForBuffer(uint64_t offset,
                                                                   uint64_t size,
                                                                   VkBuffer buffer);
};
#endif

} // namespace Vulkan
} // namespace gits
