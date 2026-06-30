// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotsLayer.h"
#include "dispatchTableAuto.h"
#include "dispatchTablesHolder.h"
#include "log.h"
#include "configurator.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <memory>

namespace gits {
namespace vulkan {

ScreenshotsLayer::ScreenshotsLayer(DispatchTablesHolder& dispatchTablesHolder)
    : Layer(LAYER_NAME),
      m_DispatchTablesHolder(dispatchTablesHolder),
      m_ScreenshotRange(Configurator::Get().common.shared.screenshots.frames) {
  auto dumpPath = Configurator::Get().common.player.outputDir;
  if (Configurator::IsRecorder()) {
    dumpPath = Configurator::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
  } else if (Configurator::IsPlayer() && dumpPath.empty()) {
    dumpPath = Configurator::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
  }
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  m_DumpPath = dumpPath;
}

void ScreenshotsLayer::Post(vkCreateDeviceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pDevice.Value) {
    return;
  }
  m_DeviceToPhysicalDevice[*command.m_pDevice.Value] = command.m_physicalDevice.Value;
}

void ScreenshotsLayer::Post(vkCreateSwapchainKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    LOG_WARNING << "ScreenshotLayer: vkCreateSwapchainKHRCommand: the call to create swapchain "
                   "failed with return value "
                << command.m_Return.Value;
    return;
  }

  VkSwapchainKHR* swapchainPtr = command.m_pSwapchain.Value;
  if (swapchainPtr == nullptr) {
    LOG_WARNING << "ScreenshotLayer: skipping screenshots for swapchain = nullptr, commandKey = "
                << command.m_Key;
    return;
  }

  VkSwapchainKHR swapchain = *swapchainPtr;
  VkDevice device = command.m_device.Value;
  const VkSwapchainCreateInfoKHR* pCreateInfo = command.m_pCreateInfo.Value;

  SwapchainInfo info;
  info.Device = device;
  info.Format = pCreateInfo->imageFormat;
  info.Extent = pCreateInfo->imageExtent;

  auto* deviceDispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  if (deviceDispatchTable == nullptr) {
    LOG_ERROR << "ScreenshotLayer: could not find the device dispatch table";
    return;
  }

  auto physDevIt = m_DeviceToPhysicalDevice.find(device);
  if (physDevIt == m_DeviceToPhysicalDevice.end()) {
    LOG_ERROR << "ScreenshotLayer: could not find the physicalDevice by device";
    return;
  }
  VkPhysicalDevice physicalDevice = physDevIt->second;

  auto* instanceDispatchTable = m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  if (instanceDispatchTable == nullptr) {
    LOG_ERROR << "ScreenshotLayer: could not find the instance dispatch table";
    return;
  }

  uint32_t imageCount = 0;
  deviceDispatchTable->vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  info.Images.resize(imageCount);
  deviceDispatchTable->vkGetSwapchainImagesKHR(device, swapchain, &imageCount, info.Images.data());

  VkPhysicalDeviceMemoryProperties memProperties{};
  instanceDispatchTable->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  info.Dumper = std::make_unique<SwapchainImagesDumper>(device, *pCreateInfo, memProperties,
                                                        *deviceDispatchTable);
  info.Dumper->AllocateBuffers();
  info.Dumper->StartWorkerThread();

  m_SwapchainInfos[swapchain] = std::move(info);
}

void ScreenshotsLayer::Pre(vkDestroySwapchainKHRCommand& command) {
  // Runs before the driver destroys the swapchain, so the device and swapchain
  // images are still valid here.  Erasing the entry destroys the
  // SwapchainImagesDumper, whose destructor joins its worker thread (writing any
  // pending screenshot, e.g. the last frame's) and frees its Vulkan resources.
  auto it = m_SwapchainInfos.find(command.m_swapchain.Value);
  if (it != m_SwapchainInfos.end()) {
    m_SwapchainInfos.erase(it);
  }
}

void ScreenshotsLayer::Pre(vkQueuePresentKHRCommand& command) {
  ++m_CurrentFrame;
  if (!m_ScreenshotRange[m_CurrentFrame]) {
    return;
  }

  VkQueue queue = command.m_queue.Value;
  const VkPresentInfoKHR* presentInfo = command.m_pPresentInfo.Value;
  if (!presentInfo) {
    LOG_ERROR << "ScreenshotLayer: command.m_pPresentInfo.Value is null";
    return;
  }

  bool hasValidSwapchain = false;
  for (uint32_t i = 0; i < presentInfo->swapchainCount; ++i) {
    VkSwapchainKHR swapchain = presentInfo->pSwapchains[i];
    uint32_t imageIndex = presentInfo->pImageIndices[i];

    auto it = m_SwapchainInfos.find(swapchain);
    if (it == m_SwapchainInfos.end()) {
      continue;
    }
    hasValidSwapchain = true;
    SwapchainInfo& swapInfo = it->second;
    VkImage image = swapInfo.Images[imageIndex];

    std::ostringstream outputName;
    outputName << m_DumpPath.string() << "/frame" << std::setw(8) << std::setfill('0')
               << m_CurrentFrame;
    if (presentInfo->swapchainCount > 1) {
      outputName << "_swap" << i;
    }

    swapInfo.Dumper->SubmitCommandBuffer(queue, image, outputName.str());
  }

  if (!hasValidSwapchain) {
    LOG_ERROR << "ScreenshotLayer: no valid swapchain found for frame: " << m_CurrentFrame;
  }
}

} // namespace vulkan
} // namespace gits
