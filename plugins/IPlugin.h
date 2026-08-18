// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#ifdef GITS_PLUGIN_DLL
#ifdef _WIN32
#ifdef GITS_PLUGIN_EXPORT_API
#define GITS_PLUGIN_API __declspec(dllexport)
#else
#define GITS_PLUGIN_API __declspec(dllimport)
#endif
#else
#define GITS_PLUGIN_API __attribute__((visibility("default")))
#endif
#else
#define GITS_PLUGIN_API
#endif

#include <cassert>
#include <cstdint>
#include <string>

namespace gits {
class CGits;
class MessageBus;
struct Configuration;
namespace vulkan {
struct VkDeviceLevelDispatchTable;
struct VkInstanceLevelDispatchTable;
} // namespace vulkan
} // namespace gits

namespace plog {
class IAppender;
} // namespace plog

struct IPluginContext {
  const gits::Configuration* config;
  gits::CGits* gits;
  gits::MessageBus* msgBus;
  plog::IAppender* logAppender;
  // Active VkDevice dispatch table (set on vkCreateDevice). This is one
  // shared pointer-to-pointer for the whole process: both the recorder and
  // the player repoint it at whichever device's table was most recently
  // created, for every plugin instance and every VkDevice, so
  // *vkDeviceDispatchTable can change out from under a plugin between two of
  // its own calls (e.g. a second, concurrently-live VkDevice appearing). A
  // plugin that must keep operating on one specific device (for example
  // because it rejected a second one) has to snapshot the pointed-to value
  // once, when it commits to that device, and use its own copy from then on
  // rather than dereferencing this field again.
  gits::vulkan::VkDeviceLevelDispatchTable** vkDeviceDispatchTable = nullptr;
  // Dispatch table of the instance owning that device, for the physical device
  // queries a stream is not guaranteed to contain (subcaptures rarely do).
  // Shares the same last-writer-wins, process-wide-single-pointer caveat as
  // vkDeviceDispatchTable above.
  gits::vulkan::VkInstanceLevelDispatchTable** vkInstanceDispatchTable = nullptr;
  // Player-only: monotonic count of driver vkQueuePresentKHR calls issued by
  // SwapchainImageSyncService during swapchain image rewind (not stream presents).
  std::uint64_t* vkDriverRewindPresentCount = nullptr;
};

class IPlugin {
public:
  IPlugin(IPluginContext context, const char* pluginPath){};
  virtual ~IPlugin() = default;

  virtual const char* getName() = 0;
  virtual void* getImpl() = 0;
};

// Plugin DLL exports

extern "C" {
using CreatePluginPtr = IPlugin* (*)(IPluginContext, const char*);
GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath);
using DestroyPluginPtr = void* (*)();
GITS_PLUGIN_API void destroyPlugin();
}
