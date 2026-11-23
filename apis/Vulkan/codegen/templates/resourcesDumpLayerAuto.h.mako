// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "resourcesDumpService.h"
#include "submitSignalsDeferral.h"

namespace gits {
namespace vulkan {

class DispatchTablesHolder;

class ResourcesDumpLayer : public Layer {
public:
  static constexpr const char* LAYER_NAME = "ResourcesDump";

  explicit ResourcesDumpLayer(DispatchTablesHolder& dispatchTablesHolder);

  // Implemented in resourcesDumpLayerCustom.cpp.
  void Post(RestoreContentDataCommand& command) override;
  void Post(StateRestoreBeginCommand& command) override;
  void Post(StateRestoreEndCommand& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
  void Pre(vkDestroyDeviceCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkCreateBufferCommand& command) override;
  void Pre(vkDestroyBufferCommand& command) override;
  void Post(vkBindBufferMemoryCommand& command) override;
  void Post(vkBindBufferMemory2Command& command) override;
  void Post(vkBindBufferMemory2KHRCommand& command) override;
  void Post(vkCreateImageCommand& command) override;
  void Pre(vkDestroyImageCommand& command) override;
  void Post(vkBindImageMemoryCommand& command) override;
  void Post(vkBindImageMemory2Command& command) override;
  void Post(vkBindImageMemory2KHRCommand& command) override;
  void Pre(vkCreateSwapchainKHRCommand& command) override;
  void Post(vkCreateSwapchainKHRCommand& command) override;
  void Post(vkGetSwapchainImagesKHRCommand& command) override;
  void Pre(vkDestroySwapchainKHRCommand& command) override;
  void Post(vkAcquireNextImageKHRCommand& command) override;
  void Post(vkAcquireNextImage2KHRCommand& command) override;
  void Post(vkReleaseSwapchainImagesKHRCommand& command) override;
  void Post(vkReleaseSwapchainImagesEXTCommand& command) override;
  void Post(vkCreateImageViewCommand& command) override;
  void Pre(vkDestroyImageViewCommand& command) override;
  void Post(vkCreateRenderPassCommand& command) override;
  void Post(vkCreateRenderPass2Command& command) override;
  void Post(vkCreateRenderPass2KHRCommand& command) override;
  void Pre(vkDestroyRenderPassCommand& command) override;
  void Post(vkCreateFramebufferCommand& command) override;
  void Pre(vkDestroyFramebufferCommand& command) override;
  void Post(vkCreateCommandPoolCommand& command) override;
  void Pre(vkDestroyCommandPoolCommand& command) override;
  void Pre(vkResetCommandPoolCommand& command) override;
  void Post(vkAllocateCommandBuffersCommand& command) override;
  void Pre(vkFreeCommandBuffersCommand& command) override;
  void Pre(vkBeginCommandBufferCommand& command) override;
  void Pre(vkResetCommandBufferCommand& command) override;
  void Pre(vkEndCommandBufferCommand& command) override;
  void Post(vkCreateSemaphoreCommand& command) override;
  void Pre(vkDestroySemaphoreCommand& command) override;
  void Pre(vkQueueSubmitCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;
  void Pre(vkQueueSubmit2Command& command) override;
  void Post(vkQueueSubmit2Command& command) override;
  void Pre(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueueBindSparseCommand& command) override;
  % for name in custom_commands:
  void Post(${name}Command& command) override;
  % endfor

  % for command in commands:
  % if is_command_buffer_command(command) and command.name not in custom_commands:
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  void Post(${command.name}Command& command) override;
  % if define:
#endif
  % endif
  % endif
  % endfor

private:
  template <typename CommandT>
  void PreSubmit(CommandT& command, SubmitSignals::Function function);
  template <typename CommandT>
  void PostSubmit(CommandT& command);

  ResourcesDumpService m_Service;
  SubmitSignalsDeferral m_SignalsDeferral;
};

} // namespace vulkan
} // namespace gits
