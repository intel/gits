// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gits.h"
#include "pragmas.h"
#include "vulkanLog.h"
#include "vulkanTools_lite.h"

#ifndef BUILD_FOR_CCODE
#include "vulkanFunctions.h"
#include "vulkanStateTracking.h"
#else
#include "vulkanTools.h"
#include "helperVk.h"
#endif

#if defined(GITS_PLATFORM_WINDOWS) && !defined(BUILD_FOR_CCODE)

#include <atlimage.h>

#endif

namespace gits {
namespace Vulkan {

std::string GetFileNameFrameScreenshot(unsigned int frameNumber) {
  auto path = Config::Get().common.player.outputDir;
  if (path.empty()) {
    if (Config::Get().IsRecorder()) {
      path = Config::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path = Config::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
    } else {
      Log(ERR) << "Neither in player nor recorder!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  std::stringstream fileName;
  fileName << "frame" << std::setw(8) << std::setfill('0') << frameNumber;
  std::filesystem::create_directories(path);
  path /= fileName.str();
  return path.string();
}

std::string GetFileNameDrawcallScreenshot(unsigned int frameNumber,
                                          unsigned int submitNumber,
                                          unsigned int cmdBufferBatchNumber,
                                          unsigned int cmdBufferNumber,
                                          unsigned int renderpass,
                                          unsigned int drawNumber,
                                          uint64_t image,
                                          VulkanDumpingMode dumpingMode) {
  auto path = Config::Get().common.player.outputDir;
  if (path.empty()) {
    if (Config::Get().IsRecorder()) {
      path = Config::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path = Config::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
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
  if (dumpingMode == VulkanDumpingMode::VULKAN_PER_RENDER_PASS ||
      dumpingMode == VulkanDumpingMode::VULKAN_PER_DRAW) {
    fileName << "_renderpass_" << renderpass;
  }
  if (dumpingMode == VulkanDumpingMode::VULKAN_PER_DRAW) {
    fileName << "_draw_" << drawNumber;
  }
  fileName << "_image_" << image;
  std::filesystem::create_directories(path);
  path /= fileName.str();
  return path.string();
}

#ifndef BUILD_FOR_CCODE
std::string GetFileNameResourcesScreenshot(unsigned int frameNumber,
                                           unsigned int submitNumber,
                                           unsigned int cmdBufferBatchNumber,
                                           unsigned int cmdBufferNumber,
                                           unsigned int renderpassNumber,
                                           unsigned int drawNumber,
                                           uint64_t objectNumber,
                                           VulkanResourceType resType,
                                           VulkanDumpingMode dumpingMode) {
  auto path = Config::Get().common.player.outputDir;
  if (path.empty()) {
    if (Config::Get().IsRecorder()) {
      path = Config::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
    } else if (Config::Get().IsPlayer()) {
      path = Config::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
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
  } else if (resType == VULKAN_RESOLVE_IMAGE) {
    suffix << "_resolveImage_";
  }
  std::stringstream fileName;
  fileName << "frame" << std::setw(4) << std::setfill('0') << frameNumber;
  fileName << "_queueSubmit_" << submitNumber;
  fileName << "_commandbufferBatch_" << cmdBufferBatchNumber;
  fileName << "_commandbuffer_" << cmdBufferNumber;
  if (dumpingMode == VulkanDumpingMode::VULKAN_PER_RENDER_PASS ||
      dumpingMode == VulkanDumpingMode::VULKAN_PER_DRAW) {
    fileName << "_renderpass_" << renderpassNumber;
  } else if (dumpingMode == VulkanDumpingMode::VULKAN_PER_BLIT) {
    fileName << "_blit_" << renderpassNumber;
  } else if (dumpingMode == VulkanDumpingMode::VULKAN_PER_DISPATCH) {
    fileName << "_dispatch_" << renderpassNumber;
  }
  if (dumpingMode == VulkanDumpingMode::VULKAN_PER_DRAW) {
    fileName << "_draw_" << drawNumber;
  }
  fileName << suffix.str() << objectNumber;
  std::filesystem::create_directories(path);
  path /= fileName.str();
  return path.string();
}

bool operator==(const CGits::CCounter& counter, const VulkanObjectRange& vulkanObjRange) {
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

bool vulkanCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer bufferHandle, std::string fileName) {
  auto& bufferState = SD()._bufferstates[bufferHandle];
  if ((!bufferState->bufferCreateInfoData.Value()) ||
      (bufferState->bufferCreateInfoData.Value()->size == 0) || (!bufferState->binding)) {
    return false;
  }

  VkBufferCreateInfo* targetBufferCreateInfo = bufferState->bufferCreateInfoData.Value();

  VkBufferCopy bufferCopy = {
      0,                           // VkDeviceSize   srcOffset
      0,                           // VkDeviceSize   dstOffset
      targetBufferCreateInfo->size // VkDeviceSize   size
  };
  VkBuffer localBuffer;
  VkDeviceMemory localMemory = VK_NULL_HANDLE;
  VkDevice device = bufferState->deviceStateStore->deviceHandle;
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
    VkPhysicalDeviceMemoryProperties memoryProperties =
        SD()._devicestates[device]->physicalDeviceStateStore->memoryPropertiesCurrent;
    uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
      if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
        if (memoryRequirements.memoryTypeBits & (1 << i)) {
          VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                          memoryRequirements.size, i};

          drvVk.vkAllocateMemory(device, &memInfo, nullptr, &localMemory);
          if (localMemory == VK_NULL_HANDLE) {
            Log(ERR) << "Could not allocate memory for a buffer.";
            throw std::runtime_error(EXCEPTION_MESSAGE);
          }
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
          bufferHandle,                            // VkBuffer buffer
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
          bufferHandle,                            // VkBuffer buffer
          0,                                       // VkDeviceSize offset
          VK_WHOLE_SIZE                            // VkDeviceSize size;
      }};

  // Set barriers and acquire data
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, (uint32_t)dataAcquisitionBufferMemoryBarriersPre.size(),
                             dataAcquisitionBufferMemoryBarriersPre.data(), 0, nullptr);
  drvVk.vkCmdCopyBuffer(commandBuffer, bufferHandle, localBuffer, 1, &bufferCopy);
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, (uint32_t)dataAcquisitionBufferMemoryBarriersPost.size(),
                             dataAcquisitionBufferMemoryBarriersPost.data(), 0, nullptr);
  std::shared_ptr<RenderGenericAttachment> attachment(new RenderGenericAttachment());
  attachment->sourceBuffer = bufferHandle;
  attachment->copiedBuffer = localBuffer;
  attachment->fileName = std::move(fileName);
  attachment->devMemory = localMemory;
  SD()._commandbufferstates[commandBuffer]->renderPassResourceBuffers.push_back(attachment);
  return true;
}

bool vulkanCopyImage(VkCommandBuffer commandBuffer,
                     VkImage imageHandle,
                     std::string fileName,
                     bool isResource,
                     VkAttachmentStoreOp imageStoreOption,
                     VulkanDumpingMode dumpingMode) {
  auto& imageState = SD()._imagestates[imageHandle];
  if (Config::Get().vulkan.player.skipNonDeterministicImages &&
      (SD().nonDeterministicImages.find(imageHandle) != SD().nonDeterministicImages.end())) {
    return false;
  }

  // Skip dumping:
  // - images with compressed format
  // - images with storeOp VK_ATTACHMENT_STORE_OP_DONT_CARE
  // - multisampled images (spec forbids copying data to/from multisampled images)
  if (isFormatCompressed(imageState->imageFormat) ||
      (imageStoreOption == VK_ATTACHMENT_STORE_OP_DONT_CARE) ||
      (!imageState->swapchainKHRStateStore &&
       (!imageState->imageCreateInfoData.Value() ||
        imageState->imageCreateInfoData.Value()->samples != VK_SAMPLE_COUNT_1_BIT))) {
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
              VkPhysicalDeviceMemoryProperties memoryProperties =
                  SD()._devicestates[imageState->deviceStateStore->deviceHandle]
                      ->physicalDeviceStateStore->memoryPropertiesCurrent;
              uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
              for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) ==
                    requiredFlags) {
                  if (memoryRequirements.memoryTypeBits & (1 << i)) {
                    VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                                    memoryRequirements.size, i};

                    drvVk.vkAllocateMemory(imageState->deviceStateStore->deviceHandle, &memInfo,
                                           nullptr, &attachment->devMemory);
                    if (attachment->devMemory == VK_NULL_HANDLE) {
                      Log(ERR) << "Could not allocate memory for a buffer.";
                      throw std::runtime_error(EXCEPTION_MESSAGE);
                    }
                    drvVk.vkBindBufferMemory(imageState->deviceStateStore->deviceHandle,
                                             attachment->copiedBuffer, attachment->devMemory, 0);
                    break;
                  }
                }
              }
            }
            {
              // Perform pre-copy image layout transition
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
            auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
            if (isResource) {
              commandBufferState->renderPassResourceImages.push_back(attachment);
            } else {
              if (dumpingMode == VulkanDumpingMode::VULKAN_PER_RENDER_PASS) {
                commandBufferState->renderPassImages.push_back(attachment);
              } else if (dumpingMode == VulkanDumpingMode::VULKAN_PER_DRAW) {
                commandBufferState->drawImages.push_back(attachment);
              }
            }
          }
        }
      }
    }
  }
  return true;
}

void vulkanScheduleCopyResources(VkCommandBuffer cmdBuffer,
                                 uint64_t queueSubmitNumber,
                                 uint32_t cmdBuffBatchNumber,
                                 uint32_t cmdBuffNumber,
                                 uint64_t objNumber,
                                 VulkanDumpingMode dumpingMode,
                                 uint64_t drawNumber) {
  if (SD()._commandbufferstates.find(cmdBuffer) != SD()._commandbufferstates.end()) {
    auto& commandBufferState = SD()._commandbufferstates[cmdBuffer];
    for (auto& obj : commandBufferState->resourceWriteBuffers) {
      VkBuffer swapBuffer = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(), queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
          objNumber, drawNumber, SD().bufferCounter[swapBuffer], obj.second, dumpingMode);
      vulkanCopyBuffer(cmdBuffer, swapBuffer, std::move(fileName));
    }
    for (auto& obj : commandBufferState->resourceWriteImages) {
      VkImage swapImg = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(), queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
          objNumber, drawNumber, SD().imageCounter[swapImg], obj.second, dumpingMode);
      vulkanCopyImage(cmdBuffer, swapImg, std::move(fileName), true, VK_ATTACHMENT_STORE_OP_STORE,
                      dumpingMode);
    }
    commandBufferState->removeBlitsFromResourceMap();
  }
}

void vulkanScheduleCopyRenderPasses(VkCommandBuffer cmdBuffer,
                                    uint64_t queueSubmitNumber,
                                    uint32_t cmdBuffBatchNumber,
                                    uint32_t cmdBuffNumber,
                                    uint64_t renderPassNumber,
                                    uint64_t drawNumber) {
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
  VulkanDumpingMode dumpingMode;
  if (drawNumber) {
    dumpingMode = VulkanDumpingMode::VULKAN_PER_DRAW;
  } else {
    dumpingMode = VulkanDumpingMode::VULKAN_PER_RENDER_PASS;
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
        renderPassNumber, drawNumber, SD().imageCounter[imageHandle], dumpingMode);
    vulkanCopyImage(cmdBuffer, imageHandle, std::move(fileName), false, imageStoreOption,
                    dumpingMode);
  }
}

void vulkanDumpBuffer(std::shared_ptr<RenderGenericAttachment> attachment) {
  VkBuffer bufferHandle = attachment->sourceBuffer;
  auto& bufferState = SD()._bufferstates[bufferHandle];
  VkDevice device = bufferState->deviceStateStore->deviceHandle;
  drvVk.vkDeviceWaitIdle(device);
  VkMemoryRequirements memoryRequirements;
  drvVk.vkGetBufferMemoryRequirements(device, attachment->copiedBuffer, &memoryRequirements);
  FILE* outFile = nullptr;
  std::vector<char> bufferData;
  bufferData.resize(bufferState->bufferCreateInfoData.Value()->size);
  if (attachment->devMemory == VK_NULL_HANDLE) {
    Log(ERR) << "Could not allocate memory for a buffer to dump.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  unsigned char* ptr;
  drvVk.vkMapMemory(device, attachment->devMemory, 0, memoryRequirements.size, 0, (void**)&ptr);
  memcpy(bufferData.data(), ptr, bufferState->bufferCreateInfoData.Value()->size);

  std::string outputFileNameBin = attachment->fileName;
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
  fwrite(&bufferData[0], sizeof(uint8_t), bufferState->bufferCreateInfoData.Value()->size, outFile);
  fclose(outFile);
  drvVk.vkUnmapMemory(device, attachment->devMemory);
  drvVk.vkFreeMemory(device, attachment->devMemory, nullptr);
  drvVk.vkDestroyBuffer(device, attachment->copiedBuffer, nullptr);
}

void vulkanDumpImage(std::shared_ptr<RenderGenericAttachment> attachment) {
  VkImage image = attachment->sourceImage;
  auto& imageState = SD()._imagestates[image];
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
          Config::Get().vulkan.player.skipNonDeterministicImages &&
          !SD().depthRangeUnrestrictedEXTEnabled) {
        // When copying to a depth aspect, and the
        // VK_EXT_depth_range_unrestricted extension is not enabled, the
        // data in buffer memory (SFLOAT format) must be in the range
        // [0,1], or the resulting values are undefined.
        depthInProperRange = false;
      }
    } else if (attachment->aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
      normalize_texture_data(texel_type::R8, screenshotData, width, height);
      convert_texture_data(texel_type::R8, screenshotData, texel_type::RGBA8, convertedData, width,
                           height);
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

void vulkanDumpRenderPasses(VkCommandBuffer cmdBuffer) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuffer];
  for (auto& attachment : commandBufferState->renderPassImages) {
    vulkanDumpImage(attachment);
  }
  commandBufferState->renderPassImages.clear();
  for (auto& attachment : commandBufferState->drawImages) {
    vulkanDumpImage(attachment);
  }
  commandBufferState->drawImages.clear();
}

void vulkanDumpRenderPassResources(VkCommandBuffer cmdBuffer) {
  for (auto& attachment : SD()._commandbufferstates[cmdBuffer]->renderPassResourceImages) {
    vulkanDumpImage(attachment);
  }
  SD()._commandbufferstates[cmdBuffer]->renderPassResourceImages.clear();
  for (auto& attachment : SD()._commandbufferstates[cmdBuffer]->renderPassResourceBuffers) {
    vulkanDumpBuffer(attachment);
  }
  SD()._commandbufferstates[cmdBuffer]->renderPassResourceBuffers.clear();
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
  bool isMultisampleImage =
      !imageState->swapchainKHRStateStore &&
      (!imageState->imageCreateInfoData.Value() ||
       imageState->imageCreateInfoData.Value()->samples != VK_SAMPLE_COUNT_1_BIT);

  if (Config::Get().vulkan.player.skipNonDeterministicImages &&
      (SD().nonDeterministicImages.find(sourceImage) != SD().nonDeterministicImages.end())) {
    return false;
  }
#else
  auto& queueState = globalState.queueStates[queue];
  VkDevice device = queueState.device;
  uint32_t queueFamilyIndex = queueState.deviceQueueList.front().queueFamilyIndex;
  auto& imageState = globalState.imageStates.at(sourceImage);
  auto& internalResources = globalState.internalResources;
  bool isMultisampleImage = !imageState->swapchainCreateInfo &&
                            (!imageState->imageCreateInfo ||
                             imageState->imageCreateInfo->samples != VK_SAMPLE_COUNT_1_BIT);
#endif

  // Skip dumping:
  // - images with compressed format
  // - images with storeOp VK_ATTACHMENT_STORE_OP_DONT_CARE
  // - multisampled images (spec forbids copying data to/from multisampled images)
  if (isFormatCompressed(imageState->imageFormat) ||
      (storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE) || isMultisampleImage) {
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
              VkPhysicalDeviceMemoryProperties memoryProperties =
                  SD()._devicestates[device]->physicalDeviceStateStore->memoryPropertiesCurrent;
#else
              VkPhysicalDeviceMemoryProperties memoryProperties;
              drvVk.vkGetPhysicalDeviceMemoryProperties(
                  globalState.deviceStates[device].physicalDevice, &memoryProperties);
#endif // !BUILD_FOR_CCODE
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
            {
              // Perform pre-copy image layout transition
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
                Config::Get().vulkan.player.skipNonDeterministicImages &&
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
        if (Config::Get().vulkan.player.captureVulkanSubmitsGroupType ==
            TCaptureGroupType::PER_COMMAND_BUFFER) {
          fileName = GetFileNameDrawcallScreenshot(
              CGits::Instance().CurrentFrame(),
              (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
              commandBufferBatchNumber, cmdBufferNumber, 0, 0, SD().imageCounter[imageHandle],
              VulkanDumpingMode::VULKAN_PER_COMMAND_BUFFER);
          if (dumpedImagesFromCmdBuff.find(imageHandle) == dumpedImagesFromCmdBuff.end()) {
            bool dumped =
                writeScreenshotUtil(std::move(fileName), queue, imageHandle,
                                    VULKAN_MODE_RENDER_PASS_ATTACHMENTS, imageStoreOption);
            if (dumped) {
              dumpedImagesFromCmdBuff.insert(imageHandle);
            }
          }
        } else {
          fileName = GetFileNameDrawcallScreenshot(
              CGits::Instance().CurrentFrame(),
              (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
              commandBufferBatchNumber, cmdBufferNumber, renderpass, 0, imageview,
              VulkanDumpingMode::VULKAN_PER_RENDER_PASS);
          writeScreenshotUtil(std::move(fileName), queue, imageHandle,
                              VULKAN_MODE_RENDER_PASS_ATTACHMENTS, imageStoreOption);
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

    if (Config::Get().common.player.captureScreenshot) {
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

    for (auto& obj : commandBufferState->resourceWriteBuffers) {
      VkBuffer swapBuffer = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(),
          (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
          commandBufferBatchNumber, cmdBufferNumber, 0, 0, SD().bufferCounter[swapBuffer],
          obj.second, VulkanDumpingMode::VULKAN_PER_COMMAND_BUFFER);
      writeBufferUtil(std::move(fileName), queue, swapBuffer);
    }
    for (auto& obj : commandBufferState->resourceWriteImages) {
      VkImage swapImg = obj.first;
      std::string fileName = GetFileNameResourcesScreenshot(
          CGits::Instance().CurrentFrame(),
          (uint32_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount(),
          commandBufferBatchNumber, cmdBufferNumber, 0, 0, SD().imageCounter[swapImg], obj.second,
          VulkanDumpingMode::VULKAN_PER_COMMAND_BUFFER);
      writeScreenshotUtil(std::move(fileName), queue, swapImg, VULKAN_MODE_RESOURCES);
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
    VkPhysicalDeviceMemoryProperties memoryProperties =
        SD()._devicestates[device]->physicalDeviceStateStore->memoryPropertiesCurrent;
#else
    VkPhysicalDeviceMemoryProperties memoryProperties;
    drvVk.vkGetPhysicalDeviceMemoryProperties(globalState.deviceStates[device].physicalDevice,
                                              &memoryProperties);
#endif // !BUILD_FOR_CCODE
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

  if (fileName.empty()) {
    Log(ERR) << "Could not generate a name for the buffer to be dumped.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  fileName.append(".bin");

  const char* binaryFileName = fileName.c_str();
  if (binaryFileName == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  outFile = fopen(binaryFileName, "wb");
  if (outFile == nullptr) {
    Log(ERR) << "Could not open a file: " << fileName;
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

#ifndef BUILD_FOR_CCODE
bool checkForSupportForQueues(VkPhysicalDevice physicalDevice,
                              uint32_t requestedQueueCreateInfoCount,
                              VkDeviceQueueCreateInfo const* requestedQueueCreateInfos) {
  const auto& availableQueueFamilies =
      SD()._physicaldevicestates[physicalDevice]->queueFamilyPropertiesCurrent;
  const auto availableQueueFamiliesCount = availableQueueFamilies.size();
  const auto& queueFamilyPropertiesOriginal =
      SD()._physicaldevicestates[physicalDevice]->queueFamilyPropertiesOriginal;

  for (uint32_t i = 0; i < requestedQueueCreateInfoCount; ++i) {
    uint32_t requestedQueueFamilyIndex = requestedQueueCreateInfos[i].queueFamilyIndex;
    uint32_t requestedQueueCountForFamily = requestedQueueCreateInfos[i].queueCount;

    auto currentFlags = availableQueueFamilies[requestedQueueFamilyIndex].queueFlags;
    auto originalFlags = queueFamilyPropertiesOriginal[requestedQueueFamilyIndex].queueFlags;

#ifdef GITS_PLATFORM_WINDOWS
    if (Config::Get().vulkan.player.renderDoc.mode != TVkRenderDocCaptureMode::NONE) {
      currentFlags &= ~VK_QUEUE_PROTECTED_BIT;
      originalFlags &= ~VK_QUEUE_PROTECTED_BIT;
    }
#endif

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
    } else if (!isBitSet(currentFlags, originalFlags)) {
      Log(ERR) << "Queue families are not compatible!";
      VkLog(ERR) << "Original queue family flags at index " << requestedQueueFamilyIndex << ": "
                 << (VkQueueFlagBits)originalFlags;
      VkLog(ERR) << "Current platform queue family flags at index " << requestedQueueFamilyIndex
                 << ": " << (VkQueueFlagBits)currentFlags;
      return false;
    }
  }

  return true;
}

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
  if ((Config::Get().vulkan.player.traceVKShaderHashes) && (VK_NULL_HANDLE != pipeline)) {
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
          SaveBinaryFileContents(Config::Get().vulkan.player.overrideVKPipelineCache.string(),
                                 cacheData);
        }
      }
    }
    // Due to driver problems, all swapchains need to be always deleted (no
    // matter if cleanResourcesOnExit option is used)
    DESTROY_VULKAN_OBJECTS(VkSwapchainKHR, _swapchainkhrstates, vkDestroySwapchainKHR)

    // Destroy all other resources (only when cleanResourcesOnExit option is used)
    if (Config::Get().common.player.cleanResourcesOnExit) {
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
        // Injected pipelines
        {
          for (auto& deviceInternalPipelinePair :
               SD().internalResources.internalPipelines.pipelinesMap) {
            if ((device == VK_NULL_HANDLE) || (device == deviceInternalPipelinePair.first)) {
              drvVk.vkDestroyPipeline(
                  deviceInternalPipelinePair.first,
                  deviceInternalPipelinePair.second.prepareDeviceAddressesForPatching, nullptr);
              deviceInternalPipelinePair.second.prepareDeviceAddressesForPatching = VK_NULL_HANDLE;
              drvVk.vkDestroyPipelineLayout(deviceInternalPipelinePair.first,
                                            deviceInternalPipelinePair.second.layout, nullptr);
            }
          }
          if (device == VK_NULL_HANDLE) {
            SD().internalResources.internalPipelines.pipelinesMap.clear();
          } else {
            SD().internalResources.internalPipelines.pipelinesMap.erase(device);
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
    if (Config::Get().common.player.cleanResourcesOnExit) {
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
  if (isUseExternalMemoryExtensionUsed()) {
    auto touchedPages = ExternalMemoryRegion::GetTouchedPagesAndReset(
        (char*)memoryState->externalMemory + mapping->offsetData.Value(), unmapSize);

    for (auto& page : touchedPages) {
      VkDeviceSize offset = (char*)page.first - (char*)pointer;
      VkDeviceSize size = page.second;

      if (Config::Get().vulkan.recorder.memorySegmentSize) {
        std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
            GetChangedMemorySubranges(&mapping->compareData[offset], (char*)pointer + offset, size,
                                      Config::Get().vulkan.recorder.memorySegmentSize);

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
  } else if (Config::Get().vulkan.recorder.memoryAccessDetection) {
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

        if (Config::Get().vulkan.recorder.memorySegmentSize) {
          std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
              GetChangedMemorySubranges(&mapping->compareData[(size_t)offset],
                                        (char*)pointer + offset, range_size,
                                        Config::Get().vulkan.recorder.memorySegmentSize);

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
  } else if (Config::Get().vulkan.recorder.memorySegmentSize) {
    std::vector<std::pair<const uint8_t*, const uint8_t*>> optimizePagesMap =
        GetChangedMemorySubranges(&mapping->compareData[0], (char*)pointer, unmapSize,
                                  Config::Get().vulkan.recorder.memorySegmentSize);

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

  if (Config::Get().vulkan.recorder.memoryAccessDetection) {
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
      for (auto& elem : SD()._instancestates[(VkInstance)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._physicaldevicestates.find((VkPhysicalDevice)obj) !=
               SD()._physicaldevicestates.end()) {
      for (auto& elem : SD()._physicaldevicestates[(VkPhysicalDevice)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._surfacekhrstates.find((VkSurfaceKHR)obj) != SD()._surfacekhrstates.end()) {
      for (auto& elem : SD()._surfacekhrstates[(VkSurfaceKHR)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._devicestates.find((VkDevice)obj) != SD()._devicestates.end()) {
      for (auto& elem : SD()._devicestates[(VkDevice)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._swapchainkhrstates.find((VkSwapchainKHR)obj) !=
               SD()._swapchainkhrstates.end()) {
      for (auto& elem : SD()._swapchainkhrstates[(VkSwapchainKHR)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorpoolstates.find((VkDescriptorPool)obj) !=
               SD()._descriptorpoolstates.end()) {
      for (auto& elem : SD()._descriptorpoolstates[(VkDescriptorPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._commandpoolstates.find((VkCommandPool)obj) != SD()._commandpoolstates.end()) {
      for (auto& elem : SD()._commandpoolstates[(VkCommandPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._samplerstates.find((VkSampler)obj) != SD()._samplerstates.end()) {
      for (auto& elem : SD()._samplerstates[(VkSampler)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._devicememorystates.find((VkDeviceMemory)obj) !=
               SD()._devicememorystates.end()) {
      for (auto& elem : SD()._devicememorystates[(VkDeviceMemory)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._imagestates.find((VkImage)obj) != SD()._imagestates.end()) {
      for (auto& elem : SD()._imagestates[(VkImage)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._imageviewstates.find((VkImageView)obj) != SD()._imageviewstates.end()) {
      for (auto& elem : SD()._imageviewstates[(VkImageView)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._bufferstates.find((VkBuffer)obj) != SD()._bufferstates.end()) {
      for (auto& elem : SD()._bufferstates[(VkBuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._bufferviewstates.find((VkBufferView)obj) != SD()._bufferviewstates.end()) {
      for (auto& elem : SD()._bufferviewstates[(VkBufferView)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorsetlayoutstates.find((VkDescriptorSetLayout)obj) !=
               SD()._descriptorsetlayoutstates.end()) {
      for (auto& elem :
           SD()._descriptorsetlayoutstates[(VkDescriptorSetLayout)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorsetstates.find((VkDescriptorSet)obj) !=
               SD()._descriptorsetstates.end()) {
      for (auto& elem : SD()._descriptorsetstates[(VkDescriptorSet)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinelayoutstates.find((VkPipelineLayout)obj) !=
               SD()._pipelinelayoutstates.end()) {
      for (auto& elem : SD()._pipelinelayoutstates[(VkPipelineLayout)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._descriptorupdatetemplatestates.find((VkDescriptorUpdateTemplate)obj) !=
               SD()._descriptorupdatetemplatestates.end()) {
      for (auto& elem : SD()._descriptorupdatetemplatestates[(VkDescriptorUpdateTemplate)obj]
                            ->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinecachestates.find((VkPipelineCache)obj) !=
               SD()._pipelinecachestates.end()) {
      for (auto& elem : SD()._pipelinecachestates[(VkPipelineCache)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._shadermodulestates.find((VkShaderModule)obj) !=
               SD()._shadermodulestates.end()) {
      for (auto& elem : SD()._shadermodulestates[(VkShaderModule)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._renderpassstates.find((VkRenderPass)obj) != SD()._renderpassstates.end()) {
      for (auto& elem : SD()._renderpassstates[(VkRenderPass)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._pipelinestates.find((VkPipeline)obj) != SD()._pipelinestates.end()) {
      for (auto& elem : SD()._pipelinestates[(VkPipeline)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._framebufferstates.find((VkFramebuffer)obj) != SD()._framebufferstates.end()) {
      for (auto& elem : SD()._framebufferstates[(VkFramebuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._fencestates.find((VkFence)obj) != SD()._fencestates.end()) {
      for (auto& elem : SD()._fencestates[(VkFence)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._eventstates.find((VkEvent)obj) != SD()._eventstates.end()) {
      for (auto& elem : SD()._eventstates[(VkEvent)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._semaphorestates.find((VkSemaphore)obj) != SD()._semaphorestates.end()) {
      for (auto& elem : SD()._semaphorestates[(VkSemaphore)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._querypoolstates.find((VkQueryPool)obj) != SD()._querypoolstates.end()) {
      for (auto& elem : SD()._querypoolstates[(VkQueryPool)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    } else if (SD()._commandbufferstates.find((VkCommandBuffer)obj) !=
               SD()._commandbufferstates.end()) {
      for (auto& elem : SD()._commandbufferstates[(VkCommandBuffer)obj]->GetMappedPointers()) {
        relatedPointers.insert(elem);
      }
    }
  }
  return relatedPointers;
}

CVkSubmitInfoArrayWrap::CVkSubmitInfoArrayWrap() : submitInfoData(), submitInfo2Data() {}
CVkSubmitInfoArrayWrap::CVkSubmitInfoArrayWrap(uint32_t submitCount, VkSubmitInfo* submitInfo)
    : submitInfoData(submitCount, submitInfo), submitInfo2Data() {}
CVkSubmitInfoArrayWrap::CVkSubmitInfoArrayWrap(uint32_t submitCount, VkSubmitInfo2* submit2Info)
    : submitInfoData(), submitInfo2Data(submitCount, submit2Info) {}

std::set<uint64_t> getPointersUsedInQueueSubmit(CVkSubmitInfoArrayWrap& submitInfoData,
                                                const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                gits::VulkanObjectMode objMode) {
  std::set<uint64_t> pointers;
  auto submitInfoDataValues = submitInfoData.submitInfoData.Value();
  auto submitInfo2DataValues = submitInfoData.submitInfo2Data.Value();
  uint32_t submitInfoSize = 0;
  uint32_t objNumber = 0;
  if (objMode == VulkanObjectMode::MODE_VK_DRAW) {
    objNumber = countersTable.back();
  }

  if (submitInfoDataValues != nullptr) {
    submitInfoSize = submitInfoData.submitInfoData.size();
    for (uint32_t i = 0; i < submitInfoSize; i++) {
      for (uint32_t j = 0; j < submitInfoDataValues[i].commandBufferCount; j++) {
        auto& commandBufferState =
            SD()._commandbufferstates[submitInfoDataValues[i].pCommandBuffers[j]];
        for (auto& obj :
             commandBufferState->tokensBuffer.GetMappedPointers(objRange, objMode, objNumber)) {
          pointers.insert((uint64_t)obj);
        }

        for (auto& secondaryCommandBufferState :
             commandBufferState->secondaryCommandBuffersStateStoreList) {
          for (auto& elem : secondaryCommandBufferState.second->tokensBuffer.GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
      }
    }
    auto temporaryQueueSubmitToken = std::make_shared<CvkQueueSubmit>(
        VK_SUCCESS, SD().lastQueueSubmit->queueStateStore->queueHandle, submitInfoSize,
        submitInfoDataValues, SD().lastQueueSubmit->fenceHandle);
    for (auto& obj : temporaryQueueSubmitToken->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
  } else if (submitInfo2DataValues != nullptr) {
    submitInfoSize = submitInfoData.submitInfo2Data.size();
    for (uint32_t i = 0; i < submitInfoSize; i++) {
      for (uint32_t j = 0; j < submitInfo2DataValues[i].commandBufferInfoCount; j++) {
        auto& commandBufferState =
            SD()._commandbufferstates
                [submitInfo2DataValues[i].pCommandBufferInfos[j].commandBuffer];
        for (auto& obj :
             commandBufferState->tokensBuffer.GetMappedPointers(objRange, objMode, objNumber)) {
          pointers.insert((uint64_t)obj);
        }

        for (auto& secondaryCommandBufferState :
             commandBufferState->secondaryCommandBuffersStateStoreList) {
          for (auto& elem : secondaryCommandBufferState.second->tokensBuffer.GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
      }
    }
    auto temporaryQueueSubmitToken = std::make_shared<CvkQueueSubmit2>(
        VK_SUCCESS, SD().lastQueueSubmit->queueStateStore->queueHandle, submitInfoSize,
        submitInfo2DataValues, SD().lastQueueSubmit->fenceHandle);
    for (auto& obj : temporaryQueueSubmitToken->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
  }

  std::set<uint64_t> relatedPointers = getRelatedPointers(pointers);
  std::set<uint64_t> relatedPointers2 = getRelatedPointers(relatedPointers);
  std::set<uint64_t> relatedPointers3 = getRelatedPointers(relatedPointers2);
  std::set<uint64_t> relatedPointers4 = getRelatedPointers(relatedPointers3);
  pointers.insert(relatedPointers.begin(), relatedPointers.end());
  pointers.insert(relatedPointers2.begin(), relatedPointers2.end());
  pointers.insert(relatedPointers3.begin(), relatedPointers3.end());
  pointers.insert(relatedPointers4.begin(), relatedPointers4.end());
  return pointers;
}

CVkSubmitInfoArrayWrap getSubmitInfoForPrepare(const std::vector<uint32_t>& countersTable,
                                               const BitRange& objRange,
                                               VulkanObjectMode objMode) {
  CVkSubmitInfoArrayWrap submitInfoDataArray;
  auto submitInfoDataArrayValues = SD().lastQueueSubmit->submitInfoDataArray.Value();
  auto submitInfo2DataArrayValues = SD().lastQueueSubmit->submitInfo2DataArray.Value();
  uint32_t commandBufferBatchNumber = 0;
  uint32_t commandBufferNumber = 0;
  if (countersTable.size() >= 3) {
    commandBufferBatchNumber = countersTable.at(1);
    commandBufferNumber = countersTable.at(2);
  } else if (countersTable.size() == 2) {
    commandBufferBatchNumber = countersTable.at(1);
  }
  if (submitInfoDataArrayValues != nullptr) {
    if ((VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode ||
         VulkanObjectMode::MODE_VK_RENDER_PASS == objMode ||
         VulkanObjectMode::MODE_VK_DRAW == objMode || VulkanObjectMode::MODE_VK_BLIT == objMode ||
         VulkanObjectMode::MODE_VK_DISPATCH == objMode) &&
        (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfoDataArray.size())) {
      for (uint32_t i = 0; i < commandBufferBatchNumber; i++) {
        submitInfoDataArray.submitInfoData.AddElem(&submitInfoDataArrayValues[i]);
      }
      VkSubmitInfo submitInfoTemp = submitInfoDataArrayValues[commandBufferBatchNumber];
      std::vector<VkCommandBuffer> commandBufferVector;

      if (VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode) {
        for (uint32_t i = 0; (i < submitInfoTemp.commandBufferCount) && !objRange[i]; i++) {
          commandBufferVector.push_back(submitInfoTemp.pCommandBuffers[i]);
        }
      } else {
        if (commandBufferNumber < submitInfoTemp.commandBufferCount) {
          for (uint32_t i = 0; i <= commandBufferNumber; i++) {
            commandBufferVector.push_back(submitInfoTemp.pCommandBuffers[i]);
          }
        }
      }

      submitInfoTemp.commandBufferCount = (uint32_t)commandBufferVector.size();
      if (commandBufferVector.size() > 0) {
        submitInfoTemp.pCommandBuffers = &commandBufferVector[0];
        submitInfoDataArray.submitInfoData.AddElem(&submitInfoTemp);
      }
    }
  } else if (submitInfo2DataArrayValues != nullptr) {
    if ((VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode ||
         VulkanObjectMode::MODE_VK_RENDER_PASS == objMode ||
         VulkanObjectMode::MODE_VK_DRAW == objMode || VulkanObjectMode::MODE_VK_BLIT == objMode ||
         VulkanObjectMode::MODE_VK_DISPATCH == objMode) &&
        (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfo2DataArray.size())) {
      for (uint32_t i = 0; i < commandBufferBatchNumber; i++) {
        submitInfoDataArray.submitInfo2Data.AddElem(&submitInfo2DataArrayValues[i]);
      }
      VkSubmitInfo2 submitInfo2Temp = submitInfo2DataArrayValues[commandBufferBatchNumber];
      std::vector<VkCommandBufferSubmitInfo> commandBufferSubmitInfoVector;

      if (VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode) {
        for (uint32_t i = 0; (i < submitInfo2Temp.commandBufferInfoCount) && !objRange[i]; i++) {
          commandBufferSubmitInfoVector.push_back(submitInfo2Temp.pCommandBufferInfos[i]);
        }
      } else {
        if (commandBufferNumber < submitInfo2Temp.commandBufferInfoCount) {
          for (uint32_t i = 0; i <= commandBufferNumber; i++) {
            commandBufferSubmitInfoVector.push_back(submitInfo2Temp.pCommandBufferInfos[i]);
          }
        }
      }

      submitInfo2Temp.commandBufferInfoCount = (uint32_t)commandBufferSubmitInfoVector.size();
      if (commandBufferSubmitInfoVector.size() > 0) {
        submitInfo2Temp.pCommandBufferInfos = &commandBufferSubmitInfoVector[0];
        submitInfoDataArray.submitInfo2Data.AddElem(&submitInfo2Temp);
      }
    }
  }

  return submitInfoDataArray;
}

VkCommandBuffer GetLastCommandBuffer(CVkSubmitInfoArrayWrap& submitInfoData) {
  VkCommandBuffer lastCommandBuffer = VK_NULL_HANDLE;
  auto submitInfoDataArrayValues = submitInfoData.submitInfoData.Value();
  auto submitInfo2DataArrayValues = submitInfoData.submitInfo2Data.Value();
  if (submitInfoDataArrayValues != nullptr) {
    lastCommandBuffer = submitInfoDataArrayValues
                            ->pCommandBuffers[submitInfoDataArrayValues->commandBufferCount - 1];
  } else if (submitInfo2DataArrayValues != nullptr) {
    lastCommandBuffer =
        submitInfo2DataArrayValues
            ->pCommandBufferInfos[submitInfo2DataArrayValues->commandBufferInfoCount - 1]
            .commandBuffer;
  }
  return lastCommandBuffer;
}

void restoreCommandBufferSettings(const BitRange& objRange,
                                  CVkSubmitInfoArrayWrap& submitInfoData,
                                  VulkanObjectMode objMode,
                                  uint64_t renderPassNumber) {
  VkCommandBuffer lastCommandBuffer = GetLastCommandBuffer(submitInfoData);
  if (lastCommandBuffer != VK_NULL_HANDLE) {
    auto& commandBufferState = SD()._commandbufferstates[lastCommandBuffer];
    drvVk.vkResetCommandBuffer(lastCommandBuffer, 0);
    vkResetCommandBuffer_SD(VK_SUCCESS, lastCommandBuffer, 0, true);
    drvVk.vkBeginCommandBuffer(
        lastCommandBuffer,
        commandBufferState->beginCommandBuffer->commandBufferBeginInfoData.Value());
    if (objMode == VulkanObjectMode::MODE_VK_DRAW) {
      commandBufferState->tokensBuffer.RestoreDraw(renderPassNumber, objRange);
    } else {
      commandBufferState->tokensBuffer.RestoreToSpecifiedObject(objRange, objMode);
    }
    drvVk.vkEndCommandBuffer(lastCommandBuffer);
  }
}

CVkSubmitInfoArrayWrap getSubmitInfoForSchedule(const std::vector<uint32_t>& countersTable,
                                                const BitRange& objRange,
                                                VulkanObjectMode objMode) {
  CVkSubmitInfoArrayWrap submitInfoDataArray;
  auto submitInfoDataArrayValues = SD().lastQueueSubmit->submitInfoDataArray.Value();
  auto submitInfo2DataArrayValues = SD().lastQueueSubmit->submitInfo2DataArray.Value();
  uint32_t commandBufferBatchNumber = 0;
  uint32_t commandBufferNumber = 0;
  if (countersTable.size() >= 3) {
    commandBufferBatchNumber = countersTable.at(1);
    commandBufferNumber = countersTable.at(2);
  } else if (countersTable.size() == 2) {
    commandBufferBatchNumber = countersTable.at(1);
  }

  if (submitInfoDataArrayValues != nullptr) {
    if (VulkanObjectMode::MODE_VK_QUEUE_SUBMIT == objMode) {
      for (uint32_t i = 0; i < SD().lastQueueSubmit->submitCount; i++) {
        submitInfoDataArray.submitInfoData.AddElem(&submitInfoDataArrayValues[i]);
      }
    } else if ((VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode) &&
               (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfoDataArray.size())) {
      VkSubmitInfo submitInfoTemp = submitInfoDataArrayValues[commandBufferBatchNumber];
      std::vector<VkCommandBuffer> commandBufferVector;
      for (uint32_t i = 0; i < submitInfoTemp.commandBufferCount; i++) {
        if (objRange[i]) {
          commandBufferVector.push_back(submitInfoTemp.pCommandBuffers[i]);
        }
      }
      submitInfoTemp.commandBufferCount = (uint32_t)commandBufferVector.size();
      submitInfoTemp.pCommandBuffers = &commandBufferVector[0];
      submitInfoDataArray.submitInfoData.AddElem(&submitInfoTemp);
    } else if ((VulkanObjectMode::MODE_VK_RENDER_PASS == objMode ||
                VulkanObjectMode::MODE_VK_DRAW == objMode ||
                VulkanObjectMode::MODE_VK_BLIT == objMode ||
                VulkanObjectMode::MODE_VK_DISPATCH == objMode) &&
               (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfoDataArray.size())) {
      VkSubmitInfo submitInfoTemp = submitInfoDataArrayValues[commandBufferBatchNumber];
      if (commandBufferNumber < submitInfoTemp.commandBufferCount) {
        submitInfoTemp.commandBufferCount = 1;
        submitInfoTemp.pCommandBuffers = &submitInfoTemp.pCommandBuffers[commandBufferNumber];
        submitInfoDataArray.submitInfoData.AddElem(&submitInfoTemp);
      }
    }
  } else if (submitInfo2DataArrayValues != nullptr) {
    if (VulkanObjectMode::MODE_VK_QUEUE_SUBMIT == objMode) {
      for (uint32_t i = 0; i < SD().lastQueueSubmit->submitCount; i++) {
        submitInfoDataArray.submitInfo2Data.AddElem(&submitInfo2DataArrayValues[i]);
      }
    } else if ((VulkanObjectMode::MODE_VK_COMMAND_BUFFER == objMode) &&
               (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfo2DataArray.size())) {
      VkSubmitInfo2 submitInfo2Temp = submitInfo2DataArrayValues[commandBufferBatchNumber];
      std::vector<VkCommandBufferSubmitInfo> commandBufferSubmitInfoVector;
      for (uint32_t i = 0; i < submitInfo2Temp.commandBufferInfoCount; i++) {
        if (objRange[i]) {
          commandBufferSubmitInfoVector.push_back(submitInfo2Temp.pCommandBufferInfos[i]);
        }
      }
      submitInfo2Temp.commandBufferInfoCount = (uint32_t)commandBufferSubmitInfoVector.size();
      submitInfo2Temp.pCommandBufferInfos = &commandBufferSubmitInfoVector[0];
      submitInfoDataArray.submitInfo2Data.AddElem(&submitInfo2Temp);
    } else if ((VulkanObjectMode::MODE_VK_RENDER_PASS == objMode ||
                VulkanObjectMode::MODE_VK_DRAW == objMode ||
                VulkanObjectMode::MODE_VK_BLIT == objMode ||
                VulkanObjectMode::MODE_VK_DISPATCH == objMode) &&
               (commandBufferBatchNumber < SD().lastQueueSubmit->submitInfo2DataArray.size())) {
      VkSubmitInfo2 submitInfo2Temp = submitInfo2DataArrayValues[commandBufferBatchNumber];
      if (commandBufferNumber < submitInfo2Temp.commandBufferInfoCount) {
        submitInfo2Temp.commandBufferInfoCount = 1;
        submitInfo2Temp.pCommandBufferInfos =
            &submitInfo2Temp.pCommandBufferInfos[commandBufferNumber];
        submitInfoDataArray.submitInfo2Data.AddElem(&submitInfo2Temp);
      }
    }
  }

  return submitInfoDataArray;
}

namespace {
std::vector<bool> getMemoryMappingFeasibility(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  std::vector<bool> mappableMemory(memoryProperties.memoryTypeCount);
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    mappableMemory[i] = isBitSet(memoryProperties.memoryTypes[i].propertyFlags,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  }
  return mappableMemory;
}

bool isMemoryMappable(VkDevice device, uint32_t memoryTypeIndex) {
  auto& physicalDeviceHandle =
      SD()._devicestates[device]->physicalDeviceStateStore->physicalDeviceHandle;
  static std::unordered_map<VkPhysicalDevice, std::vector<bool>> memoryMappableMap;
  auto it = memoryMappableMap.find(physicalDeviceHandle);
  if (it == memoryMappableMap.end()) {
    it = memoryMappableMap
             .insert({physicalDeviceHandle, getMemoryMappingFeasibility(physicalDeviceHandle)})
             .first;
  }
  return (*it).second[memoryTypeIndex];
}
} // namespace

bool checkMemoryMappingFeasibility(VkDevice device, VkDeviceMemory memory, bool throwException) {
  auto memoryIndex =
      SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->memoryTypeIndex;
  if (!isMemoryMappable(device, memoryIndex)) {
    if (throwException) {
      Log(ERR) << "Stream tries to map memory object " << memory
               << " which was allocated from a non-host-visible memory type at index "
               << memoryIndex;
      if (!Config::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
        throw std::runtime_error("Memory object cannot be mapped on a current platform. Exiting!!");
      }
    }
    return false;
  } else {
    return true;
  }
}

bool checkMemoryMappingFeasibility(VkDevice device, uint32_t memoryTypeIndex, bool throwException) {
  if (!isMemoryMappable(device, memoryTypeIndex)) {
    if (throwException) {
      Log(ERR) << "Stream tries to map memory object which was allocated from a non-host-visible "
                  "memory type at index "
               << memoryTypeIndex;
      if (!Config::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
        throw std::runtime_error("Memory object cannot be mapped on a current platform. Exiting!!");
      }
    }
    return false;
  } else {
    return true;
  }
}

std::unordered_map<uint32_t, uint32_t> matchCorrespondingMemoryTypeIndexes(
    VkPhysicalDevice physicalDevice) {
  auto& originalPlatformProperties =
      SD()._physicaldevicestates[physicalDevice]->memoryPropertiesOriginal;
  auto& currentPlatformProperties =
      SD()._physicaldevicestates[physicalDevice]->memoryPropertiesCurrent;

  std::unordered_map<uint32_t, uint32_t> memoryTypeIndexes;

  for (uint32_t i = 0; i < originalPlatformProperties.memoryTypeCount; i++) {
    // firstly look for the 1:1 match of property flags with the same index
    if (originalPlatformProperties.memoryTypes[i].propertyFlags ==
        currentPlatformProperties.memoryTypes[i].propertyFlags) {
      memoryTypeIndexes.insert({i, i});
      continue;
    }
    // then look for a mapping in which the original flags are contained in the current ones with the same index
    if (isBitSet(currentPlatformProperties.memoryTypes[i].propertyFlags,
                 originalPlatformProperties.memoryTypes[i].propertyFlags)) {
      memoryTypeIndexes.insert({i, i});
      continue;
    }
    bool foundMapping = false;
    // look for the 1:1 match of property flags
    for (uint32_t j = 0; j < currentPlatformProperties.memoryTypeCount; j++) {
      if (originalPlatformProperties.memoryTypes[i].propertyFlags ==
          currentPlatformProperties.memoryTypes[j].propertyFlags) {
        memoryTypeIndexes.insert_or_assign(i, j);
        foundMapping = true;
      }
    }
    // then look for a mapping in which the original flags are contained in the current ones
    if (!foundMapping) {
      for (uint32_t j = 0; j < currentPlatformProperties.memoryTypeCount; j++) {
        if (isBitSet(currentPlatformProperties.memoryTypes[j].propertyFlags,
                     originalPlatformProperties.memoryTypes[i].propertyFlags)) {
          memoryTypeIndexes.insert_or_assign(i, j);
          foundMapping = true;
        }
      }
    }
  }

  if (originalPlatformProperties.memoryTypeCount != memoryTypeIndexes.size()) {
    VkLog(ERR) << "Cannot find proper mapping for memory type indexes";
    VkLog(ERR) << "Original memory types: ";
    for (uint32_t i = 0; i < originalPlatformProperties.memoryTypeCount; i++) {
      VkLog(ERR) << "[" << i << "]: " << originalPlatformProperties.memoryTypes[i];
    }
    VkLog(ERR) << "Current platform memory types: ";
    for (uint32_t i = 0; i < currentPlatformProperties.memoryTypeCount; i++) {
      VkLog(ERR) << "[" << i << "]: " << currentPlatformProperties.memoryTypes[i];
    }
    throw std::runtime_error("Cannot find proper mapping for memory type indexes");
  }

  return memoryTypeIndexes;
}

uint32_t getMappedMemoryTypeIndex(VkDevice device, uint32_t memoryTypeIndexOriginal) {
  auto& correspondingMemoryTypeIndexes =
      SD()._devicestates[device]->physicalDeviceStateStore->correspondingMemoryTypeIndexes;

  if (correspondingMemoryTypeIndexes.empty()) {
    Log(ERR) << "Memory type index mapping does not exist for this stream. The reason is that "
                "there is no vkPassPhysicalDeviceMemoryPropertiesGITS token recorded. The solution "
                "is to record a new stream.";
    Log(ERR) << "Skipping mapping and returning orignal index of memory type.";
    return memoryTypeIndexOriginal;
  }

  auto mappedMemoryTypeIndex = correspondingMemoryTypeIndexes.at(memoryTypeIndexOriginal);
  return mappedMemoryTypeIndex;
}

uint32_t findCompatibleMemoryTypeIndex(VkMemoryPropertyFlags originalMemoryPropertyFlags,
                                       VkPhysicalDeviceMemoryProperties& currentMemoryProperties,
                                       uint32_t requirementsMemoryTypeBits) {
  for (uint32_t i = 0; currentMemoryProperties.memoryTypeCount; i++) {
    if ((requirementsMemoryTypeBits & (1 << i)) &&
        (currentMemoryProperties.memoryTypes[i].propertyFlags & originalMemoryPropertyFlags)) {
      return i;
    }
  }

  throw std::runtime_error("Cannot find a compatible memory type for a resource. Exiting!");
}

VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer) {
  VkBufferDeviceAddressInfo addressInfo = {
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType    sType;
      nullptr,                                      // const void       * pNext;
      buffer                                        // VkBuffer           buffer;
  };
  return drvVk.vkGetBufferDeviceAddressUnifiedGITS(device, &addressInfo);
}

VkBuffer findBufferFromDeviceAddress(VkDeviceAddress deviceAddress) {
  if (deviceAddress == 0) {
    return VK_NULL_HANDLE;
  }

  // Quick search
  {
    auto it = CBufferState::deviceAddressesQuickLook.find(deviceAddress);
    if (it != CBufferState::deviceAddressesQuickLook.end()) {
      return it->second;
    }
  }

  // Full search
  {
    auto it =
        std::find_if(CBufferState::deviceAddresses.begin(), CBufferState::deviceAddresses.end(),
                     [deviceAddress](auto const& element) {
                       return (deviceAddress >= element.start) && (deviceAddress < element.end);
                     });

    if (it != CBufferState::deviceAddresses.end()) {
      TODO("Use try_emplace when C++17 is available.")

      CBufferState::deviceAddressesQuickLook.emplace(deviceAddress, it->buffer);
      SD()._bufferstates[it->buffer]->deviceAddressesToErase.insert(deviceAddress);
      return it->buffer;
    }
  }

  // No element found
  Log(WARN) << "Trying to find a buffer from an unknown address space: " << deviceAddress;
  return VK_NULL_HANDLE;
}

uint32_t getRayTracingShaderGroupCaptureReplayHandleSize(VkDevice device) {
  static std::map<VkDevice, uint32_t> shaderGroupCaptureReplayHandleSize;

  auto it = shaderGroupCaptureReplayHandleSize.find(device);
  if (it == shaderGroupCaptureReplayHandleSize.end()) {
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR, // VkStructureType sType;
        nullptr,                                                               // void* pNext;
        0, // uint32_t shaderGroupHandleSize;
        0, // uint32_t maxRayRecursionDepth;
        0, // uint32_t maxShaderGroupStride;
        0, // uint32_t shaderGroupBaseAlignment;
        0, // uint32_t shaderGroupHandleCaptureReplaySize;
        0, // uint32_t maxRayDispatchInvocationCount;
        0, // uint32_t shaderGroupHandleAlignment;
        0  // uint32_t maxRayHitAttributeSize;
    };

    VkPhysicalDeviceProperties2 physicalDeviceProperties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, // VkStructureType sType;
        &rayTracingPipelineProperties,                  // void* pNext;
        {}                                              // VkPhysicalDeviceProperties properties;
    };
    drvVk.vkGetPhysicalDeviceProperties2(
        SD()._devicestates[device]->physicalDeviceStateStore->physicalDeviceHandle,
        &physicalDeviceProperties);

    it = shaderGroupCaptureReplayHandleSize
             .emplace(device, rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize)
             .first;
  }

  return it->second;
}

std::pair<std::shared_ptr<CDeviceMemoryState>, std::shared_ptr<CBufferState>> createTemporaryBuffer(
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags bufferUsage,
    CCommandBufferState* commandBufferState,
    VkMemoryPropertyFlags requiredMemoryPropertyFlags,
    void* hostPointer) {
  auto& deviceState = SD()._devicestates[device];

  // Create buffer
  VkBuffer buffer;
  VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType        sType;
      nullptr,                              // const void           * pNext;
      0,                                    // VkBufferCreateFlags    flags;
      size,                                 // VkDeviceSize           size;
      bufferUsage,                          // VkBufferUsageFlags     usage;
      VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode          sharingMode;
      0,                                    // uint32_t               queueFamilyIndexCount;
      nullptr                               // const uint32_t       * pQueueFamilyIndices;
  };
  if (drvVk.vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("Could not create a temporary buffer! Exiting!");
  }

  VkMemoryRequirements bufferMemoryRequirements;
  drvVk.vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

  VkPhysicalDeviceMemoryProperties memoryProperties =
      deviceState->physicalDeviceStateStore->memoryPropertiesCurrent;

  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkMemoryAllocateInfo memoryAllocateInfo;

  // Find appropriate memory type index
  for (uint32_t type = 0; type < memoryProperties.memoryTypeCount; ++type) {
    void* pNext = nullptr;
    if (isBitSet(bufferMemoryRequirements.memoryTypeBits, 1 << type) &&
        isBitSet(memoryProperties.memoryTypes[type].propertyFlags, requiredMemoryPropertyFlags)) {

      VkMemoryAllocateFlagsInfo memoryAllocateFlags = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, // VkStructureType          sType;
          nullptr,                                      // const void             * pNext;
          VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,        // VkMemoryAllocateFlags    flags;
          1                                             // uint32_t                 deviceMask;
      };

      if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        pNext = &memoryAllocateFlags;
      }

      VkImportMemoryHostPointerInfoEXT hostPointerInfo = {
          VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT, // VkStructureType sType;
          pNext,                                                 // const void* pNext;
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, // VkExternalMemoryHandleTypeFlagBits handleType;
          hostPointer                                             // void* pHostPointer;
      };

      if (hostPointer) {
        pNext = &hostPointerInfo;
      }

      memoryAllocateInfo = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType    sType
          pNext,                                  // const void       * pNext
          bufferMemoryRequirements.size,          // VkDeviceSize       allocationSize
          type                                    // uint32_t           memoryTypeIndex
      };

      VkResult result = drvVk.vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
      if (VK_SUCCESS == result) {
        break;
      }
    }
  }
  if (memory == VK_NULL_HANDLE) {
    throw std::runtime_error("Could not allocate memory for a buffer.");
  }

  // Bind memory to the buffer
  if (VK_SUCCESS != drvVk.vkBindBufferMemory(device, buffer, memory, 0)) {
    throw std::runtime_error("Could not bind memory object to a buffer.");
  }

  auto temporaryMemoryState =
      std::make_shared<CDeviceMemoryState>(&memory, &memoryAllocateInfo, deviceState, nullptr);
  auto temporaryBufferState =
      std::make_shared<CBufferState>(&buffer, &bufferCreateInfo, deviceState);
  auto memoryBufferPair = std::make_pair(temporaryMemoryState, temporaryBufferState);

  if (commandBufferState) {
    commandBufferState->temporaryBuffers.insert(memoryBufferPair);
  }

  return memoryBufferPair;
}

void mapMemoryAndCopyData(VkDevice device,
                          VkDeviceMemory destination,
                          VkDeviceSize offset,
                          void* source,
                          VkDeviceSize dataSize) {
  void* dst;
  if (drvVk.vkMapMemory(device, destination, offset, VK_WHOLE_SIZE, 0, &dst) != VK_SUCCESS) {
    throw std::runtime_error("Could not map and copy data.");
  }
  memcpy(dst, source, dataSize);

  VkMappedMemoryRange range = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType sType;
      nullptr,                               // const void* pNext;
      destination,                           // VkDeviceMemory memory;
      0,                                     // VkDeviceSize offset;
      dataSize                               // VkDeviceSize size;
  };
  drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
}

void mapMemoryAndCopyData(void* destination,
                          VkDevice device,
                          VkDeviceMemory source,
                          VkDeviceSize offset,
                          VkDeviceSize dataSize) {
  void* src;
  if (drvVk.vkMapMemory(device, source, offset, VK_WHOLE_SIZE, 0, &src) != VK_SUCCESS) {
    throw std::runtime_error("Could not map and copy data.");
  }

  VkMappedMemoryRange range = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType sType;
      nullptr,                               // const void* pNext;
      source,                                // VkDeviceMemory memory;
      0,                                     // VkDeviceSize offset;
      dataSize                               // VkDeviceSize size;
  };
  drvVk.vkInvalidateMappedMemoryRanges(device, 1, &range);

  memcpy(destination, src, dataSize);
}

VkDeviceAddress getAccelerationStructureDeviceAddress(
    VkDevice device, VkAccelerationStructureKHR accelerationStructure) {
  VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, // VkStructureType              sType;
      nullptr,              // const void                 * pNext;
      accelerationStructure // VkAccelerationStructureKHR   accelerationStructure;
  };
  return drvVk.vkGetAccelerationStructureDeviceAddressUnifiedGITS(device, &addressInfo);
}

VkAccelerationStructureBuildControlDataGITS prepareAccelerationStructureControlData(
    VkCommandBuffer commandBuffer) {
  return {
      CAccelerationStructureKHRState::
          globalAccelerationStructureBuildCommandIndex, // uint32_t buildCommandIndex
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,   // VkBuildAccelerationStructureModeKHR mode
      VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR,       // VkAccelerationStructureTypeKHR type
      SD()._commandbufferstates[commandBuffer]
          ->commandPoolStateStore->deviceStateStore->deviceHandle, // VkDevice device;
      VK_NULL_HANDLE,                         // VkAccelerationStructureKHR accelerationStructure
      commandBuffer,                          // VkCommandBuffer commandBuffer;
      getCommandExecutionSide(commandBuffer), // VkCommandExecutionSideGITS executionSide;
  };
}

VkAccelerationStructureBuildControlDataGITS prepareAccelerationStructureControlData(
    VkAccelerationStructureBuildControlDataGITS controlData,
    const VkAccelerationStructureBuildGeometryInfoKHR* buildInfo) {
  controlData.mode = buildInfo->mode;
  controlData.accelerationStructureType = buildInfo->type;
  controlData.accelerationStructure = buildInfo->dstAccelerationStructure;

  return controlData;
}

VkAccelerationStructureBuildControlDataGITS prepareAccelerationStructureControlData(
    VkAccelerationStructureBuildControlDataGITS controlData, VkStructureType sType) {
  controlData.sType = sType;

  return controlData;
}

uint64_t prepareStateTrackingHash(const VkAccelerationStructureBuildControlDataGITS& controlData,
                                  VkDeviceAddress deviceAddress,
                                  uint32_t offset,
                                  uint64_t stride,
                                  uint32_t count) {
  CAccelerationStructureKHRState::HashGenerator hashGenerator = {
      controlData.accelerationStructure,     // VkAccelerationStructureKHR     accStructure;
      deviceAddress,                         // VkDeviceAddress                deviceAddress;
      stride,                                // uint64_t                       stride;
      controlData.buildCommandIndex,         // uint32_t                       buildCommandIndex;
      controlData.mode,                      // VkBuildAccelerationStructureModeKHR mode;
      controlData.accelerationStructureType, // VkAccelerationStructureTypeKHR type;
      controlData.sType,                     // VkStructureType                sType;
      offset,                                // uint32_t                       offset;
      count                                  // uint32_t                       count;
  };
  return CGits::Instance().ResourceManager2().getHash(RESOURCE_DATA_RAW, &hashGenerator,
                                                      sizeof(hashGenerator));
}

namespace {

VkShaderModule createShaderModule(VkDevice device, uint32_t codeSize, uint32_t const* codePtr) {
  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, // VkStructureType             sType
      nullptr,                                     // const void                * pNext
      0,                                           // VkShaderModuleCreateFlags   flags
      codeSize,                                    // size_t                      codeSize
      codePtr                                      // const uint32_t            * pCode
  };

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  VkResult result = drvVk.vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
  if ((result != VK_SUCCESS) || (shaderModule == VK_NULL_HANDLE)) {
    throw std::runtime_error("Could not create a shader module for an injected pipeline. Retry "
                             "recording or use capture/replay features instead.");
  }

  return shaderModule;
}

} // namespace

VkPipelineLayout createInternalPipelineLayout(VkDevice device) {
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  VkPushConstantRange pushConstantsRange = {
      VK_SHADER_STAGE_COMPUTE_BIT, // VkShaderStageFlags stageFlags
      0,                           // uint32_t           offset
      4 * sizeof(VkDeviceAddress)  // uint32_t           size
  };

  VkPipelineLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // VkStructureType               sType
      nullptr,                                       // const void                  * pNext
      0,                                             // VkPipelineLayoutCreateFlags   flags
      0,                                             // uint32_t                      setLayoutCount
      nullptr,                                       // const VkDescriptorSetLayout * pSetLayouts
      1,                  // uint32_t                      pushConstantRangeCount
      &pushConstantsRange // const VkPushConstantRange   * pPushConstantRanges
  };

  VkPipelineLayout layout;
  VkResult result = drvVk.vkCreatePipelineLayout(device, &createInfo, nullptr, &layout);
  if ((result != VK_SUCCESS) || (layout == VK_NULL_HANDLE)) {
    throw std::runtime_error("Could not create a pipeline layout used by injected pipelines. "
                             "Retry recording or use capture/replay features instead.");
  }

  return layout;
}

VkPipeline createInternalPipeline(VkDevice device,
                                  VkPipelineLayout layout,
                                  const std::vector<uint32_t>& code) {
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  auto size = code.size() * sizeof(uint32_t);

  VkShaderModule shaderModule = createShaderModule(device, size, code.data());

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType sType
      nullptr,                     // const void                       * pNext
      0,                           // VkPipelineShaderStageCreateFlags   flags
      VK_SHADER_STAGE_COMPUTE_BIT, // VkShaderStageFlagBits              stage
      shaderModule,                // VkShaderModule                     module
      "main",                      // const char                       * pName
      nullptr                      // const VkSpecializationInfo       * pSpecializationInfo
  };

  VkComputePipelineCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, // VkStructureType                   sType;
      nullptr,                                        // const void                      * pNext;
      0,                                              // VkPipelineCreateFlags             flags;
      shaderStageCreateInfo,                          // VkPipelineShaderStageCreateInfo   stage;
      layout,                                         // VkPipelineLayout                  layout;
      VK_NULL_HANDLE, // VkPipeline                        basePipelineHandle;
      -1              // int32_t                           basePipelineIndex;
  };

  VkPipeline pipeline;
  VkResult result =
      drvVk.vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
  if ((result != VK_SUCCESS) || (pipeline == VK_NULL_HANDLE)) {
    throw std::runtime_error("Could not create a compute pipeline for injected metada copies. "
                             "Retry recording or use capture/replay features instead.");
  }

  drvVk.vkDestroyShaderModule(device, shaderModule, nullptr);

  return pipeline;
}

bool getStructStorageFromHash(hash_t hash,
                              VkAccelerationStructureKHR accelerationStructure,
                              void** ptr) {
  TODO("Hopefully it will be removed after finding a proper way to pass metadata to Vulkan "
       "arguments")
  auto& accelerationStructureState = SD()._accelerationstructurekhrstates[accelerationStructure];
  auto it = accelerationStructureState->stateTrackingHashMap.find(hash);

  if (it != accelerationStructureState->stateTrackingHashMap.end()) {
    *ptr = it->second;
    return true;
  } else {
    *ptr = nullptr;
    return false;
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

bool IsObjectToSkip(uint64_t vulkanObject) {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  if (gits::Config::Get().vulkan.recorder.minimalStateRestore &&
      (api3dIface.CfgRec_IsSubFrameMode() &&
       SD().objectsUsedInQueueSubmit.find(vulkanObject) == SD().objectsUsedInQueueSubmit.end())) {
    return true;
  } else {
    return false;
  }
}

VkResult _vkCreateRenderPass_Helper(VkDevice device,
                                    const VkRenderPassCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkRenderPass& pRenderPass,
                                    CreationFunction createdWith) {
  return drvVk.vkCreateRenderPass(device, pCreateInfo, pAllocator, &pRenderPass);
}

VkResult _vkCreateRenderPass_Helper(VkDevice device,
                                    const VkRenderPassCreateInfo2* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkRenderPass& pRenderPass,
                                    CreationFunction createdWith) {
  if (createdWith == CreationFunction::KHR_EXTENSION) {
    return drvVk.vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, &pRenderPass);
  } else {
    return drvVk.vkCreateRenderPass2(device, pCreateInfo, pAllocator, &pRenderPass);
  }
}

unsigned int getBeginRenderFunctionID(unsigned int endFuncID) {
  static std::unordered_map<unsigned int, unsigned int> correspondingFunctionsIDs = {
      {CFunction::ID_VK_CMD_END_RENDER_PASS, CFunction::ID_VK_CMD_BEGIN_RENDER_PASS},
      {CFunction::ID_VK_CMD_END_RENDER_PASS2, CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2},
      {CFunction::ID_VK_CMD_END_RENDER_PASS2KHR, CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2KHR},
      {CFunction::ID_VK_CMD_END_RENDERING, CFunction::ID_VK_CMD_BEGIN_RENDERING},
      {CFunction::ID_VK_CMD_END_RENDERING_KHR, CFunction::ID_VK_CMD_BEGIN_RENDERING_KHR}};
  return correspondingFunctionsIDs.at(endFuncID);
}

void callvkCmdBeginRenderingByID(unsigned int ID,
                                 const VkCommandBuffer& commandBuffer,
                                 const VkRenderingInfo* pRenderingInfo) {
  if (ID == CFunction::ID_VK_CMD_BEGIN_RENDERING) {
    drvVk.vkCmdBeginRendering(commandBuffer, pRenderingInfo);
  } else if (ID == CFunction::ID_VK_CMD_BEGIN_RENDERING_KHR) {
    drvVk.vkCmdBeginRenderingKHR(commandBuffer, pRenderingInfo);
  }
}

void callvkCmdEndRenderingByID(unsigned int ID, const VkCommandBuffer& commandBuffer) {
  if (ID == CFunction::ID_VK_CMD_END_RENDERING) {
    drvVk.vkCmdEndRendering(commandBuffer);
  } else if (ID == CFunction::ID_VK_CMD_END_RENDERING_KHR) {
    drvVk.vkCmdEndRenderingKHR(commandBuffer);
  }
}

void callvkCmdBeginRenderPassByID(unsigned int ID,
                                  const VkCommandBuffer& commandBuffer,
                                  const VkRenderPassBeginInfo* pRenderPassBegin,
                                  const VkSubpassContents& contents) {
  if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS) {
    drvVk.vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
  } else {
    VkSubpassBeginInfo subpassBeginInfo = {VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO, nullptr, contents};
    if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2) {
      drvVk.vkCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, &subpassBeginInfo);
    } else if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2KHR) {
      drvVk.vkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, &subpassBeginInfo);
    }
  }
}

void callvkCmdEndRenderPassByID(unsigned int ID, const VkCommandBuffer& commandBuffer) {
  if (ID == CFunction::ID_VK_CMD_END_RENDER_PASS) {
    drvVk.vkCmdEndRenderPass(commandBuffer);
  } else {
    VkSubpassEndInfo subpassEndInfo = {VK_STRUCTURE_TYPE_SUBPASS_END_INFO, nullptr};
    if (ID == CFunction::ID_VK_CMD_END_RENDER_PASS2) {
      drvVk.vkCmdEndRenderPass2(commandBuffer, &subpassEndInfo);
    } else if (ID == CFunction::ID_VK_CMD_END_RENDER_PASS2KHR) {
      drvVk.vkCmdEndRenderPass2KHR(commandBuffer, &subpassEndInfo);
    }
  }
}

void schedulevkCmdBeginRenderPassByID(unsigned int ID,
                                      void (*schedulerFunc)(Vulkan::CFunction*),
                                      const VkCommandBuffer& commandBuffer,
                                      const VkRenderPassBeginInfo* pRenderPassBegin,
                                      const VkSubpassContents& contents) {
  if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS) {
    schedulerFunc(new CvkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents));
  } else {
    VkSubpassBeginInfo subpassBeginInfo = {VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO, nullptr, contents};
    if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2) {
      schedulerFunc(new CvkCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, &subpassBeginInfo));
    } else if (ID == CFunction::ID_VK_CMD_BEGIN_RENDER_PASS2KHR) {
      schedulerFunc(
          new CvkCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, &subpassBeginInfo));
    }
  }
}

void schedulevkCmdBeginRenderingByID(unsigned int ID,
                                     void (*schedulerFunc)(Vulkan::CFunction*),
                                     const VkCommandBuffer& commandBuffer,
                                     const VkRenderingInfo* pRenderingInfo) {
  if (ID == CFunction::ID_VK_CMD_BEGIN_RENDERING) {
    schedulerFunc(new CvkCmdBeginRendering(commandBuffer, pRenderingInfo));
  } else if (ID == CFunction::ID_VK_CMD_BEGIN_RENDERING_KHR) {
    schedulerFunc(new CvkCmdBeginRenderingKHR(commandBuffer, pRenderingInfo));
  }
}

#endif
} // namespace Vulkan
} // namespace gits
