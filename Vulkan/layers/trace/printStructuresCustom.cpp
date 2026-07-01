// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printStructuresCustom.h"
#include "printBitmasksAuto.h"
#include "printEnumsAuto.h"
#include "printPnextAuto.h"
#include "printCustom.h"

namespace gits {
namespace vulkan {

FastOStream& operator<<(FastOStream& stream, const VkGraphicsPipelineCreateInfo& value) {
  stream << "VkGraphicsPipelineCreateInfo{";
  stream << value.sType << ", ";
  PrintPNext(stream, value.pNext) << ", ";
  stream << value.flags << ", ";
  stream << value.stageCount << ", ";
  if (value.stageCount > 0) {
    stream << value.pStages << ", ";
  } else {
    stream << "nullptr, ";
  }
  stream << value.pVertexInputState << ", ";
  stream << value.pInputAssemblyState << ", ";

  // pTessellationState is only needed when tessellation stages are present
  bool hasTessellation = false;
  for (uint32_t i = 0; i < value.stageCount; ++i) {
    if (value.pStages[i].stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ||
        value.pStages[i].stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
      hasTessellation = true;
      break;
    }
  }
  if (hasTessellation) {
    stream << value.pTessellationState << ", ";
  } else {
    stream << "nullptr, ";
  }

  // pViewportState, pMultisampleState, pDepthStencilState, and pColorBlendState
  // are ignored when rasterization is disabled
  bool rasterizerDiscard =
      value.pRasterizationState && value.pRasterizationState->rasterizerDiscardEnable == VK_TRUE;

  stream << value.pRasterizationState << ", ";

  if (!rasterizerDiscard) {
    stream << value.pViewportState << ", ";
    stream << value.pMultisampleState << ", ";
    stream << value.pDepthStencilState << ", ";
    stream << value.pColorBlendState << ", ";
  } else {
    stream << "nullptr, ";
    stream << "nullptr, ";
    stream << "nullptr, ";
    stream << "nullptr, ";
  }

  stream << value.pDynamicState << ", ";

  stream << value.layout << ", ";
  stream << value.renderPass << ", ";
  stream << value.subpass << ", ";
  stream << value.basePipelineHandle << ", ";
  stream << value.basePipelineIndex;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const VkGraphicsPipelineCreateInfo* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const VkWriteDescriptorSet& value) {
  stream << "VkWriteDescriptorSet{";
  stream << value.sType << ", ";
  PrintPNext(stream, value.pNext) << ", ";
  stream << value.dstSet << ", ";
  stream << value.dstBinding << ", ";
  stream << value.dstArrayElement << ", ";
  stream << value.descriptorCount << ", ";
  stream << value.descriptorType << ", ";
  // Print only the structures that are relevant for the descriptor type
  if (value.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
      value.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
      value.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
      value.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
      value.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
    stream << value.pImageInfo;
  } else if (value.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
             value.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
             value.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
             value.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
    stream << value.pBufferInfo;
  } else if (value.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
             value.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
    stream << value.pTexelBufferView;
  } else {
    stream << "unhandled_descriptor_type(" << value.descriptorType << ")";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const VkWriteDescriptorSet* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const VkSubmitInfo& value) {
  stream << "VkSubmitInfo{";
  stream << value.sType << ", ";
  PrintPNext(stream, value.pNext) << ", ";
  stream << value.waitSemaphoreCount << ", ";
  if (value.waitSemaphoreCount > 0) {
    stream << value.pWaitSemaphores << ", ";
    // pWaitDstStageMask is only valid when waitSemaphoreCount > 0
    PrintVkPipelineStageFlags(stream, value.pWaitDstStageMask) << ", ";
  } else {
    stream << "nullptr, ";
    stream << "nullptr, ";
  }
  stream << value.commandBufferCount << ", ";
  if (value.commandBufferCount > 0) {
    stream << value.pCommandBuffers << ", ";
  } else {
    stream << "nullptr, ";
  }
  stream << value.signalSemaphoreCount << ", ";
  if (value.signalSemaphoreCount > 0) {
    stream << value.pSignalSemaphores;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const VkSubmitInfo* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

} // namespace vulkan
} // namespace gits
