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
  CLibrary(const CLibrary& other) = delete;
  CLibrary& operator=(const CLibrary& other) = delete;
  ~CLibrary();

  Vulkan::CFunction* FunctionCreate(unsigned id) const override;

  const char* Name() const override {
    return "Vulkan";
  }

  CResourceManager& ProgramBinaryManager();
  class CVulkanCommandBufferTokensBuffer : public CTokensBuffer<Vulkan::CFunction> {
  public:
    std::set<uint64_t> GetMappedPointers();
    std::set<uint64_t> GetMappedPointers(const BitRange& objRange,
                                         Config::VulkanObjectMode objMode,
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
    void RestoreSettingsToSpecifiedRenderPass(uint64_t renderPassNumber);
    void RestoreSettingsToSpecifiedDraw(Vulkan::CFunction* token,
                                        uint64_t renderPassNumber,
                                        uint64_t drawNumber,
                                        VkCommandBuffer cmdBuffer);
    void RestoreToSpecifiedObject(const BitRange& objRange, Config::VulkanObjectMode objMode);
    void ScheduleObject(void (*schedulerFunc)(Vulkan::CFunction*),
                        const BitRange& renderPassRange,
                        Config::VulkanObjectMode objMode);
    void RestoreDraw(const uint64_t renderPassNumber, const BitRange& drawsRange);
    void ScheduleDraw(void (*schedulerFunc)(Vulkan::CFunction*),
                      const uint64_t renderPassNumber,
                      const BitRange& drawsRange);
  };
};

#ifndef BUILD_FOR_CCODE

class CAutoCaller {
  using FunctionPtr = void_t(STDCALL*)(void);

  FunctionPtr onDestructor;

public:
  CAutoCaller(FunctionPtr _onConstructor, FunctionPtr _onDestructor) : onDestructor(_onDestructor) {
    _onConstructor();
  }

  ~CAutoCaller() {
    onDestructor();
  }
};

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

class COnQueueSubmitEnd {
protected:
  virtual ~COnQueueSubmitEnd() = 0;

public:
  virtual void OnQueueSubmitEnd() = 0;
};

class CDeviceAddressPatcher : public COnQueueSubmitEnd {
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

#endif
} // namespace Vulkan
} // namespace gits
