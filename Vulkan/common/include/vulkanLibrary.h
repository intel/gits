// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanLibrary.h
*
* @brief Declaration of Vulkan library implementation.
*
*/

#pragma once

#include "library.h"
#include "vkFunction.h"
#include "vulkanDrivers.h"

namespace gits {
/**
  * @brief Vulkan library specific GITS namespace
  */

class CResourceManager;

namespace Vulkan {
/**
    * @brief Vulkan library class
    *
    * gits::Vulkan::CLibrary class provides Vulkan library tools
    * for GITS project. It is responsible for creating Vulkan
    * function call wrappers based on unique ID, Vulkan library state
    * getter class and for creating Vulkan display window for gitsPlayer.
    */
class CLibrary : public gits::CLibrary {
  std::unique_ptr<CResourceManager> _progBinManager;

public:
  static CLibrary& Get();
  CLibrary(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t());
  ~CLibrary();

  Vulkan::CFunction* FunctionCreate(unsigned id) const override;

  const char* Name() const override {
    return "Vulkan";
  }

  CResourceManager& ProgramBinaryManager();
  class CVulkanCommandBufferTokensBuffer : public CTokensBuffer<Vulkan::CFunction> {
  public:
    std::set<uint64_t> GetMappedPointers(const BitRange& objRange,
                                         Config::VulkanObjectMode objMode);
    void ExecAndStateTrack();
    void ExecAndDump(VkCommandBuffer cmdBuffer,
                     uint64_t queueSubmitNumber,
                     uint32_t cmdBuffBatchNumber,
                     uint32_t cmdBuffNumber);
    void RestoreRenderPass(const BitRange& renderPassRange);
    void ScheduleRenderPass(void (*schedulerFunc)(Vulkan::CFunction*),
                            const BitRange& renderPassRange);
  };
};
} // namespace Vulkan
} // namespace gits
