// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include <span>
#include <future>

namespace gits {
namespace vulkan {

class ObjectCreationTask {
public:
  ObjectCreationTask(std::vector<char>&& data, std::span<const GITSKey> inputKeys);
  virtual ~ObjectCreationTask() = default;

  virtual VkResult CreateObject() = 0;

protected:
  std::vector<char> m_Command;

private:
  friend class MultithreadedObjectCreationService;

  enum class State {
    Unscheduled,
    Queued,
    Running,
    Cancelled
  };
  State m_State{State::Unscheduled};
  std::vector<GITSKey> m_OutputKeys;
  std::vector<GITSKey> m_InputKeys;
  std::future<VkResult> m_Result;
}; // class ObjectCreationTask

class GraphicsPipelineCreationTask : public ObjectCreationTask {
public:
  GraphicsPipelineCreationTask(PFN_vkCreateGraphicsPipelines createGraphicsPipelines,
                               std::vector<char>&& data,
                               std::span<const GITSKey> inputKeys)
      : ObjectCreationTask(std::move(data), inputKeys),
        m_CreateGraphicsPipelines(createGraphicsPipelines) {}

private:
  VkResult CreateObject() override;
  PFN_vkCreateGraphicsPipelines m_CreateGraphicsPipelines;
}; // class GraphicsPipelineCreationTask

class ComputePipelineCreationTask : public ObjectCreationTask {
public:
  ComputePipelineCreationTask(PFN_vkCreateComputePipelines createComputePipelines,
                              std::vector<char>&& data,
                              std::span<const GITSKey> inputKeys)
      : ObjectCreationTask(std::move(data), inputKeys),
        m_CreateComputePipelines(createComputePipelines) {}

private:
  VkResult CreateObject() override;
  PFN_vkCreateComputePipelines m_CreateComputePipelines;
}; // class ComputePipelineCreationTask

} // namespace vulkan
} // namespace gits
