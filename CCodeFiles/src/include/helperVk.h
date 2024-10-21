// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "vulkanDrivers.h"
#include "vkWindowing.h"
#include "tools.h"
#include "vectorMapper.h"

#include <memory>
#include <unordered_map>
#include <vector>

#if defined GITS_PLATFORM_WINDOWS
// Disable warning C4312: 'type cast': conversion from 'unsigned int' to
// '<type>' of greater size
#pragma warning(disable : 4312)
typedef HWND vk_win_handle_t;
typedef HINSTANCE connection_t;
#elif defined GITS_PLATFORM_X11
// TODO: Use xcb in OGL too, then it will be just win_handle_t.
typedef xcb_window_t vk_win_handle_t;
typedef xcb_connection_t* connection_t;
#else
#error Unimplemented on this platform.
#endif

// ### Global state ###
struct DeviceState {
  VkPhysicalDevice physicalDevice;
  VkDeviceCreateInfo deviceCreateInfo;
  std::vector<VkQueue> queueList;
};

struct WindowState {
  gits::Vulkan::Window_* window;
};

struct ImageState {
  // For CCode we don't include stateDynamic.h so we must define this ourselves
  struct CImageLayoutAccessState {
    VkImageLayout Layout;
    VkAccessFlags Access;
  };

  int32_t width;
  int32_t height;
  int32_t depth;
  VkFormat imageFormat;
  VkImageType imageType;
  // Per layer, per mipmap.
  std::vector<std::vector<CImageLayoutAccessState>> currentLayout;
  // The image and swapchain CreateInfos are mutually exclusive.
  std::unique_ptr<const VkImageCreateInfo> imageCreateInfo;
  std::unique_ptr<const VkSwapchainCreateInfoKHR> swapchainCreateInfo;

  ImageState();
  ImageState(VkDevice device, const VkImageCreateInfo* imageCreateInfo);
  ImageState(VkSwapchainKHR swapchainKHR, const VkSwapchainCreateInfoKHR* swapchainCreateInfo);
};

struct QueueState {
  struct CDeviceQueueState {
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
  };
  VkDevice device;
  std::vector<CDeviceQueueState> deviceQueueList;
};

struct SwapchainKHRState {
  std::vector<VkImage> swapchainImages;
};

struct FenceState {
  bool fenceUsed;
};

// Shared mutable state is bad, but we need it here.
struct GlobalState {
  std::unordered_map<VkDeviceMemory, void*> mappedMemPtrs;
  std::unordered_map<VkDevice, DeviceState> deviceStates;
  std::unordered_map<vk_win_handle_t, WindowState> windowStates;
  std::unordered_map<VkImage, std::unique_ptr<ImageState>> imageStates;
  std::unordered_map<VkQueue, QueueState> queueStates;
  std::unordered_map<VkSwapchainKHR, SwapchainKHRState> swapchainStates;
  std::unordered_map<VkFence, FenceState> fenceStates;

  using ScreenshotTakingResources = std::map<uint32_t, std::pair<VkCommandPool, VkCommandBuffer>>;
  struct {
    std::map<VkDevice, ScreenshotTakingResources> deviceResourcesMap;
  } internalResources;
};
extern GlobalState globalState;

// ### Mapping ###

// This is a stripped, mapping-only version for CCode.
template <typename T>
class CVulkanObj {
protected:
  typedef std::unordered_map<T, T> name_map_t;
  typedef gits::VectorMapper<T, 100000> name_vec_map_t;
  typedef typename std::vector<T>::size_type size_type;

public:
  static void AddMapping(const T key, const T value) {
    if (useVectorMapper()) {
      get_vector_mapper()[(size_type)key] = value;
    } else {
      get_map()[key] = value;
    }
  }

  static void AddMapping(const T* keys, const T* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      AddMapping(keys[i], values[i]);
    }
  }

  static void RemoveMapping(const T key) {
    if (useVectorMapper()) {
      get_vector_mapper().Unmap((size_type)key);
    } else {
      get_map().erase(key);
    }
  }

  static void RemoveMapping(const T* keys, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      RemoveMapping(keys[i]);
    }
  }

  static T GetMapping(const T key) {
    if (useVectorMapper()) {
      T val = get_vector_mapper()[(size_type)key];
      if (val == get_vector_mapper().NotMappedVal()) {
        Log(ERR) << "Couldn't map Vulkan object name " << key;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      return val;
    } else {
      auto iter = get_map().find(key);
      if (iter == get_map().end()) {
        Log(ERR) << "Couldn't map Vulkan object name " << key;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      return iter->second;
    }
  }

  static void GetMapping(const T* keys, T* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      values[i] = GetMapping(keys[i]);
    }
  }

  static std::vector<T> GetMapping(const T* keys, size_t num) {
    std::vector<T> v;
    v.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      v.push_back(GetMapping(keys[i]));
    }
    return v;
  }

  static bool CheckMapping(const T key) {
    if (useVectorMapper()) {
      auto& the_vec_map = get_vector_mapper();
      if (the_vec_map[(size_type)key] == the_vec_map.NotMappedVal()) {
        return false;
      } else {
        return true;
      }
    } else {
      auto& the_map = get_map();
      return the_map.find(key) != the_map.end();
    }
  }

private:
  static name_map_t& get_map() {
    INIT_NEW_STATIC_OBJ(objects_map, name_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[0] = 0;
      initialized = true;
    }
    return objects_map;
  }
  static name_vec_map_t& get_vector_mapper() {
    INIT_NEW_STATIC_OBJ(objects_map, name_vec_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[(size_type)0] = (T)0;
      initialized = true;
    }
    return objects_map;
  }
  static bool useVectorMapper() {
    static bool useVectorMap = false;
    return useVectorMap;
  }
};

template <typename T>
void AddMapping(const T key, const T value) {
  CVulkanObj<T>::AddMapping(key, value);
}

template <typename T>
void AddMapping(const T* keys, const T* values, size_t num) {
  CVulkanObj<T>::AddMapping(keys, values, num);
}

template <typename T>
void RemoveMapping(const T key) {
  CVulkanObj<T>::RemoveMapping(key);
}

template <typename T>
void RemoveMapping(const T* keys, size_t num) {
  CVulkanObj<T>::RemoveMapping(keys, num);
}

template <typename T>
T GetMapping(const T key) {
  return CVulkanObj<T>::GetMapping(key);
}

template <typename T>
void GetMapping(const T* keys, T* values, size_t num) {
  CVulkanObj<T>::GetMapping(keys, values, num);
}

template <typename T>
std::vector<T> GetMapping(const T* keys, size_t num) {
  return CVulkanObj<T>::GetMapping(keys, num);
}

template <typename T>
bool CheckMapping(const T key) {
  return CVulkanObj<T>::CheckMapping(key);
}

// ### CCode wrap functions ###

VkResult vkEnumeratePhysicalDeviceGroupsKHR_CCODEWRAP(
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties);
VkResult vkEnumeratePhysicalDeviceGroups_CCODEWRAP(
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties);
VkResult vkEnumeratePhysicalDevices_CCODEWRAP(VkInstance instance,
                                              uint32_t* pPhysicalDeviceCount,
                                              VkPhysicalDevice* pPhysicalDevices);

VkResult vkMapMemory_CCODEWRAP(VkDevice device,
                               VkDeviceMemory memory,
                               VkDeviceSize offset,
                               VkDeviceSize size,
                               VkMemoryMapFlags flags);
void vkUnmapMemory_CCODEWRAP(VkDevice device, VkDeviceMemory memory);

VkResult vkAcquireNextImageKHR_CCODEWRAP(VkDevice device,
                                         VkSwapchainKHR swapchain,
                                         uint64_t timeout,
                                         VkSemaphore semaphore,
                                         VkFence fence,
                                         uint32_t* pImageIndex);
VkResult vkAcquireNextImage2KHR_CCODEWRAP(VkDevice device,
                                          const VkAcquireNextImageInfoKHR* pAcquireInfo,
                                          uint32_t* pImageIndex);

VkResult vkCreateDevice_CCODEWRAP(VkPhysicalDevice physicalDevice,
                                  const VkDeviceCreateInfo* pCreateInfo,
                                  const VkAllocationCallbacks* pAllocator,
                                  VkDevice* pDevice);
void vkDestroyDevice_CCODEWRAP(VkDevice device, const VkAllocationCallbacks* pAllocator);

VkResult vkCreateImage_CCODEWRAP(VkDevice device,
                                 const VkImageCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkImage* pImage);

VkResult vkCreateSwapchainKHR_CCODEWRAP(VkDevice device,
                                        const VkSwapchainCreateInfoKHR* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkSwapchainKHR* pSwapChain);

void vkDestroySwapchainKHR_CCODEWRAP(VkDevice device,
                                     VkSwapchainKHR swapchain,
                                     const VkAllocationCallbacks* pAllocator);

VkResult vkQueuePresentKHR_CCODEWRAP(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

void vkGetDeviceQueue_CCODEWRAP(VkDevice device,
                                uint32_t queueFamilyIndex,
                                uint32_t queueIndex,
                                VkQueue* pQueue);
void vkGetDeviceQueue2_CCODEWRAP(VkDevice device,
                                 const VkDeviceQueueInfo2* pQueueInfo,
                                 VkQueue* pQueue);

void vkGetFenceStatus_CCODEWRAP(VkResult recRetVal, VkDevice device, VkFence fence);
VkResult vkCreateFence_CCODEWRAP(VkDevice device,
                                 const VkFenceCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkFence* pFence);
VkResult vkWaitForFences_CCODEWRAP(VkResult recRetVal,
                                   VkDevice device,
                                   uint32_t fenceCount,
                                   const VkFence* pFences,
                                   VkBool32 waitAll,
                                   uint64_t timeout);
VkResult vkQueueBindSparse_CCODEWRAP(VkQueue queue,
                                     uint32_t bindInfoCount,
                                     const VkBindSparseInfo* pBindInfo,
                                     VkFence fence);
VkResult vkQueueSubmit_CCODEWRAP(VkQueue queue,
                                 uint32_t submitCount,
                                 const VkSubmitInfo* pSubmits,
                                 VkFence fence);
VkResult vkQueueSubmit2_CCODEWRAP(VkQueue queue,
                                  uint32_t submitCount,
                                  const VkSubmitInfo2* pSubmits,
                                  VkFence fence);
VkResult vkResetFences_CCODEWRAP(VkDevice device, uint32_t fenceCount, const VkFence* pFences);

// ### CGits* functions ###

void CGitsVkMemoryUpdate(VkDevice device,
                         VkDeviceMemory mem,
                         const uint64_t offset,
                         const uint64_t length,
                         const void* resource);
void CGitsVkMemoryUpdate2(VkDeviceMemory mem,
                          uint64_t size,
                          const uint64_t* offsets,
                          const uint64_t* lengths,
                          const void** resources);
void CGitsVkMemoryRestore(
    VkDevice device, VkDeviceMemory mem, uint64_t length, uint64_t offset, const void* resource);
void CGitsVkMemoryReset(VkDevice device, VkDeviceMemory mem, uint64_t length);

void CGitsVkCreateNativeWindow(
    int x, int y, int w, int h, bool vis, vk_win_handle_t wh, connection_t connection);
void CGitsVkCreateXlibWindow(
    int x, int y, int w, int h, bool vis, vk_win_handle_t wh, connection_t connection);
void CGitsVkUpdateNativeWindow(int x, int y, int w, int h, bool vis, vk_win_handle_t wh);

void CGitsDestroyVulkanDescriptorSets(size_t size, VkDescriptorSet* descSetsArray);
void CGitsDestroyVulkanCommandBuffers(size_t size, VkCommandBuffer* cmdbufArray);

void CGitsVkEnumerateDisplayMonitors(HMONITOR* hmonitor);

// ### Helpers ###

void InitVk();
void ReleaseVk();

VkClearColorValue MakeVkClearColorValue(uint32_t red,
                                        uint32_t green,
                                        uint32_t blue,
                                        uint32_t alpha);

// ### API calls ###

namespace api {

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  extern return_type(STDCALL*& function_name) function_arguments;
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  extern return_type(STDCALL*& function_name) function_arguments;
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  extern return_type(STDCALL*& function_name) function_arguments;

#include "vulkanDriversAuto.inl"

} // namespace api
using namespace api;
