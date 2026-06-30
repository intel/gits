// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandPlayersAuto.h"
#include "commandsAuto.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "handleMapService.h"
#include "handleArgumentUpdaters.h"

namespace gits {
namespace vulkan {

<%
structs_required_update = [
  'VkCommandBufferAllocateInfo',
  'VkDescriptorSetAllocateInfo',
  'VkImageViewCreateInfo',
  'VkBufferViewCreateInfo',
  'VkRenderingInfo',
  'VkPipelineLayoutCreateInfo',
  'VkDescriptorSetLayoutCreateInfo',
  'VkSwapchainCreateInfoKHR',
  'VkFramebufferCreateInfo',
  'VkRenderPassBeginInfo',
  'VkPresentInfoKHR',
  'VkGraphicsPipelineCreateInfo',
  'VkComputePipelineCreateInfo',
  'VkWriteDescriptorSet',
  'VkCopyDescriptorSet',
  'VkImageMemoryBarrier',
  'VkBufferMemoryBarrier',
  'VkSubmitInfo',
  'VkSubmitInfo2',
  'VkImageMemoryRequirementsInfo2',
  'VkDescriptorUpdateTemplateCreateInfo',
  'VkDependencyInfo',
  'VkBufferMemoryRequirementsInfo2',
  'VkBufferDeviceAddressInfo',
  'VkMemoryAllocateInfo',
  'VkMappedMemoryRange'
]
%>\
% for command in commands:
<%
args = generate_args(command)
define = get_define(command.platform)
dispatch_table = get_dispatch_table(command)
%>\
% if define:
#ifdef ${define}
% endif
void ${command.name}Player::Run() {
  auto& manager = PlayerManager::Get();

  % for param in command.params:
  % if param.is_handle:
  UpdateHandle(manager, command.m_${param.name});
  % elif param.base_type in structs_required_update:
  UpdateHandle(manager, command.m_${param.name});
  % endif
  % endfor

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ${'command.m_Return.Value = ' if command.return_type != 'void' else ''}manager.${dispatch_table}(${f'command.m_{command.params[0].name}.Value' if dispatch_table != 'GetGlobalDispatchTable' else ''}).${command.name}(
      % for arg in args:
      ${arg}${',' if not loop.last else ''}
      % endfor
	);


    % for param in command.params:
    % if param.is_handle_output:
    UpdateOutputHandle(manager, command.m_${param.name});
    % endif
    % endfor
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}
% if define:
#endif
% endif
% endfor
} // namespace vulkan
} // namespace gits
