// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineStateSnapshot.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "vulkanStateDocument.h"

#include <cstring>
#include <new>

namespace gits::vulkan {

PipelineStateSnapshot::PipelineStateSnapshot(Type type) : m_Type(type) {}

template <typename T>
T* PipelineStateSnapshot::CopyObject(const T& source) {
  void* memory = m_Arena.allocate(sizeof(T), alignof(T));
  return std::construct_at(static_cast<T*>(memory), source);
}

template <typename T>
T* PipelineStateSnapshot::CopyArray(const T* source, size_t count) {
  if (source == nullptr || count == 0) {
    return nullptr;
  }
  auto* destination = static_cast<T*>(m_Arena.allocate(sizeof(T) * count, alignof(T)));
  std::uninitialized_copy_n(source, count, destination);
  return destination;
}

char* PipelineStateSnapshot::CopyString(const char* source) {
  if (source == nullptr) {
    return nullptr;
  }
  const size_t size = std::strlen(source) + 1;
  auto* destination = static_cast<char*>(m_Arena.allocate(size, alignof(char)));
  std::memcpy(destination, source, size);
  return destination;
}

void* PipelineStateSnapshot::CopyBytes(const void* source, size_t size) {
  if (source == nullptr || size == 0) {
    return nullptr;
  }
  void* destination = m_Arena.allocate(size, alignof(std::max_align_t));
  std::memcpy(destination, source, size);
  return destination;
}

const void* PipelineStateSnapshot::CopyPNext(const void* chain, bool& supported) {
  if (chain == nullptr || !supported) {
    return nullptr;
  }
  const auto* base = static_cast<const VkBaseInStructure*>(chain);
  switch (base->sType) {
  case VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO: {
    const auto& source = *reinterpret_cast<const VkPipelineRenderingCreateInfo*>(chain);
    auto* destination = CopyObject(source);
    destination->pNext = CopyPNext(source.pNext, supported);
    destination->pColorAttachmentFormats =
        CopyArray(source.pColorAttachmentFormats, source.colorAttachmentCount);
    return destination;
  }
  case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO: {
    const auto& source =
        *reinterpret_cast<const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo*>(chain);
    auto* destination = CopyObject(source);
    destination->pNext = CopyPNext(source.pNext, supported);
    return destination;
  }
  case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO: {
    const auto& source = *reinterpret_cast<const VkShaderModuleCreateInfo*>(chain);
    auto* destination = CopyObject(source);
    destination->pNext = CopyPNext(source.pNext, supported);
    destination->pCode = static_cast<const uint32_t*>(CopyBytes(source.pCode, source.codeSize));
    return destination;
  }
  case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT: {
    const auto& source =
        *reinterpret_cast<const VkPipelineShaderStageModuleIdentifierCreateInfoEXT*>(chain);
    auto* destination = CopyObject(source);
    destination->pNext = CopyPNext(source.pNext, supported);
    destination->pIdentifier =
        static_cast<const uint8_t*>(CopyBytes(source.pIdentifier, source.identifierSize));
    return destination;
  }
  default:
    supported = false;
    return nullptr;
  }
}

bool PipelineStateSnapshot::CopyStage(VkPipelineShaderStageCreateInfo& destination,
                                      const VkPipelineShaderStageCreateInfo& source) {
  destination = source;
  bool supported = true;
  destination.pNext = CopyPNext(source.pNext, supported);
  destination.pName = CopyString(source.pName);
  if (source.pSpecializationInfo != nullptr) {
    auto* specialization = CopyObject(*source.pSpecializationInfo);
    specialization->pMapEntries = CopyArray(source.pSpecializationInfo->pMapEntries,
                                            source.pSpecializationInfo->mapEntryCount);
    specialization->pData =
        CopyBytes(source.pSpecializationInfo->pData, source.pSpecializationInfo->dataSize);
    destination.pSpecializationInfo = specialization;
  }
  return supported;
}

bool PipelineStateSnapshot::CopyGraphics(const VkGraphicsPipelineCreateInfo& source) {
  if (source.stageCount != 0 && source.pStages == nullptr) {
    return false;
  }
  m_Graphics = CopyObject(source);
  bool supported = true;
  m_Graphics->pNext = CopyPNext(source.pNext, supported);
  if (!supported) {
    return false;
  }

  auto* stages = CopyArray(source.pStages, source.stageCount);
  m_Graphics->pStages = stages;
  for (uint32_t i = 0; i < source.stageCount; ++i) {
    if (!CopyStage(stages[i], source.pStages[i])) {
      return false;
    }
  }

  if (source.pVertexInputState != nullptr) {
    auto* state = CopyObject(*source.pVertexInputState);
    state->pNext = CopyPNext(source.pVertexInputState->pNext, supported);
    state->pVertexBindingDescriptions =
        CopyArray(source.pVertexInputState->pVertexBindingDescriptions,
                  source.pVertexInputState->vertexBindingDescriptionCount);
    state->pVertexAttributeDescriptions =
        CopyArray(source.pVertexInputState->pVertexAttributeDescriptions,
                  source.pVertexInputState->vertexAttributeDescriptionCount);
    m_Graphics->pVertexInputState = state;
  }
  if (source.pInputAssemblyState != nullptr) {
    auto* state = CopyObject(*source.pInputAssemblyState);
    state->pNext = CopyPNext(source.pInputAssemblyState->pNext, supported);
    m_Graphics->pInputAssemblyState = state;
  }
  if (source.pTessellationState != nullptr) {
    auto* state = CopyObject(*source.pTessellationState);
    state->pNext = CopyPNext(source.pTessellationState->pNext, supported);
    m_Graphics->pTessellationState = state;
  }
  const bool discard =
      source.pRasterizationState != nullptr &&
      source.pRasterizationState->rasterizerDiscardEnable &&
      !HasDynamicState(source.pDynamicState, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE);
  m_Graphics->pViewportState = nullptr;
  if (!discard && source.pViewportState != nullptr) {
    auto* state = CopyObject(*source.pViewportState);
    state->pNext = CopyPNext(source.pViewportState->pNext, supported);
    if (HasDynamicState(source.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT)) {
      state->viewportCount = 0;
    }
    if (HasDynamicState(source.pDynamicState, VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT)) {
      state->scissorCount = 0;
    }
    state->pViewports = HasDynamicState(source.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT)
                            ? nullptr
                            : CopyArray(source.pViewportState->pViewports, state->viewportCount);
    state->pScissors = HasDynamicState(source.pDynamicState, VK_DYNAMIC_STATE_SCISSOR)
                           ? nullptr
                           : CopyArray(source.pViewportState->pScissors, state->scissorCount);
    m_Graphics->pViewportState = state;
  }
  if (source.pRasterizationState != nullptr) {
    auto* state = CopyObject(*source.pRasterizationState);
    state->pNext = CopyPNext(source.pRasterizationState->pNext, supported);
    m_Graphics->pRasterizationState = state;
  }
  if (source.pMultisampleState != nullptr) {
    auto* state = CopyObject(*source.pMultisampleState);
    state->pNext = CopyPNext(source.pMultisampleState->pNext, supported);
    const uint32_t maskWords =
        (static_cast<uint32_t>(source.pMultisampleState->rasterizationSamples) + 31u) / 32u;
    state->pSampleMask = CopyArray(source.pMultisampleState->pSampleMask, maskWords);
    m_Graphics->pMultisampleState = state;
  }
  if (source.pDepthStencilState != nullptr) {
    auto* state = CopyObject(*source.pDepthStencilState);
    state->pNext = CopyPNext(source.pDepthStencilState->pNext, supported);
    m_Graphics->pDepthStencilState = state;
  }
  if (source.pColorBlendState != nullptr) {
    auto* state = CopyObject(*source.pColorBlendState);
    state->pNext = CopyPNext(source.pColorBlendState->pNext, supported);
    state->pAttachments =
        CopyArray(source.pColorBlendState->pAttachments, source.pColorBlendState->attachmentCount);
    m_Graphics->pColorBlendState = state;
  }
  if (source.pDynamicState != nullptr) {
    auto* state = CopyObject(*source.pDynamicState);
    state->pNext = CopyPNext(source.pDynamicState->pNext, supported);
    state->pDynamicStates =
        CopyArray(source.pDynamicState->pDynamicStates, source.pDynamicState->dynamicStateCount);
    m_Graphics->pDynamicState = state;
  }
  return supported;
}

bool PipelineStateSnapshot::CopyCompute(const VkComputePipelineCreateInfo& source) {
  m_Compute = CopyObject(source);
  bool supported = true;
  m_Compute->pNext = CopyPNext(source.pNext, supported);
  return supported && CopyStage(m_Compute->stage, source.stage);
}

std::shared_ptr<PipelineStateSnapshot> PipelineStateSnapshot::Encode(
    const VkGraphicsPipelineCreateInfo& createInfo) {
  auto snapshot = std::shared_ptr<PipelineStateSnapshot>(new PipelineStateSnapshot(Type::Graphics));
  return snapshot->CopyGraphics(createInfo) ? snapshot : nullptr;
}

std::shared_ptr<PipelineStateSnapshot> PipelineStateSnapshot::Encode(
    const VkComputePipelineCreateInfo& createInfo) {
  auto snapshot = std::shared_ptr<PipelineStateSnapshot>(new PipelineStateSnapshot(Type::Compute));
  return snapshot->CopyCompute(createInfo) ? snapshot : nullptr;
}

PipelineStateSnapshot::Type PipelineStateSnapshot::GetType() const {
  return m_Type;
}

const VkGraphicsPipelineCreateInfo* PipelineStateSnapshot::Graphics() const {
  return m_Graphics;
}

const VkComputePipelineCreateInfo* PipelineStateSnapshot::Compute() const {
  return m_Compute;
}

VkGraphicsPipelineCreateInfo* PipelineStateSnapshot::MutableGraphics() {
  return m_Graphics;
}

VkComputePipelineCreateInfo* PipelineStateSnapshot::MutableCompute() {
  return m_Compute;
}

std::shared_ptr<PipelineStateSnapshot> PipelineStateSnapshot::Clone() const {
  return m_Type == Type::Graphics ? Encode(*m_Graphics) : Encode(*m_Compute);
}

} // namespace gits::vulkan
