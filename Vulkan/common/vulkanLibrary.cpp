// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanLibrary.cpp
*
* @brief Definition of Vulkan common part library implementation.
*
*/

#include "vulkanLibrary.h"
#include "vulkanTools.h"
#include "gits.h"

namespace bfs = boost::filesystem;

namespace gits {
namespace Vulkan {

/**
    * @brief Constructor
    *
    * CLibrary class constructor.
    */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc) : gits::CLibrary(ID_VULKAN, stc) {}

CLibrary::~CLibrary() {
  waitForAllDevices();
  destroyDeviceLevelResources();
  destroyInstanceLevelResources();
}

gits::CResourceManager& gits::Vulkan::CLibrary::ProgramBinaryManager() {
  if (_progBinManager) {
    return *_progBinManager;
  }

  std::unordered_map<uint32_t, bfs::path> the_map;
  the_map[RESOURCE_INDEX] = "gitsPlayerDataIndex.dat";
  the_map[RESOURCE_BUFFER] = "gitsPlayerBuffers.dat";

  auto type = Config::Get().recorder.extras.optimizations.hashType;
  const auto& ph = Config::Get().recorder.extras.optimizations.partialHash;
  _progBinManager.reset(
      new CResourceManager(the_map, Config::Get().recorder.extras.optimizations.asyncBufferWrites,
                           type, ph.enabled, ph.cutoff, ph.chunks, ph.ratio));
  return *_progBinManager;
}

/**
    * @brief Creates Vulkan method call wrapper
    *
    * Method creates Vulkan method call wrappers based on unique
    * identifier.
    *
    * @param id Unique Vulkan method identifier.
    *
    * @exception EOperationFailed Unknown Vulkan method identifier
    *
    * @return Vulkan method call wrapper.
    */
Vulkan::CFunction* Vulkan::CLibrary::FunctionCreate(unsigned id) const {
  return Vulkan::CFunction::Create(id);
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_VULKAN));
}

std::set<uint64_t> CLibrary::CVulkanCommandBufferTokensBuffer::GetMappedPointers(
    const BitRange& objRange, Config::VulkanObjectMode objMode) {
  std::set<uint64_t> returnMap;
  uint64_t renderPassCount = 0;
  bool pre_renderpass = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    }
    if (objRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      pre_renderpass = false;
    }
    for (auto obj : elem->GetMappedPointers()) {
      if (objMode == Config::MODE_VKQUEUESUBMIT || objMode == Config::MODE_VKCOMMANDBUFFER ||
          (objMode == Config::MODE_VKRENDERPASS &&
           (objRange[renderPassCount] ||
            (pre_renderpass && (elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
                                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
                                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE))))) {
        returnMap.insert((uint64_t)obj);
      }
    }
  }
  returnMap.erase(0); // 0 is not valid pointer.
  return returnMap;
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ExecAndStateTrack() {
  for (auto elem : _tokensList) {
    elem->Exec();
    elem->StateTrack();
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ExecAndDump(VkCommandBuffer cmdBuffer,
                                                             uint64_t queueSubmitNumber,
                                                             uint32_t cmdBuffBatchNumber,
                                                             uint32_t cmdBuffNumber) {
  uint64_t renderPassCount = 0;
  for (auto elem : _tokensList) {
    elem->Exec();
    elem->StateTrack();
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      renderPassCount};
      if (localCounter == Config::Get().player.captureVulkanRenderPasses) {
        vulkanScheduleCopyRenderPasses(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber,
                                       cmdBuffNumber, renderPassCount);
      }
      renderPassCount++;
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreRenderPass(
    const BitRange& renderPassRange) {
  uint64_t renderPassCount = 0;

  bool pre_renderpass = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    }
    if (renderPassRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      pre_renderpass = false;
    }
    if (pre_renderpass) {
      elem->Exec();
      elem->StateTrack();
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ScheduleRenderPass(
    void (*schedulerFunc)(Vulkan::CFunction*), const BitRange& renderPassRange) {
  uint64_t renderPassCount = 0;
  bool started = false;

  for (auto elem : _tokensList) {
    if (renderPassRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      started = true;
    }
    if (renderPassRange[renderPassCount] && started) {
      schedulerFunc(elem);
    } else if ((elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE) &&
               !started) {
      bool toSkip = false;
      for (auto obj : elem->GetMappedPointers()) {
        if (IsObjectToSkip(obj)) {
          toSkip = true;
          break;
        }
      }
      if (!toSkip) {
        schedulerFunc(elem);
      }
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    }
  }
  _tokensList.clear();
}
} //namespace Vulkan
} //namespace gits
