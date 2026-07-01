// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

public:
  static CLibrary& Get();
  CLibrary(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t());
  CLibrary(const CLibrary& other) = delete;
  CLibrary& operator=(const CLibrary& other) = delete;
  ~CLibrary();

  Vulkan::CFunction* FunctionCreate(unsigned id) const override;

  const char* Name() const override {
    return "Vulkan";
  }

  void RegisterEvents() override;

  class CVulkanCommandBufferTokensBuffer : public CTokensBuffer<Vulkan::CFunction> {
  public:
    std::set<uint64_t> GetMappedPointers();
    std::set<uint64_t> GetMappedPointers(const BitRange& objRange,
                                         VulkanObjectMode objMode,
                                         const uint64_t objNumber = 0);
    void ExecAndStateTrack();
    void ExecAndDump(uint64_t queueSubmitNumber,
                     uint32_t cmdBuffBatchNumber,
                     uint32_t cmdBuffNumber,
                     VkCommandBuffer& cmdBuffer);
    void FinishRenderPass(uint64_t renderPassNumber,
                          uint64_t drawNumber,
                          VkCommandBuffer cmdBuffer);
    void FinishCommandBufferAndSubmit(VkCommandBuffer cmdBuffer);
    void CreateNewCommandBuffer(Vulkan::CFunction* token, VkCommandBuffer cmdBuffer);
    void RestoreSettingsToSpecifiedObject(uint64_t objNumber, VulkanObjectMode objMode);
    void RestoreSettingsToSpecifiedDraw(Vulkan::CFunction* token,
                                        uint64_t renderPassNumber,
                                        uint64_t drawNumber,
                                        VkCommandBuffer cmdBuffer);
    void RestoreToSpecifiedObject(const BitRange& objRange, VulkanObjectMode objMode);
    void ScheduleObject(void (*schedulerFunc)(Vulkan::CFunction*),
                        const BitRange& renderPassRange,
                        VulkanObjectMode objMode);
    void RestoreDraw(const uint64_t renderPassNumber, const BitRange& drawsRange);
    void ScheduleDraw(void (*schedulerFunc)(Vulkan::CFunction*),
                      const uint64_t renderPassNumber,
                      const BitRange& drawsRange);
  };
};

class COnQueueSubmitEndInterface {
protected:
  virtual ~COnQueueSubmitEndInterface() = 0;

public:
  virtual void OnQueueSubmitEnd() = 0;
};

class CDeviceAddressPatcher : public COnQueueSubmitEndInterface {
  typedef uint64_t hash_t;

  struct InputDataStruct {
    VkDeviceAddress address;
    uint32_t offset;
    uint32_t padding;
  };

  struct OutputDataStruct {
    VkDeviceAddress dVA;
    VkDeviceAddress REF;
  };

  std::vector<VkDeviceAddress> _directAddresses;
  std::vector<std::pair<VkDeviceAddress, uint32_t>> _indirectAddresses;
  VkDevice _device;
  VkDeviceMemory _outputDeviceMemory;
  hash_t _hash;

public:
  CDeviceAddressPatcher()
      : _device(VK_NULL_HANDLE), _outputDeviceMemory(VK_NULL_HANDLE), _hash(0) {}

  void AddDirectAddress(VkDeviceAddress address);
  void AddIndirectAddress(VkDeviceAddress address, uint32_t offset);
  void PrepareData(VkCommandBuffer commandBuffer, hash_t hash);
  void OnQueueSubmitEnd() override;

  uint32_t Count() const;
};
} // namespace Vulkan
} // namespace gits
