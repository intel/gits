// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "configurationLib.h"

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
      LOG_ERROR << "Cannot initialize recorder: " << ex.what() << std::endl;
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

bool CRecorderWrapper::IsPaused() const {
  return _recorder.IsPaused();
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
  if (Configurator::Get().vulkan.recorder.shadowMemory) {
    auto& memoryState = SD()._devicememorystates[memory];

    if (!memoryState->shadowMemory) {
      memoryState->shadowMemory.reset(new ShadowBuffer());
    }

    uint64_t unmapSize = size;
    if (unmapSize == 0xFFFFFFFFFFFFFFFF) {
      unmapSize = memoryState->memoryAllocateInfoData.Value()->allocationSize - offset;
    }

    if (!memoryState->shadowMemory->Initialized()) {
      memoryState->shadowMemory->Init(true, (size_t)unmapSize, orig,
                                      Configurator::Get().vulkan.recorder.writeWatchDetection);
      memoryState->shadowMemory->UpdateShadow(0, (size_t)unmapSize);
      if (Configurator::Get().vulkan.recorder.writeWatchDetection) {
        WriteWatchSniffer::ResetTouchedPages((char*)memoryState->shadowMemory->GetData(),
                                             (size_t)unmapSize);
      }
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
  if (Configurator::Get()
          .vulkan.recorder
          .dumpSubmits[(size_t)gits::CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    drvVk.vkQueueWaitIdle(queue);
    writeScreenshot(queue, cmdBuffer, commandBufferBatchCounter, commandBufferCounter);
  }
}

namespace {
void ProcessOnQueueSubmitEndMessageReceivers(
    std::shared_ptr<CCommandBufferState>& commandBufferState, VkQueue queue, bool& waitVar) {
  if (!commandBufferState) {
    return;
  }

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

  if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
      Configurator::Get().vulkan.recorder.shadowMemory) {
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
              auto& mapping = memoryState->mapping;

              for (auto& obj3 : obj2.second.getIntervals()) {
                uint64_t offset = std::max(obj3.first, mapping->offset);
                uint64_t size = 0;
                uint64_t high_value = obj3.second;

                if (obj3.second > (mapping->offset + mapping->size)) {
                  high_value = mapping->offset + mapping->size;
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
            LOG_WARNING << "VkDeviceMemory " << obj2.first
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
      auto& mapping = memoryState->mapping;
      char* pointer = mapping->pData;

      for (auto& obj3 : obj2.second.getIntervals()) {
        uint64_t offset = obj3.first - mapping->offset;
        uint64_t size = obj3.second - obj3.first;

        if (offset + size <= mapping->size) {
          if (Configurator::Get().vulkan.recorder.shadowMemory) {
            if (Configurator::Get().vulkan.recorder.memoryAccessDetection) {
              SetPagesProtection(PageMemoryProtection::READ_WRITE, pointer + offset, size);

              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(offset + mapping->offset, size);
              } else {
                memoryState->shadowMemory->UpdateShadow(offset, size);
              }

              SetPagesProtection(PageMemoryProtection::READ_ONLY, pointer + offset, size);
            } else {
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(offset + mapping->offset, size);
              } else {
                memoryState->shadowMemory->UpdateShadow(offset, size);
              }
              if (Configurator::Get().vulkan.recorder.writeWatchDetection) {
                WriteWatchSniffer::ResetTouchedPages(pointer + offset, size);
              }
            }
          }
          if (Configurator::Get().vulkan.recorder.memorySegmentSize) {
            memcpy(&mapping->compareData[offset], pointer + offset, size);
          }
        } else {
          LOG_WARNING << "Updating memory after QueueSubmit failed. Invalid values.";
        }
      }
    }
  }

  // Perform tasks which were waiting for queue submit end (i.e. acceleration structure building data acquisition)
  for (uint32_t i = 0; i < submitCount; i++) {
    for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
      auto it = SD()._commandbufferstates.find(pSubmits[i].pCommandBuffers[j]);
      if (it != SD()._commandbufferstates.end()) {
        ProcessOnQueueSubmitEndMessageReceivers(it->second, queue, queueWaitIdleAlreadyUsed);
      }
    }
  }
}

void CRecorderWrapper::resetMemoryAfterQueueSubmit2(VkQueue queue,
                                                    uint32_t submitCount,
                                                    const VkSubmitInfo2* pSubmits) {
  bool queueWaitIdleAlreadyUsed = false;

  if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
      Configurator::Get().vulkan.recorder.shadowMemory) {
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
              auto& mapping = memoryState->mapping;

              for (auto& obj3 : obj2.second.getIntervals()) {
                uint64_t offset = std::max(obj3.first, mapping->offset);
                uint64_t size = 0;
                uint64_t high_value = obj3.second;

                if (obj3.second > (mapping->offset + mapping->size)) {
                  high_value = mapping->offset + mapping->size;
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
            LOG_WARNING << "VkDeviceMemory " << obj2.first
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
      auto& mapping = memoryState->mapping;
      char* pointer = mapping->pData;

      for (auto& obj3 : obj2.second.getIntervals()) {
        uint64_t offset = obj3.first - mapping->offset;
        uint64_t size = obj3.second - obj3.first;

        if (offset + size <= mapping->size) {
          if (Configurator::Get().vulkan.recorder.shadowMemory) {
            if (Configurator::Get().vulkan.recorder.memoryAccessDetection) {
              SetPagesProtection(PageMemoryProtection::READ_WRITE, pointer + offset, size);

              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(offset + mapping->offset, size);
              } else {
                memoryState->shadowMemory->UpdateShadow(offset, size);
              }

              SetPagesProtection(PageMemoryProtection::READ_ONLY, pointer + offset, size);
            } else {
              if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
                memoryState->shadowMemory->UpdateShadow(offset + mapping->offset, size);
              } else {
                memoryState->shadowMemory->UpdateShadow(offset, size);
              }
              if (Configurator::Get().vulkan.recorder.writeWatchDetection) {
                WriteWatchSniffer::ResetTouchedPages(pointer + offset, size);
              }
            }
          }
          if (Configurator::Get().vulkan.recorder.memorySegmentSize) {
            memcpy(&mapping->compareData[offset], pointer + offset, size);
          }
        } else {
          LOG_WARNING << "Updating memory after QueueSubmit failed. Invalid values.";
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
      (Configurator::Get().vulkan.recorder.delayFenceChecksCount > 0)) {
    auto& fenceState = SD()._fencestates[fence];
    if (fenceState->delayChecksCount < Configurator::Get().vulkan.recorder.delayFenceChecksCount) {
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
  auto& cfg = Configurator::GetMutable();

  // By default record substreams from GITS Player using the dedicated/custom
  // method of memory changes tracking (with tags). Use the universal
  // recording/memory tracking only when forced through config option.
  if (!cfg.vulkan.recorder.forceUniversalRecording) {
    LOG_INFO << "Recording a (sub)stream from GITS Player. Disabling all memory tracking-related "
                "options!";

    cfg.vulkan.recorder.memoryAccessDetection = false;
    cfg.vulkan.recorder.memorySegmentSize = 0;
    cfg.vulkan.recorder.shadowMemory = false;
    cfg.vulkan.recorder.writeWatchDetection = false;
    cfg.vulkan.recorder.memoryUpdateState = MemoryUpdateState::USING_TAGS;
#ifdef GITS_PLATFORM_WINDOWS
    cfg.vulkan.recorder.useExternalMemoryExtension = false;
#endif
  }

#ifdef GITS_PLATFORM_WINDOWS
  // Disable other options when recording (sub)streams from GITS streams
  cfg.vulkan.recorder.usePresentSrcLayoutTransitionAsAFrameBoundary = false;
#endif
  vkIAmGITS_SD();
}

void CRecorderWrapper::StartStateRestore() const {
  auto& cfg = Configurator::Get();
  if (cfg.vulkan.recorder.mode == TVulkanRecorderMode::ALL && Configurator::DumpCCode() &&
      !CGits::Instance().IsCCodeStateRestore()) {
    CGits::Instance().CCodeStateRestoreStart();
    _recorder.Schedule(new CTokenMarker(CToken::ID_PRE_RECORD_END));
    _recorder.Schedule(new CTokenMarker(CToken::ID_INIT_START));
  }
}

void CRecorderWrapper::EndStateRestore() const {
  auto& cfg = Configurator::Get();
  if (cfg.vulkan.recorder.mode == TVulkanRecorderMode::ALL && Configurator::DumpCCode() &&
      CGits::Instance().IsCCodeStateRestore()) {
    _recorder.Schedule(new CTokenMarker(CToken::ID_INIT_END));
    _recorder.Schedule(new CTokenMarker(CToken::ID_FRAME_START));
    CGits::Instance().CCodeStateRestoreEnd();
  }
}

bool CRecorderWrapper::IsCCodeStateRestore() const {
  return CGits::Instance().IsCCodeStateRestore();
}

void CRecorderWrapper::StartFrame() const {
  _recorder.Schedule(new CTokenMarker(CToken::ID_PRE_RECORD_END));
  _recorder.Schedule(new CTokenMarker(CToken::ID_FRAME_START));
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

void CRecorderWrapper::SetConfig(Config const& cfg) const {}

bool CRecorderWrapper::IsUseExternalMemoryExtensionUsed() const {
  return isUseExternalMemoryExtensionUsed();
}

bool CRecorderWrapper::IsSubcaptureBeforeRestorationPhase() const {
  return isSubcaptureBeforeRestorationPhase();
}

} // namespace Vulkan
} // namespace gits
