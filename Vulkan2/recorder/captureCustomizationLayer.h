// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "orderingRecorder.h"
#include "vulkanHeader2.h"
#include <optional>

namespace gits {
namespace vulkan {

class CaptureManager;

class CaptureCustomizationLayer : public Layer {
public:
  CaptureCustomizationLayer(CaptureManager& manager, stream::OrderingRecorder& recorder)
      : Layer("CaptureCustomization"), m_Manager(manager), m_Recorder(recorder) {}
#ifdef VK_USE_PLATFORM_WIN32_KHR
  void Pre(vkCreateWin32SurfaceKHRCommand& command) override;
#endif

  void Post(vkCreateDeviceCommand& command) override;
  void Post(vkGetPhysicalDeviceMemoryPropertiesCommand& command) override;
  void Post(vkGetPhysicalDeviceMemoryProperties2Command& command) override;
  void Post(vkGetPhysicalDeviceMemoryProperties2KHRCommand& command) override;
  void Pre(vkAllocateMemoryCommand& command) override;
  void Post(vkAllocateMemoryCommand& command) override;
  void Post(vkFreeMemoryCommand& command) override;
  void Post(vkMapMemoryCommand& command) override;
  void Pre(vkUnmapMemoryCommand& command) override;
  void Pre(vkQueueSubmitCommand& command) override;

  void Post(vkCreateDescriptorUpdateTemplateCommand& command) override;
  void Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) override;
  void Pre(vkDestroyDescriptorUpdateTemplateCommand& command) override;
  void Pre(vkDestroyDescriptorUpdateTemplateKHRCommand& command) override;

  void Pre(vkUpdateDescriptorSetWithTemplateCommand& command) override;
  void Pre(vkUpdateDescriptorSetWithTemplateKHRCommand& command) override;
  void Pre(vkCmdPushDescriptorSetWithTemplateCommand& command) override;
  void Pre(vkCmdPushDescriptorSetWithTemplateKHRCommand& command) override;

private:
  struct AllocateInfo {
    // Pointer to the original data
    VkMemoryAllocateInfo* AllocateInfoPtr{nullptr};
    // Local copy of the data
    VkMemoryAllocateInfo AllocateInfoModified{};

    void* ExternalMemory{nullptr};
    // Owned per-thread to avoid shared state race between concurrent vkAllocateMemory calls
    std::optional<VkImportMemoryHostPointerInfoEXT> HostPointerInfo;

    AllocateInfo() = default;
    AllocateInfo(VkMemoryAllocateInfo* p) {
      this->AllocateInfoPtr = p;
      this->AllocateInfoModified = *p;
    }
  };

  static thread_local AllocateInfo s_AllocateInfo;
  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
};

} // namespace vulkan
} // namespace gits
