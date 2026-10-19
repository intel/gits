// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateDocument.h"

#include "rtsr/shared/rtsrCommon.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <utility>

namespace gits::vulkan {
namespace {

using Json = nlohmann::json;

rtsr::RTSR_SHADER_STAGE ToRtsrStage(VkShaderStageFlagBits stage) {
  switch (stage) {
  case VK_SHADER_STAGE_VERTEX_BIT:
    return rtsr::RTSR_SHADER_STAGE_VERTEX;
  case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    return rtsr::RTSR_SHADER_STAGE_TESSELLATION_CONTROL;
  case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    return rtsr::RTSR_SHADER_STAGE_TESSELLATION_EVALUATION;
  case VK_SHADER_STAGE_GEOMETRY_BIT:
    return rtsr::RTSR_SHADER_STAGE_GEOMETRY;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    return rtsr::RTSR_SHADER_STAGE_FRAGMENT;
  case VK_SHADER_STAGE_COMPUTE_BIT:
    return rtsr::RTSR_SHADER_STAGE_COMPUTE;
  default:
    return rtsr::RTSR_SHADER_STAGE_UNKNOWN;
  }
}

void MarkUnsupported(VulkanStateDocument& document, VkStructureType type) {
  document.ReplacementEligible = false;
  if (!document.Diagnostic.empty()) {
    document.Diagnostic += "; ";
  }
  document.Diagnostic += "unsupported pNext sType " + std::to_string(static_cast<int64_t>(type));
}

Json DescribePNext(const void* chain, VulkanStateDocument& document) {
  Json result = Json::array();
  for (auto* node = static_cast<const VkBaseInStructure*>(chain); node != nullptr;
       node = node->pNext) {
    Json entry{{"sType", node->sType}};
    switch (node->sType) {
    case VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO: {
      const auto* info = reinterpret_cast<const VkPipelineRenderingCreateInfo*>(node);
      entry["viewMask"] = info->viewMask;
      entry["colorAttachmentFormats"] = Json::array();
      for (uint32_t i = 0; i < info->colorAttachmentCount; ++i) {
        entry["colorAttachmentFormats"].push_back(info->pColorAttachmentFormats[i]);
      }
      entry["depthAttachmentFormat"] = info->depthAttachmentFormat;
      entry["stencilAttachmentFormat"] = info->stencilAttachmentFormat;
      break;
    }
    case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT:
    case VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR:
      entry["replacement"] = "unsupported pipeline library state";
      document.ReplacementEligible = false;
      if (document.Diagnostic.empty()) {
        document.Diagnostic = "graphics pipeline libraries are not replaceable";
      }
      break;
    case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO:
      entry["requiredSubgroupSize"] =
          reinterpret_cast<const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo*>(node)
              ->requiredSubgroupSize;
      break;
    case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
    case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT:
      entry["replacement"] = "shader bytecode is tracked separately";
      break;
    default:
      MarkUnsupported(document, node->sType);
      entry["replacement"] = "unsupported";
      break;
    }
    result.push_back(std::move(entry));
  }
  return result;
}

Json DescribeSpecialization(const VkSpecializationInfo* specialization) {
  if (specialization == nullptr) {
    return nullptr;
  }
  Json entries = Json::array();
  for (uint32_t i = 0; i < specialization->mapEntryCount; ++i) {
    const auto& entry = specialization->pMapEntries[i];
    entries.push_back(
        {{"constantID", entry.constantID}, {"offset", entry.offset}, {"size", entry.size}});
  }
  const auto* bytes = static_cast<const uint8_t*>(specialization->pData);
  const uint64_t dataHash =
      bytes != nullptr ? rtsr::ComputeBytecodeHash(bytes, specialization->dataSize) : 0;
  return {{"entries", std::move(entries)},
          {"dataSize", specialization->dataSize},
          {"dataHash", dataHash}};
}

Json DescribeStages(const VkPipelineShaderStageCreateInfo* createStages,
                    uint32_t count,
                    const std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                    VulkanStateDocument& document) {
  Json result = Json::array();
  for (uint32_t i = 0; i < count; ++i) {
    const auto& createStage = createStages[i];
    Json entry{{"stage", createStage.stage},
               {"flags", createStage.flags},
               {"entrypoint", createStage.pName != nullptr ? createStage.pName : "main"},
               {"specialization", DescribeSpecialization(createStage.pSpecializationInfo)},
               {"pNext", DescribePNext(createStage.pNext, document)}};
    const auto resolved = std::find_if(stages.begin(), stages.end(), [&](const auto& shader) {
      return shader.ShaderStage == ToRtsrStage(createStage.stage);
    });
    if (resolved != stages.end()) {
      entry["bytecodeHash"] = resolved->BytecodeHash;
      entry["bytecodeSize"] = resolved->Bytecode.size();
    }
    result.push_back(std::move(entry));
  }
  return result;
}

Json DescribeVertexInput(const VkPipelineVertexInputStateCreateInfo* info,
                         VulkanStateDocument& document) {
  if (info == nullptr) {
    return nullptr;
  }
  Json bindings = Json::array();
  for (uint32_t i = 0; i < info->vertexBindingDescriptionCount; ++i) {
    const auto& binding = info->pVertexBindingDescriptions[i];
    bindings.push_back({{"binding", binding.binding},
                        {"stride", binding.stride},
                        {"inputRate", binding.inputRate}});
  }
  Json attributes = Json::array();
  for (uint32_t i = 0; i < info->vertexAttributeDescriptionCount; ++i) {
    const auto& attribute = info->pVertexAttributeDescriptions[i];
    attributes.push_back({{"location", attribute.location},
                          {"binding", attribute.binding},
                          {"format", attribute.format},
                          {"offset", attribute.offset}});
  }
  return {{"flags", info->flags},
          {"bindings", std::move(bindings)},
          {"attributes", std::move(attributes)},
          {"pNext", DescribePNext(info->pNext, document)}};
}

Json DescribeDynamicState(const VkPipelineDynamicStateCreateInfo* info,
                          VulkanStateDocument& document) {
  if (info == nullptr) {
    return nullptr;
  }
  Json states = Json::array();
  for (uint32_t i = 0; i < info->dynamicStateCount; ++i) {
    states.push_back(info->pDynamicStates[i]);
  }
  return {{"flags", info->flags},
          {"states", std::move(states)},
          {"pNext", DescribePNext(info->pNext, document)}};
}

Json DescribeStencil(const VkStencilOpState& stencil) {
  return {{"failOp", stencil.failOp},           {"passOp", stencil.passOp},
          {"depthFailOp", stencil.depthFailOp}, {"compareOp", stencil.compareOp},
          {"compareMask", stencil.compareMask}, {"writeMask", stencil.writeMask},
          {"reference", stencil.reference}};
}

VulkanStateDocument FinishDocument(Json state, VulkanStateDocument document, const char* type) {
  const std::string normalized = state.dump();
  document.Hash = rtsr::ComputeBytecodeHash(normalized.data(), normalized.size());
  Json root{{"Header",
             {{"Api", "Vulkan"},
              {"Version", 1},
              {"Type", type},
              {"Hash", document.Hash},
              {"ReplacementEligible", document.ReplacementEligible}}},
            {"State", std::move(state)}};
  if (!document.Diagnostic.empty()) {
    root["Header"]["Diagnostic"] = document.Diagnostic;
  }
  document.Json = root.dump();
  return document;
}

} // namespace

bool HasDynamicState(const VkPipelineDynamicStateCreateInfo* info, VkDynamicState state) {
  return info != nullptr && info->pDynamicStates != nullptr &&
         std::find(info->pDynamicStates, info->pDynamicStates + info->dynamicStateCount, state) !=
             info->pDynamicStates + info->dynamicStateCount;
}

VulkanStateDocument MakeGraphicsStateDocument(
    const VkGraphicsPipelineCreateInfo& info,
    const std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
    bool allShadersResolved,
    bool stateSnapshotAvailable,
    GITSKey layoutKey,
    GITSKey renderPassKey) {
  VulkanStateDocument document;
  if (!allShadersResolved) {
    document.ReplacementEligible = false;
    document.Diagnostic = "one or more shader modules could not be resolved";
  }
  if (!stateSnapshotAvailable) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "pipeline state contains unsupported extension data";
  }
  for (uint32_t i = 0; info.pStages != nullptr && i < info.stageCount; ++i) {
    if (ToRtsrStage(info.pStages[i].stage) == rtsr::RTSR_SHADER_STAGE_UNKNOWN) {
      document.ReplacementEligible = false;
      if (!document.Diagnostic.empty()) {
        document.Diagnostic += "; ";
      }
      document.Diagnostic += "mesh/task and extension shader stages are not replaceable";
      break;
    }
  }
  if (info.layout != VK_NULL_HANDLE && layoutKey == 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "pipeline layout key is unavailable";
  }
  if (info.renderPass != VK_NULL_HANDLE && renderPassKey == 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "render pass key is unavailable";
  }
  if ((info.flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR) != 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "pipeline-library objects are not replaceable";
  }
  if ((info.flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) != 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "derivative pipelines are not replaceable";
  }

  Json state{{"flags", info.flags},
             {"layoutKey", layoutKey},
             {"renderPassKey", renderPassKey},
             {"subpass", info.subpass},
             {"stages", DescribeStages(info.pStages, info.stageCount, stages, document)},
             {"vertexInput", DescribeVertexInput(info.pVertexInputState, document)},
             {"dynamicState", DescribeDynamicState(info.pDynamicState, document)},
             {"pNext", DescribePNext(info.pNext, document)}};
  if (info.pInputAssemblyState != nullptr) {
    state["inputAssembly"] = {
        {"flags", info.pInputAssemblyState->flags},
        {"topology", info.pInputAssemblyState->topology},
        {"primitiveRestartEnable", info.pInputAssemblyState->primitiveRestartEnable},
        {"pNext", DescribePNext(info.pInputAssemblyState->pNext, document)}};
  }
  if (info.pTessellationState != nullptr) {
    state["tessellation"] = {{"flags", info.pTessellationState->flags},
                             {"patchControlPoints", info.pTessellationState->patchControlPoints},
                             {"pNext", DescribePNext(info.pTessellationState->pNext, document)}};
  }
  const bool discard =
      info.pRasterizationState != nullptr && info.pRasterizationState->rasterizerDiscardEnable &&
      !HasDynamicState(info.pDynamicState, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE);
  if (!discard && info.pViewportState != nullptr) {
    const bool dynamicViewportCount =
        HasDynamicState(info.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
    const bool dynamicScissorCount =
        HasDynamicState(info.pDynamicState, VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
    Json viewports = Json::array();
    if (!dynamicViewportCount && !HasDynamicState(info.pDynamicState, VK_DYNAMIC_STATE_VIEWPORT) &&
        info.pViewportState->pViewports != nullptr) {
      for (uint32_t i = 0; i < info.pViewportState->viewportCount; ++i) {
        const auto& viewport = info.pViewportState->pViewports[i];
        viewports.push_back({{"x", viewport.x},
                             {"y", viewport.y},
                             {"width", viewport.width},
                             {"height", viewport.height},
                             {"minDepth", viewport.minDepth},
                             {"maxDepth", viewport.maxDepth}});
      }
    }
    Json scissors = Json::array();
    if (!dynamicScissorCount && !HasDynamicState(info.pDynamicState, VK_DYNAMIC_STATE_SCISSOR) &&
        info.pViewportState->pScissors != nullptr) {
      for (uint32_t i = 0; i < info.pViewportState->scissorCount; ++i) {
        const auto& scissor = info.pViewportState->pScissors[i];
        scissors.push_back({{"offset", {scissor.offset.x, scissor.offset.y}},
                            {"extent", {scissor.extent.width, scissor.extent.height}}});
      }
    }
    state["viewport"] = {
        {"flags", info.pViewportState->flags},
        {"viewportCount", dynamicViewportCount ? 0 : info.pViewportState->viewportCount},
        {"viewports", std::move(viewports)},
        {"scissorCount", dynamicScissorCount ? 0 : info.pViewportState->scissorCount},
        {"scissors", std::move(scissors)},
        {"pNext", DescribePNext(info.pViewportState->pNext, document)}};
  }
  if (info.pRasterizationState != nullptr) {
    state["rasterization"] = {
        {"flags", info.pRasterizationState->flags},
        {"depthClampEnable", info.pRasterizationState->depthClampEnable},
        {"rasterizerDiscardEnable", info.pRasterizationState->rasterizerDiscardEnable},
        {"polygonMode", info.pRasterizationState->polygonMode},
        {"cullMode", info.pRasterizationState->cullMode},
        {"frontFace", info.pRasterizationState->frontFace},
        {"depthBiasEnable", info.pRasterizationState->depthBiasEnable},
        {"depthBiasConstantFactor", info.pRasterizationState->depthBiasConstantFactor},
        {"depthBiasClamp", info.pRasterizationState->depthBiasClamp},
        {"depthBiasSlopeFactor", info.pRasterizationState->depthBiasSlopeFactor},
        {"lineWidth", info.pRasterizationState->lineWidth},
        {"pNext", DescribePNext(info.pRasterizationState->pNext, document)}};
  }
  if (info.pMultisampleState != nullptr) {
    Json sampleMask = Json::array();
    if (info.pMultisampleState->pSampleMask != nullptr) {
      const uint32_t words =
          (static_cast<uint32_t>(info.pMultisampleState->rasterizationSamples) + 31u) / 32u;
      for (uint32_t i = 0; i < words; ++i) {
        sampleMask.push_back(info.pMultisampleState->pSampleMask[i]);
      }
    }
    state["multisample"] = {
        {"flags", info.pMultisampleState->flags},
        {"rasterizationSamples", info.pMultisampleState->rasterizationSamples},
        {"sampleShadingEnable", info.pMultisampleState->sampleShadingEnable},
        {"minSampleShading", info.pMultisampleState->minSampleShading},
        {"sampleMask", std::move(sampleMask)},
        {"alphaToCoverageEnable", info.pMultisampleState->alphaToCoverageEnable},
        {"alphaToOneEnable", info.pMultisampleState->alphaToOneEnable},
        {"pNext", DescribePNext(info.pMultisampleState->pNext, document)}};
  }
  if (info.pDepthStencilState != nullptr) {
    state["depthStencil"] = {
        {"flags", info.pDepthStencilState->flags},
        {"depthTestEnable", info.pDepthStencilState->depthTestEnable},
        {"depthWriteEnable", info.pDepthStencilState->depthWriteEnable},
        {"depthCompareOp", info.pDepthStencilState->depthCompareOp},
        {"depthBoundsTestEnable", info.pDepthStencilState->depthBoundsTestEnable},
        {"stencilTestEnable", info.pDepthStencilState->stencilTestEnable},
        {"front", DescribeStencil(info.pDepthStencilState->front)},
        {"back", DescribeStencil(info.pDepthStencilState->back)},
        {"minDepthBounds", info.pDepthStencilState->minDepthBounds},
        {"maxDepthBounds", info.pDepthStencilState->maxDepthBounds},
        {"pNext", DescribePNext(info.pDepthStencilState->pNext, document)}};
  }
  if (info.pColorBlendState != nullptr) {
    Json attachments = Json::array();
    if (info.pColorBlendState->pAttachments != nullptr) {
      for (uint32_t i = 0; i < info.pColorBlendState->attachmentCount; ++i) {
        const auto& attachment = info.pColorBlendState->pAttachments[i];
        attachments.push_back({
            {"blendEnable", attachment.blendEnable},
            {"srcColorBlendFactor", attachment.srcColorBlendFactor},
            {"dstColorBlendFactor", attachment.dstColorBlendFactor},
            {"colorBlendOp", attachment.colorBlendOp},
            {"srcAlphaBlendFactor", attachment.srcAlphaBlendFactor},
            {"dstAlphaBlendFactor", attachment.dstAlphaBlendFactor},
            {"alphaBlendOp", attachment.alphaBlendOp},
            {"colorWriteMask", attachment.colorWriteMask},
        });
      }
    }
    state["colorBlend"] = {
        {"flags", info.pColorBlendState->flags},
        {"logicOpEnable", info.pColorBlendState->logicOpEnable},
        {"logicOp", info.pColorBlendState->logicOp},
        {"attachments", std::move(attachments)},
        {"blendConstants",
         {info.pColorBlendState->blendConstants[0], info.pColorBlendState->blendConstants[1],
          info.pColorBlendState->blendConstants[2], info.pColorBlendState->blendConstants[3]}},
        {"pNext", DescribePNext(info.pColorBlendState->pNext, document)}};
  }
  return FinishDocument(std::move(state), std::move(document), "Graphics");
}

VulkanStateDocument MakeComputeStateDocument(
    const VkComputePipelineCreateInfo& info,
    const std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
    bool allShadersResolved,
    bool stateSnapshotAvailable,
    GITSKey layoutKey) {
  VulkanStateDocument document;
  if (!allShadersResolved) {
    document.ReplacementEligible = false;
    document.Diagnostic = "the compute shader module could not be resolved";
  }
  if (!stateSnapshotAvailable) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "pipeline state contains unsupported extension data";
  }
  if (info.layout != VK_NULL_HANDLE && layoutKey == 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "pipeline layout key is unavailable";
  }
  if ((info.flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) != 0) {
    document.ReplacementEligible = false;
    if (!document.Diagnostic.empty()) {
      document.Diagnostic += "; ";
    }
    document.Diagnostic += "derivative pipelines are not replaceable";
  }
  Json state{{"flags", info.flags},
             {"layoutKey", layoutKey},
             {"stage", DescribeStages(&info.stage, 1, stages, document)},
             {"pNext", DescribePNext(info.pNext, document)}};
  return FinishDocument(std::move(state), std::move(document), "Compute");
}

} // namespace gits::vulkan
