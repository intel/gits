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

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <memory>
#include <vector>

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
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_DeviceToPhysicalDevice[*command.m_pDevice.Value] = command.m_physicalDevice.Value;
}

void ScreenshotsLayer::Post(vkGetDeviceQueueCommand& command) {
  if (command.m_pQueue.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_QueueToFamily[*command.m_pQueue.Value] = command.m_queueFamilyIndex.Value;
}

void ScreenshotsLayer::Post(vkGetDeviceQueue2Command& command) {
  if (command.m_pQueue.Value == nullptr || command.m_pQueueInfo.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_QueueToFamily[*command.m_pQueue.Value] = command.m_pQueueInfo.Value->queueFamilyIndex;
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

  // Get the device and queue indices under a lock
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  std::vector<uint32_t> queueFamilyIndices;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto physDevIt = m_DeviceToPhysicalDevice.find(device);
    if (physDevIt == m_DeviceToPhysicalDevice.end()) {
      LOG_ERROR << "ScreenshotLayer: could not find the physicalDevice by device";
      return;
    }
    physicalDevice = physDevIt->second;

    // A swapchain may be presented from more than one queue family
    for (const auto& [queue, queueFamilyIndex] : m_QueueToFamily) {
      if (std::find(queueFamilyIndices.begin(), queueFamilyIndices.end(), queueFamilyIndex) ==
          queueFamilyIndices.end()) {
        queueFamilyIndices.push_back(queueFamilyIndex);
      }
    }
  }

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
  info.Dumper->AllocateBuffers(queueFamilyIndices);
  info.Dumper->StartWorkerThread();

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_SwapchainInfos[swapchain] = std::move(info);
  }
}

void ScreenshotsLayer::Pre(vkDestroySwapchainKHRCommand& command) {
  // Extract the dumper under the lock, then let it destruct
  std::unique_ptr<SwapchainImagesDumper> dumper;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_SwapchainInfos.find(command.m_swapchain.Value);
    if (it == m_SwapchainInfos.end()) {
      return;
    }
    dumper = std::move(it->second.Dumper);
    m_SwapchainInfos.erase(it);
  }
}

void ScreenshotsLayer::Pre(vkQueuePresentKHRCommand& command) {
  const unsigned frame = m_CurrentFrame.fetch_add(1) + 1;
  if (!m_ScreenshotRange[frame]) {
    return;
  }

  VkQueue queue = command.m_queue.Value;
  const VkPresentInfoKHR* presentInfo = command.m_pPresentInfo.Value;
  if (!presentInfo) {
    LOG_ERROR << "ScreenshotLayer: command.m_pPresentInfo.Value is null";
    return;
  }

  // Guard the shared maps
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto familyIt = m_QueueToFamily.find(queue);
  if (familyIt == m_QueueToFamily.end()) {
    LOG_ERROR << "ScreenshotLayer: unknown queue family for present queue; cannot dump screenshot "
                 "for frame: "
              << frame;
    return;
  }
  const uint32_t queueFamilyIndex = familyIt->second;

  // Consume the wait semaphores in copy operation
  uint32_t pendingWaitCount = presentInfo->waitSemaphoreCount;
  const VkSemaphore* pendingWaitSemaphores = presentInfo->pWaitSemaphores;
  bool waitSemaphoresConsumed = false;

  bool hasValidSwapchain = false;
  for (uint32_t i = 0; i < presentInfo->swapchainCount; ++i) {
    auto it = m_SwapchainInfos.find(presentInfo->pSwapchains[i]);
    if (it == m_SwapchainInfos.end()) {
      continue;
    }
    hasValidSwapchain = true;
    SwapchainInfo& swapInfo = it->second;
    VkImage image = swapInfo.Images[presentInfo->pImageIndices[i]];

    std::ostringstream outputName;
    outputName << m_DumpPath.string() << "/frame" << std::setw(8) << std::setfill('0') << frame;
    if (presentInfo->swapchainCount > 1) {
      outputName << "_swap" << i;
    }

    const bool submitted = swapInfo.Dumper->SubmitCommandBuffer(
        queue, queueFamilyIndex, image, outputName.str(), pendingWaitCount, pendingWaitSemaphores);
    if (submitted && pendingWaitCount > 0) {
      waitSemaphoresConsumed = true;
      pendingWaitCount = 0;
      pendingWaitSemaphores = nullptr;
    }
  }

  // Avoid waiting on them semaphores twice
  if (waitSemaphoresConsumed) {
    command.m_pPresentInfo.Value->waitSemaphoreCount = 0;
    command.m_pPresentInfo.Value->pWaitSemaphores = nullptr;
  }

  if (!hasValidSwapchain) {
    LOG_ERROR << "ScreenshotLayer: no valid swapchain found for frame: " << frame;
  }
}

} // namespace vulkan
} // namespace gits
