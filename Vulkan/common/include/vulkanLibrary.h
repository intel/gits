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
    void ExecAndDump(uint64_t queueSubmitNumber,
                     uint32_t cmdBuffBatchNumber,
                     uint32_t cmdBuffNumber,
                     VkCommandBuffer& cmdBuffer);
    void FinishCommandBufferAndRestoreSettings(Vulkan::CFunction* token,
                                               uint64_t renderPassNumber,
                                               uint64_t drawNumber,
                                               VkCommandBuffer cmdBuffer);
    void RestoreRenderPass(const BitRange& renderPassRange);
    void ScheduleRenderPass(void (*schedulerFunc)(Vulkan::CFunction*),
                            const BitRange& renderPassRange);
  };
};

#ifndef BUILD_FOR_CCODE
// Kudos to Piotr Horodecki
class MemoryAliasingTracker {
  struct Range {
    uint64_t offset;
    uint64_t size;

    mutable std::set<std::pair<uint64_t, bool>> resources;

    // We make this struct a functor, so it can be used as a comparator.
    bool operator()(Range const& lRange, Range const& rRange) const;
  };

  // The set contains Ranges and uses Range->operator() to compare them.
  using RangeSetType = std::set<Range, Range>;
  RangeSetType MemoryRanges;

  RangeSetType::iterator GetRange(uint64_t offset);
  void SplitRange(uint64_t offset);
  void AddResource(uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);
  void RemoveResource(uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForResource(
      uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource);

public:
  MemoryAliasingTracker(uint64_t size);
  void AddImage(uint64_t offset, uint64_t size, VkImage image);
  void AddBuffer(uint64_t offset, uint64_t size, VkBuffer buffer);
  void RemoveImage(uint64_t offset, uint64_t size, VkImage image);
  void RemoveBuffer(uint64_t offset, uint64_t size, VkBuffer buffer);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForImage(uint64_t offset,
                                                                  uint64_t size,
                                                                  VkImage image);
  std::set<std::pair<uint64_t, bool>> GetAliasedResourcesForBuffer(uint64_t offset,
                                                                   uint64_t size,
                                                                   VkBuffer buffer);
};
#endif
} // namespace Vulkan
} // namespace gits
