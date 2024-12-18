// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateDynamic.h
*
* @brief Declaration of Vulkan library implementation.
*
*/
#pragma once

#include "vkWindowing.h"
#include "MemorySniffer.h"
#include "vulkanStructStorageAuto.h"
#include "intervalSet.h"

#include <array>
#include <unordered_set>

namespace gits {

/**
  * @brief Vulkan library specific GITS namespace
  */

namespace Vulkan {

struct CHWNDState;
struct CInstanceState;
struct CPhysicalDeviceState;
struct CSurfaceKHRState;
struct CDeviceState;
struct CQueueState;
struct CSwapchainKHRState;
struct CDescriptorPoolState;
struct CCommandPoolState;
struct CSamplerState;
struct CDeviceMemoryState;
struct CImageState;
struct CImageViewState;
struct CBufferState;
struct CBufferViewState;
struct CDescriptorSetLayoutState;
struct CDescriptorSetState;
struct CPipelineLayoutState;
struct CDescriptorUpdateTemplateState;
struct CPipelineCacheState;
struct CShaderModuleState;
struct CRenderPassState;
struct CPipelineState;
struct CFramebufferState;
struct CFenceState;
struct CEventState;
struct CSemaphoreState;
struct CQueryPoolState;
struct CCommandBufferState;
struct CDeferredOperationKHRState;
struct CAccelerationStructureKHRState;
struct CQueueSubmitState;
struct CMemoryUpdateState;

// Resource Type

typedef enum _VulkanResourceType {
  VULKAN_NONE_RESOURCE,
  //READ_WRITE
  VULKAN_STORAGE_IMAGE,
  VULKAN_STORAGE_BUFFER,
  VULKAN_STORAGE_TEXEL_BUFFER,
  VULKAN_STORAGE_BUFFER_DYNAMIC,
  //READ_ONLY
  VULKAN_SAMPLED_IMAGE,
  VULKAN_UNIFORM_TEXEL_BUFFER,
  VULKAN_UNIFORM_BUFFER,
  VULKAN_UNIFORM_BUFFER_DYNAMIC,
  VULKAN_INLINE_UNIFORM_BLOCK_EXT,
  VULKAN_INPUT_ATTACHMENT,
  VULKAN_ACCELARATION_STRUCTURE_KHR,
  VULKAN_ACCELARATION_STRUCTURE_NV,
  VULKAN_SAMPLER,
  VULKAN_COMBINED_IMAGE_SAMPLER,
  //WRITE
  VULKAN_BLIT_DESTINATION_BUFFER,
  VULKAN_BLIT_DESTINATION_IMAGE,
  VULKAN_RESOLVE_IMAGE
} VulkanResourceType;

// Creation function

enum class CreationFunction {
  KHR_EXTENSION,
  CORE_1_0,
  CORE_1_1,
  CORE_1_2
};

// Unique handle

class UniqueResourceHandle {
public:
  uint64_t GetUniqueStateID() {
    return stateID;
  }

protected:
  UniqueResourceHandle() {
    static uint64_t globalID = 1;
    stateID = globalID++;
  }

private:
  uint64_t stateID;
};

// HWND

struct CHWNDState {
  int x, y, w, h;
  bool vis;
  std::unique_ptr<Window_> window;

  CHWNDState(int _x, int _y, int _w, int _h, bool _vis, Window_* _window)
      : x(_x), y(_y), w(_w), h(_h), vis(_vis) {
    if (_window != nullptr) {
      window = std::make_unique<Window_>(*_window);
    }
  }
};

// Instance

struct CInstanceState : public UniqueResourceHandle {
  VkInstance instanceHandle;
  CVkInstanceCreateInfoData instanceCreateInfoData;
  uint32_t vulkanVersionMajor;
  uint32_t vulkanVersionMinor;

  CInstanceState(VkInstance const* _pInstance, VkInstanceCreateInfo const* _pCreateInfo)
      : instanceHandle(*_pInstance),
        instanceCreateInfoData(_pCreateInfo),
        vulkanVersionMajor(1),
        vulkanVersionMinor(0) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    for (auto obj : instanceCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Physical device

struct CPhysicalDeviceState : public UniqueResourceHandle {
  VkPhysicalDevice physicalDeviceHandle;
  std::vector<std::string> supportedExtensions;
  VkPhysicalDeviceMemoryProperties memoryPropertiesOriginal;
  VkPhysicalDeviceMemoryProperties memoryPropertiesCurrent;
  std::shared_ptr<CInstanceState> instanceStateStore;
  // mapping original memory type index into current platform memory type index
  std::unordered_map<uint32_t, uint32_t> correspondingMemoryTypeIndexes;
  std::vector<VkQueueFamilyProperties> queueFamilyPropertiesOriginal;
  std::vector<VkQueueFamilyProperties> queueFamilyPropertiesCurrent;

  CPhysicalDeviceState(VkPhysicalDevice _physicalDevice,
                       std::shared_ptr<CInstanceState>& _instanceState)
      : physicalDeviceHandle(_physicalDevice),
        memoryPropertiesOriginal(),
        instanceStateStore(_instanceState) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);
    drvVk.vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryPropertiesCurrent);
    getCurrentQueueFamilyProperties();
  }

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)instanceStateStore->instanceHandle);
    for (auto obj : instanceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }

private:
  void getCurrentQueueFamilyProperties() {
    uint32_t queueFamilyCount = 0;
    drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount,
                                                   nullptr);
    if (queueFamilyCount == 0) {
      throw std::runtime_error(
          "No queue family available on the current platform. Potential driver "
          "bug or driver installation issues. Exiting!");
    }
    queueFamilyPropertiesCurrent.resize(queueFamilyCount);
    drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount,
                                                   queueFamilyPropertiesCurrent.data());
  }
};

// Surface

struct CSurfaceKHRState : public UniqueResourceHandle {
  VkSurfaceKHR surfaceKHRHandle;
  CVkWin32SurfaceCreateInfoKHRData surfaceCreateInfoWin32Data;
  CVkXcbSurfaceCreateInfoKHRData surfaceCreateInfoXcbData;
  CVkXlibSurfaceCreateInfoKHRData surfaceCreateInfoXlibData;
  std::shared_ptr<CInstanceState> instanceStateStore;

  CSurfaceKHRState(VkSurfaceKHR const* _pSurface,
                   VkWin32SurfaceCreateInfoKHR const* _pCreateInfo,
                   std::shared_ptr<CInstanceState>& _instanceState)
      : surfaceKHRHandle(*_pSurface),
        surfaceCreateInfoWin32Data(_pCreateInfo),
        surfaceCreateInfoXcbData(nullptr),
        surfaceCreateInfoXlibData(nullptr),
        instanceStateStore(_instanceState) {}
  CSurfaceKHRState(VkSurfaceKHR const* _pSurface,
                   VkXcbSurfaceCreateInfoKHR const* _pCreateInfo,
                   std::shared_ptr<CInstanceState>& _instanceState)
      : surfaceKHRHandle(*_pSurface),
        surfaceCreateInfoWin32Data(nullptr),
        surfaceCreateInfoXcbData(_pCreateInfo),
        surfaceCreateInfoXlibData(nullptr),
        instanceStateStore(_instanceState) {}
  CSurfaceKHRState(VkSurfaceKHR const* _pSurface,
                   VkXlibSurfaceCreateInfoKHR const* _pCreateInfo,
                   std::shared_ptr<CInstanceState>& _instanceState)
      : surfaceKHRHandle(*_pSurface),
        surfaceCreateInfoWin32Data(nullptr),
        surfaceCreateInfoXcbData(nullptr),
        surfaceCreateInfoXlibData(_pCreateInfo),
        instanceStateStore(_instanceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)instanceStateStore->instanceHandle);
    for (auto obj : surfaceCreateInfoWin32Data.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : surfaceCreateInfoXcbData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : surfaceCreateInfoXlibData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : instanceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Device

struct CDeviceState : public UniqueResourceHandle {
  VkDevice deviceHandle;
  CVkDeviceCreateInfoData deviceCreateInfoData;
  std::shared_ptr<CPhysicalDeviceState> physicalDeviceStateStore;
  std::vector<std::shared_ptr<CQueueState>> queueStateStoreList;
  std::vector<std::string> enabledExtensions;

  CDeviceState(VkDevice const* _pDevice,
               VkDeviceCreateInfo const* _pCreateInfo,
               std::shared_ptr<CPhysicalDeviceState>& _physicalDeviceState)
      : deviceHandle(*_pDevice),
        deviceCreateInfoData(_pCreateInfo),
        physicalDeviceStateStore(_physicalDeviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)physicalDeviceStateStore->physicalDeviceHandle);
    for (auto obj : physicalDeviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : deviceCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Queue

struct CQueueState : public UniqueResourceHandle {
  VkQueue queueHandle;
  uint32_t queueFamilyIndex;
  uint32_t queueIndex;
  VkQueueFlags queueFlags;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CQueueState(VkQueue const* _pQueue,
              uint32_t _queueFamilyIndex,
              uint32_t _queueIndex,
              VkQueueFlags _queueFlags,
              std::shared_ptr<CDeviceState>& _deviceState)
      : queueHandle(*_pQueue),
        queueFamilyIndex(_queueFamilyIndex),
        queueIndex(_queueIndex),
        queueFlags(_queueFlags),
        deviceStateStore(_deviceState) {}
};

// Swapchain

struct CSwapchainKHRState : public UniqueResourceHandle {
  VkSwapchainKHR swapchainKHRHandle;
  CVkSwapchainCreateInfoKHRData swapchainCreateInfoKHRData;
  std::unordered_set<uint32_t> acquiredImages;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CSurfaceKHRState> surfaceKHRStateStore;
  std::vector<std::shared_ptr<CImageState>> imageStateStoreList;

  CSwapchainKHRState(VkSwapchainKHR const* _pSwapchain,
                     VkSwapchainCreateInfoKHR const* _pCreateInfo,
                     std::shared_ptr<CDeviceState>& _deviceState,
                     std::shared_ptr<CSurfaceKHRState>& _surfaceState)
      : swapchainKHRHandle(*_pSwapchain),
        swapchainCreateInfoKHRData(_pCreateInfo),
        deviceStateStore(_deviceState),
        surfaceKHRStateStore(_surfaceState) {}

  std::set<uint64_t>
  GetMappedPointers(); // <- due to circular dependency between CImageState and CSwapchainKHRState definition moved to .cpp file
};

// Descriptor pool

struct CDescriptorPoolState : public UniqueResourceHandle {
  VkDescriptorPool descriptorPoolHandle;
  CVkDescriptorPoolCreateInfoData descriptorPoolCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::unordered_set<std::shared_ptr<CDescriptorSetState>> descriptorSetStateStoreList;

  CDescriptorPoolState(VkDescriptorPool const* _pDescriptorPool,
                       VkDescriptorPoolCreateInfo const* _pCreateInfo,
                       std::shared_ptr<CDeviceState>& _deviceState)
      : descriptorPoolHandle(*_pDescriptorPool),
        descriptorPoolCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : descriptorPoolCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Command pool

struct CCommandPoolState : public UniqueResourceHandle {
  VkCommandPool commandPoolHandle;
  CVkCommandPoolCreateInfoData commandPoolCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::unordered_set<std::shared_ptr<CCommandBufferState>> commandBufferStateStoreList;

  CCommandPoolState(VkCommandPool const* _pCommandPool,
                    VkCommandPoolCreateInfo const* _pCreateInfo,
                    std::shared_ptr<CDeviceState>& _deviceState)
      : commandPoolHandle(*_pCommandPool),
        commandPoolCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : commandPoolCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Sampler

struct CSamplerState : public UniqueResourceHandle {
  VkSampler samplerHandle;
  CVkSamplerCreateInfoData samplerCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CSamplerState(VkSampler const* _pSampler,
                VkSamplerCreateInfo const* _pCreateInfo,
                std::shared_ptr<CDeviceState>& _deviceState)
      : samplerHandle(*_pSampler),
        samplerCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : samplerCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Device memory

struct CDeviceMemoryState : public UniqueResourceHandle {
  struct CMapping {
    Cuint64_tData offsetData;
    Cuint64_tData sizeData;
    Cuint32_tData flagsData;
    CvoidPtrData ppDataData;
    PagedMemoryRegionHandle sniffedRegionHandle;
    std::vector<char> compareData;

  public:
    CMapping(const VkDeviceSize* _offset,
             VkDeviceSize* _size,
             VkMemoryMapFlags* _flags,
             void** _ppData)
        : offsetData(_offset),
          sizeData(_size),
          flagsData(_flags),
          ppDataData(*_ppData),
          sniffedRegionHandle(0) {
      if (Config::Get().IsRecorder() && Config::Get().vulkan.recorder.memorySegmentSize) {
        compareData.resize((size_t)*_size);
        memcpy(&compareData[0], (char*)*_ppData, (size_t)*_size);
      }
    }
  };

  VkDeviceMemory deviceMemoryHandle;
  CVkMemoryAllocateInfoData memoryAllocateInfoData;
  std::shared_ptr<CMapping> mapping;
  std::shared_ptr<ShadowBuffer> shadowMemory;
  std::shared_ptr<CDeviceState> deviceStateStore;
  void* externalMemory;

  MemoryAliasingTracker aliasingTracker;

  CDeviceMemoryState(VkDeviceMemory const* _pDeviceMemory,
                     VkMemoryAllocateInfo const* _pAllocateInfo,
                     std::shared_ptr<CDeviceState>& _deviceState,
                     void* _externalMemory)
      : deviceMemoryHandle(*_pDeviceMemory),
        memoryAllocateInfoData(_pAllocateInfo),
        deviceStateStore(_deviceState),
        externalMemory(_externalMemory),
        aliasingTracker(_pAllocateInfo->allocationSize) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : memoryAllocateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }

  bool IsMapped() const {
    return static_cast<bool>(mapping);
  }
};

struct CMemoryBinding {
  VkDeviceSize memoryOffset;
  VkDeviceSize memorySizeRequirement;
  std::shared_ptr<CDeviceMemoryState> deviceMemoryStateStore;

  CMemoryBinding(VkDeviceSize _memoryOffset,
                 VkDeviceSize _memorySizeRequirement,
                 std::shared_ptr<CDeviceMemoryState>& _deviceMemoryState)
      : memoryOffset(_memoryOffset),
        memorySizeRequirement(_memorySizeRequirement),
        deviceMemoryStateStore(_deviceMemoryState) {}
};

// Image

struct CImageLayoutAccessOwnershipState {
  VkImageLayout Layout;
  VkAccessFlags2 Access;
  uint32_t QueueFamilyIndex;
};

struct CImageState : public UniqueResourceHandle {
  VkImage imageHandle;
  CVkImageCreateInfoData imageCreateInfoData;
  int32_t width;
  int32_t height;
  int32_t depth;
  uint32_t mipLevels;
  uint32_t arrayLayers;
  VkFormat imageFormat;
  VkImageType imageType;
  std::shared_ptr<CMemoryBinding> binding;
  std::vector<std::pair<std::shared_ptr<CVkSparseMemoryBindData>,
                        std::shared_ptr<CVkSparseImageMemoryBindData>>>
      sparseBindings;
  VkMemoryRequirements memoryRequirements;
  std::vector<std::vector<CImageLayoutAccessOwnershipState>> currentLayout; // per layer, per mipmap
  uint64_t timestamp;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CSwapchainKHRState> swapchainKHRStateStore;

  CImageState(VkImage const* _pImage,
              VkImageCreateInfo const* _pCreateInfo,
              std::shared_ptr<CDeviceState>& _deviceState)
      : imageHandle(*_pImage),
        imageCreateInfoData(_pCreateInfo),
        width(_pCreateInfo->extent.width),
        height(_pCreateInfo->extent.height),
        depth(_pCreateInfo->extent.depth),
        mipLevels(_pCreateInfo->mipLevels),
        arrayLayers(_pCreateInfo->arrayLayers),
        imageFormat(_pCreateInfo->format),
        imageType(_pCreateInfo->imageType),
        memoryRequirements{},
        currentLayout(_pCreateInfo->arrayLayers),
        timestamp(0),
        deviceStateStore(_deviceState) {
    for (auto& layer : currentLayout) {
      layer.resize(_pCreateInfo->mipLevels,
                   {_pCreateInfo->initialLayout, 0, VK_QUEUE_FAMILY_IGNORED});
    }
  }

  CImageState(VkImage const* _pImage, std::shared_ptr<CSwapchainKHRState>& _swapchainKHRState)
      : imageHandle(*_pImage),
        imageCreateInfoData(nullptr),
        width(_swapchainKHRState->swapchainCreateInfoKHRData.Value()->imageExtent.width),
        height(_swapchainKHRState->swapchainCreateInfoKHRData.Value()->imageExtent.height),
        depth(1),
        mipLevels(1),
        arrayLayers(_swapchainKHRState->swapchainCreateInfoKHRData.Value()->imageArrayLayers),
        imageFormat(_swapchainKHRState->swapchainCreateInfoKHRData.Value()->imageFormat),
        imageType(VK_IMAGE_TYPE_2D),
        currentLayout(_swapchainKHRState->swapchainCreateInfoKHRData.Value()->imageArrayLayers),
        timestamp(0),
        deviceStateStore(_swapchainKHRState->deviceStateStore),
        swapchainKHRStateStore(_swapchainKHRState) {
    for (auto& layer : currentLayout) {
      layer.resize(1, {VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_QUEUE_FAMILY_IGNORED});
    }
  }

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    if (swapchainKHRStateStore) {
      pointers.insert((uint64_t)swapchainKHRStateStore->swapchainKHRHandle);
    } else {
      pointers.insert((uint64_t)deviceStateStore->deviceHandle);
      for (auto obj : deviceStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
      for (auto obj : imageCreateInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    if (binding) {
      pointers.insert((uint64_t)binding->deviceMemoryStateStore->deviceMemoryHandle);
      for (auto obj : binding->deviceMemoryStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    return pointers;
  }
};

// Image view

struct CImageViewState : public UniqueResourceHandle {
  VkImageView imageViewHandle;
  CVkImageViewCreateInfoData imageViewCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CImageState> imageStateStore;

  CImageViewState(VkImageView const* _pImageView,
                  const VkImageViewCreateInfo* _pCreateInfo,
                  std::shared_ptr<CDeviceState>& _deviceState,
                  std::shared_ptr<CImageState>& _imageState)
      : imageViewHandle(*_pImageView),
        imageViewCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState),
        imageStateStore(_imageState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)imageStateStore->imageHandle);
    for (auto obj : imageStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : imageViewCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Buffer

struct CBufferState : public UniqueResourceHandle {
  VkBuffer bufferHandle;
  CVkBufferCreateInfoData bufferCreateInfoData;
  std::shared_ptr<CMemoryBinding> binding;
  VkMemoryRequirements memoryRequirements;
  std::vector<std::shared_ptr<CVkSparseMemoryBindData>> sparseBindings;
  VkDeviceAddress deviceAddress;
  std::unordered_set<VkDeviceAddress> deviceAddressesToErase;
  uint64_t timestamp;
  std::shared_ptr<CDeviceState> deviceStateStore;

  struct DeviceAddressRangeState {
    VkDeviceAddress start;
    VkDeviceAddress end;
    VkBuffer buffer;

    bool operator()(DeviceAddressRangeState const& x, DeviceAddressRangeState const& y) const {
      return x.start < y.start;
    }
  };
  static std::set<DeviceAddressRangeState, DeviceAddressRangeState> deviceAddresses;
  static std::unordered_map<VkDeviceAddress, VkBuffer> deviceAddressesQuickLook;

  // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
  // Please, (un)comment all the areas with the above token together, at the same time
  //
  // static std::unordered_map<VkBuffer, std::shared_ptr<CBufferState>> shaderDeviceAddressBuffers;

  CBufferState(VkBuffer const* _pBuffer,
               VkBufferCreateInfo const* _pCreateInfo,
               std::shared_ptr<CDeviceState>& _deviceState)
      : bufferHandle(*_pBuffer),
        bufferCreateInfoData(_pCreateInfo),
        memoryRequirements{},
        deviceAddress(0),
        timestamp(0),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : bufferCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    if (binding) {
      pointers.insert((uint64_t)binding->deviceMemoryStateStore->deviceMemoryHandle);
      for (auto obj : binding->deviceMemoryStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    return pointers;
  }
};

// Buffer view

struct CBufferViewState : public UniqueResourceHandle {
  VkBufferView bufferViewHandle;
  CVkBufferViewCreateInfoData bufferViewCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CBufferState> bufferStateStore;

  CBufferViewState(VkBufferView const* _pBufferView,
                   const VkBufferViewCreateInfo* _pCreateInfo,
                   std::shared_ptr<CDeviceState>& _deviceState,
                   std::shared_ptr<CBufferState>& _bufferState)
      : bufferViewHandle(*_pBufferView),
        bufferViewCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState),
        bufferStateStore(_bufferState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)bufferStateStore->bufferHandle);
    for (auto obj : bufferStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : bufferViewCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Descriptor set layout

struct CDescriptorSetLayoutState : public UniqueResourceHandle {
  VkDescriptorSetLayout descriptorSetLayoutHandle;
  CVkDescriptorSetLayoutCreateInfoData descriptorSetLayoutCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CDescriptorSetLayoutState(VkDescriptorSetLayout const* _pDescriptorSetLayout,
                            VkDescriptorSetLayoutCreateInfo const* _pCreateInfo,
                            std::shared_ptr<CDeviceState>& _deviceState)
      : descriptorSetLayoutHandle(*_pDescriptorSetLayout),
        descriptorSetLayoutCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : descriptorSetLayoutCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Descriptor set

struct CDescriptorSetState : public UniqueResourceHandle {
  struct CDescriptorSetBindingData {
    struct CDescriptorData {
      std::shared_ptr<CVkDescriptorImageInfoData> pImageInfo;
      std::shared_ptr<CSamplerState> samplerStateStore;
      std::shared_ptr<CImageViewState> imageViewStateStore;

      std::shared_ptr<CVkDescriptorBufferInfoData> pBufferInfo;
      std::shared_ptr<CBufferState> bufferStateStore;

      std::shared_ptr<CVkBufferViewDataArray> pTexelBufferView;
      std::shared_ptr<CBufferViewState> bufferViewStateStore;

      std::vector<unsigned char> inlineUniformBlockData;
      std::shared_ptr<CAccelerationStructureKHRState> accelerationStructureStateStore;
    };

    VkDescriptorType descriptorType;
    std::vector<CDescriptorData> descriptorData;
    uint32_t descriptorCount;

    std::set<uint64_t> GetMappedPointers() {
      std::set<uint64_t> pointers;
      for (auto& obj : descriptorData) {
        if (obj.pImageInfo) {
          for (auto elem : obj.pImageInfo->GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
        if (obj.pBufferInfo) {
          for (auto elem : obj.pBufferInfo->GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
        if (obj.pTexelBufferView) {
          for (auto elem : obj.pTexelBufferView->GetMappedPointers()) {
            pointers.insert((uint64_t)elem);
          }
        }
      }
      return pointers;
    }
  };

  VkDescriptorSet descriptorSetHandle;
  CpNextWrapperData extensionsDataChain;
  std::unordered_map<uint32_t, VkBuffer> descriptorBuffers;
  std::unordered_map<uint32_t, VkImage> descriptorImages;
  std::unordered_map<uint32_t, std::pair<VulkanResourceType, VkBuffer>> descriptorWriteBuffers;
  std::unordered_map<uint32_t, std::pair<VulkanResourceType, VkImage>> descriptorWriteImages;
  std::unordered_map<uint32_t, CDescriptorSetBindingData> descriptorSetBindings;
  std::unordered_map<uint32_t, std::unordered_map<VkDeviceMemory, IntervalSet<uint64_t>>>
      descriptorMapMemory;
  std::shared_ptr<CDescriptorPoolState> descriptorPoolStateStore;
  std::shared_ptr<CDescriptorSetLayoutState> descriptorSetLayoutStateStore;

  CDescriptorSetState(VkDescriptorSet const* _pDescriptorSet,
                      const void* _pNextChain,
                      std::shared_ptr<CDescriptorPoolState>& _descriptorPoolState,
                      std::shared_ptr<CDescriptorSetLayoutState>& _descriptorSetLayoutState)
      : descriptorSetHandle(*_pDescriptorSet),
        extensionsDataChain(_pNextChain),
        descriptorPoolStateStore(_descriptorPoolState),
        descriptorSetLayoutStateStore(_descriptorSetLayoutState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)descriptorPoolStateStore->descriptorPoolHandle);
    for (auto obj : descriptorPoolStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    pointers.insert((uint64_t)descriptorSetLayoutStateStore->descriptorSetLayoutHandle);
    for (auto obj : descriptorSetLayoutStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto& elem : descriptorSetBindings) {
      for (uint64_t obj : elem.second.GetMappedPointers()) {
        pointers.insert(obj);
      }
    }
    return pointers;
  }
};

// Pipeline layout

struct CPipelineLayoutState : public UniqueResourceHandle {
  VkPipelineLayout pipelineLayoutHandle;
  CVkPipelineLayoutCreateInfoData pipelineLayoutCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::vector<std::shared_ptr<CDescriptorSetLayoutState>> descriptorSetLayoutStates;

  CPipelineLayoutState(VkPipelineLayout const* _pPipelineLayout,
                       VkPipelineLayoutCreateInfo const* _pCreateInfo,
                       std::shared_ptr<CDeviceState>& _deviceState)
      : pipelineLayoutHandle(*_pPipelineLayout),
        pipelineLayoutCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : pipelineLayoutCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Descriptor update template

struct CDescriptorUpdateTemplateState : public UniqueResourceHandle {
  VkDescriptorUpdateTemplate descriptorUpdateTemplateHandle;
  CVkDescriptorUpdateTemplateCreateInfoData descriptorUpdateTemplateCreateInfoData;
  CreationFunction createdWith;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CPipelineLayoutState> pipelineLayoutStateStore;

  CDescriptorUpdateTemplateState(VkDescriptorUpdateTemplate const* _pDescriptorUpdateTemplate,
                                 VkDescriptorUpdateTemplateCreateInfo const* _pCreateInfo,
                                 CreationFunction _createdWith,
                                 std::shared_ptr<CDeviceState>& _deviceState)
      : descriptorUpdateTemplateHandle(*_pDescriptorUpdateTemplate),
        descriptorUpdateTemplateCreateInfoData(_pCreateInfo),
        createdWith(_createdWith),
        deviceStateStore(_deviceState) {}
  CDescriptorUpdateTemplateState(VkDescriptorUpdateTemplate const* _pDescriptorUpdateTemplate,
                                 VkDescriptorUpdateTemplateCreateInfo const* _pCreateInfo,
                                 CreationFunction _createdWith,
                                 std::shared_ptr<CPipelineLayoutState>& _pipelineLayoutState)
      : descriptorUpdateTemplateHandle(*_pDescriptorUpdateTemplate),
        descriptorUpdateTemplateCreateInfoData(_pCreateInfo),
        createdWith(_createdWith),
        deviceStateStore(_pipelineLayoutState->deviceStateStore),
        pipelineLayoutStateStore(_pipelineLayoutState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    if (pipelineLayoutStateStore) {
      pointers.insert((uint64_t)pipelineLayoutStateStore->pipelineLayoutHandle);
      for (auto obj : pipelineLayoutStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    } else {
      pointers.insert((uint64_t)deviceStateStore->deviceHandle);
      for (auto obj : deviceStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    for (auto obj : descriptorUpdateTemplateCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Pipeline cache

struct CPipelineCacheState : public UniqueResourceHandle {
  VkPipelineCache pipelineCacheHandle;
  CVkPipelineCacheCreateInfoData pipelineCacheCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CPipelineCacheState(VkPipelineCache const* _pPipelineCache,
                      VkPipelineCacheCreateInfo const* _pCreateInfo,
                      std::shared_ptr<CDeviceState>& _deviceState)
      : pipelineCacheHandle(*_pPipelineCache),
        pipelineCacheCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : pipelineCacheCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Shader module

struct CShaderModuleState : public UniqueResourceHandle {
  VkShaderModule shaderModuleHandle;
  CVkShaderModuleCreateInfoData shaderModuleCreateInfoData;
  uint32_t shaderHash;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CShaderModuleState(VkShaderModule const* _pShaderModule,
                     VkShaderModuleCreateInfo const* _pCreateInfo,
                     uint32_t _hash,
                     std::shared_ptr<CDeviceState>& _deviceState)
      : shaderModuleHandle(*_pShaderModule),
        shaderModuleCreateInfoData(_pCreateInfo),
        shaderHash(_hash),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : shaderModuleCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Render pass

struct CRenderPassState : public UniqueResourceHandle {
  VkRenderPass renderPassHandle;
  CVkRenderPassCreateInfoData renderPassCreateInfoData;
  CVkRenderPassCreateInfo2Data renderPassCreateInfo2Data;
  CreationFunction createdWith;
  std::vector<VkImageLayout> finalImageLayoutList;
  std::shared_ptr<CDeviceState> deviceStateStore;
  VkRenderPass loadAndStoreRenderPassHandle;
  VkRenderPass restoreRenderPassHandle;
  std::shared_ptr<CRenderPassState> restoreRenderPassStateStore;
  VkRenderPass storeNoLoadRenderPassHandle;

  CRenderPassState(VkRenderPass const* _pRenderPass,
                   VkRenderPassCreateInfo const* _pCreateInfo,
                   CreationFunction _createdWith,
                   std::shared_ptr<CDeviceState>& _deviceState)
      : renderPassHandle(*_pRenderPass),
        renderPassCreateInfoData(_pCreateInfo),
        renderPassCreateInfo2Data(nullptr),
        createdWith(_createdWith),
        finalImageLayoutList(_pCreateInfo->attachmentCount),
        deviceStateStore(_deviceState) {
    for (uint32_t i = 0; i < _pCreateInfo->attachmentCount; ++i) {
      finalImageLayoutList[i] = _pCreateInfo->pAttachments[i].finalLayout;
    }
  }
  CRenderPassState(VkRenderPass const* _pRenderPass,
                   VkRenderPassCreateInfo2 const* _pCreateInfo,
                   CreationFunction _createdWith,
                   std::shared_ptr<CDeviceState>& _deviceState)
      : renderPassHandle(*_pRenderPass),
        renderPassCreateInfoData(nullptr),
        renderPassCreateInfo2Data(_pCreateInfo),
        createdWith(_createdWith),
        finalImageLayoutList(_pCreateInfo->attachmentCount),
        deviceStateStore(_deviceState) {
    for (uint32_t i = 0; i < _pCreateInfo->attachmentCount; ++i) {
      finalImageLayoutList[i] = _pCreateInfo->pAttachments[i].finalLayout;
    }
  }

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    if (renderPassCreateInfoData.Value()) {
      for (auto obj : renderPassCreateInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    } else if (renderPassCreateInfo2Data.Value()) {
      for (auto obj : renderPassCreateInfo2Data.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    return pointers;
  }
};

// Pipeline

struct CPipelineState : public UniqueResourceHandle {
  VkPipeline pipelineHandle;
  CVkGraphicsPipelineCreateInfoData graphicsPipelineCreateInfoData;
  CVkComputePipelineCreateInfoData computePipelineCreateInfoData;
  CVkRayTracingPipelineCreateInfoKHRData rayTracingPipelineCreateInfoData;
  std::unordered_map<VkShaderStageFlagBits, uint32_t> stageShaderHashMapping;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CPipelineLayoutState> pipelineLayoutStateStore;
  std::shared_ptr<CRenderPassState> renderPassStateStore;
  std::vector<std::shared_ptr<CShaderModuleState>> shaderModuleStateStoreList;

  CPipelineState(VkPipeline const* _pPipeline,
                 VkGraphicsPipelineCreateInfo const* _pCreateInfo,
                 std::shared_ptr<CDeviceState>& _deviceState,
                 std::shared_ptr<CPipelineLayoutState>& _pipelineLayoutState,
                 std::shared_ptr<CRenderPassState>& _renderPassState)
      : pipelineHandle(*_pPipeline),
        graphicsPipelineCreateInfoData(_pCreateInfo),
        computePipelineCreateInfoData(nullptr),
        rayTracingPipelineCreateInfoData(nullptr, VK_NULL_HANDLE),
        deviceStateStore(_deviceState),
        pipelineLayoutStateStore(_pipelineLayoutState),
        renderPassStateStore(_renderPassState) {}

  CPipelineState(VkPipeline const* _pPipeline,
                 VkComputePipelineCreateInfo const* _pCreateInfo,
                 std::shared_ptr<CDeviceState>& _deviceState,
                 std::shared_ptr<CPipelineLayoutState>& _pipelineLayoutState)
      : pipelineHandle(*_pPipeline),
        graphicsPipelineCreateInfoData(nullptr),
        computePipelineCreateInfoData(_pCreateInfo),
        rayTracingPipelineCreateInfoData(nullptr, VK_NULL_HANDLE),
        deviceStateStore(_deviceState),
        pipelineLayoutStateStore(_pipelineLayoutState) {}

  CPipelineState(VkPipeline const* _pPipeline,
                 VkRayTracingPipelineCreateInfoKHR const* _pCreateInfo,
                 std::shared_ptr<CDeviceState>& _deviceState,
                 std::shared_ptr<CPipelineLayoutState>& _pipelineLayoutState)
      : pipelineHandle(*_pPipeline),
        graphicsPipelineCreateInfoData(nullptr),
        computePipelineCreateInfoData(nullptr),
        rayTracingPipelineCreateInfoData(_pCreateInfo, _deviceState->deviceHandle),
        deviceStateStore(_deviceState),
        pipelineLayoutStateStore(_pipelineLayoutState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)pipelineLayoutStateStore->pipelineLayoutHandle);
    for (auto obj : pipelineLayoutStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    if (renderPassStateStore) {
      pointers.insert((uint64_t)renderPassStateStore->renderPassHandle);
      for (auto obj : renderPassStateStore->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    for (auto& shaderModuleState : shaderModuleStateStoreList) {
      if (shaderModuleState == nullptr) {
        continue;
      }

      pointers.insert((uint64_t)shaderModuleState->shaderModuleHandle);
      for (auto obj : shaderModuleState->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    if (graphicsPipelineCreateInfoData.Value()) {
      for (auto obj : graphicsPipelineCreateInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    if (computePipelineCreateInfoData.Value()) {
      for (auto obj : computePipelineCreateInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    if (rayTracingPipelineCreateInfoData.Value()) {
      for (auto obj : rayTracingPipelineCreateInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    return pointers;
  }
};

// Framebuffer

struct CFramebufferState : public UniqueResourceHandle {
  VkFramebuffer framebufferHandle;
  CVkFramebufferCreateInfoData framebufferCreateInfoData;
  std::shared_ptr<CDeviceState> deviceStateStore;
  std::shared_ptr<CRenderPassState> renderPassStateStore;
  std::vector<std::shared_ptr<CImageViewState>> imageViewStateStoreList;

  CFramebufferState(VkFramebuffer const* _pFramebuffer,
                    VkFramebufferCreateInfo const* _pCreateInfo,
                    std::shared_ptr<CDeviceState>& _deviceState,
                    std::shared_ptr<CRenderPassState>& _renderPassState)
      : framebufferHandle(*_pFramebuffer),
        framebufferCreateInfoData(_pCreateInfo),
        deviceStateStore(_deviceState),
        renderPassStateStore(_renderPassState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)renderPassStateStore->renderPassHandle);
    for (auto obj : renderPassStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : framebufferCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Fence

struct CFenceState : public UniqueResourceHandle {
  VkFence fenceHandle;
  CVkFenceCreateInfoData fenceCreateInfoData;
  bool fenceUsed;
  unsigned int delayChecksCount;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CFenceState(VkFence const* _pFence,
              VkFenceCreateInfo const* _pCreateInfo,
              std::shared_ptr<CDeviceState>& _deviceState)
      : fenceHandle(*_pFence),
        fenceCreateInfoData(_pCreateInfo),
        fenceUsed(_pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT),
        delayChecksCount((_pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT)
                             ? Config::Get().vulkan.recorder.delayFenceChecksCount
                             : 0),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : fenceCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Event

struct CEventState : public UniqueResourceHandle {
  VkEvent eventHandle;
  CVkEventCreateInfoData eventCreateInfoData;
  bool eventUsed;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CEventState(VkEvent const* _pEvent,
              VkEventCreateInfo const* _pCreateInfo,
              std::shared_ptr<CDeviceState>& _deviceState)
      : eventHandle(*_pEvent),
        eventCreateInfoData(_pCreateInfo),
        eventUsed(false),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : eventCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Semaphore

struct CSemaphoreState : public UniqueResourceHandle {
  VkSemaphore semaphoreHandle;
  CVkSemaphoreCreateInfoData semaphoreCreateInfoData;
  bool semaphoreUsed;
  bool isTimeline;
  uint64_t timelineSemaphoreValue;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CSemaphoreState(VkSemaphore const* _pSemaphore,
                  VkSemaphoreCreateInfo const* _pCreateInfo,
                  std::shared_ptr<CDeviceState>& _deviceState)
      : semaphoreHandle(*_pSemaphore),
        semaphoreCreateInfoData(_pCreateInfo),
        semaphoreUsed(false),
        isTimeline(false),
        timelineSemaphoreValue(0),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : semaphoreCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Query pool

struct CQueryPoolState : public UniqueResourceHandle {
  VkQueryPool queryPoolHandle;
  CVkQueryPoolCreateInfoData queryPoolCreateInfoData;
  std::vector<bool> resetQueries;
  std::vector<bool> usedQueries;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CQueryPoolState(VkQueryPool const* _pQueryPool,
                  VkQueryPoolCreateInfo const* _pCreateInfo,
                  std::shared_ptr<CDeviceState>& _deviceState)
      : queryPoolHandle(*_pQueryPool),
        queryPoolCreateInfoData(_pCreateInfo),
        resetQueries(_pCreateInfo->queryCount, false),
        usedQueries(_pCreateInfo->queryCount, false),
        deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : queryPoolCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

struct RenderGenericAttachment {
  VkBuffer copiedBuffer;
  VkImage sourceImage;
  VkBuffer sourceBuffer;
  VkDeviceMemory devMemory;
  uint32_t layer;
  uint32_t mipmap;
  VkImageAspectFlags aspect;
  std::string fileName;
  RenderGenericAttachment()
      : copiedBuffer(0),
        sourceImage(0),
        sourceBuffer(0),
        devMemory(0),
        layer(0),
        mipmap(0),
        aspect(0),
        fileName() {}
};

// Command buffer

struct CCommandBufferState : public UniqueResourceHandle {
  struct CBeginCommandBuffer {
    CVkCommandBufferBeginInfoData commandBufferBeginInfoData;
    bool oneTimeSubmit;

    CBeginCommandBuffer(VkCommandBufferBeginInfo const* _pBeginInfo)
        : commandBufferBeginInfoData(_pBeginInfo),
          oneTimeSubmit(_pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {}

    std::set<uint64_t> GetMappedPointers() {
      std::set<uint64_t> pointers;
      for (auto obj : commandBufferBeginInfoData.GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
      return pointers;
    }
  };

  struct CBeginRenderPass {
    CVkRenderPassBeginInfoData renderPassBeginInfoData;
    CVkRenderingInfoData renderingInfoData;
    CVkSubpassContentsData subpassContentsData;
    std::shared_ptr<CRenderPassState> renderPassStateStore;
    std::shared_ptr<CFramebufferState> framebufferStateStore;
    std::vector<std::shared_ptr<CImageViewState>> imageViewStateStoreListKHR;
    std::vector<VkAttachmentStoreOp> imageStoreOp;
    std::vector<VkAttachmentLoadOp> imageLoadOp;

    CBeginRenderPass(VkRenderPassBeginInfo const* _pRenderPassBeginInfo,
                     VkSubpassContents const* _pSubpassContents,
                     std::shared_ptr<CRenderPassState>& _renderPassState,
                     std::shared_ptr<CFramebufferState>& _framebufferState)
        : renderPassBeginInfoData(_pRenderPassBeginInfo),
          renderingInfoData(0),
          subpassContentsData(_pSubpassContents),
          renderPassStateStore(_renderPassState),
          framebufferStateStore(_framebufferState) {}
    CBeginRenderPass(VkRenderingInfo const* _pRenderingInfo)
        : renderPassBeginInfoData(0), renderingInfoData(_pRenderingInfo) {}

    std::set<uint64_t> GetMappedPointers() {
      std::set<uint64_t> pointers;
      if (renderPassBeginInfoData.Value()) {
        for (auto obj : renderPassBeginInfoData.GetMappedPointers()) {
          pointers.insert((uint64_t)obj);
        }
      }
      if (renderingInfoData.Value()) {
        for (auto obj : renderingInfoData.GetMappedPointers()) {
          pointers.insert((uint64_t)obj);
        }
      }

      return pointers;
    }
  };

  VkCommandBuffer commandBufferHandle;
  CVkCommandBufferAllocateInfoData commandBufferAllocateInfoData;
  std::shared_ptr<CBeginCommandBuffer> beginCommandBuffer;
  std::vector<std::shared_ptr<CBeginRenderPass>> beginRenderPassesList;
  bool ended;
  bool submitted;
  bool restored;
  VkPipeline currentPipeline;
  VkPipelineBindPoint currentPipelineBindPoint;
  CLibrary::CVulkanCommandBufferTokensBuffer tokensBuffer;
  std::unordered_map<VkEvent, bool> eventStatesAfterSubmit;
  std::unordered_map<VkQueryPool, std::unordered_set<uint32_t>>
      resetQueriesAfterSubmit; // per query pool, per query index
  std::unordered_map<VkQueryPool, std::unordered_set<uint32_t>>
      usedQueriesAfterSubmit; // per query pool, per query index
  std::unordered_map<VkImage, std::vector<std::vector<CImageLayoutAccessOwnershipState>>>
      imageLayoutAfterSubmit; // per layer, per mipmap
  std::shared_ptr<CCommandPoolState> commandPoolStateStore;
  std::unordered_map<VkDescriptorSet, std::shared_ptr<CDescriptorSetState>>
      descriptorSetStateStoreList;
  std::unordered_map<VkPipeline, std::shared_ptr<CPipelineState>> pipelineStateStoreList;
  std::unordered_map<VkCommandBuffer, std::shared_ptr<CCommandBufferState>>
      secondaryCommandBuffersStateStoreList;
  std::unordered_map<VkBuffer, VulkanResourceType> resourceWriteBuffers;
  std::unordered_map<VkImage, VulkanResourceType> resourceWriteImages;
  std::vector<std::pair<uint64_t, bool>> touchedResources; // true for images, false for buffers
  std::unordered_set<VkImage> clearedImages;
  std::vector<std::shared_ptr<RenderGenericAttachment>> renderPassImages;
  std::vector<std::shared_ptr<RenderGenericAttachment>> renderPassResourceImages;
  std::vector<std::shared_ptr<RenderGenericAttachment>> renderPassResourceBuffers;
  std::vector<std::shared_ptr<RenderGenericAttachment>> drawImages;
  std::vector<VkCommandBuffer> secondaryCommandBuffers;
  std::list<std::pair<std::shared_ptr<CDeviceMemoryState>, std::shared_ptr<CBufferState>>>
      temporaryBuffers; // To be deleted when cmdbuffer is reset or destroyed
  std::list<std::shared_ptr<CDescriptorPoolState>> temporaryDescriptors;
  std::unordered_map<uint32_t, CDeviceAddressPatcher> addressPatchers;
  std::vector<COnQueueSubmitEndInterface*> queueSubmitEndMessageReceivers;

  CCommandBufferState(VkCommandBuffer const* _pCommandBuffer,
                      VkCommandBufferAllocateInfo const* _pAllocateInfo,
                      std::shared_ptr<CCommandPoolState>& _commandPoolState)
      : commandBufferHandle(*_pCommandBuffer),
        commandBufferAllocateInfoData(_pAllocateInfo),
        ended(false),
        submitted(false),
        restored(false),
        currentPipeline(VK_NULL_HANDLE),
        currentPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS),
        tokensBuffer(),
        commandPoolStateStore(_commandPoolState) {}
  void removeBlitsFromResourceMap() {
    for (auto obj = begin(resourceWriteBuffers); obj != end(resourceWriteBuffers);) {
      if (obj->second == VULKAN_BLIT_DESTINATION_BUFFER) {
        obj = resourceWriteBuffers.erase(obj);
      } else {
        ++obj;
      }
    }
    for (auto obj = begin(resourceWriteImages); obj != end(resourceWriteImages);) {
      if (obj->second == VULKAN_BLIT_DESTINATION_IMAGE || obj->second == VULKAN_RESOLVE_IMAGE) {
        obj = resourceWriteImages.erase(obj);
      } else {
        ++obj;
      }
    }
  }

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)commandPoolStateStore->commandPoolHandle);
    for (auto obj : commandPoolStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : commandBufferAllocateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : beginCommandBuffer->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto& beginRenderPass : beginRenderPassesList) {
      for (auto obj : beginRenderPass->GetMappedPointers()) {
        pointers.insert((uint64_t)obj);
      }
    }
    return pointers;
  }
};

// Deferred operation

struct CDeferredOperationKHRState : public UniqueResourceHandle {
  VkDeferredOperationKHR deferredOperationKHRHandle;
  std::shared_ptr<CDeviceState> deviceStateStore;

  CDeferredOperationKHRState(VkDeferredOperationKHR const* _pDeferredOperation,
                             std::shared_ptr<CDeviceState>& _deviceState)
      : deferredOperationKHRHandle(*_pDeferredOperation), deviceStateStore(_deviceState) {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)deviceStateStore->deviceHandle);
    for (auto obj : deviceStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Acceleration structure

struct CAccelerationStructureKHRState : public UniqueResourceHandle {
  struct CBuildInfo {
    std::shared_ptr<CAccelerationStructureKHRState> srcAccelerationStructureStateStore;
    CVkAccelerationStructureBuildGeometryInfoKHRData buildGeometryInfoData;
    CVkAccelerationStructureBuildRangeInfoKHRDataArray buildRangeInfoDataArray;
    VkAccelerationStructureBuildControlDataGITS controlData;

    CBuildInfo(const VkAccelerationStructureBuildGeometryInfoKHR* _pBuildGeometryInfo,
               const VkAccelerationStructureBuildRangeInfoKHR* _pBuildRangeInfos,
               VkAccelerationStructureBuildControlDataGITS _controlData,
               std::shared_ptr<CAccelerationStructureKHRState> _srcAccelerationStructureState = {})
        : srcAccelerationStructureStateStore(std::move(_srcAccelerationStructureState)),
          buildGeometryInfoData(_pBuildGeometryInfo, _pBuildRangeInfos, _controlData),
          buildRangeInfoDataArray(_pBuildGeometryInfo->geometryCount, _pBuildRangeInfos),
          controlData(_controlData) {}
  };

  struct CCopyInfo {
    std::shared_ptr<CAccelerationStructureKHRState> srcAccelerationStructureStateStore;
    CVkCopyAccelerationStructureInfoKHRData copyAccelerationStructureInfoData;
    VkCommandExecutionSideGITS executionSide;

    CCopyInfo(const VkCopyAccelerationStructureInfoKHR* _pCopyInfo,
              std::shared_ptr<CAccelerationStructureKHRState>& _srcAccelerationStructureState,
              VkCommandExecutionSideGITS _executionSide)
        : srcAccelerationStructureStateStore(_srcAccelerationStructureState),
          copyAccelerationStructureInfoData(_pCopyInfo),
          executionSide(_executionSide) {}
  };

  VkAccelerationStructureKHR accelerationStructureHandle;
  CVkAccelerationStructureCreateInfoKHRData accelerationStructureCreateInfoData;
  VkDeviceAddress deviceAddress;
  std::shared_ptr<CBufferState> bufferStateStore;
  std::shared_ptr<CBuildInfo> buildInfo;
  std::shared_ptr<CBuildInfo> updateInfo;
  std::shared_ptr<CCopyInfo> copyInfo;
  VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo;

  static std::unordered_map<VkDeviceAddress, VkAccelerationStructureKHR> deviceAddresses;
  static uint32_t globalAccelerationStructureBuildCommandIndex; // used for generating hashes

  CAccelerationStructureKHRState(VkAccelerationStructureKHR const* _pAccelerationStructure,
                                 VkAccelerationStructureCreateInfoKHR const* _pCreateInfo,
                                 std::shared_ptr<CBufferState>& _bufferState)
      : accelerationStructureHandle(*_pAccelerationStructure),
        accelerationStructureCreateInfoData(_pCreateInfo),
        deviceAddress(0),
        bufferStateStore(_bufferState),
        buildSizeInfo{} {}

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)bufferStateStore->bufferHandle);
    for (auto obj : bufferStateStore->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : accelerationStructureCreateInfoData.GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};

// Queue submit

struct CQueueSubmitState {
  uint32_t submitCount;
  CVkSubmitInfoDataArray submitInfoDataArray;
  CVkSubmitInfo2DataArray submitInfo2DataArray;
  VkFence fenceHandle;
  std::shared_ptr<CQueueState> queueStateStore;

  CQueueSubmitState(uint32_t const* _pSubmitCount,
                    VkSubmitInfo const* _pSubmits,
                    VkFence _fence,
                    std::shared_ptr<CQueueState>& _queueState)
      : submitCount(*_pSubmitCount),
        submitInfoDataArray(*_pSubmitCount, _pSubmits),
        submitInfo2DataArray(),
        fenceHandle(_fence),
        queueStateStore(_queueState) {}
  CQueueSubmitState(uint32_t const* _pSubmitCount,
                    VkSubmitInfo2 const* _pSubmits,
                    VkFence _fence,
                    std::shared_ptr<CQueueState>& _queueState)
      : submitCount(*_pSubmitCount),
        submitInfoDataArray(),
        submitInfo2DataArray(*_pSubmitCount, _pSubmits),
        fenceHandle(_fence),
        queueStateStore(_queueState) {}
};

// Memory update

struct CMemoryUpdateState {
  std::unordered_map<VkDeviceMemory, IntervalSet<uint64_t>> intervalMapMemory;

  void AddToMap(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size) {
    intervalMapMemory[memory].insert(offset, size + offset);
  }
};

// Internal resources

struct InternalPipelinesManager {
  class InternalPipelines {
  private:
    VkDevice device;
    std::shared_ptr<CDescriptorSetLayoutState> descriptorSetLayoutState;
    VkPipelineLayout pipelineLayout;
    VkPipeline prepareDeviceAddressesForPatchingPipeline;
    VkPipeline patchDeviceAddressesPipeline;
    VkPipeline prepareIndirectCopyFor16BitIndexedVerticesPipeline;
    VkPipeline prepareIndirectCopyFor32BitIndexedVerticesPipeline;
    VkPipeline performIndirectCopyPipeline;

  public:
    InternalPipelines(VkDevice _device);
    ~InternalPipelines();

    // Copy constructor
    InternalPipelines(const InternalPipelines& other);

    // Copy-assignment operator
    InternalPipelines& operator=(const InternalPipelines& other);

    std::shared_ptr<CDescriptorSetLayoutState> getDescriptorSetLayoutState();
    VkPipelineLayout getLayout();
    VkPipeline getPrepareDeviceAddressesForPatchingPipeline();
    VkPipeline getPatchDeviceAddressesPipeline();
    VkPipeline getPrepareIndirectCopyFor16BitIndexedVerticesPipeline();
    VkPipeline getPrepareIndirectCopyFor32BitIndexedVerticesPipeline();
    VkPipeline getPerformIndirectCopyPipeline();
  };

  std::unordered_map<VkDevice, InternalPipelines> pipelinesMap;

  InternalPipelines& operator[](VkDevice device) {
    auto it = pipelinesMap.find(device);
    if (it == pipelinesMap.end()) {
      it = pipelinesMap.insert(std::make_pair(device, InternalPipelines(device))).first;
    }

    return it->second;
  }
};

struct CInternalResources {
  struct CPresentationData {
    VkSemaphore imageAcquiredSemaphore;
    VkSemaphore readyToPresentSemaphore;
    VkFence fence;
    VkCommandBuffer commandBuffer;
    VkImage swapchainImage;
  };

  // Common class for both recorder and player
  // - Recorder uses it for scheduling fake swapchain and presentation calls
  // in offscreen applications.
  // - Player uses it, as name suggests, for virtual presentation replacing
  // original swapchain.
  struct CVirtualSwapchain {
    static const uint32_t imageCount = 3;
    uint32_t nextImage;
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkQueue universalQueue;
    VkCommandPool commandPool;

    std::array<CPresentationData, imageCount> presentationData;

    CVirtualSwapchain()
        : nextImage(0),
          swapchain(VK_NULL_HANDLE),
          extent{0, 0},
          universalQueue(VK_NULL_HANDLE),
          commandPool(VK_NULL_HANDLE),
          presentationData{} {}
  };

  struct COffscreenAppsSupport {
    static uint64_t uniqueHandleCounter;
    HWND hwnd;
    VkSurfaceKHR surface;
    VkImage imageToPresent;
    VkCommandBuffer commandBufferWithTransitionToPresentSRC;

    COffscreenAppsSupport()
        : hwnd(nullptr),
          surface(VK_NULL_HANDLE),
          imageToPresent(VK_NULL_HANDLE),
          commandBufferWithTransitionToPresentSRC(VK_NULL_HANDLE) {}
  };

  // (Queue family index) -> (command pool, command buffer) map used for capturing swapchain images
  using ScreenshotTakingResources =
      std::unordered_map<uint32_t, std::pair<VkCommandPool, VkCommandBuffer>>;

  // Timestamp used to mark an order in which resources are being used
  uint64_t timestamp;
  bool attachedToGITS;
  std::unordered_map<VkDevice, ScreenshotTakingResources> deviceResourcesMap;

  // Pipeline cache handles used when --overrideVKPipelineCache player options is being used
  std::unordered_map<VkDevice, VkPipelineCache> pipelineCacheHandles;
  std::unordered_map<VkDevice, CVirtualSwapchain> virtualSwapchain;
  std::unordered_map<VkDevice, COffscreenAppsSupport> offscreenApps;

  // Additional pipelines which are injected by GITS
  InternalPipelinesManager internalPipelines;

  CInternalResources() : timestamp(0), attachedToGITS(false) {}
};

/**
    *
    * CStateDynamic holds all contexts state data
    *
    */
class CStateDynamic {
private:
  CStateDynamic();

public:
#if defined(GITS_PLATFORM_WINDOWS)
  typedef std::unordered_map<HWND, std::shared_ptr<CHWNDState>> THWNDStates;
#elif defined(GITS_PLATFORM_X11)
  typedef std::unordered_map<Window, std::shared_ptr<CHWNDState>> THWNDStates;
#endif
  typedef std::unordered_map<VkInstance, std::shared_ptr<CInstanceState>> TInstanceStates;
  typedef std::unordered_map<VkPhysicalDevice, std::shared_ptr<CPhysicalDeviceState>>
      TPhysicalDeviceStates;
  typedef std::unordered_map<VkSurfaceKHR, std::shared_ptr<CSurfaceKHRState>> TSurfaceKHRStates;
  typedef std::unordered_map<VkQueue, std::shared_ptr<CQueueState>> TQueueStates;
  typedef std::unordered_map<VkDevice, std::shared_ptr<CDeviceState>> TDeviceStates;
  typedef std::unordered_map<VkSwapchainKHR, std::shared_ptr<CSwapchainKHRState>>
      TSwapchainKHRStates;
  typedef std::unordered_map<VkDescriptorPool, std::shared_ptr<CDescriptorPoolState>>
      TDescriptorPoolStates;
  typedef std::unordered_map<VkCommandPool, std::shared_ptr<CCommandPoolState>> TCommandPoolStates;
  typedef std::unordered_map<VkSampler, std::shared_ptr<CSamplerState>> TSamplerStates;
  typedef std::unordered_map<VkDeviceMemory, std::shared_ptr<CDeviceMemoryState>>
      TDeviceMemoryStates;
  typedef std::unordered_map<VkImage, std::shared_ptr<CImageState>> TImageStates;
  typedef std::unordered_map<VkImageView, std::shared_ptr<CImageViewState>> TImageViewStates;
  typedef std::unordered_map<VkBuffer, std::shared_ptr<CBufferState>> TBufferStates;
  typedef std::unordered_map<VkBufferView, std::shared_ptr<CBufferViewState>> TBufferViewStates;
  typedef std::unordered_map<VkDescriptorSetLayout, std::shared_ptr<CDescriptorSetLayoutState>>
      TDescriptorSetLayoutStates;
  typedef std::unordered_map<VkDescriptorSet, std::shared_ptr<CDescriptorSetState>>
      TDescriptorSetStates;
  typedef std::unordered_map<VkPipelineLayout, std::shared_ptr<CPipelineLayoutState>>
      TPipelineLayoutStates;
  typedef std::unordered_map<VkDescriptorUpdateTemplate,
                             std::shared_ptr<CDescriptorUpdateTemplateState>>
      TDescriptorUpdateTemplateStates;
  typedef std::unordered_map<VkPipelineCache, std::shared_ptr<CPipelineCacheState>>
      TPipelineCacheStates;
  typedef std::unordered_map<VkShaderModule, std::shared_ptr<CShaderModuleState>>
      TShaderModuleStates;
  typedef std::unordered_map<VkRenderPass, std::shared_ptr<CRenderPassState>> TRenderPassStates;
  typedef std::unordered_map<VkPipeline, std::shared_ptr<CPipelineState>> TPipelineStates;
  typedef std::unordered_map<VkFramebuffer, std::shared_ptr<CFramebufferState>> TFramebufferStates;
  typedef std::unordered_map<VkFence, std::shared_ptr<CFenceState>> TFenceStates;
  typedef std::unordered_map<VkEvent, std::shared_ptr<CEventState>> TEventStates;
  typedef std::unordered_map<VkSemaphore, std::shared_ptr<CSemaphoreState>> TSemaphoreStates;
  typedef std::unordered_map<VkQueryPool, std::shared_ptr<CQueryPoolState>> TQueryPoolStates;
  typedef std::unordered_map<VkCommandBuffer, std::shared_ptr<CCommandBufferState>>
      TCommandBufferStates;
  typedef std::unordered_map<VkDeferredOperationKHR, std::shared_ptr<CDeferredOperationKHRState>>
      TDeferredOperationKHRStates;
  typedef std::unordered_map<VkAccelerationStructureKHR,
                             std::shared_ptr<CAccelerationStructureKHRState>>
      TAccelerationStructureKHRStates;

  THWNDStates _hwndstates;
  TInstanceStates _instancestates;
  TPhysicalDeviceStates _physicaldevicestates;
  TSurfaceKHRStates _surfacekhrstates;
  TQueueStates _queuestates;
  TDeviceStates _devicestates;
  TSwapchainKHRStates _swapchainkhrstates;
  TDescriptorPoolStates _descriptorpoolstates;
  TCommandPoolStates _commandpoolstates;
  TSamplerStates _samplerstates;
  TDeviceMemoryStates _devicememorystates;
  TImageStates _imagestates;
  TImageViewStates _imageviewstates;
  TBufferStates _bufferstates;
  TBufferViewStates _bufferviewstates;
  TDescriptorSetLayoutStates _descriptorsetlayoutstates;
  TDescriptorSetStates _descriptorsetstates;
  TPipelineLayoutStates _pipelinelayoutstates;
  TDescriptorUpdateTemplateStates _descriptorupdatetemplatestates;
  TPipelineCacheStates _pipelinecachestates;
  TShaderModuleStates _shadermodulestates;
  TRenderPassStates _renderpassstates;
  TPipelineStates _pipelinestates;
  TFramebufferStates _framebufferstates;
  TFenceStates _fencestates;
  TEventStates _eventstates;
  TSemaphoreStates _semaphorestates;
  TQueryPoolStates _querypoolstates;
  TCommandBufferStates _commandbufferstates;
  TDeferredOperationKHRStates _deferredoperationkhrstates;
  TAccelerationStructureKHRStates _accelerationstructurekhrstates;

  std::unordered_map<VkCommandBuffer, std::unordered_set<VkBuffer>> bindingBuffers;
  std::unordered_map<VkCommandBuffer, std::unordered_set<VkImage>> bindingImages;
  std::unordered_map<VkCommandBuffer, CMemoryUpdateState> updatedMemoryInCmdBuffer;
  std::shared_ptr<CQueueSubmitState> lastQueueSubmit;
  std::set<uint64_t> objectsUsedInQueueSubmit;
  CInternalResources internalResources;
  std::unordered_set<VkImage> nonDeterministicImages;
  std::unordered_map<VkImage, uint64_t> imageCounter;
  std::unordered_map<VkBuffer, uint64_t> bufferCounter;
  uint64_t currentlyAllocatedMemoryAll;
  uint64_t currentlyAllocatedMemoryGPU;
  uint64_t currentlyAllocatedMemoryCPU_GPU;
  uint64_t currentlyMappedMemory;
  bool depthRangeUnrestrictedEXTEnabled;
  bool stateRestoreFinished;

  ~CStateDynamic();
  static CStateDynamic& Get();
};

inline CStateDynamic& SD() {
  return CStateDynamic::Get();
}

} // namespace Vulkan
} // namespace gits
