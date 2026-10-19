// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <memory>
#include <memory_resource>

namespace gits::vulkan {

class PipelineStateSnapshot : public std::enable_shared_from_this<PipelineStateSnapshot> {
public:
  enum class Type {
    Graphics,
    Compute
  };

  static std::shared_ptr<PipelineStateSnapshot> Encode(
      const VkGraphicsPipelineCreateInfo& createInfo);
  static std::shared_ptr<PipelineStateSnapshot> Encode(
      const VkComputePipelineCreateInfo& createInfo);

  PipelineStateSnapshot(const PipelineStateSnapshot&) = delete;
  PipelineStateSnapshot& operator=(const PipelineStateSnapshot&) = delete;

  Type GetType() const;
  const VkGraphicsPipelineCreateInfo* Graphics() const;
  const VkComputePipelineCreateInfo* Compute() const;
  VkGraphicsPipelineCreateInfo* MutableGraphics();
  VkComputePipelineCreateInfo* MutableCompute();
  std::shared_ptr<PipelineStateSnapshot> Clone() const;

private:
  explicit PipelineStateSnapshot(Type type);

  template <typename T>
  T* CopyObject(const T& source);
  template <typename T>
  T* CopyArray(const T* source, size_t count);
  char* CopyString(const char* source);
  void* CopyBytes(const void* source, size_t size);
  const void* CopyPNext(const void* chain, bool& supported);
  bool CopyStage(VkPipelineShaderStageCreateInfo& destination,
                 const VkPipelineShaderStageCreateInfo& source);
  bool CopyGraphics(const VkGraphicsPipelineCreateInfo& source);
  bool CopyCompute(const VkComputePipelineCreateInfo& source);

  Type m_Type;
  std::pmr::monotonic_buffer_resource m_Arena;
  VkGraphicsPipelineCreateInfo* m_Graphics{};
  VkComputePipelineCreateInfo* m_Compute{};
};

} // namespace gits::vulkan
