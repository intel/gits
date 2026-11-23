// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectCreationService.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"
#include "handleArgumentUpdaters.h"
#include "handleMapService.h"

#include <stdexcept>

namespace gits {
namespace vulkan {

ObjectCreationTask::ObjectCreationTask(std::vector<char>&& data, std::span<const GITSKey> inputKeys)
    : m_Command(std::move(data)) {
  std::unordered_set<GITSKey> uniqueInputs(inputKeys.begin(), inputKeys.end());
  uniqueInputs.erase(0);
  m_InputKeys.assign(uniqueInputs.begin(), uniqueInputs.end());
}

VkResult GraphicsPipelineCreationTask::CreateObject() {
  vkCreateGraphicsPipelinesCommand command;
  Decode(m_Command.data(), command);

  if (command.m_pPipelines.Keys.size() != command.m_createInfoCount.Value) {
    throw std::logic_error("Pipeline creation output count does not match output keys");
  }

  const HandleResolver resolver(nullptr);
  UpdateHandle(command.m_device, resolver);
  UpdateHandle(command.m_pipelineCache, resolver);
  UpdateHandle(command.m_pCreateInfos, resolver);

  std::vector<VkPipeline> pipelines(command.m_createInfoCount.Value, VK_NULL_HANDLE);

  VkResult result = m_CreateGraphicsPipelines(
      command.m_device.Value, VK_NULL_HANDLE, command.m_createInfoCount.Value,
      command.m_pCreateInfos.Value, nullptr, pipelines.data());

  for (size_t i = 0; i < pipelines.size(); ++i) {
    if (command.m_pPipelines.Keys[i] && (pipelines[i] != VK_NULL_HANDLE)) {
      HandleMapService::Get().SetHandle(command.m_pPipelines.Keys[i],
                                        reinterpret_cast<uint64_t>(pipelines[i]));
    }
  }
  return result;
}

VkResult ComputePipelineCreationTask::CreateObject() {
  vkCreateComputePipelinesCommand command;
  Decode(m_Command.data(), command);

  if (command.m_pPipelines.Keys.size() != command.m_createInfoCount.Value) {
    throw std::logic_error("Pipeline creation output count does not match output keys");
  }

  const HandleResolver resolver(nullptr);
  UpdateHandle(command.m_device, resolver);
  UpdateHandle(command.m_pipelineCache, resolver);
  UpdateHandle(command.m_pCreateInfos, resolver);

  std::vector<VkPipeline> pipelines(command.m_createInfoCount.Value, VK_NULL_HANDLE);

  VkResult result = m_CreateComputePipelines(
      command.m_device.Value, VK_NULL_HANDLE, command.m_createInfoCount.Value,
      command.m_pCreateInfos.Value, nullptr, pipelines.data());

  for (size_t i = 0; i < pipelines.size(); ++i) {
    if (command.m_pPipelines.Keys[i] && (pipelines[i] != VK_NULL_HANDLE)) {
      HandleMapService::Get().SetHandle(command.m_pPipelines.Keys[i],
                                        reinterpret_cast<uint64_t>(pipelines[i]));
    }
  }
  return result;
}

} // namespace vulkan
} // namespace gits
