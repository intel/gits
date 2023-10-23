// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanRecorderWrapper.cpp
*
* @brief Definition of Vulkan recorder wrapper.
*/

#include "recorder.h"
#include "vulkan_apis_iface.h"
#include "vulkanStateRestore.h"
#include "vulkanStateTracking.h"
#include "vulkanRecorderWrapper.h"

static gits::Vulkan::CRecorderWrapper* wrapper = nullptr;

namespace gits {
VulkanQueueSubmitPrePost::VulkanQueueSubmitPrePost() {
  gits::CGits::Instance().vkCounters.QueueSubmitCountUp();
}

VulkanQueueSubmitPrePost::~VulkanQueueSubmitPrePost() {}

VulkanCreateImagePrePost::VulkanCreateImagePrePost() {
  gits::CGits::Instance().vkCounters.ImageCountUp();
}

VulkanCreateImagePrePost::~VulkanCreateImagePrePost() {}

VulkanCreateBufferPrePost::VulkanCreateBufferPrePost() {
  gits::CGits::Instance().vkCounters.BufferCountUp();
}

VulkanCreateBufferPrePost::~VulkanCreateBufferPrePost() {}
} // namespace gits

gits::Vulkan::IRecorderWrapper* STDCALL GITSRecorderVulkan() {
  if (wrapper == nullptr) {
    try {
      // library not set - perform initialization
      gits::CGits::Instance().apis.UseApi3dIface(
          std::shared_ptr<gits::ApisIface::Api3d>(new gits::Vulkan::VulkanApi()));
      gits::CRecorder& recorder = gits::CRecorder::Instance();
      wrapper = new gits::Vulkan::CRecorderWrapper(recorder);
      auto library = new gits::Vulkan::CLibrary([] { return new gits::Vulkan::CState; });
      recorder.Register(std::shared_ptr<gits::CLibrary>(library));
    } catch (const std::exception& ex) {
      Log(ERR) << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return wrapper;
}

namespace gits {
namespace Vulkan {

CRecorderWrapper::CRecorderWrapper(CRecorder& recorder)
    : _recorder(recorder), _ignoreNextQueuePresentGITS(false) {}

void CRecorderWrapper::PauseRecording() {
  _recorder.Pause();
}

void CRecorderWrapper::ContinueRecording() {
  _recorder.Continue();
}

void CRecorderWrapper::StreamFinishedEvent(std::function<void()> event) {
  _recorder.RegisterDisposeEvent(std::move(event));
}

void CRecorderWrapper::CloseRecorderIfRequired() {
  if (_recorder.IsMarkedForDeletion()) {
    _recorder.Close();
  }
}

CVkDriver& CRecorderWrapper::Drivers() const {
  return drvVk;
}

void CRecorderWrapper::SetDriverMode(CVkDriver::DriverMode mode) const {
  drvVk.SetMode(mode);
}

void* CRecorderWrapper::GetShadowMemory(VkDeviceMemory memory,
                                        void* orig,
                                        uint64_t size,
                                        uint64_t offset) {
  //Returns a reference to shadow buffer data. Data size is being set on first use
  //if shadowBuffers option enabled
  if (gits::Config::Get().recorder.vulkan.utilities.shadowMemory) {
    auto& memoryState = SD()._devicememorystates[memory];

    if (!memoryState->shadowMemory) {
      memoryState->shadowMemory.reset(new ShadowBuffer());
    }

    uint64_t unmapSize = size;
    if (unmapSize == 0xFFFFFFFFFFFFFFFF) {
      unmapSize = memoryState->memoryAllocateInfoData.Value()->allocationSize - offset;
    }

    if (!memoryState->shadowMemory->Initialized()) {
      memoryState->shadowMemory->Init(Config::Get().recorder.vulkan.utilities.memoryAccessDetection,
                                      (size_t)unmapSize, orig);
      memoryState->shadowMemory->UpdateShadow(0, (size_t)unmapSize);
    } else {
      memoryState->shadowMemory->SetOriginalBuffer(orig);
    }

    return memoryState->shadowMemory->GetData();
  }
  return nullptr;
}

uint64_t CRecorderWrapper::GetWholeMemorySize(VkDeviceMemory memory) const {
  return SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->allocationSize;
}

void CRecorderWrapper::dumpScreenshot(VkQueue queue,
                                      VkCommandBuffer cmdBuffer,
                                      uint32_t commandBufferBatchCounter,
                                      uint32_t commandBufferCounter) {
  if (gits::Config::Get()
          .recorder.vulkan.images
          .dumpSubmits[(size_t)gits::CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    drvVk.vkQueueWaitIdle(queue);
    writeScreenshot(queue, cmdBuffer, commandBufferBatchCounter, commandBufferCounter);
  }
}

namespace {
void ProcessOnQueueSubmitEndMessageReceivers(
    std::shared_ptr<CCommandBufferState>& commandBufferState, VkQueue queue, bool& waitVar) {
  if (!commandBufferState->queueSubmitEndMessageReceivers.empty() && !waitVar) {
    drvVk.vkQueueWaitIdle(queue);
    waitVar = true;
  }

  for (auto receiver : commandBufferState->queueSubmitEndMessageReceivers) {
    receiver->OnQueueSubmitEnd();
  }
}
} // namespace

void CRecorderWrapper::resetMemoryAfterQueueSubmit(VkQueue queue,
                                                   uint32_t submitCount,
                                                   const VkSubmitInfo* pSubmits) {
  bool queueWaitIdleAlreadyUsed = false;

  if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
      Config::Get().recorder.vulkan.utilities.shadowMemory) {
    CMemoryUpdateState toUpdate;
    toUpdate.intervalMapMemory.clear();

    for (uint32_t i = 0; i < submitCount; i++) {
      for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
        for (auto& obj2 :
             SD().updatedMemoryInCmdBuffer[pSubmits[i].pCommandBuffers[j]].intervalMapMemory) {
          auto memoryStatePair = SD()._devicememorystates.find(obj2.first);
          if (memoryStatePair != SD()._devicememorystates.end()) {
            auto& memoryState = memoryStatePair->second;
            if (memoryState->IsMapped()) {
              for (auto& obj3 : obj2.second) {
                uint64_t offset = std::max(obj3.lower(), memoryState->mapping->offsetData.Value());
                uint64_t size = 0;
                uint64_t high_value = obj3.upper();

                if (obj3.upper() > (memoryState->mapping->offsetData.Value() +
                                    memoryState->mapping->sizeData.Value())) {
                  high_value = memoryState->mapping->offsetData.Value() +
                               memoryState->mapping->sizeData.Value();
                }
                if (offset < high_value) {
                  size = high_value - offset;
                }
                if (size > 0) {
                  toUpdate.AddToMap(obj2.first, offset, size);
                }
              }
            }
          } else {
            Log(WARN) << "VkDeviceMemory " << obj2.first
                      << " scheduled for update in command buffer "
                      << pSubmits[i].pCommandBuffers[j]
                      << " has been freed, before the command buffer finished execution!";
          }
        }
      }
    }

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
    //   if (bufferState.second->binding != nullptr) {
    //     auto& memoryState = bufferState.second->binding->deviceMemoryStateStore;
    //     if ((SD()._devicememorystates.find(memoryState->deviceMemoryHandle) !=
    //          SD()._devicememorystates.end()) &&
    //         memoryState->IsMapped()) {
    //       toUpdate.AddToMap(memoryState->deviceMemoryHandle,
    //                         bufferState.second->binding->memoryOffset,
    //                         bufferState.second->binding->memorySizeRequirement);
    //     }
    //   }
    // }

    if (toUpdate.intervalMapMemory.size() > 0) {
      drvVk.vkQueueWaitIdle(queue);
      queueWaitIdleAlreadyUsed = true;
    }

    for (auto& obj2 : toUpdate.intervalMapMemory) {
      auto& memoryState = SD()._devicememorystates[obj2.first];
      void* pointer = (char*)memoryState->mapping->ppDataData.Value();

      for (auto& obj3 : obj2.second) {
        uint64_t offset = obj3.lower() - memoryState->mapping->offsetData.Value();
        uint64_t size = obj3.upper() - obj3.lower();

        if (offset + size <= memoryState->mapping->sizeData.Value()) {
          if (Config::Get().recorder.vulkan.utilities.shadowMemory) {
            if (Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
              SetPagesProtection(READ_WRITE, (char*)pointer + offset, (size_t)size);
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(
                    (size_t)(offset + memoryState->mapping->offsetData.Value()), (size_t)size);
              } else {
                memoryState->shadowMemory->UpdateShadow((size_t)offset, (size_t)size);
              }
              SetPagesProtection(READ_ONLY, (char*)pointer + offset, (size_t)size);
            } else {
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(
                    (size_t)(offset + memoryState->mapping->offsetData.Value()), (size_t)size);
              } else {
                memoryState->shadowMemory->UpdateShadow((size_t)offset, (size_t)size);
              }
            }
          }
          if (Config::Get().recorder.vulkan.utilities.memorySegmentSize) {
            memcpy(&memoryState->mapping->compareData[(size_t)offset], (char*)pointer + offset,
                   (size_t)size);
          }
        } else {
          Log(WARN) << "Updating memory after QueueSubmit failed. Invalid values.";
        }
      }
    }
  }

  // Perform tasks which were waiting for queue submit end (i.e. acceleration structure building data acquisition)
  for (uint32_t i = 0; i < submitCount; i++) {
    for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
      auto& commandBufferState = SD()._commandbufferstates[pSubmits[i].pCommandBuffers[j]];

      ProcessOnQueueSubmitEndMessageReceivers(commandBufferState, queue, queueWaitIdleAlreadyUsed);
    }
  }
}

void CRecorderWrapper::resetMemoryAfterQueueSubmit2(VkQueue queue,
                                                    uint32_t submitCount,
                                                    const VkSubmitInfo2* pSubmits) {
  bool queueWaitIdleAlreadyUsed = false;

  if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
      Config::Get().recorder.vulkan.utilities.shadowMemory) {
    CMemoryUpdateState toUpdate;
    toUpdate.intervalMapMemory.clear();

    for (uint32_t i = 0; i < submitCount; i++) {
      for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
        for (auto& obj2 :
             SD().updatedMemoryInCmdBuffer[pSubmits[i].pCommandBufferInfos[j].commandBuffer]
                 .intervalMapMemory) {
          auto memoryStatePair = SD()._devicememorystates.find(obj2.first);
          if (memoryStatePair != SD()._devicememorystates.end()) {
            auto& memoryState = memoryStatePair->second;
            if (memoryState->IsMapped()) {
              for (auto& obj3 : obj2.second) {
                uint64_t offset = std::max(obj3.lower(), memoryState->mapping->offsetData.Value());
                uint64_t size = 0;
                uint64_t high_value = obj3.upper();

                if (obj3.upper() > (memoryState->mapping->offsetData.Value() +
                                    memoryState->mapping->sizeData.Value())) {
                  high_value = memoryState->mapping->offsetData.Value() +
                               memoryState->mapping->sizeData.Value();
                }
                if (offset < high_value) {
                  size = high_value - offset;
                }
                if (size > 0) {
                  toUpdate.AddToMap(obj2.first, offset, size);
                }
              }
            }
          } else {
            Log(WARN) << "VkDeviceMemory " << obj2.first
                      << " scheduled for update in command buffer "
                      << pSubmits[i].pCommandBufferInfos[j].commandBuffer
                      << " has been freed, before the command buffer finished execution!";
          }
        }
      }
    }

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
    //   if (bufferState.second->binding != nullptr) {
    //     auto& memoryState = bufferState.second->binding->deviceMemoryStateStore;
    //     if ((SD()._devicememorystates.find(memoryState->deviceMemoryHandle) !=
    //          SD()._devicememorystates.end()) &&
    //         memoryState->IsMapped()) {
    //       toUpdate.AddToMap(memoryState->deviceMemoryHandle,
    //                         bufferState.second->binding->memoryOffset,
    //                         bufferState.second->binding->memorySizeRequirement);
    //     }
    //   }
    // }

    if (toUpdate.intervalMapMemory.size() > 0) {
      drvVk.vkQueueWaitIdle(queue);
      queueWaitIdleAlreadyUsed = true;
    }

    for (auto& obj2 : toUpdate.intervalMapMemory) {
      auto& memoryState = SD()._devicememorystates[obj2.first];
      void* pointer = (char*)memoryState->mapping->ppDataData.Value();

      for (auto& obj3 : obj2.second) {
        uint64_t offset = obj3.lower() - memoryState->mapping->offsetData.Value();
        uint64_t size = obj3.upper() - obj3.lower();

        if (offset + size <= memoryState->mapping->sizeData.Value()) {
          if (Config::Get().recorder.vulkan.utilities.shadowMemory) {
            if (Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
              SetPagesProtection(READ_WRITE, (char*)pointer + offset, (size_t)size);
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(
                    (size_t)(offset + memoryState->mapping->offsetData.Value()), (size_t)size);
              } else {
                memoryState->shadowMemory->UpdateShadow((size_t)offset, (size_t)size);
              }
              SetPagesProtection(READ_ONLY, (char*)pointer + offset, (size_t)size);
            } else {
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(
                    (size_t)(offset + memoryState->mapping->offsetData.Value()), (size_t)size);
              } else {
                memoryState->shadowMemory->UpdateShadow((size_t)offset, (size_t)size);
              }
            }
          }
          if (Config::Get().recorder.vulkan.utilities.memorySegmentSize) {
            memcpy(&memoryState->mapping->compareData[(size_t)offset], (char*)pointer + offset,
                   (size_t)size);
          }
        } else {
          Log(WARN) << "Updating memory after QueueSubmit failed. Invalid values.";
        }
      }
    }
  }

  // Perform tasks which were waiting for queue submit end (i.e. acceleration structure building data acquisition)
  for (uint32_t i = 0; i < submitCount; i++) {
    for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
      auto& commandBufferState =
          SD()._commandbufferstates[pSubmits[i].pCommandBufferInfos[j].commandBuffer];

      ProcessOnQueueSubmitEndMessageReceivers(commandBufferState, queue, queueWaitIdleAlreadyUsed);
    }
  }
}

VkResult CRecorderWrapper::CheckFenceStatus(VkDevice device, VkFence fence) const {
  VkResult return_value = drvVk.vkGetFenceStatus(device, fence);
  if ((VK_SUCCESS == return_value) &&
      (gits::Config::Get().recorder.vulkan.utilities.delayFenceChecksCount > 0)) {
    auto& fenceState = SD()._fencestates[fence];
    if (fenceState->delayChecksCount <
        gits::Config::Get().recorder.vulkan.utilities.delayFenceChecksCount) {
      return_value = VK_NOT_READY;
      fenceState->delayChecksCount++;
    }
  }
  return return_value;
}

void CRecorderWrapper::EndFramePost() const {
  _recorder.EndFramePost();
}

void CRecorderWrapper::SuppressPhysicalDeviceFeatures(
    std::vector<std::string> const& suppressFeatures, VkPhysicalDeviceFeatures* features) const {
  suppressPhysicalDeviceFeatures(suppressFeatures, features);
}

VkPhysicalDevice CRecorderWrapper::GetPhysicalDevice(VkDevice device) const {
  return SD()._devicestates[device]->physicalDeviceStateStore->physicalDeviceHandle;
}

void CRecorderWrapper::IgnoreNextQueuePresentGITS() {
  _ignoreNextQueuePresentGITS = true;
}

bool CRecorderWrapper::IsNextQueuePresentIgnored() {
  return _ignoreNextQueuePresentGITS;
}

void CRecorderWrapper::AcceptNextQueuePresentGITS() {
  _ignoreNextQueuePresentGITS = false;
}

bool CRecorderWrapper::CheckMemoryMappingFeasibility(VkDevice device,
                                                     uint32_t memoryTypeIndex,
                                                     bool throwException) const {
  return checkMemoryMappingFeasibility(device, memoryTypeIndex, throwException);
}

bool CRecorderWrapper::AreDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                                    uint32_t requestedExtensionsCount,
                                                    char const* const* requestedExtensions) const {
  return areDeviceExtensionsSupported(physicalDevice, requestedExtensionsCount,
                                      requestedExtensions);
}

const void* CRecorderWrapper::GetPNextStructure(const void* pNext,
                                                VkStructureType structureType) const {
  return getPNextStructure(pNext, structureType);
}

bool CRecorderWrapper::IsImagePresentable(const VkImageCreateInfo* pCreateInfo) const {
  return isImagePresentable(pCreateInfo);
}

void CRecorderWrapper::DisableConfigOptions() const {
  auto cfg = gits::Config::Get();

  // By default record substreams from GITS Player using the dedicated/custom
  // method of memory changes tracking (with tags). Use the universal
  // recording/memory tracking only when forced through config option.
  if (!cfg.recorder.vulkan.utilities.forceUniversalRecording) {
    Log(INFO) << "Recording a (sub)stream from GITS Player. Disabling all memory tracking-related "
                 "options!";

    cfg.recorder.vulkan.utilities.memoryAccessDetection = false;
    cfg.recorder.vulkan.utilities.memorySegmentSize = 0;
    cfg.recorder.vulkan.utilities.shadowMemory = false;
    cfg.recorder.vulkan.utilities.useExternalMemoryExtension = false;
    cfg.recorder.vulkan.utilities.memoryUpdateState.setFromString("UsingTags");
  }

#ifdef GITS_PLATFORM_WINDOWS
  // Disable other options when recording (sub)streams from GITS streams
  cfg.recorder.vulkan.utilities.usePresentSrcLayoutTransitionAsAFrameBoundary = false;
#endif

  gits::Config::Set(cfg);
}

void CRecorderWrapper::StartStateRestore() const {
  auto& cfg = gits::Config::Get();
  if (cfg.recorder.vulkan.capture.mode.find("All") != std::string::npos &&
      cfg.recorder.basic.dumpCCode && !CGits::Instance().IsCCodeStateRestore()) {
    CGits::Instance().CCodeStateRestoreStart();
    _recorder.Schedule(
        new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, CGits::Instance().CurrentFrame()));
    _recorder.Schedule(
        new CTokenFrameNumber(CToken::ID_INIT_START, CGits::Instance().CurrentFrame()));
  }
}

void CRecorderWrapper::EndStateRestore() const {
  auto& cfg = gits::Config::Get();
  if (cfg.recorder.vulkan.capture.mode.find("All") != std::string::npos &&
      cfg.recorder.basic.dumpCCode && CGits::Instance().IsCCodeStateRestore()) {
    _recorder.Schedule(
        new CTokenFrameNumber(CToken::ID_INIT_END, CGits::Instance().CurrentFrame()));
    _recorder.Schedule(
        new CTokenFrameNumber(CToken::ID_FRAME_START, CGits::Instance().CurrentFrame()));
    CGits::Instance().CCodeStateRestoreEnd();
  }
}

bool CRecorderWrapper::IsCCodeStateRestore() const {
  return CGits::Instance().IsCCodeStateRestore();
}

void CRecorderWrapper::StartFrame() const {
  _recorder.Schedule(
      new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, CGits::Instance().CurrentFrame()));
  _recorder.Schedule(
      new CTokenFrameNumber(CToken::ID_FRAME_START, CGits::Instance().CurrentFrame()));
}

void* CRecorderWrapper::CreateExternalMemory(VkDeviceSize size) const {
#ifdef GITS_PLATFORM_WINDOWS
  return VirtualAlloc(nullptr, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE | MEM_WRITE_WATCH,
                      PAGE_READWRITE);
#else
  return nullptr;
#endif
}

void CRecorderWrapper::FreeExternalMemory(VkDeviceMemory memory) const {
#ifdef GITS_PLATFORM_WINDOWS
  auto& memoryState = SD()._devicememorystates[memory];
  if (memoryState->externalMemory) {
    VirtualFree(memoryState->externalMemory, 0, MEM_RELEASE);
  }
#endif
}

// This is a recorder-side workaround for calling vkAllocateMemory_SD() function
void CRecorderWrapper::TrackMemoryState(VkResult return_value,
                                        VkDevice device,
                                        const VkMemoryAllocateInfo* pAllocateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDeviceMemory* pMemory,
                                        void* externalMemory) const {
  vkAllocateMemory_SD(return_value, device, pAllocateInfo, pAllocator, pMemory, externalMemory);
}

bool CRecorderWrapper::IsVulkanAPIVersionSupported(uint32_t major,
                                                   uint32_t minor,
                                                   VkPhysicalDevice physicalDevice) const {
  return isVulkanAPIVersionSupported(major, minor, physicalDevice);
}

void CRecorderWrapper::SetConfig(Config const& cfg) const {
  Config::Set(cfg);
}

} // namespace Vulkan
} // namespace gits
