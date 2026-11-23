// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"
#include "multithreadedObjectCreationLayer.h"
#include "multithreadedObjectCreationService.h"
#include "handleMapService.h"
#include "commandCodersAuto.h"

#include <algorithm>

namespace gits {
namespace vulkan {

void MultithreadedObjectCreationLayer::Pre(vkCreateGraphicsPipelinesCommand& command) {
  if (!m_Manager.ExecuteCommands() || command.m_Skip ||
      std::none_of(command.m_pPipelines.Keys.begin(), command.m_pPipelines.Keys.end(),
                   [](GITSKey key) { return key != 0; })) {
    return;
  }
  std::vector<char> commandData(GetSize(command));
  Encode(command, commandData.data());

  auto inputKeys = command.m_pCreateInfos.HandleKeys;
  inputKeys.push_back(command.m_device.Key);
  inputKeys.push_back(command.m_pipelineCache.Key);
  m_Manager.GetMultithreadedObjectCreationService()->Schedule(
      command.m_pPipelines.Keys,
      std::make_shared<GraphicsPipelineCreationTask>(
          m_Manager.GetDeviceDispatchTable(command.m_device.Value).vkCreateGraphicsPipelines,
          std::move(commandData), inputKeys));

  command.m_Skip = true;
}

void MultithreadedObjectCreationLayer::Pre(vkCreateComputePipelinesCommand& command) {
  if (!m_Manager.ExecuteCommands() || command.m_Skip ||
      std::none_of(command.m_pPipelines.Keys.begin(), command.m_pPipelines.Keys.end(),
                   [](GITSKey key) { return key != 0; })) {
    return;
  }
  std::vector<char> commandData(GetSize(command));
  Encode(command, commandData.data());

  auto inputKeys = command.m_pCreateInfos.HandleKeys;
  inputKeys.push_back(command.m_device.Key);
  inputKeys.push_back(command.m_pipelineCache.Key);
  m_Manager.GetMultithreadedObjectCreationService()->Schedule(
      command.m_pPipelines.Keys,
      std::make_shared<ComputePipelineCreationTask>(
          m_Manager.GetDeviceDispatchTable(command.m_device.Value).vkCreateComputePipelines,
          std::move(commandData), inputKeys));

  command.m_Skip = true;
}

} // namespace vulkan
} // namespace gits
