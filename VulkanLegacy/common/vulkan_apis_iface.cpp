// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gits.h"
#include "vulkan_apis_iface.h"
#include "vulkanStateDynamic.h"

namespace gits {
namespace Vulkan {

void VulkanApi::Play_SwapAfterPrepare() const {
  auto swapchain = SD()._swapchainkhrstates.begin();
  auto device = swapchain->second->deviceStateStore->deviceHandle;
  uint32_t imageIndex = 0;
  drvVk.vkAcquireNextImageKHR(device, swapchain->first, 2000000000, VK_NULL_HANDLE, VK_NULL_HANDLE,
                              &imageIndex);

  VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                  nullptr,
                                  0,
                                  nullptr,
                                  1,
                                  &swapchain->first,
                                  &imageIndex,
                                  nullptr};
  drvVk.vkDeviceWaitIdle(device);
  drvVk.vkGetDeviceProcAddr(device, VK_SWAP_AFTER_PREPARE_GITS_FUNCTION_NAME);
  drvVk.vkQueuePresentKHR(SD()._devicestates[device]->queueStateStoreList.front()->queueHandle,
                          &presentInfo);
  drvVk.vkDeviceWaitIdle(device);
}

void VulkanApi::Play_StateRestoreBegin() const {
  drvVk.vkGetInstanceProcAddr(VK_NULL_HANDLE, VK_STATE_RESTORE_BEGIN_GITS_FUNCTION_NAME);
}

void VulkanApi::Play_StateRestoreEnd() const {
  drvVk.vkGetInstanceProcAddr(VK_NULL_HANDLE, VK_STATE_RESTORE_END_GITS_FUNCTION_NAME);
}

void VulkanApi::Rec_StateRestoreFinished() const {
  SD().stateRestoreFinished = true;
}

bool VulkanApi::CfgRec_IsObjectToRecord() const {
  return Configurator::Get()
      .vulkan.recorder.objRange
      .rangeSpecial[CGits::Instance().vkCounters.CurrentQueueSubmitCount()];
}

} // namespace Vulkan
} // namespace gits
