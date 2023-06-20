// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanTools.h"
DISABLE_WARNINGS
#include <boost/filesystem.hpp>
ENABLE_WARNINGS
#ifndef BUILD_FOR_CCODE
#include "vulkanStateDynamic.h"
#include "vulkanFunctions.h"
#else
#include "helperVk.h"
#endif
#include "gits.h"
#include "vulkanDrivers.h"
#include "vulkanLog.h"

#if defined(GITS_PLATFORM_WINDOWS) && !defined(BUILD_FOR_CCODE)

#include <Windows.h>
using namespace std;
#include <atlimage.h>

#endif

namespace gits {
namespace Vulkan {

std::string GetFileNameFrameScreenshot(unsigned int frameNumber) {
  auto path = Config::Get().player.outputDir;
  if (path.empty()) {
    path = Config::Get().common.streamDir / "gitsScreenshots";
    if (Config::Get().IsRecorder()) {
      path /= "gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path /= "gitsPlayer";
    } else {
      Log(ERR) << "Neither in player nor recorder!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  std::stringstream fileName;
  fileName << "frame" << std::setw(8) << std::setfill('0') << frameNumber;
  bfs::create_directories(path);
  path /= fileName.str();
  return path.string();
}

std::string GetFileNameDrawcallScreenshot(unsigned int frameNumber,
                                          unsigned int submitNumber,
                                          unsigned int cmdBufferBatchNumber,
                                          unsigned int cmdBufferNumber,
                                          unsigned int renderpass,
                                          uint64_t image,
                                          bool perCommandBuffer) {
  auto path = Config::Get().player.outputDir;
  if (path.empty()) {
    path = Config::Get().common.streamDir / "gitsScreenshots";
    if (Config::Get().IsRecorder()) {
      path /= "gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path /= "gitsPlayer";
    } else {
      Log(ERR) << "Neither in player nor recorder!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  std::stringstream fileName;
  fileName << "frame" << std::setw(4) << std::setfill('0') << frameNumber;
  fileName << "_queueSubmit_" << submitNumber;
  fileName << "_commandbufferBatch_" << cmdBufferBatchNumber;
  fileName << "_commandbuffer_" << cmdBufferNumber;
  if (!perCommandBuffer) {
    fileName << "_renderpass_" << renderpass;
  }
  fileName << "_image_" << image;
  bfs::create_directories(path);
  path /= fileName.str();
  return path.string();
}

std::string GetFileNameResourcesScreenshot(unsigned int frameNumber,
                                           unsigned int submitNumber,
                                           unsigned int cmdBufferBatchNumber,
                                           unsigned int cmdBufferNumber,
                                           uint64_t objectNumber,
                                           VulkanResourceType resType) {
  auto path = Config::Get().player.outputDir;
  if (path.empty()) {
    path = Config::Get().common.streamDir / "gitsScreenshots";
    if (Config::Get().IsRecorder()) {
      path /= "gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path /= "gitsPlayer";
    } else {
      Log(ERR) << "Neither in player nor recorder!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  std::stringstream suffix;
  if (resType == VULKAN_STORAGE_IMAGE) {
    suffix << "_storageImage_";
  } else if (resType == VULKAN_STORAGE_BUFFER) {
    suffix << "_storageBuffer_";
  } else if (resType == VULKAN_STORAGE_TEXEL_BUFFER) {
    suffix << "_storageTexelBuffer_";
  } else if (resType == VULKAN_STORAGE_BUFFER_DYNAMIC) {
    suffix << "_storageBufferDynamic_";
  } else if (resType == VULKAN_BLIT_DESTINATION_IMAGE) {
    suffix << "_blitDestinationImage_";
  } else if (resType == VULKAN_BLIT_DESTINATION_BUFFER) {
    suffix << "_blitDestinationBuffer_";
  }
  std::stringstream fileName;
  fileName << "frame" << std::setw(4) << std::setfill('0') << frameNumber;
  fileName << "_queueSubmit_" << submitNumber;
  fileName << "_commandbufferBatch_" << cmdBufferBatchNumber;
  fileName << "_commandbuffer_" << cmdBufferNumber;
  fileName << suffix.str() << objectNumber;
  bfs::create_directories(path);
  path /= fileName.str();
  return path.string();
}

#ifndef BUILD_FOR_CCODE
bool operator==(const CGits::CCounter& counter, const Config::VulkanObjectRange& vulkanObjRange) {
  if (vulkanObjRange.empty()) {
    return false;
  }
  if (vulkanObjRange.objVector.size() > counter.countersTable.size()) {
    return false;
  }
  for (size_t i = 0; i < vulkanObjRange.objVector.size(); i++) {
    if (counter.countersTable[i] != vulkanObjRange.objVector[i]) {
      return false;
    }
  }
  if (vulkanObjRange.range[(size_t)counter.countersTable[vulkanObjRange.objVector.size()]]) {
    return true;
  }
  return false;
}

bool vulkanCopyImage(VkCommandBuffer commandBuffer,
                     VkImage imageHandle,
                     VkAttachmentStoreOp imageStoreOption,
                     std::string fileName) {
  auto& imageState = SD()._imagestates[imageHandle];
  if (Config::Get().player.skipNonDeterministicImages &&
      (SD().nonDeterministicImages.find(imageHandle) != SD().nonDeterministicImages.end())) {
    return false;
  }

  if (isFormatCompressed(imageState->imageFormat) ||
      (imageStoreOption == VK_ATTACHMENT_STORE_OP_DONT_CARE)) {
    return false;
  }

  uint32_t numArrayLayers = 1;
  uint32_t numMipmapLevels = 1;
  VkImageAspectFlags imageAspect = getFormatAspectFlags(imageState->imageFormat);
  decltype(imageState->currentLayout) currentLayout;

  if (imageState->swapchainKHRStateStore) {
    numArrayLayers =
        imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData.Value()->imageArrayLayers;
    numMipmapLevels = 1;
  } else if (imageState->imageCreateInfoData.Value()) {
    numArrayLayers = imageState->imageCreateInfoData.Value()->arrayLayers;
    numMipmapLevels = imageState->imageCreateInfoData.Value()->mipLevels;
  }
  currentLayout = imageState->currentLayout;
  for (uint32_t l = 0; l < numArrayLayers; ++l) {
    for (uint32_t m = 0; m < numMipmapLevels; ++m) {
      uint32_t width = std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, m));
      uint32_t height = std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, m));
      VkImageLayout layout = currentLayout[l][m].Layout;
      VkAccessFlags access = currentLayout[l][m].Access;

      if (layout != VK_IMAGE_LAYOUT_UNDEFINED) {

        for (uint32_t a = 0; a < 3; ++a) {
          VkImageAspectFlagBits aspect = static_cast<VkImageAspectFlagBits>(1 << a);

          if (imageAspect & aspect) {
            std::shared_ptr<RenderGenericAttachment> attachment(new RenderGenericAttachment());
            attachment->aspect = aspect;
            attachment->layer = l;
            attachment->mipmap = m;
            attachment->localCounter = SD().imageCounter[imageHandle];
            attachment->fileName = fileName;
            attachment->sourceImage = imageHandle;

            // Create buffer
            {
              VkBufferCreateInfo bufferCreateInfo = {
                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
                  nullptr,                              // const void* pNext;
                  0,                                    // VkBufferCreateFlags flags;
                  width * height *
                      getFormatBytesPerPixel(imageState->imageFormat), // VkDeviceSize size;
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,                    // VkBufferUsageFlags usage;
                  VK_SHARING_MODE_EXCLUSIVE,                           // sharingMode;
                  0,      // uint32_t queueFamilyIndexCount;
                  nullptr // const uint32_t* pQueueFamilyIndices;
              };

              drvVk.vkCreateBuffer(imageState->deviceStateStore->deviceHandle, &bufferCreateInfo,
                                   nullptr, &attachment->copiedBuffer);
              if (VK_NULL_HANDLE == attachment->copiedBuffer) {
                throw EOperationFailed("Could not create a buffer to store screenshot data.");
              }
            }

            // Allocate buffer memory
            {
              VkMemoryRequirements memoryRequirements;
              drvVk.vkGetBufferMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                  attachment->copiedBuffer, &memoryRequirements);

              if (!memoryRequirements.size) {
                throw EOperationFailed(
                    "vkGetBufferMemoryRequirements() returned requirement with 0 size.");
              }

              // To maintain stream compatibility, use memory properties of the platform the (original) stream was recorded on
              // If there are none, get memory properties of the current platform
              auto& physicalDeviceState =
                  SD()._devicestates[imageState->deviceStateStore->deviceHandle]
                      ->physicalDeviceStateStore;
              VkPhysicalDeviceMemoryProperties memoryProperties =
                  physicalDeviceState->memoryProperties;
              if (memoryProperties.memoryHeapCount == 0) {
                drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDeviceState->physicalDeviceHandle,
                                                          &memoryProperties);
              }
              uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
              for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) ==
                    requiredFlags) {
                  if (memoryRequirements.memoryTypeBits & (1 << i)) {
                    VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                                    memoryRequirements.size, i};

                    drvVk.vkAllocateMemory(imageState->deviceStateStore->deviceHandle, &memInfo,
                                           nullptr, &attachment->devMemory);
                    drvVk.vkBindBufferMemory(imageState->deviceStateStore->deviceHandle,
                                             attachment->copiedBuffer, attachment->devMemory, 0);
                    break;
                  }
                }
              }
              if (VK_NULL_HANDLE == attachment->devMemory) {
                throw EOperationFailed("Could not allocate memory for a buffer.");
              }
            }
            // Perform pre-copy image layout transition
            {
              VkImageMemoryBarrier preCopyImageBarrier = {
                  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                // const void* pNext;
                  access,                                 // VkAccessFlags srcAccessMask;
                  VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags dstAccessMask;
                  layout,                                 // VkImageLayout oldLayout;
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout newLayout;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
                  imageHandle,                            // VkImage image;
                  {
                      // VkImageSubresourceRange subresourceRange;
                      static_cast<VkImageAspectFlags>(
                          imageAspect), // VkImageAspectFlags aspectMask;
                      m,                // uint32_t baseMipLevel;
                      1,                // uint32_t levelCount;
                      l,                // uint32_t baseArrayLayer;
                      1                 // uint32_t layerCount;
                  }};
              VkBufferMemoryBarrier preCopyBufferBarrier = {
                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                 // const void* pNext;
                  0,                                       // VkAccessFlags srcAccessMask;
                  VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                  attachment->copiedBuffer,                // VkBuffer buffer;
                  0,                                       // VkDeviceSize offset;
                  VK_WHOLE_SIZE                            // VkDeviceSize size;
              };
              drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                                         &preCopyBufferBarrier, 1, &preCopyImageBarrier);
            }

            // Copy image data from swapchain to buffer
            {
              VkBufferImageCopy bufferImageCopyRegion = {
                  0, // VkDeviceSize bufferOffset;
                  0, // uint32_t bufferRowLength;
                  0, // uint32_t bufferImageHeight;
                  {
                      // VkImageSubresourceLayers imageSubresource;
                      static_cast<VkImageAspectFlags>(aspect), // VkImageAspectFlags aspectMask;
                      m,                                       // uint32_t mipLevel;
                      l,                                       // uint32_t baseArrayLayer;
                      1                                        // uint32_t layerCount;
                  },
                  {
                      // VkOffset3D imageOffset;
                      0, // int32_t x;
                      0, // int32_t y;
                      0  // int32_t z;
                  },
                  {
                      // VkExtent3D imageExtent;
                      width,  // uint32_t width;
                      height, // uint32_t height;
                      1       // uint32_t depth;
                  }};
              drvVk.vkCmdCopyImageToBuffer(commandBuffer, imageHandle,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           attachment->copiedBuffer, 1, &bufferImageCopyRegion);
            }

            // Perform post-copy image layout transition
            {
              VkImageMemoryBarrier postCopyImageBarrier = {
                  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                // const void* pNext;
                  VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags srcAccessMask;
                  access,                                 // VkAccessFlags dstAccessMask;
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout oldLayout;
                  layout,                                 // VkImageLayout newLayout;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
                  imageHandle,                            // VkImage image;
                  {
                      // VkImageSubresourceRange subresourceRange;
                      static_cast<VkImageAspectFlags>(
                          imageAspect), // VkImageAspectFlags aspectMask;
                      m,                // uint32_t baseMipLevel;
                      1,                // uint32_t levelCount;
                      l,                // uint32_t baseArrayLayer;
                      1                 // uint32_t layerCount;
                  }};
              VkBufferMemoryBarrier postCopyBufferBarrier = {
                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                 // const void* pNext;
                  VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask;
                  VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                  attachment->copiedBuffer,                // VkBuffer buffer;
                  0,                                       // VkDeviceSize offset;
                  VK_WHOLE_SIZE                            // VkDeviceSize size;
              };
              drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1,
                                         &postCopyBufferBarrier, 1, &postCopyImageBarrier);
            }
            SD()._commandbufferstates[commandBuffer]->renderPassImages.push_back(attachment);
          }
        }
      }
    }
  }
  return true;
}

void vulkanScheduleCopyRenderPasses(VkCommandBuffer cmdBuffer,
                                    uint64_t queueSubmitNumber,
                                    uint32_t cmdBuffBatchNumber,
                                    uint32_t cmdBuffNumber,
                                    uint64_t renderPassNumber) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuffer];
  auto& framebufferState =
      commandBufferState->beginRenderPassesList[renderPassNumber]->framebufferStateStore;
  uint32_t imageViewSize;
  if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                            VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
    imageViewSize = (uint32_t)framebufferState->imageViewStateStoreList.size();
  } else {
    imageViewSize = (uint32_t)commandBufferState->beginRenderPassesList[renderPassNumber]
                        ->imageViewStateStoreListKHR.size();
  }
  for (uint32_t imageview = 0; imageview < imageViewSize; ++imageview) {
    VkImage imageHandle;
    VkAttachmentStoreOp imageStoreOption =
        commandBufferState->beginRenderPassesList[renderPassNumber]->imageStoreOp[imageview];
    if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                              VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      imageHandle =
          framebufferState->imageViewStateStoreList[imageview]->imageStateStore->imageHandle;
    } else {
      imageHandle = commandBufferState->beginRenderPassesList[renderPassNumber]
                        ->imageViewStateStoreListKHR[imageview]
                        ->imageStateStore->imageHandle;
    }

    std::string fileName = GetFileNameDrawcallScreenshot(
        CGits::Instance().CurrentFrame(), queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
        renderPassNumber, SD().imageCounter[imageHandle], false);
    vulkanCopyImage(cmdBuffer, imageHandle, imageStoreOption, fileName);
  }
}

void vulkanDumpRenderPasses(VkCommandBuffer cmdBuffer) {
  for (auto attachment : SD()._commandbufferstates[cmdBuffer]->renderPassImages) {
    VkImage image = attachment->sourceImage;
    auto imageState = SD()._imagestates[image];
    uint32_t width =
        std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, attachment->mipmap));
    uint32_t height =
        std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, attachment->mipmap));

    unsigned char* ptr;
    drvVk.vkDeviceWaitIdle(imageState->deviceStateStore->deviceHandle);
    drvVk.vkMapMemory(imageState->deviceStateStore->deviceHandle, attachment->devMemory, 0,
                      width * height * getFormatBytesPerPixel(imageState->imageFormat), 0,
                      (void**)&ptr);
    std::vector<uint8_t> screenshotData(
        ptr, ptr + (width * height * getFormatBytesPerPixel(imageState->imageFormat)));

    try {
      std::vector<uint8_t> convertedData(width * height * 4);
      bool depthInProperRange = true;
      if (attachment->aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
        convert_texture_data(getTexelToConvertFromImageFormat(imageState->imageFormat),
                             screenshotData, texel_type::RGBA8, convertedData, width, height);
      } else if (attachment->aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
        auto fmt = imageState->imageFormat;
        std::pair<double, double> minMaxValues(0.0, 0.0);
        if (fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT) {
          minMaxValues = get_min_max_values(texel_type::R32f, screenshotData, width, height);
          normalize_texture_data(texel_type::R32f, screenshotData, width, height);
          convert_texture_data(texel_type::R32f, screenshotData, texel_type::RGBA8, convertedData,
                               width, height);
        } else if (fmt == VK_FORMAT_D24_UNORM_S8_UINT || fmt == VK_FORMAT_X8_D24_UNORM_PACK32) {
          normalize_texture_data(texel_type::D24, screenshotData, width, height);
          convert_texture_data(texel_type::D24, screenshotData, texel_type::RGBA8, convertedData,
                               width, height);
        } else if (fmt == VK_FORMAT_D16_UNORM || fmt == VK_FORMAT_D16_UNORM_S8_UINT) {
          normalize_texture_data(texel_type::R16, screenshotData, width, height);
          convert_texture_data(texel_type::R16, screenshotData, texel_type::RGBA8, convertedData,
                               width, height);
        }
        if ((minMaxValues.first < 0.0 || minMaxValues.second > 1.0) &&
            Config::Get().player.skipNonDeterministicImages &&
            !SD().depthRangeUnrestrictedEXTEnabled) {
          // When copying to a depth aspect, and the
          // VK_EXT_depth_range_unrestricted extension is not enabled, the
          // data in buffer memory (SFLOAT format) must be in the range
          // [0,1], or the resulting values are undefined.
          depthInProperRange = false;
        }
      } else if (attachment->aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
        normalize_texture_data(texel_type::R8, screenshotData, width, height);
        convert_texture_data(texel_type::R8, screenshotData, texel_type::RGBA8, convertedData,
                             width, height);
      }
      {
        std::stringstream nameSuffix;
        if (attachment->layer > 1) {
          nameSuffix << "_layer_" << std::setw(2) << std::setfill('0') << attachment->layer;
        }
        if (attachment->mipmap > 1) {
          nameSuffix << "_mipmap_" << std::setw(2) << std::setfill('0') << attachment->mipmap;
        }
        switch (attachment->aspect) {
        case VK_IMAGE_ASPECT_DEPTH_BIT:
          nameSuffix << "_depth";
          break;
        case VK_IMAGE_ASPECT_STENCIL_BIT:
          nameSuffix << "_stencil";
          break;
        default:
          Log(TRACE) << "Not handled VkImageAspectFlagBits enumeration: " +
                            std::to_string(attachment->aspect);
          break;
        }
        nameSuffix << ".png";
        if (depthInProperRange) {
          CGits::Instance().WriteImage(attachment->fileName + nameSuffix.str(), width, height, true,
                                       convertedData, false, false, false);
        }
      }
    } catch (...) {
      Log(ERR) << "Could not convert image data to RGBA8 format.";
    }
    drvVk.vkUnmapMemory(imageState->deviceStateStore->deviceHandle, attachment->devMemory);
    drvVk.vkFreeMemory(imageState->deviceStateStore->deviceHandle, attachment->devMemory, nullptr);
    drvVk.vkDestroyBuffer(imageState->deviceStateStore->deviceHandle, attachment->copiedBuffer,
                          nullptr);
  }
  SD()._commandbufferstates[cmdBuffer]->renderPassImages.clear();
}
#endif

bool writeScreenshotUtil(std::string fileName,
                         VkQueue& queue,
                         VkImage& sourceImage,
                         VulkanWriteScreenshotMode mode,
                         VkAttachmentStoreOp storeOp) {
#ifndef BUILD_FOR_CCODE
  auto& queueState = SD()._queuestates[queue];
  VkDevice device = queueState->deviceStateStore->deviceHandle;
  uint32_t queueFamilyIndex = queueState->queueFamilyIndex;
  auto& imageState = SD()._imagestates[sourceImage];
  auto& internalResources = SD().internalResources;

  if (Config::Get().player.skipNonDeterministicImages &&
      (SD().nonDeterministicImages.find(sourceImage) != SD().nonDeterministicImages.end())) {
    return false;
  }
#else
  auto& queueState = globalState.queueStates[queue];
  VkDevice device = queueState.device;
  uint32_t queueFamilyIndex = queueState.deviceQueueList.front().queueFamilyIndex;
  auto& imageState = globalState.imageStates.at(sourceImage);
  auto& internalResources = globalState.internalResources;
#endif

  if (isFormatCompressed(imageState->imageFormat) ||
      (storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE)) {
    return false;
  }

  auto& queueFamilyCommandPoolMap = internalResources.deviceResourcesMap[device];
  auto iterator = queueFamilyCommandPoolMap.find(queueFamilyIndex);
  if (iterator == queueFamilyCommandPoolMap.end()) {
    // Create command pool
    VkCommandPool commandPool;
    {
      VkCommandPoolCreateInfo commandPoolCreateInfo = {
          VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // VkStructureType sType;
          nullptr,                                    // const void* pNext;
          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // VkCommandPoolCreateFlags flags;
          queueFamilyIndex                                     // uint32_t queueFamilyIndex;
      };
      drvVk.vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

      if (VK_NULL_HANDLE == commandPool) {
        throw EOperationFailed(
            "Could not create command pool for screenshot data copying command buffer.");
      }
    }

    // Allocate command buffer
    VkCommandBuffer commandBuffer;
    {
      VkCommandBufferAllocateInfo allocateCommandBufferInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType sType;
          nullptr,                                        // const void* pNext;
          commandPool,                                    // VkCommandPool commandPool;
          VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel level;
          1                                               // uint32_t commandBufferCount;
      };
      drvVk.vkAllocateCommandBuffers(device, &allocateCommandBufferInfo, &commandBuffer);

      if (VK_NULL_HANDLE == commandPool) {
        throw EOperationFailed("Could not allocate screenshot data copying command buffer.");
      }
    }

    iterator =
        queueFamilyCommandPoolMap.insert({queueFamilyIndex, {commandPool, commandBuffer}}).first;
  }

  VkCommandBuffer commandBuffer = iterator->second.second;

  // Begin command buffer
  {
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType sType;
        nullptr,                                     // const void* pNext;
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags flags;
        nullptr // const VkCommandBufferInheritanceInfo* pInheritanceInfo;
    };
    drvVk.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  }

  // Get image data
  struct CScreenshotData {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkImageAspectFlagBits aspect;
  };
  uint32_t numArrayLayers = 1;
  uint32_t numMipmapLevels = 1;
  VkImageAspectFlags imageAspect = getFormatAspectFlags(imageState->imageFormat);
  decltype(imageState->currentLayout) currentLayout;
  std::vector<std::vector<std::vector<CScreenshotData>>> target; // Per layer, mipmap, aspect

#ifndef BUILD_FOR_CCODE
  if (imageState->swapchainKHRStateStore) {
    numArrayLayers =
        imageState->swapchainKHRStateStore->swapchainCreateInfoKHRData.Value()->imageArrayLayers;
    numMipmapLevels = 1;
    currentLayout = {{{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT}}};
  } else if (imageState->imageCreateInfoData.Value()) {
    numArrayLayers = imageState->imageCreateInfoData.Value()->arrayLayers;
    numMipmapLevels = imageState->imageCreateInfoData.Value()->mipLevels;
    currentLayout = imageState->currentLayout;
  }
#else
  if (imageState->swapchainCreateInfo) {
    numArrayLayers = imageState->swapchainCreateInfo->imageArrayLayers;
    numMipmapLevels = 1;
    currentLayout = {{{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT}}};
  } else if (imageState->imageCreateInfo) {
    numArrayLayers = imageState->imageCreateInfo->arrayLayers;
    numMipmapLevels = imageState->imageCreateInfo->mipLevels;
    currentLayout = imageState->currentLayout;
  }
#endif
  for (uint32_t l = 0; l < numArrayLayers; ++l) {
    target.push_back({});

    for (uint32_t m = 0; m < numMipmapLevels; ++m) {
      uint32_t width = std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, m));
      uint32_t height = std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, m));
      VkImageLayout layout = currentLayout[l][m].Layout;
      VkAccessFlags access = currentLayout[l][m].Access;

      if (layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        while (target[l].size() <= m) {
          target[l].push_back({});
        }

        for (uint32_t a = 0; a < 3; ++a) {
          VkImageAspectFlagBits aspect = static_cast<VkImageAspectFlagBits>(1 << a);

          if (imageAspect & aspect) {
            target[l][m].push_back({});
            target[l][m].back().aspect = aspect;

            // Create buffer
            {
              VkBufferCreateInfo bufferCreateInfo = {
                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
                  nullptr,                              // const void* pNext;
                  0,                                    // VkBufferCreateFlags flags;
                  width * height *
                      getFormatBytesPerPixel(imageState->imageFormat), // VkDeviceSize size;
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,                    // VkBufferUsageFlags usage;
                  VK_SHARING_MODE_EXCLUSIVE,                           // sharingMode;
                  0,      // uint32_t queueFamilyIndexCount;
                  nullptr // const uint32_t* pQueueFamilyIndices;
              };

              drvVk.vkCreateBuffer(device, &bufferCreateInfo, nullptr, &target[l][m].back().buffer);
              if (VK_NULL_HANDLE == target[l][m].back().buffer) {
                throw EOperationFailed("Could not create a buffer to store screenshot data.");
              }
            }

            // Allocate buffer memory
            {
              VkMemoryRequirements memoryRequirements;
              drvVk.vkGetBufferMemoryRequirements(device, target[l][m].back().buffer,
                                                  &memoryRequirements);

              if (!memoryRequirements.size) {
                throw EOperationFailed(
                    "vkGetBufferMemoryRequirements() returned requirement with 0 size.");
              }

#ifndef BUILD_FOR_CCODE
              // To maintain stream compatibility, use memory properties of the platform the (original) stream was recorded on
              // If there are none, get memory properties of the current platform
              auto& physicalDeviceState = SD()._devicestates[device]->physicalDeviceStateStore;
              VkPhysicalDeviceMemoryProperties memoryProperties =
                  physicalDeviceState->memoryProperties;
              if (memoryProperties.memoryHeapCount == 0) {
                drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDeviceState->physicalDeviceHandle,
                                                          &memoryProperties);
              }
#else
              VkPhysicalDeviceMemoryProperties memoryProperties;
              drvVk.vkGetPhysicalDeviceMemoryProperties(
                  globalState.deviceStates[device].physicalDevice, &memoryProperties);
#endif
              uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
              for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) ==
                    requiredFlags) {
                  if (memoryRequirements.memoryTypeBits & (1 << i)) {
                    VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                                    memoryRequirements.size, i};

                    drvVk.vkAllocateMemory(device, &memInfo, nullptr, &target[l][m].back().memory);
                    drvVk.vkBindBufferMemory(device, target[l][m].back().buffer,
                                             target[l][m].back().memory, 0);
                    break;
                  }
                }
              }
              if (VK_NULL_HANDLE == target[l][m].back().memory) {
                throw EOperationFailed("Could not allocate memory for a buffer.");
              }
            }
            // Perform pre-copy image layout transition
            {
              VkImageMemoryBarrier preCopyImageBarrier = {
                  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                // const void* pNext;
                  access,                                 // VkAccessFlags srcAccessMask;
                  VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags dstAccessMask;
                  layout,                                 // VkImageLayout oldLayout;
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout newLayout;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
                  sourceImage,                            // VkImage image;
                  {
                      // VkImageSubresourceRange subresourceRange;
                      static_cast<VkImageAspectFlags>(
                          imageAspect), // VkImageAspectFlags aspectMask;
                      m,                // uint32_t baseMipLevel;
                      1,                // uint32_t levelCount;
                      l,                // uint32_t baseArrayLayer;
                      1                 // uint32_t layerCount;
                  }};
              VkBufferMemoryBarrier preCopyBufferBarrier = {
                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                 // const void* pNext;
                  0,                                       // VkAccessFlags srcAccessMask;
                  VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                  target[l][m].back().buffer,              // VkBuffer buffer;
                  0,                                       // VkDeviceSize offset;
                  VK_WHOLE_SIZE                            // VkDeviceSize size;
              };
              drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                                         &preCopyBufferBarrier, 1, &preCopyImageBarrier);
            }

            // Copy image data from swapchain to buffer
            {
              VkBufferImageCopy bufferImageCopyRegion = {
                  0, // VkDeviceSize bufferOffset;
                  0, // uint32_t bufferRowLength;
                  0, // uint32_t bufferImageHeight;
                  {
                      // VkImageSubresourceLayers imageSubresource;
                      static_cast<VkImageAspectFlags>(aspect), // VkImageAspectFlags aspectMask;
                      m,                                       // uint32_t mipLevel;
                      l,                                       // uint32_t baseArrayLayer;
                      1                                        // uint32_t layerCount;
                  },
                  {
                      // VkOffset3D imageOffset;
                      0, // int32_t x;
                      0, // int32_t y;
                      0  // int32_t z;
                  },
                  {
                      // VkExtent3D imageExtent;
                      width,  // uint32_t width;
                      height, // uint32_t height;
                      1       // uint32_t depth;
                  }};
              drvVk.vkCmdCopyImageToBuffer(commandBuffer, sourceImage,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           target[l][m].back().buffer, 1, &bufferImageCopyRegion);
            }

            // Perform post-copy image layout transition
            {
              VkImageMemoryBarrier postCopyImageBarrier = {
                  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                // const void* pNext;
                  VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags srcAccessMask;
                  access,                                 // VkAccessFlags dstAccessMask;
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout oldLayout;
                  layout,                                 // VkImageLayout newLayout;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
                  sourceImage,                            // VkImage image;
                  {
                      // VkImageSubresourceRange subresourceRange;
                      static_cast<VkImageAspectFlags>(
                          imageAspect), // VkImageAspectFlags aspectMask;
                      m,                // uint32_t baseMipLevel;
                      1,                // uint32_t levelCount;
                      l,                // uint32_t baseArrayLayer;
                      1                 // uint32_t layerCount;
                  }};
              VkBufferMemoryBarrier postCopyBufferBarrier = {
                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                  nullptr,                                 // const void* pNext;
                  VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask;
                  VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                  VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                  target[l][m].back().buffer,              // VkBuffer buffer;
                  0,                                       // VkDeviceSize offset;
                  VK_WHOLE_SIZE                            // VkDeviceSize size;
              };
              drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1,
                                         &postCopyBufferBarrier, 1, &postCopyImageBarrier);
            }
          }
        }
      }
    }
  }

  // End command buffer
  drvVk.vkEndCommandBuffer(commandBuffer);

  // Submit command buffer and wait for copy operation
  {
    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, // VkStructureType sType;
        nullptr,                       // const void* pNext;
        0,                             // uint32_t waitSemaphoreCount;
        nullptr,                       // const VkSemaphore* pWaitSemaphores;
        nullptr,                       // const VkPipelineStageFlags* pWaitDstStageMask;
        1,                             // uint32_t commandBufferCount;
        &commandBuffer,                // const VkCommandBuffer* pCommandBuffers;
        0,                             // uint32_t signalSemaphoreCount;
        nullptr                        // const VkSemaphore* pSignalSemaphores;
    };
    drvVk.vkDeviceWaitIdle(device);
    drvVk.vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    drvVk.vkDeviceWaitIdle(device);
  }
  bool dumped = false;
  // Map buffer and save data
  for (uint32_t l = 0; l < target.size(); ++l) {
    for (uint32_t m = 0; m < target[l].size(); ++m) {
      for (uint32_t a = 0; a < target[l][m].size(); ++a) {
        uint32_t width = std::max(1u, imageState->width / (uint32_t)std::pow<uint32_t>(2u, m));
        uint32_t height = std::max(1u, imageState->height / (uint32_t)std::pow<uint32_t>(2u, m));

        unsigned char* ptr;
        drvVk.vkMapMemory(device, target[l][m][a].memory, 0,
                          width * height * getFormatBytesPerPixel(imageState->imageFormat), 0,
                          (void**)&ptr);
        std::vector<uint8_t> screenshotData(
            ptr, ptr + (width * height * getFormatBytesPerPixel(imageState->imageFormat)));

        // Normalize ???
        /*
        if ((VK_IMAGE_ASPECT_COLOR_BIT & target[l][m][a].aspect) 
            && isFormatFloat(imageState.imageFormat)) {
          try {
            normalize_texture_data(getTexelToConvertFromImageFormat(imageState.imageFormat), screenshotData, width, height);
          } catch(...) {
            VkLog(WARN) << "Could not normalize texture data with format " << imageState.imageFormat << ".";
          }
        }
        */
        try {
          std::vector<uint8_t> convertedData(width * height * 4);
          bool depthInProperRange = true;
          if (target[l][m][a].aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
            convert_texture_data(getTexelToConvertFromImageFormat(imageState->imageFormat),
                                 screenshotData, texel_type::RGBA8, convertedData, width, height);
          } else if (target[l][m][a].aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
            auto fmt = imageState->imageFormat;
            std::pair<double, double> minMaxValues(0.0, 0.0);
            if (fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT) {
              minMaxValues = get_min_max_values(texel_type::R32f, screenshotData, width, height);
              normalize_texture_data(texel_type::R32f, screenshotData, width, height);
              convert_texture_data(texel_type::R32f, screenshotData, texel_type::RGBA8,
                                   convertedData, width, height);
            } else if (fmt == VK_FORMAT_D24_UNORM_S8_UINT || fmt == VK_FORMAT_X8_D24_UNORM_PACK32) {
              normalize_texture_data(texel_type::D24, screenshotData, width, height);
              convert_texture_data(texel_type::D24, screenshotData, texel_type::RGBA8,
                                   convertedData, width, height);
            } else if (fmt == VK_FORMAT_D16_UNORM || fmt == VK_FORMAT_D16_UNORM_S8_UINT) {
              normalize_texture_data(texel_type::R16, screenshotData, width, height);
              convert_texture_data(texel_type::R16, screenshotData, texel_type::RGBA8,
                                   convertedData, width, height);
            }
#ifndef BUILD_FOR_CCODE
            if ((minMaxValues.first < 0.0 || minMaxValues.second > 1.0) &&
                Config::Get().player.skipNonDeterministicImages &&
                !SD().depthRangeUnrestrictedEXTEnabled) {
              // When copying to a depth aspect, and the
              // VK_EXT_depth_range_unrestricted extension is not enabled, the
              // data in buffer memory (SFLOAT format) must be in the range
              // [0,1], or the resulting values are undefined.
              depthInProperRange = false;
            }
#endif
          } else if (target[l][m][a].aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
            normalize_texture_data(texel_type::R8, screenshotData, width, height);
            convert_texture_data(texel_type::R8, screenshotData, texel_type::RGBA8, convertedData,
                                 width, height);
          }
          {
            std::stringstream nameSuffix;
            if (numArrayLayers > 1) {
              nameSuffix << "_layer_" << std::setw(2) << std::setfill('0') << l;
            }
            if (numMipmapLevels > 1) {
              nameSuffix << "_mipmap_" << std::setw(2) << std::setfill('0') << m;
            }
            switch (target[l][m][a].aspect) {
            case VK_IMAGE_ASPECT_DEPTH_BIT:
              nameSuffix << "_depth";
              break;
            case VK_IMAGE_ASPECT_STENCIL_BIT:
              nameSuffix << "_stencil";
              break;
            default:
              Log(TRACE) << "Not handled VkImageAspectFlagBits enumeration: " +
                                std::to_string(target[l][m][a].aspect);
              break;
            }
            nameSuffix << ".png";
            if (depthInProperRange) {
              CGits::Instance().WriteImage(fileName + nameSuffix.str(), width, height, true,
                                           convertedData, false, false, false);
              dumped = true;
            }
          }
        } catch (...) {
          Log(ERR) << "Could not convert image data to RGBA8 format.";
        }

        drvVk.vkFreeMemory(device, target[l][m][a].memory, nullptr);
        drvVk.vkDestroyBuffer(device, target[l][m][a].buffer, nullptr);
      }
    }
  }
  return dumped;
}

namespace {

#if defined(GITS_PLATFORM_WINDOWS) && !defined(BUILD_FOR_CCODE)

void captureDesktopScreenshot(VkSurfaceKHR surface, const std::string& fileName) {
  std::string filename = fileName + ".png";
  HWND hWND = SD()._surfacekhrstates[surface]->surfaceCreateInfoWin32Data.Value()->hwnd;

  RECT rc;
  GetClientRect(hWND, &rc);
  int appWidth = rc.right - rc.left;
  int appHeight = rc.bottom - rc.top;

  SetWindowPos(hWND, HWND_TOP, 0, 0, appWidth, appHeight, SWP_SHOWWINDOW);

  HDC hDCdesktop = GetDC(NULL);
  if (hDCdesktop == NULL) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  HDC hDCdestination = CreateCompatibleDC(hDCdesktop);
  if (hDCdestination == NULL) {
    ReleaseDC(NULL, hDCdesktop);
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  HBITMAP hBMPdestination = CreateCompatibleBitmap(hDCdesktop, appWidth, appHeight);
  if (hBMPdestination == NULL) {
    ReleaseDC(NULL, hDCdesktop);
    DeleteDC(hDCdestination);
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  SelectObject(hDCdestination, hBMPdestination);

  BitBlt(hDCdestination, rc.left, rc.top, appWidth, appHeight, hDCdesktop, 0, 0, SRCCOPY);
  CImage img;
  img.Attach(hBMPdestination);

  img.Save(filename.c_str(), Gdiplus::ImageFormatPNG);

  ReleaseDC(NULL, hDCdesktop);
  DeleteDC(hDCdestination);
  DeleteObject(hBMPdestination);
}
#else
void captureDesktopScreenshot(VkSurfaceKHR surface, const std::string& fileName) {}
#endif

} // namespace

#ifndef BUILD_FOR_CCODE
void writeScreenshot(VkQueue queue,
                     VkCommandBuffer cmdbuffer,
                     uint32_t commandBufferBatchNumber,
                     uint32_t cmdBufferNumber) {
  if (SD()._commandbufferstates.find(cmdbuffer) != SD()._commandbufferstates.end()) {
    auto& commandBufferState = SD()._commandbufferstates[cmdbuffer];
    std::set<VkImage> dumpedImagesFromCmdBuff;

    for (uint32_t renderpass = 0; renderpass < commandBufferState->beginRenderPassesList.size();
         ++renderpass) {
      auto& framebufferState =
          commandBufferState->beginRenderPassesList[renderpass]->framebufferStateStore;
      uint32_t imageViewSize;
      if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                                VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
        imageViewSize = (uint32_t)framebufferState->imageViewStateStoreList.size();
      } else {
        imageViewSize = (uint32_t)commandBufferState->beginRenderPassesList[renderpass]
                            ->imageViewStateStoreListKHR.size();
      }
      for (uint32_t imageview = 0; imageview < imageViewSize; ++imageview) {
        std::string fileName;
        VkImage imageHandle;
        VkAttachmentStoreOp imageStoreOption =
            commandBufferState->beginRenderPassesList[renderpass]->imageStoreOp[imageview];
        if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                                  VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
          imageHandle =
              framebufferState->imageViewStateStoreList[imageview]->imageStateStore->imageHandle;
        } else {
          imageHandle = commandBufferState->beginRenderPassesList[renderpass]
                            ->imageViewStateStoreListKHR[imageview]
                            ->imageStateStore->imageHandle;
        }
        if (Config::Get().player.captureVulkanSubmitsGroupType ==
            TCaptureGroupType::PER_COMMANDBUFFER) {
          fileName = GetFileNameDrawcallScreenshot(
              CGits::Instance().CurrentFrame(),
              (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
              commandBufferBatchNumber, cmdBufferNumber, 0, SD().imageCounter[imageHandle], true);
          if (dumpedImagesFromCmdBuff.find(imageHandle) == dumpedImagesFromCmdBuff.end()) {
            bool dumped =
                writeScreenshotUtil(fileName, queue, imageHandle,
                                    VULKAN_MODE_RENDER_PASS_ATTACHMENTS, imageStoreOption);
            if (dumped) {
              dumpedImagesFromCmdBuff.insert(imageHandle);
            }
          }
        } else {
          fileName = GetFileNameDrawcallScreenshot(
              CGits::Instance().CurrentFrame(),
              (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
              commandBufferBatchNumber, cmdBufferNumber, renderpass, imageview, false);
          writeScreenshotUtil(fileName, queue, imageHandle, VULKAN_MODE_RENDER_PASS_ATTACHMENTS,
                              imageStoreOption);
        }
      }
    }
  }
}

void writeScreenshot(VkQueue queue, const VkPresentInfoKHR& presentInfo) {
  std::string fileName = GetFileNameFrameScreenshot(CGits::Instance().CurrentFrame());
  for (uint32_t i = 0; i < presentInfo.swapchainCount; ++i) {
    std::ostringstream nameSuffix;
    auto& swapchainState = SD()._swapchainkhrstates[presentInfo.pSwapchains[i]];

    if (presentInfo.swapchainCount > 1) {
      nameSuffix << "_swapchain_" << i;
    }

    if (Config::Get().player.captureScreenshot) {
      captureDesktopScreenshot(swapchainState->surfaceKHRStateStore->surfaceKHRHandle,
                               fileName + nameSuffix.str());
    } else {
      unsigned int imageIndex = presentInfo.pImageIndices[i];
      VkImage swapImg = swapchainState->imageStateStoreList[imageIndex]->imageHandle;

      writeScreenshotUtil(fileName + nameSuffix.str(), queue, swapImg);
    }
  }
}

void writeResources(VkQueue queue,
                    VkCommandBuffer cmdbuffer,
                    uint32_t commandBufferBatchNumber,
                    uint32_t cmdBufferNumber) {
  if (SD()._commandbufferstates.find(cmdbuffer) != SD()._commandbufferstates.end()) {
    auto& commandBufferState = SD()._commandbufferstates[cmdbuffer];

    for (auto obj : commandBufferState->resourceWriteBuffers) {
      VkBuffer swapBuffer = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(),
          (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
          commandBufferBatchNumber, cmdBufferNumber, SD().bufferCounter[swapBuffer], obj.second);
      writeBufferUtil(fileName, queue, swapBuffer);
    }
    for (auto obj : commandBufferState->resourceWriteImages) {
      VkImage swapImg = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(),
          (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
          commandBufferBatchNumber, cmdBufferNumber, SD().imageCounter[swapImg], obj.second);
      writeScreenshotUtil(fileName, queue, swapImg, VULKAN_MODE_RESOURCES);
    }
  }
}

void writeBufferUtil(std::string fileName, VkQueue& queue, VkBuffer& sourceBuffer) {
  auto& bufferState = SD()._bufferstates[sourceBuffer];
  if ((!bufferState->bufferCreateInfoData.Value()) ||
      (bufferState->bufferCreateInfoData.Value()->size == 0) || (!bufferState->binding)) {
    return;
  }
#ifndef BUILD_FOR_CCODE
  auto& queueState = SD()._queuestates[queue];
  VkDevice device = queueState->deviceStateStore->deviceHandle;
  uint32_t queueFamilyIndex = queueState->queueFamilyIndex;
#else
  auto& queueState = globalState.queueStates[queue];
  VkDevice device = queueState.device;
  uint32_t queueFamilyIndex = queueState.deviceQueueList.front().queueFamilyIndex;
#endif

  auto& queueFamilyCommandPoolMap = SD().internalResources.deviceResourcesMap[device];
  auto iterator = queueFamilyCommandPoolMap.find(queueFamilyIndex);
  if (iterator == queueFamilyCommandPoolMap.end()) {
    // Create command pool
    VkCommandPool commandPool;
    {
      VkCommandPoolCreateInfo commandPoolCreateInfo = {
          VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // VkStructureType sType;
          nullptr,                                    // const void* pNext;
          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // VkCommandPoolCreateFlags flags;
          queueFamilyIndex                                     // uint32_t queueFamilyIndex;
      };
      drvVk.vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

      if (VK_NULL_HANDLE == commandPool) {
        throw EOperationFailed(
            "Could not create command pool for screenshot data copying command buffer.");
      }
    }

    // Allocate command buffer
    VkCommandBuffer commandBuffer;
    {
      VkCommandBufferAllocateInfo allocateCommandBufferInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType sType;
          nullptr,                                        // const void* pNext;
          commandPool,                                    // VkCommandPool commandPool;
          VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel level;
          1                                               // uint32_t commandBufferCount;
      };
      drvVk.vkAllocateCommandBuffers(device, &allocateCommandBufferInfo, &commandBuffer);

      if (VK_NULL_HANDLE == commandPool) {
        throw EOperationFailed("Could not allocate screenshot data copying command buffer.");
      }
    }

    iterator =
        queueFamilyCommandPoolMap.insert({queueFamilyIndex, {commandPool, commandBuffer}}).first;
  }

  VkCommandBuffer commandBuffer = iterator->second.second;

  // Begin command buffer
  {
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType sType;
        nullptr,                                     // const void* pNext;
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags flags;
        nullptr // const VkCommandBufferInheritanceInfo* pInheritanceInfo;
    };
    drvVk.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  }

  VkBufferCreateInfo* targetBufferCreateInfo = bufferState->bufferCreateInfoData.Value();

  VkBufferCopy bufferCopy = {
      0,                           // VkDeviceSize   srcOffset
      0,                           // VkDeviceSize   dstOffset
      targetBufferCreateInfo->size // VkDeviceSize   size
  };
  VkBuffer localBuffer;
  VkDeviceMemory localMemory = VK_NULL_HANDLE;
  // Create local buffer
  {
    VkBufferCreateInfo localBufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
        nullptr,                              // const void* pNext;
        0,                                    // VkBufferCreateFlags flags;
        targetBufferCreateInfo->size,         // VkDeviceSize size;
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,     // VkBufferUsageFlags usage;
        VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode sharingMode;
        0,                                    // uint32_t queueFamilyIndexCount;
        nullptr                               // const uint32_t* pQueueFamilyIndices;
    };
    VkResult result = drvVk.vkCreateBuffer(device, &localBufferCreateInfo, nullptr, &localBuffer);
    if (result != VK_SUCCESS) {
      VkLog(ERR) << "Could not create buffer: " << result << "!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  VkMemoryRequirements memoryRequirements;
  // Allocate buffer memory
  {
    drvVk.vkGetBufferMemoryRequirements(device, localBuffer, &memoryRequirements);

    if (!memoryRequirements.size) {
      throw EOperationFailed("vkGetBufferMemoryRequirements() returned requirement with 0 size.");
    }
#ifndef BUILD_FOR_CCODE
    // To maintain stream compatibility, use memory properties of the platform
    // the (original) stream was recorded on If there are none, get memory
    // properties of the current platform
    auto& physicalDeviceState = SD()._devicestates[device]->physicalDeviceStateStore;
    VkPhysicalDeviceMemoryProperties memoryProperties = physicalDeviceState->memoryProperties;
    if (memoryProperties.memoryHeapCount == 0) {
      drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDeviceState->physicalDeviceHandle,
                                                &memoryProperties);
    }
#else
    VkPhysicalDeviceMemoryProperties memoryProperties;
    drvVk.vkGetPhysicalDeviceMemoryProperties(globalState.deviceStates[device].physicalDevice,
                                              &memoryProperties);
#endif
    uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
      if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
        if (memoryRequirements.memoryTypeBits & (1 << i)) {
          VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                          memoryRequirements.size, i};

          drvVk.vkAllocateMemory(device, &memInfo, nullptr, &localMemory);
          drvVk.vkBindBufferMemory(device, localBuffer, localMemory, 0);
          break;
        }
      }
    }
  }

#define ALL_VULKAN_BUFFER_ACCESS_BITS                                                              \
  VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |                                 \
      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |                           \
      VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |                            \
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |                           \
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |         \
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT |                 \
      VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT

  // Pre-transfer buffer memory barriers
  std::vector<VkBufferMemoryBarrier> dataAcquisitionBufferMemoryBarriersPre = {
      {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
          nullptr,                                 // const void* pNext
          VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
          localBuffer,                             // VkBuffer buffer
          0,                                       // VkDeviceSize offset
          VK_WHOLE_SIZE                            // VkDeviceSize size;
      },
      {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
          nullptr,                                 // const void* pNext
          ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
          sourceBuffer,                            // VkBuffer buffer
          0,                                       // VkDeviceSize offset
          VK_WHOLE_SIZE                            // VkDeviceSize size;
      }};

  // Post-transfer buffer memory barriers
  std::vector<VkBufferMemoryBarrier> dataAcquisitionBufferMemoryBarriersPost = {
      {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
          nullptr,                                 // const void* pNext
          VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask
          VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
          localBuffer,                             // VkBuffer buffer
          0,                                       // VkDeviceSize offset
          VK_WHOLE_SIZE                            // VkDeviceSize size;
      },
      {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
          nullptr,                                 // const void* pNext
          VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
          ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
          sourceBuffer,                            // VkBuffer buffer
          0,                                       // VkDeviceSize offset
          VK_WHOLE_SIZE                            // VkDeviceSize size;
      }};

  // Set barriers and acquire data
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, (uint32_t)dataAcquisitionBufferMemoryBarriersPre.size(),
                             dataAcquisitionBufferMemoryBarriersPre.data(), 0, nullptr);
  drvVk.vkCmdCopyBuffer(commandBuffer, sourceBuffer, localBuffer, 1, &bufferCopy);
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, (uint32_t)dataAcquisitionBufferMemoryBarriersPost.size(),
                             dataAcquisitionBufferMemoryBarriersPost.data(), 0, nullptr);
  drvVk.vkEndCommandBuffer(commandBuffer);

  // Submit
  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr};
  drvVk.vkDeviceWaitIdle(device);
  drvVk.vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  drvVk.vkDeviceWaitIdle(device);
  FILE* outFile = nullptr;
  std::vector<char> bufferData;
  bufferData.resize(targetBufferCreateInfo->size);
  if (localMemory == VK_NULL_HANDLE) {
    Log(ERR) << "Could not allocate memory for a buffer to dump.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  unsigned char* ptr;
  drvVk.vkMapMemory(device, localMemory, 0, memoryRequirements.size, 0, (void**)&ptr);
  memcpy(bufferData.data(), ptr, targetBufferCreateInfo->size);

  std::string outputFileNameBin = fileName;
  if (outputFileNameBin.empty()) {
    Log(ERR) << "Could not generate a name for the buffer to be dumped.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  outputFileNameBin.append(".bin");

  const char* binaryFileName = outputFileNameBin.c_str();
  if (binaryFileName == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  outFile = fopen(binaryFileName, "wb");
  if (outFile == nullptr) {
    Log(ERR) << "Could not open a file: " << outputFileNameBin;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  fwrite(&bufferData[0], sizeof(uint8_t), targetBufferCreateInfo->size, outFile);
  fclose(outFile);
  drvVk.vkUnmapMemory(device, localMemory);
  drvVk.vkFreeMemory(device, localMemory, nullptr);
  drvVk.vkDestroyBuffer(device, localBuffer, nullptr);
}
#endif

void writeCCodeScreenshot(
    VkQueue queue,
    const VkPresentInfoKHR& presentInfo,
    const std::function<VkImage(VkSwapchainKHR, unsigned int)>& getImageFromSwapchain) {
  std::string fileName = GetFileNameFrameScreenshot(CGits::Instance().CurrentFrame());
  for (uint32_t i = 0; i < presentInfo.swapchainCount; ++i) {
    std::ostringstream nameSuffix;
    if (presentInfo.swapchainCount > 1) {
      nameSuffix << "_swapchain_" << i;
    }

    unsigned int imageIndex = presentInfo.pImageIndices[i];
    VkSwapchainKHR swapchain = presentInfo.pSwapchains[i];
    VkImage image = getImageFromSwapchain(swapchain, imageIndex);

    writeScreenshotUtil(fileName + nameSuffix.str(), queue, image);
  }
}

texel_type getTexelToConvertFromImageFormat(VkFormat format) {
  switch (format) {
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SRGB:
    return texel_type::R8;
  case VK_FORMAT_R8_SNORM:
    return texel_type::R8snorm;
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_S8_UINT:
    return texel_type::R8ui;
  case VK_FORMAT_R8_SINT:
    return texel_type::R8i;
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_D16_UNORM:
    return texel_type::R16;
  case VK_FORMAT_R16_SNORM:
    return texel_type::R16snorm;
  case VK_FORMAT_R16_UINT:
    return texel_type::R16ui;
  case VK_FORMAT_R16_SINT:
    return texel_type::R16i;
  case VK_FORMAT_R16_SFLOAT:
    return texel_type::R16f;
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SRGB:
    return texel_type::RG8;
  case VK_FORMAT_R8G8_SNORM:
    return texel_type::RG8snorm;
  case VK_FORMAT_R8G8_UINT:
    return texel_type::RG8ui;
  case VK_FORMAT_R8G8_SINT:
    return texel_type::RG8i;
  case VK_FORMAT_R8G8B8_SINT:
    return texel_type::RGB8i;
  case VK_FORMAT_R8G8B8_SNORM:
    return texel_type::RGB8snorm;
  case VK_FORMAT_R8G8B8_UINT:
    return texel_type::RGB8ui;
  case VK_FORMAT_R8G8B8_SRGB:
  case VK_FORMAT_R8G8B8_UNORM:
    return texel_type::RGB8;
  case VK_FORMAT_B8G8R8_SINT:
    return texel_type::BGR8i;
  case VK_FORMAT_B8G8R8_SNORM:
    return texel_type::BGR8snorm;
  case VK_FORMAT_B8G8R8_UINT:
    return texel_type::BGR8ui;
  case VK_FORMAT_B8G8R8_UNORM:
  case VK_FORMAT_B8G8R8_SRGB:
    return texel_type::BGR8;
  case VK_FORMAT_B5G6R5_UNORM_PACK16:
    return texel_type::B5G6R5;
  case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    return texel_type::BGR5A1;
  case VK_FORMAT_R32_UINT:
    return texel_type::R32ui;
  case VK_FORMAT_R32_SINT:
    return texel_type::R32i;
  case VK_FORMAT_R32_SFLOAT:
  case VK_FORMAT_D32_SFLOAT:
    return texel_type::R32f;
  case VK_FORMAT_R16G16_UNORM:
    return texel_type::RG16;
  case VK_FORMAT_R16G16_SNORM:
    return texel_type::RG16snorm;
  case VK_FORMAT_R16G16_UINT:
    return texel_type::RG16ui;
  case VK_FORMAT_R16G16_SINT:
    return texel_type::RG16i;
  case VK_FORMAT_R16G16_SFLOAT:
    return texel_type::RG16f;
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SRGB:
    return texel_type::RGBA8;
  case VK_FORMAT_R8G8B8A8_SNORM:
    return texel_type::RGBA8snorm;
  case VK_FORMAT_R8G8B8A8_UINT:
    return texel_type::RGBA8ui;
  case VK_FORMAT_R8G8B8A8_SINT:
    return texel_type::RGBA8i;
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
    return texel_type::BGRA8;
  case VK_FORMAT_B8G8R8A8_SINT:
    return texel_type::BGRA8i;
  case VK_FORMAT_B8G8R8A8_SNORM:
    return texel_type::BGRA8snorm;
  case VK_FORMAT_B8G8R8A8_UINT:
    return texel_type::BGRA8ui;
  case VK_FORMAT_R32G32_UINT:
    return texel_type::RG32ui;
  case VK_FORMAT_R32G32_SINT:
    return texel_type::RG32i;
  case VK_FORMAT_R32G32_SFLOAT:
    return texel_type::RG32f;
  case VK_FORMAT_R16G16B16A16_UNORM:
    return texel_type::RGBA16;
  case VK_FORMAT_R16G16B16A16_SNORM:
    return texel_type::RGBA16snorm;
  case VK_FORMAT_R16G16B16A16_UINT:
    return texel_type::RGBA16ui;
  case VK_FORMAT_R16G16B16A16_SINT:
    return texel_type::RGBA16i;
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return texel_type::RGBA16f;
  case VK_FORMAT_R32G32B32A32_UINT:
    return texel_type::RGBA32ui;
  case VK_FORMAT_R32G32B32A32_SINT:
    return texel_type::RGBA32i;
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return texel_type::RGBA32f;
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    return texel_type::ABGR8;
  case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    return texel_type::ABGR8i;
  case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    return texel_type::ABGR8snorm;
  case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    return texel_type::ABGR8ui;
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    return texel_type::RGB10A2;
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    return texel_type::RG11B10f;
  case VK_FORMAT_X8_D24_UNORM_PACK32:
    return texel_type::X8D24;
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return texel_type::D32fS8ui;
    // All other formats are currently unsupported
  default:
    VkLog(WARN) << "The " << format << " format cannot be converted to GITS internal format.";
    throw ENotSupported(EXCEPTION_MESSAGE);
  }
}

void suppressPhysicalDeviceFeatures(std::vector<std::string> const& suppressFeatures,
                                    VkPhysicalDeviceFeatures* features) {
  if (!features) {
    return;
  }

  for (auto& suppress : suppressFeatures) {
#define SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(feature)                                         \
  if (suppress == #feature) {                                                                      \
    VkLog(WARN) << "Disabling physical device feature named: \"" #feature "\"!!!";                 \
    features->feature = 0;                                                                         \
    continue;                                                                                      \
  }

    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(robustBufferAccess)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(fullDrawIndexUint32)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(imageCubeArray)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(independentBlend)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(geometryShader)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(tessellationShader)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sampleRateShading)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(dualSrcBlend)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(logicOp)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(multiDrawIndirect)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(drawIndirectFirstInstance)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(depthClamp)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(depthBiasClamp)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(fillModeNonSolid)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(depthBounds)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(wideLines)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(largePoints)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(alphaToOne)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(multiViewport)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(samplerAnisotropy)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(textureCompressionETC2)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(textureCompressionASTC_LDR)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(textureCompressionBC)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(occlusionQueryPrecise)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(pipelineStatisticsQuery)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(vertexPipelineStoresAndAtomics)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(fragmentStoresAndAtomics)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderTessellationAndGeometryPointSize)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderImageGatherExtended)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageImageExtendedFormats)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageImageMultisample)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderClipDistance)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderCullDistance)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderFloat64)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderInt64)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderInt16)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderResourceResidency)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(shaderResourceMinLod)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseBinding)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidencyBuffer)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage2D)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage3D)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidency2Samples)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidency4Samples)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidency8Samples)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidency16Samples)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(sparseResidencyAliased)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(variableMultisampleRate)
    SUPPRESS_SELECTED_PHYSICAL_DEVICE_FEATURE(inheritedQueries)
  }
}

void suppressRequestedNames(std::vector<const char*>& requestedNames,
                            std::vector<std::string> const& suppressedNames,
                            uint32_t& updatedCount,
                            const char* const*& updatedPtr) {
  requestedNames.erase(std::remove_if(requestedNames.begin(), requestedNames.end(),
                                      [&suppressedNames](const char* name) {
                                        return std::find(suppressedNames.begin(),
                                                         suppressedNames.end(),
                                                         name) != suppressedNames.end();
                                      }),
                       requestedNames.end());

  updatedCount = static_cast<uint32_t>(requestedNames.size());
  updatedPtr = requestedNames.data();
}

bool checkForSupportForInstanceExtensions(uint32_t requestedExtensionsCount,
                                          char const* const* requestedExtensions) {
  if ((requestedExtensionsCount == 0) || (requestedExtensions == nullptr)) {
    return true;
  }

  uint32_t availableExtensionsCount = 0;
  if ((drvVk.vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr) !=
       VK_SUCCESS) ||
      (availableExtensionsCount == 0)) {
    return true;
  }

  std::vector<VkExtensionProperties> availableExtension(availableExtensionsCount);
  if (drvVk.vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount,
                                                   availableExtension.data()) != VK_SUCCESS) {
    return true;
  }

  std::ostringstream warningLog;
  for (uint32_t i = 0; i < requestedExtensionsCount; ++i) {
    if (std::find_if(availableExtension.begin(), availableExtension.end(),
                     [i, requestedExtensions](VkExtensionProperties& extensionProperty) {
                       if (strcmp(extensionProperty.extensionName, requestedExtensions[i]) == 0) {
                         return true;
                       }
                       return false;
                     }) == availableExtension.end()) {
      warningLog << "\n - " << requestedExtensions[i];
    }
  }

  if (warningLog.str().size() > 0) {
    Log(ERR) << "The following instance extensions are enabled but are not supported by the "
                "current hardware:"
             << warningLog.str();
    return false;
  }
  return true;
}

bool checkForSupportForInstanceLayers(uint32_t requestedLayersCount,
                                      char const* const* requestedLayers) {
  if ((requestedLayersCount == 0) || (requestedLayers == nullptr)) {
    return true;
  }

  uint32_t availableLayersCount = 0;
  if ((drvVk.vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr) != VK_SUCCESS) ||
      (availableLayersCount == 0)) {
    return true;
  }

  std::vector<VkLayerProperties> availableLayers(availableLayersCount);
  if (drvVk.vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data()) !=
      VK_SUCCESS) {
    return true;
  }

  std::ostringstream warningLog;
  for (uint32_t i = 0; i < requestedLayersCount; ++i) {
    if (std::find_if(availableLayers.begin(), availableLayers.end(),
                     [i, requestedLayers](VkLayerProperties& layerProperty) {
                       if (strcmp(layerProperty.layerName, requestedLayers[i]) == 0) {
                         return true;
                       }
                       return false;
                     }) == availableLayers.end()) {
      warningLog << "\n - " << requestedLayers[i];
    }
  }

  if (warningLog.str().size() > 0) {
    Log(ERR) << "The following instance layers are enabled but are not supported by the current "
                "hardware:"
             << warningLog.str();
    return false;
  }
  return true;
}

bool checkForSupportForPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                              VkPhysicalDeviceFeatures* enabledFeatures) {
  if (!enabledFeatures) {
    return true;
  }

  VkPhysicalDeviceFeatures availableFeatures;
  drvVk.vkGetPhysicalDeviceFeatures(physicalDevice, &availableFeatures);

  std::ostringstream warningLog;

#define COMPARE_PHYSICAL_DEVICE_FEATURE(feature)                                                   \
  if (!availableFeatures.feature && enabledFeatures->feature) {                                    \
    enabledFeatures->feature = VK_FALSE;                                                           \
    warningLog << "\n - " << #feature;                                                             \
  }

  COMPARE_PHYSICAL_DEVICE_FEATURE(robustBufferAccess)
  COMPARE_PHYSICAL_DEVICE_FEATURE(fullDrawIndexUint32)
  COMPARE_PHYSICAL_DEVICE_FEATURE(imageCubeArray)
  COMPARE_PHYSICAL_DEVICE_FEATURE(independentBlend)
  COMPARE_PHYSICAL_DEVICE_FEATURE(geometryShader)
  COMPARE_PHYSICAL_DEVICE_FEATURE(tessellationShader)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sampleRateShading)
  COMPARE_PHYSICAL_DEVICE_FEATURE(dualSrcBlend)
  COMPARE_PHYSICAL_DEVICE_FEATURE(logicOp)
  COMPARE_PHYSICAL_DEVICE_FEATURE(multiDrawIndirect)
  COMPARE_PHYSICAL_DEVICE_FEATURE(drawIndirectFirstInstance)
  COMPARE_PHYSICAL_DEVICE_FEATURE(depthClamp)
  COMPARE_PHYSICAL_DEVICE_FEATURE(depthBiasClamp)
  COMPARE_PHYSICAL_DEVICE_FEATURE(fillModeNonSolid)
  COMPARE_PHYSICAL_DEVICE_FEATURE(depthBounds)
  COMPARE_PHYSICAL_DEVICE_FEATURE(wideLines)
  COMPARE_PHYSICAL_DEVICE_FEATURE(largePoints)
  COMPARE_PHYSICAL_DEVICE_FEATURE(alphaToOne)
  COMPARE_PHYSICAL_DEVICE_FEATURE(multiViewport)
  COMPARE_PHYSICAL_DEVICE_FEATURE(samplerAnisotropy)
  COMPARE_PHYSICAL_DEVICE_FEATURE(textureCompressionETC2)
  COMPARE_PHYSICAL_DEVICE_FEATURE(textureCompressionASTC_LDR)
  COMPARE_PHYSICAL_DEVICE_FEATURE(textureCompressionBC)
  COMPARE_PHYSICAL_DEVICE_FEATURE(occlusionQueryPrecise)
  COMPARE_PHYSICAL_DEVICE_FEATURE(pipelineStatisticsQuery)
  COMPARE_PHYSICAL_DEVICE_FEATURE(vertexPipelineStoresAndAtomics)
  COMPARE_PHYSICAL_DEVICE_FEATURE(fragmentStoresAndAtomics)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderTessellationAndGeometryPointSize)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderImageGatherExtended)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageImageExtendedFormats)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageImageMultisample)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderClipDistance)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderCullDistance)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderFloat64)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderInt64)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderInt16)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderResourceResidency)
  COMPARE_PHYSICAL_DEVICE_FEATURE(shaderResourceMinLod)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseBinding)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidencyBuffer)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage2D)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidencyImage3D)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidency2Samples)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidency4Samples)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidency8Samples)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidency16Samples)
  COMPARE_PHYSICAL_DEVICE_FEATURE(sparseResidencyAliased)
  COMPARE_PHYSICAL_DEVICE_FEATURE(variableMultisampleRate)
  COMPARE_PHYSICAL_DEVICE_FEATURE(inheritedQueries)

  if (warningLog.str().size() > 0) {
    Log(ERR) << "The following features are enabled in the stream but are not supported by the "
                "current hardware:"
             << warningLog.str() << "\n";
    Log(ERR) << "The unsupported features will be disabled but remember this may lead to incorect "
                "results!";
    return false;
  }
  return true;
}

bool checkForSupportForQueues(VkPhysicalDevice physicalDevice,
                              uint32_t requestedQueueCreateInfoCount,
                              VkDeviceQueueCreateInfo const* requestedQueueCreateInfos) {
  uint32_t availableQueueFamiliesCount = 0;
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &availableQueueFamiliesCount,
                                                 nullptr);
  if (availableQueueFamiliesCount == 0) {
    throw std::runtime_error("No queue family available on the current platform. Potential driver "
                             "bug or driver installation issues. Exiting!");
  }

  std::vector<VkQueueFamilyProperties> availableQueueFamilies(availableQueueFamiliesCount);
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &availableQueueFamiliesCount,
                                                 availableQueueFamilies.data());

  for (uint32_t i = 0; i < requestedQueueCreateInfoCount; ++i) {
    uint32_t requestedQueueFamilyIndex = requestedQueueCreateInfos[i].queueFamilyIndex;
    uint32_t requestedQueueCountForFamily = requestedQueueCreateInfos[i].queueCount;

    if ((requestedQueueFamilyIndex > availableQueueFamiliesCount - 1) ||
        (requestedQueueCountForFamily >
         availableQueueFamilies[requestedQueueFamilyIndex].queueCount)) {
      Log(ERR) << "Stream requests queues that are not available on the current hardware!";
      Log(ERR) << "Requested queue family index: " << requestedQueueFamilyIndex << " with "
               << requestedQueueCountForFamily << " queue(s).";

      if (requestedQueueFamilyIndex > availableQueueFamiliesCount - 1) {
        Log(ERR) << "Current platform supports " << availableQueueFamiliesCount
                 << " queue family(ies).";
      } else if (requestedQueueCountForFamily >
                 availableQueueFamilies[requestedQueueFamilyIndex].queueCount) {
        Log(ERR) << "Current platform supports "
                 << availableQueueFamilies[requestedQueueFamilyIndex].queueCount
                 << " queue(s) for family index: " << requestedQueueFamilyIndex << ".";
      }
      return false;
    }
  }

  return true;
}

#ifndef BUILD_FOR_CCODE
bool areDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                  uint32_t requestedExtensionsCount,
                                  char const* const* requestedExtensions,
                                  bool printErrorLog) {
  if ((requestedExtensionsCount == 0) || (requestedExtensions == nullptr)) {
    return true;
  }

  auto& supportedExtensions = SD()._physicaldevicestates[physicalDevice]->supportedExtensions;

  bool allExtensionsSupported = true;

  std::ostringstream warningLog;
  for (uint32_t i = 0; i < requestedExtensionsCount; ++i) {
    bool extensionSupported = false;

    for (auto& extension : supportedExtensions) {
      if (extension == requestedExtensions[i]) {
        extensionSupported = true;
        break;
      }
    }

    if (!extensionSupported) {
      warningLog << "\n - " << requestedExtensions[i];
      allExtensionsSupported = false;
    }
  }

  if (printErrorLog && (warningLog.str().size() > 0)) {
    Log(ERR) << "The following device extensions are enabled but are not supported by the current "
                "hardware:"
             << warningLog.str();
  }
  return allExtensionsSupported;
}

bool areDeviceExtensionsEnabled(VkDevice device,
                                uint32_t requestedExtensionsCount,
                                char const* const* requestedExtensions) {
  if ((requestedExtensionsCount == 0) || (requestedExtensions == nullptr)) {
    return true;
  }

  auto& enabledExtensions = SD()._devicestates[device]->enabledExtensions;

  for (uint32_t i = 0; i < requestedExtensionsCount; ++i) {
    bool extensionEnabled = false;

    for (auto& extension : enabledExtensions) {
      if (extension == requestedExtensions[i]) {
        extensionEnabled = true;
        break;
      }
    }

    // Stop the loop and return false if any of the extensions is not enabled.
    if (!extensionEnabled) {
      return false;
    }
  }

  // Return true only when all extensions are enabled.
  return true;
}

void printShaderHashes(VkPipeline pipeline) {
  if ((Config::Get().player.traceVKShaderHashes) && (VK_NULL_HANDLE != pipeline)) {
    auto& pipelineState = SD()._pipelinestates[pipeline];

    VkLog(TRACE) << "Shader hashes:";
    for (auto& stage : pipelineState->stageShaderHashMapping) {
#ifdef GITS_PLATFORM_WINDOWS
      VkLog(TRACE) << "               " << stage.first << ": " << std::hex << stage.second;
#else
      VkLog(TRACE) << "               " << stage.first << ": " << stage.second;
#endif
    }
  }
}

void waitForAllDevices() {
  for (auto& device : SD()._devicestates) {
    drvVk.vkDeviceWaitIdle(device.first);
  }
}

// Destroy resources that are descendants of a specified device
// If device is VK_NULL_HANDLE, all resources should be destroyed
void destroyDeviceLevelResources(VkDevice device) {

#define DESTROY_VULKAN_OBJECTS(VULKAN_TYPE, CONTAINER, DESTROYING_FUNCTION)                        \
  {                                                                                                \
    std::vector<VULKAN_TYPE> objectsToRemove;                                                      \
    if (SD().CONTAINER.size() > 0) {                                                               \
      objectsToRemove.reserve(SD().CONTAINER.size());                                              \
    }                                                                                              \
    /* Destroy Vulkan handles */                                                                   \
    for (auto& vulkanObjectState : SD().CONTAINER) {                                               \
      if ((device == VK_NULL_HANDLE) ||                                                            \
          (device == vulkanObjectState.second->deviceStateStore->deviceHandle)) {                  \
        drvVk.DESTROYING_FUNCTION(vulkanObjectState.second->deviceStateStore->deviceHandle,        \
                                  vulkanObjectState.first, nullptr);                               \
        objectsToRemove.push_back(vulkanObjectState.first);                                        \
      }                                                                                            \
    }                                                                                              \
    /* Remove tracked objects */                                                                   \
    for (auto trackedObject : objectsToRemove) {                                                   \
      SD().CONTAINER.erase(trackedObject);                                                         \
    }                                                                                              \
  }

  if (Config::Get().IsPlayer()) {
    // If needed, get and store pipeline cache data
    for (auto& devicePipelineCachePair : SD().internalResources.pipelineCacheHandles) {
      if (((device == VK_NULL_HANDLE) || (device == devicePipelineCachePair.first)) &&
          (devicePipelineCachePair.second != VK_NULL_HANDLE)) {
        size_t cacheDataSize = 0;
        drvVk.vkGetPipelineCacheData(devicePipelineCachePair.first, devicePipelineCachePair.second,
                                     &cacheDataSize, nullptr);
        if (cacheDataSize > 0) {
          std::vector<char> cacheData(cacheDataSize);
          drvVk.vkGetPipelineCacheData(devicePipelineCachePair.first,
                                       devicePipelineCachePair.second, &cacheDataSize,
                                       cacheData.data());
          SaveBinaryFileContents(Config::Get().player.overrideVKPipelineCache.string(), cacheData);
        }
      }
    }
    // Due to driver problems, all swapchains need to be always deleted (no
    // matter if cleanResourcesOnExit option is used)
    DESTROY_VULKAN_OBJECTS(VkSwapchainKHR, _swapchainkhrstates, vkDestroySwapchainKHR)

    // Destroy all other resources (only when cleanResourcesOnExit option is used)
    if (Config::Get().player.cleanResourcesOnExit) {
      DESTROY_VULKAN_OBJECTS(VkQueryPool, _querypoolstates, vkDestroyQueryPool)
      DESTROY_VULKAN_OBJECTS(VkSemaphore, _semaphorestates, vkDestroySemaphore)
      DESTROY_VULKAN_OBJECTS(VkEvent, _eventstates, vkDestroyEvent)
      DESTROY_VULKAN_OBJECTS(VkFence, _fencestates, vkDestroyFence)
      DESTROY_VULKAN_OBJECTS(VkFramebuffer, _framebufferstates, vkDestroyFramebuffer)
      DESTROY_VULKAN_OBJECTS(VkPipeline, _pipelinestates, vkDestroyPipeline)
      DESTROY_VULKAN_OBJECTS(VkRenderPass, _renderpassstates, vkDestroyRenderPass)
      DESTROY_VULKAN_OBJECTS(VkShaderModule, _shadermodulestates, vkDestroyShaderModule)
      DESTROY_VULKAN_OBJECTS(VkPipelineCache, _pipelinecachestates, vkDestroyPipelineCache)
      DESTROY_VULKAN_OBJECTS(VkPipelineLayout, _pipelinelayoutstates, vkDestroyPipelineLayout)
      DESTROY_VULKAN_OBJECTS(VkDescriptorSetLayout, _descriptorsetlayoutstates,
                             vkDestroyDescriptorSetLayout)
      DESTROY_VULKAN_OBJECTS(VkBufferView, _bufferviewstates, vkDestroyBufferView)
      DESTROY_VULKAN_OBJECTS(VkBuffer, _bufferstates, vkDestroyBuffer)
      DESTROY_VULKAN_OBJECTS(VkImageView, _imageviewstates, vkDestroyImageView)
      DESTROY_VULKAN_OBJECTS(VkDeviceMemory, _devicememorystates, vkFreeMemory)
      DESTROY_VULKAN_OBJECTS(VkSampler, _samplerstates, vkDestroySampler)
      DESTROY_VULKAN_OBJECTS(VkDescriptorPool, _descriptorpoolstates, vkDestroyDescriptorPool)
      DESTROY_VULKAN_OBJECTS(VkCommandPool, _commandpoolstates, vkDestroyCommandPool)

      // Images (need to be treated separately because swapchain images cannot
      // be destroyed)
      {
        std::vector<VkImage> objectsToRemove;
        if (SD()._imagestates.size() > 0) {
          objectsToRemove.reserve(SD()._imagestates.size());
        }

        for (auto& vulkanObjectState : SD()._imagestates) {
          if ((device == VK_NULL_HANDLE) ||
              (device == vulkanObjectState.second->deviceStateStore->deviceHandle)) {
            if (vulkanObjectState.second->imageCreateInfoData.Value() != nullptr) {
              drvVk.vkDestroyImage(vulkanObjectState.second->deviceStateStore->deviceHandle,
                                   vulkanObjectState.first, nullptr);
              objectsToRemove.push_back(vulkanObjectState.first);
            }
          }
        }
        for (auto trackedObject : objectsToRemove) {
          SD()._imagestates.erase(trackedObject);
        }
      }
      // Descriptor update templates
      {
        std::vector<VkDescriptorUpdateTemplate> objectsToRemove;
        if (SD()._descriptorupdatetemplatestates.size() > 0) {
          objectsToRemove.reserve(SD()._descriptorupdatetemplatestates.size());
        }

        for (auto& vulkanObjectState : SD()._descriptorupdatetemplatestates) {
          if ((device == VK_NULL_HANDLE) ||
              (device == vulkanObjectState.second->deviceStateStore->deviceHandle)) {
            switch (vulkanObjectState.second->createdWith) {
            case CreationFunction::KHR_EXTENSION:
              drvVk.vkDestroyDescriptorUpdateTemplateKHR(
                  vulkanObjectState.second->deviceStateStore->deviceHandle, vulkanObjectState.first,
                  nullptr);
              break;
            default:
              drvVk.vkDestroyDescriptorUpdateTemplate(
                  vulkanObjectState.second->deviceStateStore->deviceHandle, vulkanObjectState.first,
                  nullptr);
              break;
            }
            objectsToRemove.push_back(vulkanObjectState.first);
          }
        }

        for (auto trackedObject : objectsToRemove) {
          SD()._descriptorupdatetemplatestates.erase(trackedObject);
        }
      }
      // GITS internal resources
      {
        // Descriptor pools
        {
          for (auto& deviceQueueFamilyMapPair : SD().internalResources.deviceResourcesMap) {
            auto deviceFromMap = deviceQueueFamilyMapPair.first;

            if ((device == VK_NULL_HANDLE) || (device == deviceFromMap)) {
              for (auto& queueFamilyCommandPoolPair : deviceQueueFamilyMapPair.second) {
                VkCommandPool commandPool = queueFamilyCommandPoolPair.second.first;
                drvVk.vkDestroyCommandPool(deviceFromMap, commandPool, nullptr);
              }
            }
          }
          if (device == VK_NULL_HANDLE) {
            SD().internalResources.deviceResourcesMap.clear();
          } else {
            SD().internalResources.deviceResourcesMap.erase(device);
          }
        }
        // Pipeline cache
        {
          for (auto& devicePipelineCachePair : SD().internalResources.pipelineCacheHandles) {
            if ((device == VK_NULL_HANDLE) || (device == devicePipelineCachePair.first)) {
              drvVk.vkDestroyPipelineCache(devicePipelineCachePair.first,
                                           devicePipelineCachePair.second, nullptr);
              devicePipelineCachePair.second = VK_NULL_HANDLE;
            }
          }
          if (device == VK_NULL_HANDLE) {
            SD().internalResources.pipelineCacheHandles.clear();
          } else {
            SD().internalResources.pipelineCacheHandles.erase(device);
          }
        }
      }
    } // if (Config::Get().player.cleanResourcesOnExit)

    // Devices
    {
      // If device is not specified, destroy all the devices
      // Otherwise, specific device is destroyed from vkDestroyDevice_WRAPRUN()
      // function
      if (device == VK_NULL_HANDLE) {
        for (auto& deviceState : SD()._devicestates) {
          drvVk.vkDestroyDevice(deviceState.first, nullptr);
        }
        SD()._devicestates.clear();
      }
    }
  }
}

void destroyInstanceLevelResources(VkInstance instance) {
  if (Config::Get().IsPlayer()) {
    if (Config::Get().player.cleanResourcesOnExit) {
      // Surfaces
      {
        std::vector<VkSurfaceKHR> objectsToRemove;
        for (auto& surfaceState : SD()._surfacekhrstates) {
          if ((instance == VK_NULL_HANDLE) ||
              (instance == surfaceState.second->instanceStateStore->instanceHandle)) {
            drvVk.vkDestroySurfaceKHR(surfaceState.second->instanceStateStore->instanceHandle,
                                      surfaceState.first, nullptr);
            objectsToRemove.push_back(surfaceState.first);
          }
        }
        /* Remove tracked objects */
        for (auto trackedObject : objectsToRemove) {
          SD()._surfacekhrstates.erase(trackedObject);
        }
      }
    }

    // Instances
    {
      // If instance is not specified, destroy all the instances
      // Otherwise, specific instance is destroyed from
      // vkDestroyInstance_WRAPRUN() function
      if (instance == VK_NULL_HANDLE) {
        for (auto& instanceState : SD()._instancestates) {
          drvVk.vkDestroyInstance(instanceState.first, nullptr);
        }
        SD()._instancestates.clear();
      }
    }
  }
}

void getRangesForMemoryUpdate(VkDeviceMemory memory,
                              std::vector<VkBufferCopy>& updatedRanges,
                              bool unmap) {
  auto& memoryState = SD()._devicememorystates[memory];

  auto& mapping = memoryState->mapping;
  std::uint64_t unmapSize = mapping->sizeData.Value();
  char* pointer = (char*)mapping->ppDataData.Value();

  // External memory option enabled
  if (Config::Get().recorder.vulkan.utilities.useExternalMemoryExtension) {
    auto touchedPages = ExternalMemoryRegion::GetTouchedPagesAndReset(
        (char*)memoryState->externalMemory + mapping->offsetData.Value(), unmapSize);

    for (auto page : touchedPages) {
      VkDeviceSize offset = (char*)page.first - (char*)pointer;
      VkDeviceSize size = page.second;

      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize) {
        std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
            GetChangedMemorySubranges(&mapping->compareData[offset], (char*)pointer + offset, size,
                                      Config::Get().recorder.vulkan.utilities.memorySegmentSize);

        for (auto& startEndPtrPair : optimizePagesMap) {
          std::uint64_t optimize_range_size = startEndPtrPair.second - startEndPtrPair.first;

          if (optimize_range_size > 0) {
            std::uint64_t optimize_offset =
                (std::uint64_t)((char*)startEndPtrPair.first - (char*)pointer);

            memcpy(&mapping->compareData[(size_t)optimize_offset], (char*)pointer + optimize_offset,
                   optimize_range_size);
            updatedRanges.push_back({
                optimize_offset,    // VkDeviceSize srcOffset;
                optimize_offset,    // VkDeviceSize dstOffset;
                optimize_range_size // VkDeviceSize size;
            });
          }
        }
      } else {
        updatedRanges.push_back({
            offset, // VkDeviceSize srcOffset;
            offset, // VkDeviceSize dstOffset;
            size    // VkDeviceSize size;
        });
      }
    }
  } else if (Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
    std::pair<const void*, size_t> baseRange = {(char*)pointer, (size_t)unmapSize};
    auto sniffedRegionHandle = mapping->sniffedRegionHandle;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> pagesMap =
        GetIntervalSetFromMemoryPages(baseRange, (**sniffedRegionHandle).GetTouchedPagesAndReset());

    if (!unmap) {
      if (!MemorySniffer::Get().Protect(sniffedRegionHandle)) {
        Log(WARN) << "Protecting memory region: " << (**sniffedRegionHandle).BeginAddress() << " - "
                  << (**sniffedRegionHandle).EndAddress() << " FAILED!.";
      }
    }

    for (auto& startEndPtrPair : pagesMap) {
      std::uint64_t range_size = startEndPtrPair.second - startEndPtrPair.first;

      if (range_size > 0) {
        std::uint64_t offset = (std::uint64_t)((char*)startEndPtrPair.first - (char*)pointer);

        if (Config::Get().recorder.vulkan.utilities.memorySegmentSize) {
          std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
              GetChangedMemorySubranges(&mapping->compareData[(size_t)offset],
                                        (char*)pointer + offset, range_size,
                                        Config::Get().recorder.vulkan.utilities.memorySegmentSize);

          for (auto& startEndPtrPair : optimizePagesMap) {
            std::uint64_t optimize_range_size = startEndPtrPair.second - startEndPtrPair.first;
            std::uint64_t optimize_offset =
                (std::uint64_t)((char*)startEndPtrPair.first - (char*)pointer);

            if (optimize_range_size > 0) {
              memcpy(&mapping->compareData[(size_t)optimize_offset],
                     (char*)pointer + optimize_offset, optimize_range_size);
              updatedRanges.push_back({
                  optimize_offset,    // VkDeviceSize srcOffset;
                  optimize_offset,    // VkDeviceSize dstOffset;
                  optimize_range_size // VkDeviceSize size;
              });
            }
          }
        } else {
          updatedRanges.push_back({
              offset,    // VkDeviceSize srcOffset;
              offset,    // VkDeviceSize dstOffset;
              range_size // VkDeviceSize size;
          });
        }
      }
    }
  } else if (Config::Get().recorder.vulkan.utilities.memorySegmentSize) {
    std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
        GetChangedMemorySubranges(&mapping->compareData[0], (char*)pointer, unmapSize,
                                  Config::Get().recorder.vulkan.utilities.memorySegmentSize);

    for (auto& startEndPtrPair : optimizePagesMap) {
      std::uint64_t optimize_range_size = startEndPtrPair.second - startEndPtrPair.first;

      if (optimize_range_size > 0) {
        std::uint64_t optimize_offset =
            (std::uint64_t)((char*)startEndPtrPair.first - (char*)pointer);

        memcpy(&mapping->compareData[(size_t)optimize_offset], (char*)pointer + optimize_offset,
               optimize_range_size);
        updatedRanges.push_back({
            optimize_offset,    // VkDeviceSize srcOffset;
            optimize_offset,    // VkDeviceSize dstOffset;
            optimize_range_size // VkDeviceSize size;
        });
      }
    }
  } else {
    updatedRanges.push_back({
        0,        // VkDeviceSize srcOffset;
        0,        // VkDeviceSize dstOffset;
        unmapSize // VkDeviceSize size;
    });
  }
}

void flushShadowMemory(VkDeviceMemory memory, bool unmap) {
  auto& memoryState = SD()._devicememorystates[memory];

  std::uint64_t unmapSize = memoryState->mapping->sizeData.Value();
  void* pointer = (char*)memoryState->mapping->ppDataData.Value();

  std::uint64_t offset = memoryState->mapping->offsetData.Value();

  if (Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
    std::pair<const void*, size_t> baseRange;
    baseRange.first = (char*)pointer;
    baseRange.second = (size_t)unmapSize;
    auto subRange = GetSubrangeOverlappingMemoryPages(
        baseRange, (**memoryState->mapping->sniffedRegionHandle).GetTouchedPagesAndReset());

    if (!unmap) {
      if (!MemorySniffer::Get().Protect(memoryState->mapping->sniffedRegionHandle)) {
        Log(WARN) << "Protecting memory region: "
                  << (**memoryState->mapping->sniffedRegionHandle).BeginAddress() << " - "
                  << (**memoryState->mapping->sniffedRegionHandle).EndAddress() << " FAILED!.";
      }
    }

    offset += (std::uint64_t)subRange.first - (std::uint64_t)pointer;
    unmapSize = subRange.second;

    if (unmapSize == 0) {
      return;
    }
  }
  memoryState->shadowMemory->Flush((size_t)offset, (size_t)unmapSize);
}

std::set<uint64_t> getRelatedPointers(std::set<uint64_t>& originalSet) {
  std::set<uint64_t> relatedPointers;
  for (uint64_t obj : originalSet) {
    if (SD()._instancestates.find((VkInstance)obj) != SD()._instancestates.end()) {
      for (auto elem : SD()._instancestates[(VkInstance)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._physicaldevicestates.find((VkPhysicalDevice)obj) !=
               SD()._physicaldevicestates.end()) {
      for (auto elem : SD()._physicaldevicestates[(VkPhysicalDevice)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._surfacekhrstates.find((VkSurfaceKHR)obj) != SD()._surfacekhrstates.end()) {
      for (auto elem : SD()._surfacekhrstates[(VkSurfaceKHR)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._devicestates.find((VkDevice)obj) != SD()._devicestates.end()) {
      for (auto elem : SD()._devicestates[(VkDevice)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._swapchainkhrstates.find((VkSwapchainKHR)obj) !=
               SD()._swapchainkhrstates.end()) {
      for (auto elem : SD()._swapchainkhrstates[(VkSwapchainKHR)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorpoolstates.find((VkDescriptorPool)obj) !=
               SD()._descriptorpoolstates.end()) {
      for (auto elem : SD()._descriptorpoolstates[(VkDescriptorPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._commandpoolstates.find((VkCommandPool)obj) != SD()._commandpoolstates.end()) {
      for (auto elem : SD()._commandpoolstates[(VkCommandPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._samplerstates.find((VkSampler)obj) != SD()._samplerstates.end()) {
      for (auto elem : SD()._samplerstates[(VkSampler)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._devicememorystates.find((VkDeviceMemory)obj) !=
               SD()._devicememorystates.end()) {
      for (auto elem : SD()._devicememorystates[(VkDeviceMemory)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._imagestates.find((VkImage)obj) != SD()._imagestates.end()) {
      for (auto elem : SD()._imagestates[(VkImage)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._imageviewstates.find((VkImageView)obj) != SD()._imageviewstates.end()) {
      for (auto elem : SD()._imageviewstates[(VkImageView)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._bufferstates.find((VkBuffer)obj) != SD()._bufferstates.end()) {
      for (auto elem : SD()._bufferstates[(VkBuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._bufferviewstates.find((VkBufferView)obj) != SD()._bufferviewstates.end()) {
      for (auto elem : SD()._bufferviewstates[(VkBufferView)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorsetlayoutstates.find((VkDescriptorSetLayout)obj) !=
               SD()._descriptorsetlayoutstates.end()) {
      for (auto elem :
           SD()._descriptorsetlayoutstates[(VkDescriptorSetLayout)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorsetstates.find((VkDescriptorSet)obj) !=
               SD()._descriptorsetstates.end()) {
      for (auto elem : SD()._descriptorsetstates[(VkDescriptorSet)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinelayoutstates.find((VkPipelineLayout)obj) !=
               SD()._pipelinelayoutstates.end()) {
      for (auto elem : SD()._pipelinelayoutstates[(VkPipelineLayout)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorupdatetemplatestates.find((VkDescriptorUpdateTemplate)obj) !=
               SD()._descriptorupdatetemplatestates.end()) {
      for (auto elem : SD()._descriptorupdatetemplatestates[(VkDescriptorUpdateTemplate)obj]
                           ->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinecachestates.find((VkPipelineCache)obj) !=
               SD()._pipelinecachestates.end()) {
      for (auto elem : SD()._pipelinecachestates[(VkPipelineCache)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._shadermodulestates.find((VkShaderModule)obj) !=
               SD()._shadermodulestates.end()) {
      for (auto elem : SD()._shadermodulestates[(VkShaderModule)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._renderpassstates.find((VkRenderPass)obj) != SD()._renderpassstates.end()) {
      for (auto elem : SD()._renderpassstates[(VkRenderPass)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinestates.find((VkPipeline)obj) != SD()._pipelinestates.end()) {
      for (auto elem : SD()._pipelinestates[(VkPipeline)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._framebufferstates.find((VkFramebuffer)obj) != SD()._framebufferstates.end()) {
      for (auto elem : SD()._framebufferstates[(VkFramebuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._fencestates.find((VkFence)obj) != SD()._fencestates.end()) {
      for (auto elem : SD()._fencestates[(VkFence)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._eventstates.find((VkEvent)obj) != SD()._eventstates.end()) {
      for (auto elem : SD()._eventstates[(VkEvent)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._semaphorestates.find((VkSemaphore)obj) != SD()._semaphorestates.end()) {
      for (auto elem : SD()._semaphorestates[(VkSemaphore)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._querypoolstates.find((VkQueryPool)obj) != SD()._querypoolstates.end()) {
      for (auto elem : SD()._querypoolstates[(VkQueryPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._commandbufferstates.find((VkCommandBuffer)obj) !=
               SD()._commandbufferstates.end()) {
      for (auto elem : SD()._commandbufferstates[(VkCommandBuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    }
  }
  return relatedPointers;
}

std::set<uint64_t> getPointersUsedInQueueSubmit(CVkSubmitInfoDataArray& submitInfoData) {
  std::set<uint64_t> pointers;
  auto submitInfoDataValues = submitInfoData.Value();
  if (submitInfoDataValues != nullptr) {
    for (uint32_t i = 0; i < submitInfoData.size(); i++) {
      for (uint32_t j = 0; j < submitInfoDataValues[i].commandBufferCount; j++) {
        auto& commandBufferState =
            SD()._commandbufferstates[submitInfoDataValues[i].pCommandBuffers[j]];
        for (auto obj : commandBufferState->tokensBuffer.GetMappedPointers()) {
          pointers.insert((uint64_t)obj);
        }

        for (auto& secondaryCommandBufferState :
             commandBufferState->secondaryCommandBuffersStateStoreList) {
          for (auto elem : secondaryCommandBufferState.second->tokensBuffer.GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
      }
    }

    auto temporaryQueueSubmitToken = std::make_shared<CvkQueueSubmit>(
        VK_SUCCESS, SD().lastQueueSubmit->queueStateStore->queueHandle,
        (uint32_t)submitInfoData.size(), submitInfoDataValues, SD().lastQueueSubmit->fenceHandle);
    for (auto obj : temporaryQueueSubmitToken->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    std::set<uint64_t> relatedPointers = getRelatedPointers(pointers);
    std::set<uint64_t> relatedPointers2 = getRelatedPointers(relatedPointers);
    std::set<uint64_t> relatedPointers3 = getRelatedPointers(relatedPointers2);
    std::set<uint64_t> relatedPointers4 = getRelatedPointers(relatedPointers3);
    pointers.insert(relatedPointers.begin(), relatedPointers.end());
    pointers.insert(relatedPointers2.begin(), relatedPointers2.end());
    pointers.insert(relatedPointers3.begin(), relatedPointers3.end());
    pointers.insert(relatedPointers4.begin(), relatedPointers4.end());
  }

  return pointers;
}

CVkSubmitInfoDataArray getSubmitInfoForPrepare(const std::vector<uint32_t>& countersTable,
                                               const BitRange& objRange,
                                               Config::VulkanObjectMode objMode) {
  CVkSubmitInfoDataArray submitInfoDataVector;
  if ((Config::MODE_VKCOMMANDBUFFER == objMode) &&
      (countersTable.back() < SD().lastQueueSubmit->submitInfoDataArray.size())) {
    auto submitInfoDataArrayValues = SD().lastQueueSubmit->submitInfoDataArray.Value();
    if (submitInfoDataArrayValues != nullptr) {
      for (uint32_t i = 0; i < countersTable.back(); i++) {
        submitInfoDataVector.AddElem(&submitInfoDataArrayValues[i]);
      }
      VkSubmitInfo submitInfoTemp = submitInfoDataArrayValues[countersTable.back()];
      std::vector<VkCommandBuffer> commandBufferVector;

      for (uint32_t i = 0; (i < submitInfoTemp.commandBufferCount) && !objRange[i]; i++) {
        commandBufferVector.push_back(submitInfoTemp.pCommandBuffers[i]);
      }

      submitInfoTemp.commandBufferCount = (uint32_t)commandBufferVector.size();
      if (commandBufferVector.size() > 0) {
        submitInfoTemp.pCommandBuffers = &commandBufferVector[0];
        submitInfoDataVector.AddElem(&submitInfoTemp);
      }
    }
  }
  return submitInfoDataVector;
}

CVkSubmitInfoDataArray getSubmitInfoForSchedule(const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                Config::VulkanObjectMode objMode) {
  CVkSubmitInfoDataArray submitInfoDataVector;
  auto submitInfoDataArrayValues = SD().lastQueueSubmit->submitInfoDataArray.Value();
  if (Config::MODE_VKQUEUESUBMIT == objMode && submitInfoDataArrayValues != nullptr) {
    for (uint32_t i = 0; i < SD().lastQueueSubmit->submitCount; i++) {
      submitInfoDataVector.AddElem(&submitInfoDataArrayValues[i]);
    }
  } else if ((Config::MODE_VKCOMMANDBUFFER == objMode) &&
             (countersTable.back() < SD().lastQueueSubmit->submitInfoDataArray.size()) &&
             submitInfoDataArrayValues != nullptr) {
    VkSubmitInfo submitInfoTemp = submitInfoDataArrayValues[countersTable.back()];
    std::vector<VkCommandBuffer> commandBufferVector;
    for (uint32_t i = 0; i < submitInfoTemp.commandBufferCount; i++) {
      if (objRange[i]) {
        commandBufferVector.push_back(submitInfoTemp.pCommandBuffers[i]);
      }
    }
    submitInfoTemp.commandBufferCount = (uint32_t)commandBufferVector.size();
    submitInfoTemp.pCommandBuffers = &commandBufferVector[0];
    submitInfoDataVector.AddElem(&submitInfoTemp);
  }
  return submitInfoDataVector;
}

namespace {
std::vector<bool>& IsMemoryMappable(VkDevice device) {
  // Prepare mappable memory data
  static std::vector<bool> isMemoryMappable = [&]() {
    auto& physicalDeviceState = SD()._devicestates[device]->physicalDeviceStateStore;

    VkPhysicalDeviceMemoryProperties originalPlatformMemoryProperties =
        physicalDeviceState->memoryProperties;
    VkPhysicalDeviceMemoryProperties currentPlatformMemoryProperties;
    drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDeviceState->physicalDeviceHandle,
                                              &currentPlatformMemoryProperties);

    std::vector<bool> _mappableMemory(std::max(originalPlatformMemoryProperties.memoryTypeCount,
                                               currentPlatformMemoryProperties.memoryTypeCount),
                                      false);

    // Check which memory type is mappable on a current platform
    for (uint32_t i = 0; i < currentPlatformMemoryProperties.memoryTypeCount; ++i) {
      _mappableMemory[i] =
          ((currentPlatformMemoryProperties.memoryTypes[i].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    // If the same memory type was not mappable on the platform the stream was
    // recorded on, there is no point in mapping it on a current platform.
    // COMPATIBILITY NOTE: if the stream does not contain memory properties of
    // the original platform, these properties are cleared (count is set to 0),
    // so the below check is omitted and only current platform properties are
    // used.
    for (uint32_t i = 0; i < originalPlatformMemoryProperties.memoryTypeCount; ++i) {
      _mappableMemory[i] =
          _mappableMemory[i] &
          ((originalPlatformMemoryProperties.memoryTypes[i].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    return _mappableMemory;
  }();

  return isMemoryMappable;
}
} // namespace

bool checkMemoryMappingFeasibility(VkDevice device, VkDeviceMemory memory, bool throwException) {
  if (!IsMemoryMappable(device)
          [SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->memoryTypeIndex]) {
    if (throwException) {
      Log(ERR) << "Stream tries to map memory object " << memory
               << " which was allocated from a non-host-visible memory type at index "
               << SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->memoryTypeIndex;
      if (!Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
        throw std::runtime_error("Memory object cannot be mapped on a current platform. Exiting!!");
      }
    }
    return false;
  } else {
    return true;
  }
}

bool checkMemoryMappingFeasibility(VkDevice device, uint32_t memoryTypeIndex, bool throwException) {
  if (!IsMemoryMappable(device)[memoryTypeIndex]) {
    if (throwException) {
      Log(ERR) << "Stream tries to map memory object which was allocated from a non-host-visible "
                  "memory type at index "
               << memoryTypeIndex;
      if (!Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
        throw std::runtime_error("Memory object cannot be mapped on a current platform. Exiting!!");
      }
    }
    return false;
  } else {
    return true;
  }
}

uint32_t findCompatibleMemoryTypeIndex(VkPhysicalDevice physicalDevice,
                                       uint32_t originalMemoryTypeIndex,
                                       uint32_t currentCompatibleMemoryTypes) {
  static std::unordered_map<VkPhysicalDevice, VkPhysicalDeviceMemoryProperties>
      currentPlatformMemoryPropertiesMap;
  auto it = currentPlatformMemoryPropertiesMap.find(physicalDevice);

  if (it == currentPlatformMemoryPropertiesMap.end()) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    it = currentPlatformMemoryPropertiesMap.insert({physicalDevice, memoryProperties}).first;
  }

  auto& originalPlatformProperties = SD()._physicaldevicestates[physicalDevice]->memoryProperties;
  auto& currentPlatformProperties = it->second;

  auto originalFlags =
      originalPlatformProperties.memoryTypes[originalMemoryTypeIndex].propertyFlags;

  for (uint32_t i = 0; i < currentPlatformProperties.memoryTypeCount; ++i) {
    if ((currentCompatibleMemoryTypes & (1 << i)) &&
        (currentPlatformProperties.memoryTypes[i].propertyFlags & originalFlags)) {
      return i;
    }
  }

  throw std::runtime_error("Cannot find a compatbile memory type for a resource. Exiting!");
}

std::shared_ptr<CBufferState> findBufferStateFromDeviceAddress(VkDeviceAddress deviceAddress) {
  auto isInRange =
      [&deviceAddress](
          std::pair<VkDeviceAddress, std::pair<VkDeviceAddress, std::shared_ptr<CBufferState>>>
              element) {
        return (deviceAddress >= element.first) && (deviceAddress < element.second.first);
      };

  auto it = std::find_if(CBufferState::deviceAddressesMap.begin(),
                         CBufferState::deviceAddressesMap.end(), isInRange);

  if (it == CBufferState::deviceAddressesMap.end()) {
    throw std::runtime_error("Trying to find a buffer from an unknown address space. Exiting!");
  } else {
    return it->second.second;
  }
}

bool isVulkanAPIVersionSupported(uint32_t major, uint32_t minor, VkPhysicalDevice physicalDevice) {
  auto instanceState = SD()._physicaldevicestates[physicalDevice]->instanceStateStore;
  return (instanceState->vulkanVersionMajor > major) ||
         ((instanceState->vulkanVersionMajor == major) &&
          (instanceState->vulkanVersionMinor >= minor));
}

void checkReturnValue(VkResult playerSideReturnValue,
                      CVkResult& recorderSideReturnValue,
                      const char* functionName) {
  if ((playerSideReturnValue < 0) && (playerSideReturnValue != *recorderSideReturnValue)) {
    VkLog(ERR) << functionName << " returned \"" << playerSideReturnValue
               << "\" error which is different than the \"" << *recorderSideReturnValue
               << "\" value returned during recording. Exiting!";
    throw std::runtime_error("Return value mismatch error occurred.");
  }
}

bool isEndRenderPassToken(unsigned vulkanID) {
  if (vulkanID == CFunction::ID_VK_CMD_END_RENDER_PASS ||
      vulkanID == CFunction::ID_VK_CMD_END_RENDER_PASS2 ||
      vulkanID == CFunction::ID_VK_CMD_END_RENDER_PASS2KHR ||
      vulkanID == CFunction::ID_VK_CMD_END_RENDERING) {
    return true;
  } else {
    return false;
  }
}

// Code borrowed from development team's Piotr Horodecki
bool MemoryAliasingTracker::Range::operator()(Range const& lRange, Range const& rRange) const {
  return lRange.offset < rRange.offset;
}

MemoryAliasingTracker::RangeSetType::iterator MemoryAliasingTracker::GetRange(uint64_t offset) {
  // Get an iterator to the first offset greater than given offset
  auto iterator = MemoryRanges.upper_bound({offset, 0, {}});
  // Decrement to obtain an iterator to the range which includes given offset
  iterator--;
  return iterator;
}

void MemoryAliasingTracker::SplitRange(uint64_t offset) {
  auto existingRangeIterator = GetRange(offset);
  if (existingRangeIterator->offset < offset) {
    // Create two new ranges: left range and right range by cutting existing range at offset
    Range leftRange = {existingRangeIterator->offset, offset - existingRangeIterator->offset,
                       existingRangeIterator->resources};
    Range rightRange = {offset, existingRangeIterator->size - leftRange.size, leftRange.resources};
    assert(leftRange.size > 0);
    assert(rightRange.size > 0);
    // Remove existing range
    MemoryRanges.erase(existingRangeIterator);
    // Insert two new ranges
    MemoryRanges.insert(leftRange);
    MemoryRanges.insert(rightRange);
  }
}

void MemoryAliasingTracker::AddResource(uint64_t offset,
                                        uint64_t size,
                                        std::pair<uint64_t, bool> const& resource) {
  // Split range if needed on the resource beginning.
  SplitRange(offset);

  // Split range if needed on the resource end.
  SplitRange(offset + size);

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.insert(resource);
  }
}

void MemoryAliasingTracker::RemoveResource(uint64_t offset,
                                           uint64_t size,
                                           std::pair<uint64_t, bool> const& resource) {
  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.erase(resource);
  }

  for (auto current = beginIterator; current->offset < endIterator->offset;) {
    auto next = current;
    next++;

    if ((next->offset < endIterator->offset) && (current->resources == next->resources)) {
      Range newRange = {current->offset, current->size + next->size, current->resources};
      MemoryRanges.erase(current);
      MemoryRanges.erase(next);
      MemoryRanges.insert(newRange);

      beginIterator = GetRange(offset);
      endIterator = GetRange(offset + size);
      current = beginIterator;
    } else {
      current++;
    }
  }
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForResource(
    uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource) {
  std::set<std::pair<uint64_t, bool>> aliasedResources;

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  for (auto i = beginIterator; i->offset < endIterator->offset; ++i) {
    aliasedResources.insert(i->resources.begin(), i->resources.end());
  }

  aliasedResources.erase(resource);

  return aliasedResources;
}

MemoryAliasingTracker::MemoryAliasingTracker(uint64_t size) {
  // Add whole device memory range.
  MemoryRanges.insert({0, size, {}});
  // Add "after whole device memory empty range" (fake) with offset == size of
  // vkDeviceMemory and size == 0. This allows to use SplitRange() method in
  // case of splitting range at the end of adding resource.
  MemoryRanges.insert({size, 0, {}});
}

void MemoryAliasingTracker::AddImage(uint64_t offset, uint64_t size, VkImage image) {
  AddResource(offset, size, {(uint64_t)image, true});
}

void MemoryAliasingTracker::AddBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  AddResource(offset, size, {(uint64_t)buffer, false});
}

void MemoryAliasingTracker::RemoveImage(uint64_t offset, uint64_t size, VkImage image) {
  RemoveResource(offset, size, {(uint64_t)image, true});
}

void MemoryAliasingTracker::RemoveBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  RemoveResource(offset, size, {(uint64_t)buffer, false});
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForImage(
    uint64_t offset, uint64_t size, VkImage image) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)image, true});
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForBuffer(
    uint64_t offset, uint64_t size, VkBuffer buffer) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)buffer, false});
}

#endif
} // namespace Vulkan
} // namespace gits
