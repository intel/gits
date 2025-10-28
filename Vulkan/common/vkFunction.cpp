// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vkFunction.h"
#include "vulkanFunctions.h"
#include "vulkanPreToken.h"

namespace gits {
namespace Vulkan {
/**
    * @brief Creates Vulkan function call wrapper
    *
    * Method creates Vulkan function call wrappers based on unique
    * identifier.
    *
    * @param id Unique Vulkan function identifer.
    *
    * @exception EOperationFailed Unknown Vulkan function identifier
    *
    * @return Vulkan function call wrapper.
    */
CFunction* CFunction::Create(unsigned id) {
  if (id < CFunction::ID_VULKAN || id >= CFunction::ID_FUNCTION_END) {
    return nullptr;
  }
  switch (id) {
  case ID_GITS_VK_WINDOW_CREATOR:
    return new CGitsVkCreateNativeWindow;
  case ID_GITS_VK_MEMORY_UPDATE:
    return new CGitsVkMemoryUpdate;
  case ID_GITS_VK_MEMORY_RESTORE:
    return new CGitsVkMemoryRestore;
  case ID_GITS_VK_WINDOW_UPDATE:
    return new CGitsVkUpdateNativeWindow;
  case ID_GITS_VK_MEMORY_UPDATE2:
    return new CGitsVkMemoryUpdate2;
  case ID_VK_DESTROY_VULKAN_DESCRIPTOR_SETS:
    return new CDestroyVulkanDescriptorSets;
  case ID_VK_DESTROY_VULKAN_COMMAND_BUFFERS:
    return new CDestroyVulkanCommandBuffers;
  case ID_GITS_VK_MEMORY_RESET:
    return new CGitsVkMemoryReset;
  case ID_GITS_VK_ENUMERATE_DISPLAY_MONITORS:
    return new CGitsVkEnumerateDisplayMonitors;
  case ID_GITS_VK_CMD_INITIALIZE_IMAGE_INTEL:
    return new CGitsInitializeImage;
  case ID_GITS_VK_CMD_INITIALIZE_BUFFER_INTEL:
    return new CGitsInitializeBuffer;
  case ID_GITS_VK_STATE_RESTORE_INFO:
    return new CGitsVkStateRestoreInfo;
  case ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS:
    return new CGitsVkCmdInsertMemoryBarriers;
  case ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_IMAGES_INTEL:
    return new CGitsInitializeMultipleImages;
  case ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_BUFFERS_INTEL:
    return new CGitsInitializeMultipleBuffers;
  case ID_GITS_VK_XLIB_WINDOW_CREATOR:
    return new CGitsVkCreateXlibWindow;
  case ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS_2:
    return new CGitsVkCmdInsertMemoryBarriers2;
  case ID_GITS_VK_CMD_PATCH_DEVICE_ADDRESSES:
    return new CGitsVkCmdPatchDeviceAddresses;
#include "vulkanIDswitch.h"
  default:;
  }
  LOG_ERROR << "Unknown Vulkan function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

CArgument& CFunction::Result(unsigned idx) {
  LOG_ERROR << "Results not supported in Vulkan!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

VkCommandBuffer CFunction::CommandBuffer() {
  return VK_NULL_HANDLE;
}

CQueueSubmitFunction::CQueueSubmitFunction() {}

void CQueueSubmitFunction::Trace() {
  LOG_FORMAT_RAW
  LOG_TRACE << "QueueSubmit nr: " << CGits::Instance().vkCounters.CurrentQueueSubmitCount() << " ";
}

void CQueueSubmitFunction::CountUp() {
  CGits::Instance().vkCounters.QueueSubmitCountUp();
}

void CQueueSubmitFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}

CImageFunction::CImageFunction() {}

void CImageFunction::Trace() {
  LOG_FORMAT_RAW
  LOG_TRACE << "Image nr: " << CGits::Instance().vkCounters.CurrentImageCount() << " ";
}

void CImageFunction::CountUp() {
  CGits::Instance().vkCounters.ImageCountUp();
}

void CImageFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}

CBufferFunction::CBufferFunction() {}

void CBufferFunction::Trace() {
  LOG_FORMAT_RAW
  LOG_TRACE << "Buffer nr: " << CGits::Instance().vkCounters.CurrentBufferCount() << " ";
}

void CBufferFunction::CountUp() {
  CGits::Instance().vkCounters.BufferCountUp();
}

void CBufferFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}
} // namespace Vulkan
} // namespace gits
