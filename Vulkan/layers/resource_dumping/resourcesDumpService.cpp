// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcesDumpService.h"
#include "dumpKeys.h"
#include "formatBlockInfo.h"
#include "dispatchTablesHolder.h"
#include "enumToStrAuto.h"
#include "configurator.h"
#include "messageBus.h"
#include "log.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace gits {
namespace vulkan {

namespace {

std::unordered_set<GITSKey> ParseOption(const std::string& keys, const char* option) {
  try {
    return ParseDumpKeys(keys);
  } catch (const std::invalid_argument& e) {
    throw std::runtime_error(std::string("Vulkan.Player.ResourcesDump.") + option + ": " +
                             e.what());
  }
}

VkImageAspectFlags DepthStencilAspects(VkFormat format) {
  const FormatBlockInfo info = GetFormatBlockInfo(format);
  if (!info.IsDepthStencil) {
    return 0;
  }
  return (info.DepthBytes > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
         (info.StencilBytes > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
}

std::string DepthStencilAspectNames(VkImageAspectFlags aspects) {
  if (aspects == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
    return "depth and stencil";
  }
  return aspects == VK_IMAGE_ASPECT_STENCIL_BIT ? "stencil" : "depth";
}

std::vector<uint32_t> SharingQueueFamilies(VkSharingMode sharingMode,
                                           uint32_t queueFamilyIndexCount,
                                           const uint32_t* queueFamilyIndices) {
  if (sharingMode != VK_SHARING_MODE_CONCURRENT || !queueFamilyIndices) {
    return {};
  }
  return {queueFamilyIndices, queueFamilyIndices + queueFamilyIndexCount};
}

template <typename T>
const T* FindInChain(const void* pNext, VkStructureType sType) {
  for (auto* node = static_cast<const VkBaseInStructure*>(pNext); node != nullptr;
       node = node->pNext) {
    if (node->sType == sType) {
      return reinterpret_cast<const T*>(node);
    }
  }
  return nullptr;
}

bool HasTransferSrcUsage(const VkBufferCreateInfo& info) {
  const auto* usage = FindInChain<VkBufferUsageFlags2CreateInfo>(
      info.pNext, VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO);
  if (usage) {
    return usage->usage & VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
  }
  return info.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
}

// Whether the range names every subresource of an image with the layouts.
bool CoversImage(const ImageLayouts& layouts, const VkImageSubresourceRange& range) {
  VkImageAspectFlags aspects = range.aspectMask;
  if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
    aspects |=
        VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT;
  }
  for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
    if (!(aspects & layouts.SlotAspect(slot))) {
      return false;
    }
  }
  const auto coversAll = [](uint32_t base, uint32_t count, uint32_t total) {
    return base == 0 && (count == VK_REMAINING_MIP_LEVELS || count >= total);
  };
  return coversAll(range.baseMipLevel, range.levelCount, layouts.MipLevels()) &&
         coversAll(range.baseArrayLayer, range.layerCount, layouts.ArrayLayers());
}

// The subresources of an attachment view that a render pass instance uses. Its operations and
// layout transitions apply to every aspect of a depth/stencil format, whatever the aspects of the
// view.
VkImageSubresourceRange AttachmentRange(const ImageLayouts& layouts,
                                        VkImageSubresourceRange viewRange) {
  constexpr VkImageAspectFlags depthStencil =
      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  if (layouts.SlotAspect(0) & depthStencil) {
    viewRange.aspectMask = 0;
    for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
      viewRange.aspectMask |= layouts.SlotAspect(slot);
    }
  }
  return viewRange;
}

std::string QueueFamilyToString(uint32_t queueFamilyIndex) {
  if (queueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL) {
    return "VK_QUEUE_FAMILY_EXTERNAL";
  }
  if (queueFamilyIndex == VK_QUEUE_FAMILY_FOREIGN_EXT) {
    return "VK_QUEUE_FAMILY_FOREIGN_EXT";
  }
  return std::to_string(queueFamilyIndex);
}

template <typename Attachments>
void MarkRenderTarget(Attachments& attachments, uint32_t index) {
  if (index < attachments.size()) {
    attachments[index].RenderTarget = true;
  }
}

bool HasDepth(VkFormat format) {
  const FormatBlockInfo info = GetFormatBlockInfo(format);
  return info.IsDepthStencil && info.DepthBytes > 0;
}

bool HasStencil(VkFormat format) {
  const FormatBlockInfo info = GetFormatBlockInfo(format);
  return info.IsDepthStencil && info.StencilBytes > 0;
}

const char* AspectName(VkImageAspectFlags aspect) {
  switch (aspect) {
  case VK_IMAGE_ASPECT_DEPTH_BIT:
    return "depth";
  case VK_IMAGE_ASPECT_STENCIL_BIT:
    return "stencil";
  default:
    return "color";
  }
}

} // namespace

ResourcesDumpService::ResourcesDumpService(DispatchTablesHolder& dispatchTablesHolder)
    : m_DispatchTablesHolder(dispatchTablesHolder) {
  const auto& playerConfig = Configurator::Get().common.player;
  const auto dumpPath = [&playerConfig](const char* folder) {
    std::filesystem::path path =
        playerConfig.outputDir.empty() ? playerConfig.streamDir / folder : playerConfig.outputDir;
    if (!path.empty() && !std::filesystem::exists(path)) {
      std::filesystem::create_directories(path);
    }
    return path;
  };

  const auto& resourcesConfig = Configurator::Get().vulkan.player.resourcesDump;
  m_ResourcesDump = resourcesConfig.enabled;
  if (m_ResourcesDump) {
    m_ResourceKeys = ParseOption(resourcesConfig.resourceKeys, "resourceKeys");
    m_CommandKeys = ParseOption(resourcesConfig.commandKeys, "commandKeys");
    m_ResourcesFormat = resourcesConfig.format;
    if (m_ResourcesFormat == ImageFormat::DDS) {
      throw std::runtime_error(
          "Vulkan.Player.ResourcesDump.format: DDS is not supported, use PNG or JPEG");
    }
    if (m_CommandKeys.empty() || m_ResourceKeys.empty()) {
      LOG_WARNING << "ResourcesDump: Vulkan.Player.ResourcesDump."
                  << (m_CommandKeys.empty() ? "commandKeys" : "resourceKeys")
                  << " is empty, nothing is dumped";
    }
    m_ResourcesPath = dumpPath("resources");
  }

  const auto& renderTargetsConfig = Configurator::Get().vulkan.player.renderTargetsDump;
  m_RenderTargetsDump = renderTargetsConfig.enabled;
  if (m_RenderTargetsDump) {
    m_RenderTargetFrames = BitRange(renderTargetsConfig.frames);
    m_RenderTargetPasses = BitRange(renderTargetsConfig.renderPasses);
    m_RenderTargetsDryRun = renderTargetsConfig.dryRun;
    m_RenderTargetsFormat = renderTargetsConfig.format;
    if (m_RenderTargetsFormat == ImageFormat::DDS) {
      throw std::runtime_error(
          "Vulkan.Player.RenderTargetsDump.format: DDS is not supported, use PNG or JPEG");
    }
    m_RenderTargetsPath = dumpPath("render_targets");
  }

  m_EndSubscription =
      MessageBus::get().subscribe({PUBLISHER_PLAYER, TOPIC_END}, [this](Topic, const MessagePtr&) {
        const std::string error = Close();
        if (!error.empty()) {
          throw std::runtime_error(error);
        }
      });
}

ResourcesDumpService::~ResourcesDumpService() {
  MessageBus::get().unsubscribe(m_EndSubscription);
  const std::string error = Close();
  if (!error.empty()) {
    LOG_ERROR << error;
  }
  // The staging memory of the command buffers is released before the devices' dumps stop.
  for (auto& [commandBuffer, state] : m_CommandBuffers) {
    ClearCommandBuffer(state);
  }
  m_CommandBuffers.clear();
  m_Devices.clear();
}

std::string ResourcesDumpService::FinishDump(ResourcesDump& dump) {
  // The replay may end before it signals the semaphore values that the work of the dumps waits for.
  constexpr std::chrono::seconds timeout{10};
  const size_t unwritten = dump.Finish(timeout);
  std::string error = dump.TakeWorkerError();
  if (unwritten > 0 && error.empty()) {
    error =
        "ResourcesDump: " + std::to_string(unwritten) +
        " dumped resources are not written, as the work they wait for did not complete within " +
        std::to_string(timeout.count()) +
        " seconds. It may wait for semaphore values that the replay did not signal";
  }
  return error;
}

std::string ResourcesDumpService::FinishDumps() {
  std::string error;
  for (auto& [device, state] : m_Devices) {
    if (state.Dump) {
      std::string deviceError = FinishDump(*state.Dump);
      if (error.empty()) {
        error = std::move(deviceError);
      }
    }
  }
  return error;
}

std::string ResourcesDumpService::Close() {
  if (m_Closed) {
    return {};
  }
  m_Closed = true;
  const std::string error = FinishDumps();
  for (GITSKey key : m_CommandKeys) {
    if (!m_ReachedCommandKeys.count(key)) {
      LOG_WARNING << "ResourcesDump: command key " << DumpKeyToString(key)
                  << " was not replayed, or is not a vkCmd*, vkEndCommandBuffer or vkQueueSubmit* "
                     "call, so nothing is dumped after it";
    }
  }
  for (GITSKey key : m_ResourceKeys) {
    if (!m_CreatedResourceKeys.count(key)) {
      LOG_WARNING << "ResourcesDump: resource key O" << DumpKeyToString(key)
                  << " is not a buffer, image or swapchain image created during the replay, so "
                     "it is not dumped";
    }
  }
  if (m_RenderTargetsDump && m_SelectedRenderPasses == 0) {
    LOG_WARNING << "RenderTargetsDump: no render pass instance of Vulkan.Player.RenderTargetsDump."
                   "frames and renderPasses was replayed, so nothing is dumped";
  }
  if (m_RenderTargetsDryRun) {
    WriteDryRun();
  }
  return error;
}

void ResourcesDumpService::WriteDryRun() const {
  const std::filesystem::path path = m_RenderTargetsPath / "RenderTargetsDumpDryRun.yaml";
  std::ofstream file(path);
  file << "RenderPassesWithTargetsByFrame:";
  if (m_DryRunRenderPasses.empty()) {
    file << " {}";
  }
  file << "\n";
  for (const auto& [frame, renderPasses] : m_DryRunRenderPasses) {
    file << "  " << frame << ": [";
    const char* separator = "";
    for (unsigned renderPass : renderPasses) {
      file << separator << renderPass;
      separator = ", ";
    }
    file << "]\n";
  }
  file.close();
  if (!file.fail()) {
    LOG_INFO << "RenderTargetsDump: written " << path.string();
  } else {
    LOG_ERROR << "RenderTargetsDump: cannot write " << path.string();
  }
}

const char* ResourcesDumpService::LogName() const {
  return m_ResourcesDump ? "ResourcesDump" : "RenderTargetsDump";
}

void ResourcesDumpService::StateRestoreBegin() {
  m_StateRestore = true;
  m_Frame = 0;
  m_RenderPassCount = 0;
}

void ResourcesDumpService::StateRestoreEnd() {
  m_StateRestore = false;
  m_Frame = 1;
  m_RenderPassCount = 0;
}

void ResourcesDumpService::RestoredImageLayouts(
    VkQueue queue, const std::vector<std::pair<uint64_t, VkImageLayout>>& layouts) {
  auto queueIt = m_Queues.find(queue);
  std::unordered_map<GITSKey, Image*> images;
  for (auto& [handle, image] : m_Images) {
    images[image.Key] = &image;
  }
  constexpr VkImageSubresourceRange wholeImage{
      VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0,
      VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};
  for (const auto& [key, layout] : layouts) {
    auto imageIt = images.find(key);
    if (imageIt == images.end()) {
      continue;
    }
    Image& image = *imageIt->second;
    image.Layouts.Set(wholeImage, layout);
    if (queueIt != m_Queues.end()) {
      ApplyOwnershipChange(image.Owner, {OwnershipChange::Kind::Use, queueIt->second.second, true});
    }
  }
}

void ResourcesDumpService::Present() {
  if (!m_StateRestore) {
    ++m_Frame;
    m_RenderPassCount = 0;
  }
}

ResourcesDump& ResourcesDumpService::GetDump(VkDevice device) {
  Device& state = m_Devices[device];
  if (!state.Dump) {
    VkDeviceLevelDispatchTable* deviceTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
    VkInstanceLevelDispatchTable* instanceTable =
        state.PhysicalDevice ? m_DispatchTablesHolder.GetInstanceDispatchTable(state.PhysicalDevice)
                             : nullptr;
    GITS_ASSERT(deviceTable && instanceTable, "ResourcesDump: the device is unknown");
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    instanceTable->vkGetPhysicalDeviceMemoryProperties(state.PhysicalDevice, &memoryProperties);
    state.Dump = std::make_unique<ResourcesDump>(device, *deviceTable, memoryProperties,
                                                 state.SeparateDepthStencilLayouts);
  }
  return *state.Dump;
}

void ResourcesDumpService::CreateDevice(VkPhysicalDevice physicalDevice,
                                        VkDevice device,
                                        const VkDeviceCreateInfo& info) {
  Device& state = m_Devices[device];
  state.PhysicalDevice = physicalDevice;
  const auto* vulkan12 = FindInChain<VkPhysicalDeviceVulkan12Features>(
      info.pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
  const auto* separate = FindInChain<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>(
      info.pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES);
  state.SeparateDepthStencilLayouts = (vulkan12 && vulkan12->separateDepthStencilLayouts) ||
                                      (separate && separate->separateDepthStencilLayouts);

  VkInstanceLevelDispatchTable* instanceTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  GITS_ASSERT(instanceTable, "ResourcesDump: the physical device is unknown");
  uint32_t count = 0;
  instanceTable->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  std::vector<VkQueueFamilyProperties> properties(count);
  instanceTable->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count,
                                                          properties.data());
  state.QueueFamilyFlags.clear();
  for (const VkQueueFamilyProperties& family : properties) {
    state.QueueFamilyFlags.push_back(family.queueFlags);
  }

  const auto* maintenance10 = FindInChain<VkPhysicalDeviceMaintenance10FeaturesKHR>(
      info.pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_10_FEATURES_KHR);
  bool formatFeatures2 = false;
  for (uint32_t i = 0; info.ppEnabledExtensionNames && i < info.enabledExtensionCount; ++i) {
    formatFeatures2 = formatFeatures2 || std::string_view(info.ppEnabledExtensionNames[i]) ==
                                             VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME;
  }
  VkPhysicalDeviceProperties deviceProperties{};
  instanceTable->vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  state.NonGraphicsDepthStencilCopies =
      maintenance10 && maintenance10->maintenance10 &&
      (formatFeatures2 || deviceProperties.apiVersion >= VK_API_VERSION_1_3);
}

void ResourcesDumpService::DestroyDevice(VkDevice device) {
  for (auto it = m_CommandBuffers.begin(); it != m_CommandBuffers.end();) {
    if (it->second.Device == device) {
      ClearCommandBuffer(it->second);
      it = m_CommandBuffers.erase(it);
    } else {
      ++it;
    }
  }
  std::erase_if(m_Buffers, [device](const auto& item) { return item.second.Device == device; });
  std::erase_if(m_Images, [device](const auto& item) { return item.second.Device == device; });
  std::erase_if(m_Queues, [device](const auto& item) { return item.second.first == device; });
  std::erase_if(m_Semaphores, [device](const auto& item) { return item.second.first == device; });
  std::string error;
  auto deviceIt = m_Devices.find(device);
  if (deviceIt != m_Devices.end() && deviceIt->second.Dump) {
    error = FinishDump(*deviceIt->second.Dump);
  }
  m_Devices.erase(device);
  if (!error.empty()) {
    throw std::runtime_error(error);
  }
}

void ResourcesDumpService::GetDeviceQueue(VkDevice device,
                                          uint32_t queueFamilyIndex,
                                          VkQueue queue) {
  m_Queues[queue] = {device, queueFamilyIndex};
}

void ResourcesDumpService::TrackSemaphore(VkDevice device,
                                          VkSemaphore semaphore,
                                          const VkSemaphoreCreateInfo& info) {
  const auto* type = FindInChain<VkSemaphoreTypeCreateInfo>(
      info.pNext, VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO);
  m_Semaphores[semaphore] = {device, type && type->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE};
}

void ResourcesDumpService::DestroySemaphore(VkSemaphore semaphore) {
  m_Semaphores.erase(semaphore);
  m_AcquireSemaphores.erase(semaphore);
  // The work using a semaphore completes before it is destroyed.
  m_BinarySignalWaits.erase(semaphore);
  m_TimelineSignals.erase(semaphore);
  const auto onSemaphore = [semaphore](const TimelineWait& wait) {
    return wait.first == semaphore;
  };
  for (auto& [queue, waits] : m_QueueWaits) {
    std::erase_if(waits, onSemaphore);
  }
  for (auto& [binary, waits] : m_BinarySignalWaits) {
    std::erase_if(waits, onSemaphore);
  }
  for (auto& [timeline, signals] : m_TimelineSignals) {
    for (TimelineSignal& signal : signals) {
      std::erase_if(signal.Waits, onSemaphore);
    }
  }
}

void ResourcesDumpService::CreateBuffer(GITSKey key,
                                        VkDevice device,
                                        VkBuffer buffer,
                                        const VkBufferCreateInfo& info) {
  if (!m_ResourceKeys.count(key)) {
    return;
  }
  m_CreatedResourceKeys.insert(key);
  Buffer state;
  state.Key = key;
  state.Device = device;
  state.TransferSrc = HasTransferSrcUsage(info);
  state.Sparse = info.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
  state.Protected = info.flags & VK_BUFFER_CREATE_PROTECTED_BIT;
  state.Exclusive = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE;
  state.SharingQueueFamilies =
      SharingQueueFamilies(info.sharingMode, info.queueFamilyIndexCount, info.pQueueFamilyIndices);
  state.Size = info.size;
  m_Buffers[buffer] = state;
}

void ResourcesDumpService::BindBuffer(VkBuffer buffer) {
  auto it = m_Buffers.find(buffer);
  if (it != m_Buffers.end()) {
    it->second.Bound = true;
  }
}

void ResourcesDumpService::DestroyBuffer(VkBuffer buffer) {
  m_Buffers.erase(buffer);
}

void ResourcesDumpService::CreateImage(GITSKey key,
                                       VkDevice device,
                                       VkImage image,
                                       const VkImageCreateInfo& info) {
  constexpr VkImageUsageFlags attachmentUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (m_ResourceKeys.count(key)) {
    m_CreatedResourceKeys.insert(key);
  } else if (!TracksRenderTargets() || !(info.usage & attachmentUsage)) {
    return;
  }
  Image state;
  state.Key = key;
  state.Device = device;
  state.TransferSrc = info.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  const FormatBlockInfo formatInfo = GetFormatBlockInfo(info.format);
  if (formatInfo.StencilBytes > 0) {
    const auto* stencilUsage = FindInChain<VkImageStencilUsageCreateInfo>(
        info.pNext, VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO);
    const bool stencilTransferSrc =
        stencilUsage ? (stencilUsage->stencilUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0
                     : state.TransferSrc;
    if (formatInfo.DepthBytes == 0) {
      state.TransferSrc = stencilTransferSrc;
    } else if (state.TransferSrc != stencilTransferSrc) {
      state.NoTransferSrcAspect =
          stencilTransferSrc ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_STENCIL_BIT;
      state.TransferSrc = true;
    }
  }
  state.Sparse = info.flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
  state.Protected = info.flags & VK_IMAGE_CREATE_PROTECTED_BIT;
  state.Exclusive = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE;
  state.SharingQueueFamilies =
      SharingQueueFamilies(info.sharingMode, info.queueFamilyIndexCount, info.pQueueFamilyIndices);
  state.Format = info.format;
  state.Tiling = info.tiling;
  state.Type = info.imageType;
  state.Extent = info.extent;
  state.Samples = info.samples;
  state.Disjoint = info.flags & VK_IMAGE_CREATE_DISJOINT_BIT;
  state.Layouts = ImageLayouts(info.format, state.Disjoint, info.mipLevels, info.arrayLayers,
                               info.initialLayout);
  m_Images[image] = std::move(state);
}

void ResourcesDumpService::BindImage(VkImage image) {
  auto it = m_Images.find(image);
  if (it != m_Images.end()) {
    it->second.Bound = true;
  }
}

void ResourcesDumpService::DestroyImage(VkImage image) {
  m_Images.erase(image);
}

void ResourcesDumpService::PrepareSwapchain(VkDevice device, VkSwapchainCreateInfoKHR& info) {
  if ((m_ResourceKeys.empty() && !TracksRenderTargets()) ||
      (info.imageUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
    return;
  }
  auto deviceIt = m_Devices.find(device);
  if (deviceIt == m_Devices.end() || !deviceIt->second.PhysicalDevice) {
    return;
  }
  VkPhysicalDevice physicalDevice = deviceIt->second.PhysicalDevice;
  VkInstanceLevelDispatchTable* instanceTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  if (!instanceTable || !instanceTable->vkGetPhysicalDeviceSurfaceCapabilitiesKHR) {
    return;
  }
  VkSurfaceCapabilitiesKHR capabilities{};
  if (instanceTable->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, info.surface,
                                                               &capabilities) != VK_SUCCESS ||
      !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
    LOG_WARNING << LogName()
                << ": the surface does not support the transfer source usage, so the swapchain "
                   "images cannot be dumped";
    return;
  }
  info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
}

void ResourcesDumpService::CreateSwapchain(VkSwapchainKHR swapchain,
                                           const VkSwapchainCreateInfoKHR& info) {
  VkSwapchainCreateInfoKHR& stored = m_Swapchains[swapchain];
  stored = info;
  stored.pNext = nullptr;
  stored.pQueueFamilyIndices = nullptr;
  m_SwapchainSharingQueueFamilies[swapchain] = SharingQueueFamilies(
      info.imageSharingMode, info.queueFamilyIndexCount, info.pQueueFamilyIndices);
}

void ResourcesDumpService::GetSwapchainImages(VkDevice device,
                                              VkSwapchainKHR swapchain,
                                              const VkImage* images,
                                              const GITSKey* keys,
                                              uint32_t count) {
  auto swapchainIt = m_Swapchains.find(swapchain);
  if (swapchainIt == m_Swapchains.end()) {
    return;
  }
  const VkSwapchainCreateInfoKHR& info = swapchainIt->second;
  m_SwapchainImages[swapchain].assign(images, images + count);
  for (uint32_t i = 0; i < count; ++i) {
    const bool listed = m_ResourceKeys.count(keys[i]);
    if (m_Images.count(images[i]) || (!listed && !TracksRenderTargets())) {
      continue;
    }
    if (listed) {
      m_CreatedResourceKeys.insert(keys[i]);
    }
    Image state;
    state.Key = keys[i];
    state.Device = device;
    state.TransferSrc = info.imageUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    state.Protected = info.flags & VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR;
    state.Bound = true;
    state.Exclusive = info.imageSharingMode == VK_SHARING_MODE_EXCLUSIVE;
    state.SharingQueueFamilies = m_SwapchainSharingQueueFamilies[swapchain];
    state.Format = info.imageFormat;
    state.Type = VK_IMAGE_TYPE_2D;
    state.Extent = {info.imageExtent.width, info.imageExtent.height, 1};
    state.Samples = VK_SAMPLE_COUNT_1_BIT;
    state.Swapchain = swapchain;
    state.Layouts =
        ImageLayouts(info.imageFormat, false, 1, info.imageArrayLayers, VK_IMAGE_LAYOUT_UNDEFINED);
    m_Images[images[i]] = std::move(state);
  }
}

void ResourcesDumpService::DestroySwapchain(VkSwapchainKHR swapchain) {
  std::erase_if(m_Images,
                [swapchain](const auto& item) { return item.second.Swapchain == swapchain; });
  m_Swapchains.erase(swapchain);
  m_SwapchainSharingQueueFamilies.erase(swapchain);
  m_SwapchainImages.erase(swapchain);
}

void ResourcesDumpService::AcquireSwapchainImage(VkSwapchainKHR swapchain,
                                                 uint32_t imageIndex,
                                                 VkSemaphore semaphore) {
  // The presentation engine signals the semaphore without waiting for submitted work.
  m_BinarySignalWaits.erase(semaphore);
  auto swapchainIt = m_SwapchainImages.find(swapchain);
  if (swapchainIt == m_SwapchainImages.end() || imageIndex >= swapchainIt->second.size()) {
    return;
  }
  const VkImage handle = swapchainIt->second[imageIndex];
  auto imageIt = m_Images.find(handle);
  if (imageIt == m_Images.end()) {
    return;
  }
  imageIt->second.Acquired = true;
  imageIt->second.AcquisitionWaits.clear();
  imageIt->second.AcquiredWithFence = !semaphore;
  if (semaphore) {
    m_AcquireSemaphores[semaphore] = handle;
  }
}

void ResourcesDumpService::ReleaseSwapchainImages(VkSwapchainKHR swapchain,
                                                  uint32_t imageIndexCount,
                                                  const uint32_t* imageIndices) {
  auto swapchainIt = m_SwapchainImages.find(swapchain);
  if (swapchainIt == m_SwapchainImages.end()) {
    return;
  }
  for (uint32_t i = 0; imageIndices && i < imageIndexCount; ++i) {
    if (imageIndices[i] >= swapchainIt->second.size()) {
      continue;
    }
    const VkImage handle = swapchainIt->second[imageIndices[i]];
    auto imageIt = m_Images.find(handle);
    if (imageIt != m_Images.end()) {
      imageIt->second.Acquired = false;
      imageIt->second.AcquisitionWaits.clear();
    }
    std::erase_if(m_AcquireSemaphores,
                  [handle](const auto& item) { return item.second == handle; });
  }
}

void ResourcesDumpService::CreateImageView(VkImageView view, const VkImageViewCreateInfo& info) {
  auto imageIt = m_Images.find(info.image);
  if (imageIt == m_Images.end()) {
    return;
  }
  View state{info.image, info.subresourceRange};
  if (imageIt->second.Type == VK_IMAGE_TYPE_3D) {
    state.BaseSlice = info.subresourceRange.baseArrayLayer;
    state.SliceCount = info.subresourceRange.layerCount;
    state.Range.baseArrayLayer = 0;
    state.Range.layerCount = 1;
  }
  m_Views[view] = state;
}

void ResourcesDumpService::DestroyImageView(VkImageView view) {
  m_Views.erase(view);
}

void ResourcesDumpService::CreateRenderPass(VkRenderPass renderPass,
                                            const VkRenderPassCreateInfo& info) {
  std::vector<RenderPassAttachment>& attachments = m_RenderPasses[renderPass];
  attachments.clear();
  for (uint32_t i = 0; info.pAttachments && i < info.attachmentCount; ++i) {
    const VkAttachmentDescription& description = info.pAttachments[i];
    attachments.push_back({description.format, description.finalLayout, description.finalLayout,
                           description.storeOp, description.stencilStoreOp});
  }
  for (uint32_t i = 0; info.pSubpasses && i < info.subpassCount; ++i) {
    const VkSubpassDescription& subpass = info.pSubpasses[i];
    for (uint32_t j = 0; subpass.pColorAttachments && j < subpass.colorAttachmentCount; ++j) {
      MarkRenderTarget(attachments, subpass.pColorAttachments[j].attachment);
      if (subpass.pResolveAttachments) {
        MarkRenderTarget(attachments, subpass.pResolveAttachments[j].attachment);
      }
    }
    if (subpass.pDepthStencilAttachment) {
      MarkRenderTarget(attachments, subpass.pDepthStencilAttachment->attachment);
    }
  }
  uint32_t viewMask = 0;
  const auto* multiview = FindInChain<VkRenderPassMultiviewCreateInfo>(
      info.pNext, VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO);
  for (uint32_t i = 0; multiview && multiview->pViewMasks && i < multiview->subpassCount; ++i) {
    viewMask |= multiview->pViewMasks[i];
  }
  m_RenderPassViewMasks[renderPass] = viewMask;
}

void ResourcesDumpService::CreateRenderPass(VkRenderPass renderPass,
                                            const VkRenderPassCreateInfo2& info) {
  std::vector<RenderPassAttachment>& attachments = m_RenderPasses[renderPass];
  attachments.clear();
  uint32_t& viewMask = m_RenderPassViewMasks[renderPass];
  viewMask = 0;
  for (uint32_t i = 0; info.pAttachments && i < info.attachmentCount; ++i) {
    const VkAttachmentDescription2& description = info.pAttachments[i];
    RenderPassAttachment attachment{description.format, description.finalLayout,
                                    description.finalLayout, description.storeOp,
                                    description.stencilStoreOp};
    const auto* stencilLayout = FindInChain<VkAttachmentDescriptionStencilLayout>(
        description.pNext, VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT);
    if (stencilLayout) {
      attachment.StencilFinalLayout = stencilLayout->stencilFinalLayout;
    }
    attachments.push_back(attachment);
  }
  for (uint32_t i = 0; info.pSubpasses && i < info.subpassCount; ++i) {
    const VkSubpassDescription2& subpass = info.pSubpasses[i];
    viewMask |= subpass.viewMask;
    for (uint32_t j = 0; subpass.pColorAttachments && j < subpass.colorAttachmentCount; ++j) {
      MarkRenderTarget(attachments, subpass.pColorAttachments[j].attachment);
      if (subpass.pResolveAttachments) {
        MarkRenderTarget(attachments, subpass.pResolveAttachments[j].attachment);
      }
    }
    if (subpass.pDepthStencilAttachment) {
      MarkRenderTarget(attachments, subpass.pDepthStencilAttachment->attachment);
    }
    const auto* depthStencilResolve = FindInChain<VkSubpassDescriptionDepthStencilResolve>(
        subpass.pNext, VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE);
    if (depthStencilResolve && depthStencilResolve->pDepthStencilResolveAttachment) {
      MarkRenderTarget(attachments,
                       depthStencilResolve->pDepthStencilResolveAttachment->attachment);
    }
  }
}

void ResourcesDumpService::DestroyRenderPass(VkRenderPass renderPass) {
  m_RenderPasses.erase(renderPass);
  m_RenderPassViewMasks.erase(renderPass);
}

void ResourcesDumpService::CreateFramebuffer(VkFramebuffer framebuffer,
                                             const VkFramebufferCreateInfo& info) {
  Framebuffer& state = m_Framebuffers[framebuffer];
  state.Views.clear();
  if (!(info.flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT) && info.pAttachments) {
    state.Views.assign(info.pAttachments, info.pAttachments + info.attachmentCount);
  }
  state.Layers = info.layers;
}

void ResourcesDumpService::DestroyFramebuffer(VkFramebuffer framebuffer) {
  m_Framebuffers.erase(framebuffer);
}

void ResourcesDumpService::CreateCommandPool(VkDevice device,
                                             VkCommandPool commandPool,
                                             const VkCommandPoolCreateInfo& info) {
  m_CommandPools[commandPool] = {info.queueFamilyIndex,
                                 (info.flags & VK_COMMAND_POOL_CREATE_PROTECTED_BIT) != 0};
  m_Devices[device].QueueFamilies.insert(info.queueFamilyIndex);
}

void ResourcesDumpService::DestroyCommandPool(VkCommandPool commandPool) {
  for (auto it = m_CommandBuffers.begin(); it != m_CommandBuffers.end();) {
    if (it->second.CommandPool == commandPool) {
      ClearCommandBuffer(it->second);
      it = m_CommandBuffers.erase(it);
    } else {
      ++it;
    }
  }
  m_CommandPools.erase(commandPool);
}

void ResourcesDumpService::ResetCommandPool(VkCommandPool commandPool) {
  for (auto& [commandBuffer, state] : m_CommandBuffers) {
    if (state.CommandPool == commandPool) {
      ClearCommandBuffer(state);
    }
  }
}

void ResourcesDumpService::AllocateCommandBuffers(VkDevice device,
                                                  const VkCommandBufferAllocateInfo& info,
                                                  const VkCommandBuffer* commandBuffers) {
  auto poolIt = m_CommandPools.find(info.commandPool);
  if (poolIt == m_CommandPools.end()) {
    return;
  }
  for (uint32_t i = 0; i < info.commandBufferCount; ++i) {
    CommandBufferState& state = m_CommandBuffers[commandBuffers[i]];
    ClearCommandBuffer(state);
    state.Device = device;
    state.CommandPool = info.commandPool;
    state.QueueFamilyIndex = poolIt->second.QueueFamilyIndex;
    state.Protected = poolIt->second.Protected;
    state.Secondary = info.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  }
}

void ResourcesDumpService::FreeCommandBuffers(const VkCommandBuffer* commandBuffers,
                                              uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    auto it = m_CommandBuffers.find(commandBuffers[i]);
    if (it != m_CommandBuffers.end()) {
      ClearCommandBuffer(it->second);
      m_CommandBuffers.erase(it);
    }
  }
}

void ResourcesDumpService::BeginCommandBuffer(VkCommandBuffer commandBuffer,
                                              const VkCommandBufferBeginInfo& info) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    return;
  }
  ClearCommandBuffer(*state);
  state->SimultaneousUse = info.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  state->InRenderPass =
      state->Secondary && (info.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
}

void ResourcesDumpService::ResetCommandBuffer(VkCommandBuffer commandBuffer) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (state) {
    ClearCommandBuffer(*state);
  }
}

ResourcesDumpService::CommandBufferState* ResourcesDumpService::GetCommandBuffer(
    VkCommandBuffer commandBuffer) {
  auto it = m_CommandBuffers.find(commandBuffer);
  return it == m_CommandBuffers.end() ? nullptr : &it->second;
}

void ResourcesDumpService::ClearCommandBuffer(CommandBufferState& state) {
  // A command buffer is reset or begun only once its submissions complete, so its copies are
  // written now.
  for (const DumpPoint& point : state.Dumps) {
    if (point.Batch->Submitted) {
      GetDump(state.Device).WriteExecuted(*point.Batch);
    }
  }
  state.Dumps.clear();
  state.Changes.clear();
  state.Attachments.clear();
  state.RenderTargets.clear();
  state.RenderPass = VK_NULL_HANDLE;
  state.InRenderPass = false;
  state.Suspending = false;
  state.DumpsBeforeRenderPass.reset();
  state.SimultaneousUse = false;
  state.Unsubmitted = false;
  state.InVideoCoding = false;
}

void ResourcesDumpService::TrackOwnershipChange(const CommandBufferState& state,
                                                StateChange& change,
                                                uint32_t srcQueueFamilyIndex,
                                                uint32_t dstQueueFamilyIndex,
                                                bool whole) {
  if (srcQueueFamilyIndex == dstQueueFamilyIndex ||
      srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED ||
      dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
    change.Owner = {OwnershipChange::Kind::Use, state.QueueFamilyIndex, whole};
  } else if (state.QueueFamilyIndex == srcQueueFamilyIndex) {
    change.Owner = {OwnershipChange::Kind::Release, dstQueueFamilyIndex, whole};
  } else if (state.QueueFamilyIndex == dstQueueFamilyIndex) {
    change.Owner = {OwnershipChange::Kind::Acquire, dstQueueFamilyIndex, whole};
  }
}

void ResourcesDumpService::TrackBufferBarrier(CommandBufferState& state,
                                              VkBuffer buffer,
                                              VkDeviceSize offset,
                                              VkDeviceSize size,
                                              uint32_t srcQueueFamilyIndex,
                                              uint32_t dstQueueFamilyIndex) {
  auto it = m_Buffers.find(buffer);
  if (it == m_Buffers.end()) {
    return;
  }
  StateChange change;
  change.Buffer = buffer;
  const bool whole = offset == 0 && (size == VK_WHOLE_SIZE || size >= it->second.Size);
  TrackOwnershipChange(state, change, srcQueueFamilyIndex, dstQueueFamilyIndex, whole);
  if (change.Owner) {
    state.Changes.push_back(change);
  }
}

void ResourcesDumpService::TrackImageBarrier(CommandBufferState& state,
                                             VkImage image,
                                             const VkImageSubresourceRange& range,
                                             VkImageLayout oldLayout,
                                             VkImageLayout newLayout,
                                             uint32_t srcQueueFamilyIndex,
                                             uint32_t dstQueueFamilyIndex) {
  auto it = m_Images.find(image);
  if (it == m_Images.end()) {
    return;
  }
  StateChange change;
  change.Image = image;
  change.Range = range;
  if (oldLayout != newLayout) {
    change.Layout = newLayout;
  }
  TrackOwnershipChange(state, change, srcQueueFamilyIndex, dstQueueFamilyIndex,
                       CoversImage(it->second.Layouts, range));
  if (change.Layout || change.Owner) {
    state.Changes.push_back(change);
  }
}

void ResourcesDumpService::TrackAttachmentUse(CommandBufferState& state, VkImageView view) {
  auto viewIt = m_Views.find(view);
  if (viewIt == m_Views.end()) {
    return;
  }
  auto imageIt = m_Images.find(viewIt->second.Image);
  if (imageIt == m_Images.end()) {
    return;
  }
  for (const VkImageSubresourceRange& range :
       RenderedRanges(state, viewIt->second, imageIt->second)) {
    StateChange change;
    change.Image = viewIt->second.Image;
    change.Range = range;
    change.Owner = {OwnershipChange::Kind::Use, state.QueueFamilyIndex,
                    CoversImage(imageIt->second.Layouts, range)};
    state.Changes.push_back(change);
  }
}

std::vector<VkImageSubresourceRange> ResourcesDumpService::RenderedRanges(
    const CommandBufferState& state, const View& view, const Image& image) const {
  const VkImageSubresourceRange range = AttachmentRange(image.Layouts, view.Range);
  // The layers of a 2D view of a 3D image are depth slices, which share the layout of the image.
  if (image.Type == VK_IMAGE_TYPE_3D) {
    return {range};
  }
  return RenderedLayers(range, image.Layouts.ArrayLayers(), state.LayerCount, state.ViewMask);
}

void ResourcesDumpService::Barriers(VkCommandBuffer commandBuffer,
                                    uint32_t bufferBarrierCount,
                                    const VkBufferMemoryBarrier* bufferBarriers,
                                    uint32_t imageBarrierCount,
                                    const VkImageMemoryBarrier* imageBarriers) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    return;
  }
  for (uint32_t i = 0; bufferBarriers && i < bufferBarrierCount; ++i) {
    const VkBufferMemoryBarrier& barrier = bufferBarriers[i];
    TrackBufferBarrier(*state, barrier.buffer, barrier.offset, barrier.size,
                       barrier.srcQueueFamilyIndex, barrier.dstQueueFamilyIndex);
  }
  for (uint32_t i = 0; imageBarriers && i < imageBarrierCount; ++i) {
    const VkImageMemoryBarrier& barrier = imageBarriers[i];
    TrackImageBarrier(*state, barrier.image, barrier.subresourceRange, barrier.oldLayout,
                      barrier.newLayout, barrier.srcQueueFamilyIndex, barrier.dstQueueFamilyIndex);
  }
}

void ResourcesDumpService::Barriers(VkCommandBuffer commandBuffer,
                                    const VkDependencyInfo& dependencyInfo) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    return;
  }
  for (uint32_t i = 0;
       dependencyInfo.pBufferMemoryBarriers && i < dependencyInfo.bufferMemoryBarrierCount; ++i) {
    const VkBufferMemoryBarrier2& barrier = dependencyInfo.pBufferMemoryBarriers[i];
    TrackBufferBarrier(*state, barrier.buffer, barrier.offset, barrier.size,
                       barrier.srcQueueFamilyIndex, barrier.dstQueueFamilyIndex);
  }
  for (uint32_t i = 0;
       dependencyInfo.pImageMemoryBarriers && i < dependencyInfo.imageMemoryBarrierCount; ++i) {
    const VkImageMemoryBarrier2& barrier = dependencyInfo.pImageMemoryBarriers[i];
    TrackImageBarrier(*state, barrier.image, barrier.subresourceRange, barrier.oldLayout,
                      barrier.newLayout, barrier.srcQueueFamilyIndex, barrier.dstQueueFamilyIndex);
  }
}

void ResourcesDumpService::BeginRenderPass(VkCommandBuffer commandBuffer,
                                           const VkRenderPassBeginInfo& info) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    return;
  }
  state->InRenderPass = true;
  state->Suspending = false;
  state->DumpsBeforeRenderPass = state->DumpsBeforeRenderPass.value_or(state->Dumps.size());
  state->RenderPassFrame = m_Frame;
  state->RenderPassNumber = ++m_RenderPassCount;
  state->RenderPass = info.renderPass;
  state->Attachments.clear();
  state->RenderTargets.clear();
  auto viewMaskIt = m_RenderPassViewMasks.find(info.renderPass);
  state->ViewMask = viewMaskIt == m_RenderPassViewMasks.end() ? 0 : viewMaskIt->second;
  state->LayerCount = VK_REMAINING_ARRAY_LAYERS;
  auto framebufferIt = m_Framebuffers.find(info.framebuffer);
  if (framebufferIt != m_Framebuffers.end()) {
    state->Attachments = framebufferIt->second.Views;
    state->LayerCount = framebufferIt->second.Layers;
  }
  const auto* attachmentViews = FindInChain<VkRenderPassAttachmentBeginInfo>(
      info.pNext, VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO);
  if (attachmentViews && attachmentViews->pAttachments) {
    state->Attachments.assign(attachmentViews->pAttachments,
                              attachmentViews->pAttachments + attachmentViews->attachmentCount);
  }
  for (VkImageView view : state->Attachments) {
    TrackAttachmentUse(*state, view);
  }

  auto renderPassIt = m_RenderPasses.find(info.renderPass);
  if (renderPassIt == m_RenderPasses.end()) {
    return;
  }
  const std::vector<RenderPassAttachment>& attachments = renderPassIt->second;
  const auto depthStencilCount =
      std::count_if(attachments.begin(), attachments.end(), [](const RenderPassAttachment& a) {
        return a.RenderTarget && (HasDepth(a.Format) || HasStencil(a.Format));
      });
  const size_t count = std::min(attachments.size(), state->Attachments.size());
  for (size_t i = 0; i < count; ++i) {
    const RenderPassAttachment& attachment = attachments[i];
    if (!attachment.RenderTarget) {
      continue;
    }
    const VkImageView view = state->Attachments[i];
    const bool depth = HasDepth(attachment.Format);
    const bool stencil = HasStencil(attachment.Format);
    if (!depth && !stencil) {
      state->RenderTargets.push_back({"rt_" + std::to_string(i), view, VK_IMAGE_ASPECT_COLOR_BIT,
                                      attachment.FinalLayout, attachment.StoreOp});
      continue;
    }
    const std::string name = depthStencilCount > 1 ? "ds_" + std::to_string(i) : "ds";
    if (depth) {
      state->RenderTargets.push_back(
          {name, view, VK_IMAGE_ASPECT_DEPTH_BIT, attachment.FinalLayout, attachment.StoreOp});
    }
    if (stencil) {
      state->RenderTargets.push_back({name, view, VK_IMAGE_ASPECT_STENCIL_BIT,
                                      attachment.StencilFinalLayout, attachment.StencilStoreOp});
    }
  }
}

void ResourcesDumpService::EndRenderPass(GITSKey key, VkCommandBuffer commandBuffer) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    CommandBufferCall(key, commandBuffer);
    return;
  }
  auto renderPassIt = m_RenderPasses.find(state->RenderPass);
  if (renderPassIt != m_RenderPasses.end()) {
    const std::vector<RenderPassAttachment>& attachments = renderPassIt->second;
    const size_t count = std::min(attachments.size(), state->Attachments.size());
    for (size_t i = 0; i < count; ++i) {
      auto viewIt = m_Views.find(state->Attachments[i]);
      if (viewIt == m_Views.end()) {
        continue;
      }
      auto imageIt = m_Images.find(viewIt->second.Image);
      if (imageIt == m_Images.end()) {
        continue;
      }
      const RenderPassAttachment& attachment = attachments[i];
      for (const VkImageSubresourceRange& range :
           RenderedRanges(*state, viewIt->second, imageIt->second)) {
        StateChange change;
        change.Image = viewIt->second.Image;
        change.Range = range;
        change.Layout = attachment.FinalLayout;
        if ((change.Range.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) &&
            attachment.StencilFinalLayout != attachment.FinalLayout) {
          StateChange stencilChange = change;
          stencilChange.Range.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
          stencilChange.Layout = attachment.StencilFinalLayout;
          change.Range.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
          if (change.Range.aspectMask) {
            state->Changes.push_back(change);
          }
          state->Changes.push_back(stencilChange);
        } else {
          state->Changes.push_back(change);
        }
      }
    }
  }
  const std::vector<RenderTarget> renderTargets = std::move(state->RenderTargets);
  state->InRenderPass = false;
  state->RenderPass = VK_NULL_HANDLE;
  state->Attachments.clear();
  state->RenderTargets.clear();
  EndRenderPassInstance(key, commandBuffer, *state, renderTargets);
}

void ResourcesDumpService::BeginRendering(VkCommandBuffer commandBuffer,
                                          const VkRenderingInfo& info) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    return;
  }
  state->InRenderPass = true;
  state->Suspending = info.flags & VK_RENDERING_SUSPENDING_BIT;
  state->DumpsBeforeRenderPass = state->DumpsBeforeRenderPass.value_or(state->Dumps.size());
  state->LayerCount = info.layerCount;
  state->ViewMask = info.viewMask;
  if (info.flags & VK_RENDERING_RESUMING_BIT) {
    std::tie(state->RenderPassFrame, state->RenderPassNumber) = m_SuspendedRenderPass;
  } else {
    state->RenderPassFrame = m_Frame;
    state->RenderPassNumber = ++m_RenderPassCount;
  }
  state->RenderTargets.clear();
  const auto addAttachment = [this, state](const VkRenderingAttachmentInfo& attachment,
                                           const std::string& name, VkImageAspectFlags aspect) {
    if (attachment.imageView) {
      TrackAttachmentUse(*state, attachment.imageView);
      state->RenderTargets.push_back(
          {name, attachment.imageView, aspect, attachment.imageLayout, attachment.storeOp});
    }
    if (attachment.resolveMode != VK_RESOLVE_MODE_NONE && attachment.resolveImageView) {
      TrackAttachmentUse(*state, attachment.resolveImageView);
      state->RenderTargets.push_back({name + "_resolve", attachment.resolveImageView, aspect,
                                      attachment.resolveImageLayout, VK_ATTACHMENT_STORE_OP_STORE});
    }
  };
  for (uint32_t i = 0; info.pColorAttachments && i < info.colorAttachmentCount; ++i) {
    addAttachment(info.pColorAttachments[i], "rt_" + std::to_string(i), VK_IMAGE_ASPECT_COLOR_BIT);
  }
  if (info.pDepthAttachment) {
    addAttachment(*info.pDepthAttachment, "ds", VK_IMAGE_ASPECT_DEPTH_BIT);
  }
  if (info.pStencilAttachment) {
    addAttachment(*info.pStencilAttachment, "ds", VK_IMAGE_ASPECT_STENCIL_BIT);
  }
}

void ResourcesDumpService::EndRendering(GITSKey key, VkCommandBuffer commandBuffer) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    CommandBufferCall(key, commandBuffer);
    return;
  }
  if (state->Suspending) {
    state->RenderTargets.clear();
    m_SuspendedRenderPass = {state->RenderPassFrame, state->RenderPassNumber};
    if (m_CommandKeys.count(key)) {
      m_ReachedCommandKeys.insert(key);
      LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
                << " suspends a render pass instance that a later command buffer resumes, so "
                   "resources cannot be copied after it. Use the key of the command that ends "
                   "the resumed render pass instance, or of the vkQueueSubmit that submits the "
                   "command buffers, instead.";
    }
    return;
  }
  const std::vector<RenderTarget> renderTargets = std::move(state->RenderTargets);
  state->InRenderPass = false;
  state->RenderTargets.clear();
  EndRenderPassInstance(key, commandBuffer, *state, renderTargets);
}

void ResourcesDumpService::ExecuteCommands(VkCommandBuffer commandBuffer,
                                           const VkCommandBuffer* secondaries,
                                           uint32_t count) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state || !secondaries) {
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    const CommandBufferState* secondary = GetCommandBuffer(secondaries[i]);
    if (!secondary) {
      continue;
    }
    const size_t changesBefore = state->Changes.size();
    state->Changes.insert(state->Changes.end(), secondary->Changes.begin(),
                          secondary->Changes.end());
    for (const DumpPoint& point : secondary->Dumps) {
      state->Dumps.push_back({changesBefore + point.ChangeIndex, point.Batch,
                              point.MayOverlap || state->SimultaneousUse});
    }
  }
}

void ResourcesDumpService::BeginVideoCoding(GITSKey key, VkCommandBuffer commandBuffer) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (state) {
    state->InVideoCoding = true;
  }
  CommandBufferCall(key, commandBuffer);
}

void ResourcesDumpService::EndVideoCoding(GITSKey key, VkCommandBuffer commandBuffer) {
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (state) {
    state->InVideoCoding = false;
  }
  CommandBufferCall(key, commandBuffer);
}

void ResourcesDumpService::CommandBufferCall(GITSKey key, VkCommandBuffer commandBuffer) {
  if (!m_CommandKeys.count(key)) {
    return;
  }
  m_ReachedCommandKeys.insert(key);
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (!state) {
    LOG_WARNING << "ResourcesDump: command " << DumpKeyToString(key)
                << " is recorded into an unknown command buffer, nothing is dumped after it";
    return;
  }
  if (state->InRenderPass && state->Suspending) {
    LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
              << " is recorded after a render pass instance suspends, and before a later command "
                 "buffer resumes it, where resources cannot be copied. Use the key of the command "
                 "that ends the resumed render pass instance, or of the vkQueueSubmit that submits "
                 "the command buffers, instead.";
    return;
  }
  if (state->InRenderPass) {
    LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
              << " is recorded inside a render pass instance, where resources cannot be copied. "
                 "Use the key of the command that ends the render pass instance, or of the "
                 "vkQueueSubmit that submits the command buffer, instead.";
    return;
  }
  if (state->InVideoCoding) {
    LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
              << " is recorded inside a video coding scope, where resources cannot be copied. "
                 "Use the key of the vkCmdEndVideoCodingKHR that ends the scope, or of the "
                 "vkQueueSubmit that submits the command buffer, instead.";
    return;
  }
  if (state->Protected) {
    LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
              << " is recorded into a protected command buffer, which cannot write the staging "
                 "memory. Use the key of the vkQueueSubmit that submits the command buffer "
                 "instead.";
    return;
  }
  if (!SupportsCopies(state->Device, state->QueueFamilyIndex)) {
    LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
              << " is recorded into a command buffer of queue family " << state->QueueFamilyIndex
              << ", which does not support transfer commands, so nothing is dumped after it";
    return;
  }
  RecordDumps(commandBuffer, *state,
              CollectTargets(key, state->Device, state->QueueFamilyIndex, state));
}

void ResourcesDumpService::EndCommandBuffer(GITSKey key, VkCommandBuffer commandBuffer) {
  CommandBufferCall(key, commandBuffer);
  CommandBufferState* state = GetCommandBuffer(commandBuffer);
  if (state) {
    state->Unsubmitted = true;
    state->RecordingEnd = ++m_RecordingEnds;
  }
}

void ResourcesDumpService::EndRenderPassInstance(GITSKey key,
                                                 VkCommandBuffer commandBuffer,
                                                 CommandBufferState& state,
                                                 const std::vector<RenderTarget>& renderTargets) {
  if (m_CommandKeys.count(key)) {
    m_ReachedCommandKeys.insert(key);
    if (state.Protected) {
      LOG_ERROR << "ResourcesDump: command " << DumpKeyToString(key)
                << " is recorded into a protected command buffer, which cannot write the staging "
                   "memory. Use the key of the vkQueueSubmit that submits the command buffer "
                   "instead.";
    } else {
      RecordDumps(commandBuffer, state,
                  CollectTargets(key, state.Device, state.QueueFamilyIndex, &state));
    }
  }
  if (!m_RenderTargetsDump || !m_RenderTargetFrames[state.RenderPassFrame] ||
      !m_RenderTargetPasses[state.RenderPassNumber]) {
    return;
  }
  ++m_SelectedRenderPasses;
  if (m_RenderTargetsDryRun) {
    if (!renderTargets.empty()) {
      m_DryRunRenderPasses[state.RenderPassFrame].insert(state.RenderPassNumber);
    }
    return;
  }
  if (state.Protected) {
    LOG_WARNING << "RenderTargetsDump: frame " << state.RenderPassFrame << " render pass "
                << state.RenderPassNumber
                << " is recorded into a protected command buffer, which cannot write the staging "
                   "memory, so its render targets are not dumped";
    return;
  }
  RecordDumps(commandBuffer, state, RenderTargetTargets(state, renderTargets));
}

void ResourcesDumpService::RecordDumps(VkCommandBuffer commandBuffer,
                                       CommandBufferState& state,
                                       std::vector<DumpTarget> targets) {
  if (targets.empty()) {
    return;
  }
  std::shared_ptr<DumpBatch> batch =
      GetDump(state.Device).RecordDumps(commandBuffer, std::move(targets));
  if (!batch->Resources.empty()) {
    state.Dumps.push_back({state.Changes.size(), std::move(batch), state.SimultaneousUse});
  }
}

void ResourcesDumpService::PreSubmit(const std::vector<VkCommandBuffer>& commandBuffers) {
  std::vector<const CommandBufferState*> states;
  std::unordered_map<VkImage, ImageLayouts> layouts;
  std::unordered_map<VkBuffer, Ownership> bufferOwners;
  std::unordered_map<VkImage, Ownership> imageOwners;
  for (VkCommandBuffer commandBuffer : commandBuffers) {
    const CommandBufferState* state = GetCommandBuffer(commandBuffer);
    if (!state) {
      continue;
    }
    states.push_back(state);
    for (const DumpPoint& point : state->Dumps) {
      for (const StagedResource& resource : point.Batch->Resources) {
        auto bufferIt = m_Buffers.find(resource.Target.Buffer);
        if (bufferIt != m_Buffers.end() && bufferIt->second.Exclusive) {
          bufferOwners.emplace(bufferIt->first, bufferIt->second.Owner);
        }
        auto imageIt = m_Images.find(resource.Target.Image);
        if (imageIt != m_Images.end()) {
          layouts.emplace(imageIt->first, imageIt->second.Layouts);
          if (imageIt->second.Exclusive) {
            imageOwners.emplace(imageIt->first, imageIt->second.Owner);
          }
        }
      }
    }
  }

  // The first render pass instance after one that suspends, in submission order, resumes it, and
  // no copies may execute between them.
  bool suspended = false;
  for (const CommandBufferState* state : states) {
    if (suspended && state->DumpsBeforeRenderPass.value_or(state->Dumps.size()) > 0) {
      throw std::runtime_error(
          "ResourcesDump: the copies of " +
          state->Dumps.front().Batch->Resources.front().Target.Name.filename().string() +
          " execute after a render pass instance suspends and before it is resumed, where "
          "resources cannot be copied. Dump after the command that ends the resumed render pass "
          "instance, or after the vkQueueSubmit, instead.");
    }
    if (state->DumpsBeforeRenderPass) {
      suspended = state->Suspending;
    }
  }

  // The copies were recorded with the layouts and queue family owners expected when the command
  // buffer executes. Replay the changes of every command buffer of the submission in execution
  // order to check them, as barriers from wrong layouts must not execute, and copies of a resource
  // that their queue family does not own read undefined contents.
  std::unordered_set<const DumpBatch*> executed;
  for (const CommandBufferState* state : states) {
    size_t applied = 0;
    const auto applyChanges = [&](size_t end) {
      for (; applied < end && applied < state->Changes.size(); ++applied) {
        const StateChange& change = state->Changes[applied];
        auto it = layouts.find(change.Image);
        if (change.Layout && it != layouts.end()) {
          it->second.Set(change.Range, *change.Layout);
        }
        if (change.Owner) {
          auto bufferIt = change.Buffer ? bufferOwners.find(change.Buffer) : bufferOwners.end();
          if (bufferIt != bufferOwners.end()) {
            ApplyOwnershipChange(bufferIt->second, *change.Owner);
          }
          auto imageIt = change.Image ? imageOwners.find(change.Image) : imageOwners.end();
          if (imageIt != imageOwners.end()) {
            ApplyOwnershipChange(imageIt->second, *change.Owner);
          }
        }
      }
    };
    for (const DumpPoint& point : state->Dumps) {
      applyChanges(point.ChangeIndex);
      const std::string name = point.Batch->Resources.front().Target.Name.filename().string();
      if (!executed.insert(point.Batch.get()).second) {
        throw std::runtime_error(
            "ResourcesDump: the submission executes the copies of " + name +
            " more than once, so they overwrite each other. Dump after the vkQueueSubmit "
            "instead.");
      }
      for (const StagedResource& resource : point.Batch->Resources) {
        auto bufferIt = bufferOwners.find(resource.Target.Buffer);
        auto imageIt = imageOwners.find(resource.Target.Image);
        std::string ownershipProblem;
        if (bufferIt != bufferOwners.end()) {
          ownershipProblem = OwnershipProblem(m_Buffers.at(bufferIt->first), bufferIt->second,
                                              state->QueueFamilyIndex);
        } else if (imageIt != imageOwners.end()) {
          ownershipProblem = OwnershipProblem(m_Images.at(imageIt->first), imageIt->second,
                                              state->QueueFamilyIndex);
        }
        if (!ownershipProblem.empty()) {
          throw std::runtime_error(
              "ResourcesDump: the copies of " + resource.Target.Name.filename().string() +
              " were recorded for a resource that their queue family owns, but when they execute "
              "it " +
              ownershipProblem +
              ", as the command buffers executing before them are not the ones recorded before "
              "them. Dump after the vkQueueSubmit that submits the command buffer instead.");
        }
        auto it = layouts.find(resource.Target.Image);
        if (it == layouts.end()) {
          continue;
        }
        const std::optional<LayoutMismatch> mismatch =
            FindLayoutMismatch(resource.Target, it->second);
        if (mismatch) {
          throw std::runtime_error(
              "ResourcesDump: the copies of " + resource.Target.Name.filename().string() +
              " were recorded for the layout " + toStr(mismatch->Expected) +
              ", but the image is in the layout " + toStr(mismatch->Actual) +
              " when they execute, as the command buffers executing before them are not the ones "
              "recorded before them. Dump after the vkQueueSubmit that submits the command buffer "
              "instead.");
        }
      }
    }
    applyChanges(state->Changes.size());
  }

  // Executing the copies again must not overwrite staging memory that is still being written.
  // Command buffers without VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT are resubmitted only once
  // their earlier execution completes, so only the writing remains. Other earlier executions may
  // wait for work of later replayed calls, so their fence, submitted right after them, is only
  // given a moment to signal.
  constexpr uint64_t fenceSignalDelay = 100000000ull;
  for (const CommandBufferState* state : states) {
    for (const DumpPoint& point : state->Dumps) {
      if (!point.Batch->Submitted) {
        continue;
      }
      if (point.MayOverlap && point.Batch->Execution->Wait(fenceSignalDelay) != VK_SUCCESS) {
        throw std::runtime_error(
            "ResourcesDump: the copies of " +
            point.Batch->Resources.front().Target.Name.filename().string() +
            " execute again before their earlier execution completes, so they overwrite each "
            "other. Dump after the vkQueueSubmit that submits the command buffer instead.");
      }
      GetDump(state->Device).WriteExecuted(*point.Batch);
    }
  }
}

std::vector<ResourcesDumpService::SubmitBatch> ResourcesDumpService::Batches(
    uint32_t submitCount, const VkSubmitInfo* submits) {
  std::vector<SubmitBatch> batches;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    const VkSubmitInfo& submit = submits[i];
    const auto* timelineInfo = FindInChain<VkTimelineSemaphoreSubmitInfo>(
        submit.pNext, VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO);
    SubmitBatch& batch = batches.emplace_back();
    for (uint32_t j = 0;
         submit.pWaitSemaphores && submit.pWaitDstStageMask && j < submit.waitSemaphoreCount; ++j) {
      VkSemaphoreSubmitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait.semaphore = submit.pWaitSemaphores[j];
      if (timelineInfo && timelineInfo->pWaitSemaphoreValues &&
          j < timelineInfo->waitSemaphoreValueCount) {
        wait.value = timelineInfo->pWaitSemaphoreValues[j];
      }
      wait.stageMask = submit.pWaitDstStageMask[j];
      batch.Waits.push_back(wait);
    }
    if (submit.pCommandBuffers) {
      batch.CommandBuffers.assign(submit.pCommandBuffers,
                                  submit.pCommandBuffers + submit.commandBufferCount);
    }
    for (uint32_t j = 0; submit.pSignalSemaphores && j < submit.signalSemaphoreCount; ++j) {
      VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      signal.semaphore = submit.pSignalSemaphores[j];
      if (timelineInfo && timelineInfo->pSignalSemaphoreValues &&
          j < timelineInfo->signalSemaphoreValueCount) {
        signal.value = timelineInfo->pSignalSemaphoreValues[j];
      }
      signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      batch.Signals.push_back(signal);
    }
  }
  return batches;
}

std::vector<ResourcesDumpService::SubmitBatch> ResourcesDumpService::Batches(
    uint32_t submitCount, const VkSubmitInfo2* submits) {
  std::vector<SubmitBatch> batches;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    const VkSubmitInfo2& submit = submits[i];
    SubmitBatch& batch = batches.emplace_back();
    if (submit.pWaitSemaphoreInfos) {
      batch.Waits.assign(submit.pWaitSemaphoreInfos,
                         submit.pWaitSemaphoreInfos + submit.waitSemaphoreInfoCount);
    }
    for (uint32_t j = 0; submit.pCommandBufferInfos && j < submit.commandBufferInfoCount; ++j) {
      batch.CommandBuffers.push_back(submit.pCommandBufferInfos[j].commandBuffer);
    }
    if (submit.pSignalSemaphoreInfos) {
      batch.Signals.assign(submit.pSignalSemaphoreInfos,
                           submit.pSignalSemaphoreInfos + submit.signalSemaphoreInfoCount);
    }
  }
  return batches;
}

std::vector<ResourcesDumpService::SubmitBatch> ResourcesDumpService::Batches(
    uint32_t bindInfoCount, const VkBindSparseInfo* bindInfos) {
  std::vector<SubmitBatch> batches;
  for (uint32_t i = 0; bindInfos && i < bindInfoCount; ++i) {
    const VkBindSparseInfo& bindInfo = bindInfos[i];
    const auto* timelineInfo = FindInChain<VkTimelineSemaphoreSubmitInfo>(
        bindInfo.pNext, VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO);
    SubmitBatch& batch = batches.emplace_back();
    for (uint32_t j = 0; bindInfo.pWaitSemaphores && j < bindInfo.waitSemaphoreCount; ++j) {
      VkSemaphoreSubmitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait.semaphore = bindInfo.pWaitSemaphores[j];
      if (timelineInfo && timelineInfo->pWaitSemaphoreValues &&
          j < timelineInfo->waitSemaphoreValueCount) {
        wait.value = timelineInfo->pWaitSemaphoreValues[j];
      }
      wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      batch.Waits.push_back(wait);
    }
    for (uint32_t j = 0; bindInfo.pSignalSemaphores && j < bindInfo.signalSemaphoreCount; ++j) {
      VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      signal.semaphore = bindInfo.pSignalSemaphores[j];
      if (timelineInfo && timelineInfo->pSignalSemaphoreValues &&
          j < timelineInfo->signalSemaphoreValueCount) {
        signal.value = timelineInfo->pSignalSemaphoreValues[j];
      }
      signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      batch.Signals.push_back(signal);
    }
  }
  return batches;
}

void ResourcesDumpService::CheckSubmission(
    GITSKey key, VkQueue queue, uint32_t submitCount, const VkSubmitInfo* submits, VkFence fence) {
  CheckSubmission(key, queue, Batches(submitCount, submits), fence);
}

void ResourcesDumpService::CheckSubmission(
    GITSKey key, VkQueue queue, uint32_t submitCount, const VkSubmitInfo2* submits, VkFence fence) {
  CheckSubmission(key, queue, Batches(submitCount, submits), fence);
}

void ResourcesDumpService::TrackSignals(VkQueue queue,
                                        uint32_t submitCount,
                                        const VkSubmitInfo* submits) {
  TrackSignals(queue, Batches(submitCount, submits));
}

void ResourcesDumpService::TrackSignals(VkQueue queue,
                                        uint32_t submitCount,
                                        const VkSubmitInfo2* submits) {
  TrackSignals(queue, Batches(submitCount, submits));
}

void ResourcesDumpService::TrackSignals(VkQueue queue,
                                        uint32_t bindInfoCount,
                                        const VkBindSparseInfo* bindInfos) {
  TrackSignals(queue, Batches(bindInfoCount, bindInfos));
}

void ResourcesDumpService::TrackSignals(VkQueue queue, const std::vector<SubmitBatch>& batches) {
  std::vector<TimelineWait>& queueWaits = m_QueueWaits[queue];
  std::erase_if(queueWaits, [this](const TimelineWait& wait) { return Signaled(wait, 0); });
  for (const SubmitBatch& batch : batches) {
    for (const VkSemaphoreSubmitInfo& wait : batch.Waits) {
      auto semaphoreIt = m_Semaphores.find(wait.semaphore);
      if (semaphoreIt == m_Semaphores.end()) {
        continue;
      }
      if (semaphoreIt->second.second) {
        if (!Signaled({wait.semaphore, wait.value}, 0)) {
          queueWaits.push_back({wait.semaphore, wait.value});
        }
        continue;
      }
      auto signalIt = m_BinarySignalWaits.find(wait.semaphore);
      if (signalIt != m_BinarySignalWaits.end()) {
        for (const TimelineWait& signalWait : signalIt->second) {
          if (!Signaled(signalWait, 0)) {
            queueWaits.push_back(signalWait);
          }
        }
        m_BinarySignalWaits.erase(signalIt);
      }
    }
    for (const VkSemaphoreSubmitInfo& signal : batch.Signals) {
      auto semaphoreIt = m_Semaphores.find(signal.semaphore);
      if (semaphoreIt == m_Semaphores.end()) {
        continue;
      }
      if (!semaphoreIt->second.second) {
        m_BinarySignalWaits[signal.semaphore] = queueWaits;
        continue;
      }
      std::vector<TimelineSignal>& signals = m_TimelineSignals[signal.semaphore];
      std::erase_if(signals, [&](const TimelineSignal& earlier) {
        return TimelineReached(signal.semaphore, earlier.Value);
      });
      signals.push_back({signal.value, queueWaits});
    }
  }
}

bool ResourcesDumpService::TimelineReached(VkSemaphore semaphore, uint64_t value) const {
  auto it = m_Semaphores.find(semaphore);
  if (it == m_Semaphores.end()) {
    return false;
  }
  const VkDevice device = it->second.first;
  VkDeviceLevelDispatchTable* table = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  PFN_vkGetSemaphoreCounterValue getCounterValue = !table ? nullptr
                                                   : table->vkGetSemaphoreCounterValue
                                                       ? table->vkGetSemaphoreCounterValue
                                                       : table->vkGetSemaphoreCounterValueKHR;
  uint64_t counter = 0;
  return getCounterValue && getCounterValue(device, semaphore, &counter) == VK_SUCCESS &&
         counter >= value;
}

bool ResourcesDumpService::Signaled(const TimelineWait& wait, unsigned depth) const {
  if (TimelineReached(wait.first, wait.second)) {
    return true;
  }
  // Signal operations waiting for each other never execute.
  constexpr unsigned maxDepth = 16;
  auto it = m_TimelineSignals.find(wait.first);
  if (depth >= maxDepth || it == m_TimelineSignals.end()) {
    return false;
  }
  return std::any_of(it->second.begin(), it->second.end(), [&](const TimelineSignal& signal) {
    return signal.Value >= wait.second && AllSignaled(signal.Waits, depth + 1);
  });
}

bool ResourcesDumpService::AllSignaled(const std::vector<TimelineWait>& waits,
                                       unsigned depth) const {
  return std::all_of(waits.begin(), waits.end(),
                     [&](const TimelineWait& wait) { return Signaled(wait, depth); });
}

bool ResourcesDumpService::QueueMayWaitForLaterSignals(VkQueue queue) const {
  auto it = m_QueueWaits.find(queue);
  return it != m_QueueWaits.end() && !AllSignaled(it->second, 0);
}

void ResourcesDumpService::CheckSubmission(GITSKey key,
                                           VkQueue queue,
                                           const std::vector<SubmitBatch>& batches,
                                           VkFence fence) {
  // The batches executing copies, and whether a batch from the first one on signals semaphores.
  std::optional<size_t> firstCopies;
  size_t lastCopies = 0;
  bool signalsAfterCopies = false;
  for (size_t i = 0; i < batches.size(); ++i) {
    for (const VkSemaphoreSubmitInfo& wait : batches[i].Waits) {
      auto acquireIt = m_AcquireSemaphores.find(wait.semaphore);
      if (acquireIt == m_AcquireSemaphores.end()) {
        continue;
      }
      auto imageIt = m_Images.find(acquireIt->second);
      if (imageIt != m_Images.end()) {
        imageIt->second.AcquisitionWaits.insert(queue);
      }
      m_AcquireSemaphores.erase(acquireIt);
    }
    for (VkCommandBuffer commandBuffer : batches[i].CommandBuffers) {
      const CommandBufferState* state = GetCommandBuffer(commandBuffer);
      if (!state) {
        continue;
      }
      for (const DumpPoint& point : state->Dumps) {
        firstCopies = firstCopies.value_or(i);
        lastCopies = i;
        for (const StagedResource& resource : point.Batch->Resources) {
          if (resource.Target.RenderTarget) {
            continue;
          }
          auto imageIt = m_Images.find(resource.Target.Image);
          const std::string problem =
              imageIt == m_Images.end() ? "" : SwapchainImageProblem(imageIt->second, queue);
          if (!problem.empty()) {
            throw std::runtime_error(
                "ResourcesDump: the submission " + DumpKeyToString(key) +
                " executes the copies of " + resource.Target.Name.filename().string() +
                ", and the image O" + DumpKeyToString(imageIt->second.Key) + " " + problem +
                ", so the presentation engine may use it while it is copied. Dump it after a "
                "command of a submission that waits for its acquisition instead.");
          }
        }
      }
    }
    // A signal at specific stages waits for the copies only if commands at those stages follow
    // them, so the work waiting for it could change the resources, or use the images in the
    // layout of the copies, while they are copied. Widening its stages could make it wait for
    // work that waits for it.
    if (!firstCopies) {
      continue;
    }
    for (const VkSemaphoreSubmitInfo& signal : batches[i].Signals) {
      constexpr VkPipelineStageFlags2 allCommands =
          VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
      if (!(signal.stageMask & allCommands)) {
        throw std::runtime_error(
            "ResourcesDump: the submission " + DumpKeyToString(key) +
            " executes the copies of dumped resources, and signals semaphores at specific "
            "pipeline stages after them, which may not wait for the copies. Dump after commands "
            "of command buffers submitted without such signals instead.");
      }
      signalsAfterCopies = true;
    }
  }
  // The copies also wait for the semaphore waits of the earlier submissions to the queue, which
  // neither the signals nor the fence of the submission wait for otherwise.
  if (firstCopies && (signalsAfterCopies || fence != VK_NULL_HANDLE) &&
      QueueMayWaitForLaterSignals(queue)) {
    throw std::runtime_error(
        "ResourcesDump: the submission " + DumpKeyToString(key) +
        " executes the copies of dumped resources, which wait for the semaphore waits of the "
        "earlier submissions to the queue, and signals semaphores or a fence after them, while "
        "an earlier submission waits for a semaphore value that may be signaled after those "
        "signals, so they may never execute. Dump after commands of a submission to a queue "
        "without such waits instead.");
  }
  if (!signalsAfterCopies) {
    return;
  }

  // The barrier before the copies waits for every stage, so the copies wait for every semaphore
  // wait before them, and the signals after the copies wait for those waits. A wait for a value
  // that an earlier batch of the submission signals does not depend on later signals.
  std::unordered_map<VkSemaphore, uint64_t> signaled;
  for (size_t i = 0; i <= lastCopies; ++i) {
    for (const VkSemaphoreSubmitInfo& wait : batches[i].Waits) {
      auto signaledIt = signaled.find(wait.semaphore);
      if (wait.stageMask == 0 ||
          (signaledIt != signaled.end() && signaledIt->second >= wait.value)) {
        continue;
      }
      if (WaitMayDependOnLaterSignals(wait.semaphore, wait.value)) {
        throw std::runtime_error(
            "ResourcesDump: the submission " + DumpKeyToString(key) +
            " executes the copies of dumped resources, which wait for its semaphore waits, and "
            "waits for a semaphore value that may be signaled after the semaphores it signals "
            "after the copies, so those signals may never execute. Dump after commands of "
            "command buffers submitted without such waits instead.");
      }
    }
    for (const VkSemaphoreSubmitInfo& signal : batches[i].Signals) {
      uint64_t& value = signaled[signal.semaphore];
      value = std::max(value, signal.value);
    }
  }
}

std::string ResourcesDumpService::SwapchainImageProblem(const Image& image, VkQueue queue) const {
  // Offscreen replay acquires every swapchain image once and never presents them.
  if (!image.Swapchain || Configurator::Get().common.player.renderOffscreen) {
    return {};
  }
  if (!image.Acquired) {
    return "is a swapchain image that is not acquired";
  }
  if (!image.AcquiredWithFence && !image.AcquisitionWaits.count(queue)) {
    return "is a swapchain image whose acquisition semaphore the queue has not waited for";
  }
  return {};
}

bool ResourcesDumpService::DumpsAfterSubmission(GITSKey key, VkQueue queue) {
  auto queueIt = m_Queues.find(queue);
  if (!m_CommandKeys.count(key) || queueIt == m_Queues.end()) {
    return false;
  }
  const auto [device, queueFamilyIndex] = queueIt->second;
  if (!SupportsCopies(device, queueFamilyIndex)) {
    m_ReachedCommandKeys.insert(key);
    LOG_ERROR << "ResourcesDump: the submission " << DumpKeyToString(key)
              << " is to a queue of queue family " << queueFamilyIndex
              << ", which does not support transfer commands, so nothing is dumped after it";
    return false;
  }
  return true;
}

void ResourcesDumpService::CannotDeferSignals(GITSKey key, const char* reason) {
  m_ReachedCommandKeys.insert(key);
  LOG_ERROR << "ResourcesDump: the submission " << DumpKeyToString(key) << " " << reason
            << ", so the work waiting for it cannot be made to wait for the copies of the "
               "resources, and nothing is dumped after it";
}

bool ResourcesDumpService::WaitMayDependOnLaterSignals(VkSemaphore semaphore,
                                                       uint64_t value) const {
  auto it = m_Semaphores.find(semaphore);
  if (it == m_Semaphores.end()) {
    return true;
  }
  if (it->second.second) {
    return !Signaled({semaphore, value}, 0);
  }
  auto signalIt = m_BinarySignalWaits.find(semaphore);
  return signalIt != m_BinarySignalWaits.end() && !AllSignaled(signalIt->second, 0);
}

void ResourcesDumpService::Submit(GITSKey key,
                                  VkQueue queue,
                                  const std::vector<VkCommandBuffer>& commandBuffers,
                                  const SubmitSignals* signals) {
  auto queueIt = m_Queues.find(queue);
  if (queueIt == m_Queues.end()) {
    return;
  }
  const auto [device, queueFamilyIndex] = queueIt->second;

  std::vector<DumpTarget> targets;
  std::exception_ptr error;
  try {
    std::vector<std::shared_ptr<DumpBatch>> executed;
    std::vector<std::shared_ptr<DumpBatch>> executedAgain;
    for (VkCommandBuffer commandBuffer : commandBuffers) {
      CommandBufferState* state = GetCommandBuffer(commandBuffer);
      if (!state) {
        continue;
      }
      state->Unsubmitted = false;
      for (const StateChange& change : state->Changes) {
        Apply(change);
      }
      for (const DumpPoint& point : state->Dumps) {
        if (!point.Batch->Submitted) {
          executed.push_back(point.Batch);
        } else {
          executedAgain.push_back(point.Batch);
        }
      }
    }
    if (!executed.empty() || !executedAgain.empty()) {
      GetDump(device).WriteAfterExecution(queue, std::move(executed), std::move(executedAgain));
    }

    if (m_CommandKeys.count(key)) {
      m_ReachedCommandKeys.insert(key);
    }
    if (signals) {
      targets = CollectTargets(key, device, queueFamilyIndex, nullptr);
      std::erase_if(targets, [&](const DumpTarget& target) {
        auto imageIt = m_Images.find(target.Image);
        const std::string problem =
            imageIt == m_Images.end() ? "" : SwapchainImageProblem(imageIt->second, queue);
        if (!problem.empty()) {
          LOG_WARNING << CommandContext(key) << ", the image O"
                      << DumpKeyToString(imageIt->second.Key) << " " << problem
                      << ", so the presentation engine may use it and it is not dumped";
        }
        return !problem.empty();
      });
    }
  } catch (...) {
    if (!signals) {
      throw;
    }
    error = std::current_exception();
    targets.clear();
  }
  // The signals taken out of the submission are submitted even if nothing is dumped, or the work
  // waiting for them never runs.
  if (signals) {
    GetDump(device).DumpAfterSubmissions(queue, queueFamilyIndex, std::move(targets), *signals);
  }
  if (error) {
    std::rethrow_exception(error);
  }
  auto deviceIt = m_Devices.find(device);
  if (deviceIt != m_Devices.end() && deviceIt->second.Dump) {
    const std::string error = deviceIt->second.Dump->TakeWorkerError();
    if (!error.empty()) {
      throw std::runtime_error(error);
    }
  }
}

void ResourcesDumpService::Apply(const StateChange& change) {
  Resource* resource = nullptr;
  if (change.Buffer) {
    auto it = m_Buffers.find(change.Buffer);
    resource = it == m_Buffers.end() ? nullptr : &it->second;
  } else {
    auto it = m_Images.find(change.Image);
    if (it != m_Images.end()) {
      resource = &it->second;
      if (change.Layout) {
        it->second.Layouts.Set(change.Range, *change.Layout);
      }
    }
  }
  if (resource && change.Owner) {
    ApplyOwnershipChange(resource->Owner, *change.Owner);
  }
}

ResourcesDumpService::ChangeLists ResourcesDumpService::ChangesBefore(
    const CommandBufferState* state) const {
  ChangeLists before;
  if (!state) {
    return before;
  }
  std::vector<const CommandBufferState*> unsubmitted;
  for (const auto& [commandBuffer, other] : m_CommandBuffers) {
    if (&other != state && other.Unsubmitted && !other.Secondary && other.Device == state->Device) {
      unsubmitted.push_back(&other);
    }
  }
  std::sort(unsubmitted.begin(), unsubmitted.end(),
            [](const CommandBufferState* lhs, const CommandBufferState* rhs) {
              return lhs->RecordingEnd < rhs->RecordingEnd;
            });
  for (const CommandBufferState* other : unsubmitted) {
    before.push_back(&other->Changes);
  }
  before.push_back(&state->Changes);
  return before;
}

ImageLayouts ResourcesDumpService::ResolveLayouts(VkImage handle,
                                                  const Image& image,
                                                  const ChangeLists& before) const {
  ImageLayouts layouts = image.Layouts;
  for (const std::vector<StateChange>* changes : before) {
    for (const StateChange& change : *changes) {
      if (change.Image == handle && change.Layout) {
        layouts.Set(change.Range, *change.Layout);
      }
    }
  }
  return layouts;
}

Ownership ResourcesDumpService::ResolveOwner(VkBuffer buffer,
                                             VkImage image,
                                             const Resource& resource,
                                             const ChangeLists& before) const {
  Ownership owner = resource.Owner;
  for (const std::vector<StateChange>* changes : before) {
    for (const StateChange& change : *changes) {
      if (change.Owner &&
          ((buffer && change.Buffer == buffer) || (image && change.Image == image))) {
        ApplyOwnershipChange(owner, *change.Owner);
      }
    }
  }
  return owner;
}

std::vector<DumpTarget> ResourcesDumpService::CollectTargets(GITSKey commandKey,
                                                             VkDevice device,
                                                             uint32_t queueFamilyIndex,
                                                             const CommandBufferState* state) {
  const ChangeLists before = ChangesBefore(state);
  std::vector<DumpTarget> targets;
  for (const auto& [handle, buffer] : m_Buffers) {
    if (buffer.Device != device) {
      continue;
    }
    std::optional<DumpTarget> target =
        BufferTarget(commandKey, handle, buffer, queueFamilyIndex, before);
    if (target) {
      targets.push_back(std::move(*target));
    }
  }
  for (const auto& [handle, image] : m_Images) {
    if (image.Device != device || !m_ResourceKeys.count(image.Key)) {
      continue;
    }
    std::optional<DumpTarget> target =
        ImageTarget(commandKey, handle, image, queueFamilyIndex, before);
    if (target) {
      targets.push_back(std::move(*target));
    }
  }
  return targets;
}

bool ResourcesDumpService::CanDump(const std::string& context,
                                   const char* kind,
                                   const Resource& resource,
                                   const Ownership& owner,
                                   uint32_t queueFamilyIndex) {
  std::string reason;
  if (resource.Sparse) {
    reason = "is a sparse resource, which is not supported";
  } else if (resource.Protected) {
    reason = "is protected, so it cannot be copied to the staging memory";
  } else if (!resource.Bound) {
    reason = "is not bound to memory";
  } else if (!resource.TransferSrc) {
    reason = "was created without the transfer source usage that copying it requires";
  } else if (resource.Exclusive) {
    reason = OwnershipProblem(resource, owner, queueFamilyIndex);
  } else if (std::find(resource.SharingQueueFamilies.begin(), resource.SharingQueueFamilies.end(),
                       queueFamilyIndex) == resource.SharingQueueFamilies.end()) {
    std::string families;
    for (uint32_t family : resource.SharingQueueFamilies) {
      families += (families.empty() ? "" : ", ") + std::to_string(family);
    }
    reason = "is shared by queue families " + families +
             ", which do not include the queue family " + std::to_string(queueFamilyIndex) +
             " of the command";
  }
  if (reason.empty()) {
    return true;
  }
  LOG_WARNING << context << ", the " << kind << " O" << DumpKeyToString(resource.Key) << " "
              << reason << ", so it is not dumped";
  return false;
}

std::string ResourcesDumpService::OwnershipProblem(const Resource& resource,
                                                   const Ownership& owner,
                                                   uint32_t queueFamilyIndex) const {
  switch (owner.Is) {
  case Ownership::State::Owned:
    if (owner.QueueFamilyIndex == queueFamilyIndex) {
      return {};
    }
    return "is owned by queue family " + QueueFamilyToString(owner.QueueFamilyIndex) +
           ", not by the queue family " + std::to_string(queueFamilyIndex) + " of the command";
  case Ownership::State::Released:
    return "is released to queue family " + QueueFamilyToString(owner.QueueFamilyIndex) +
           ", which has not acquired it yet, so no queue family owns it";
  case Ownership::State::Mixed:
    return "has parts owned by different queue families, or released while other parts are "
           "not, which is not supported";
  case Ownership::State::Unknown:
    break;
  }
  // Without a tracked use or transfer, only a device that records commands for one queue family
  // tells the owner.
  auto deviceIt = m_Devices.find(resource.Device);
  const bool onlyQueueFamily = deviceIt != m_Devices.end() &&
                               deviceIt->second.QueueFamilies.size() == 1 &&
                               *deviceIt->second.QueueFamilies.begin() == queueFamilyIndex;
  if (onlyQueueFamily) {
    return {};
  }
  return "has an unknown queue family owner, as no barrier or render pass instance used it and "
         "the device records commands for several queue families";
}

bool ResourcesDumpService::SupportsCopies(VkDevice device, uint32_t queueFamilyIndex) const {
  // An unknown device is reported when its dump is created.
  auto deviceIt = m_Devices.find(device);
  if (deviceIt == m_Devices.end() || queueFamilyIndex >= deviceIt->second.QueueFamilyFlags.size()) {
    return true;
  }
  // Graphics and compute queue families support transfer commands, even if they do not report
  // VK_QUEUE_TRANSFER_BIT.
  constexpr VkQueueFlags copyFlags =
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
  return deviceIt->second.QueueFamilyFlags[queueFamilyIndex] & copyFlags;
}

VkImageAspectFlags ResourcesDumpService::AspectsWithoutCopies(const Image& image,
                                                              uint32_t queueFamilyIndex) const {
  VkImageAspectFlags aspects = DepthStencilAspects(image.Format);
  auto deviceIt = m_Devices.find(image.Device);
  if (!aspects || deviceIt == m_Devices.end() ||
      queueFamilyIndex >= deviceIt->second.QueueFamilyFlags.size()) {
    return 0;
  }
  const Device& device = deviceIt->second;
  const VkQueueFlags queueFlags = device.QueueFamilyFlags[queueFamilyIndex];
  if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
    return 0;
  }
  if (!device.NonGraphicsDepthStencilCopies) {
    return aspects;
  }

  VkInstanceLevelDispatchTable* instanceTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(device.PhysicalDevice);
  PFN_vkGetPhysicalDeviceFormatProperties2 getFormatProperties = nullptr;
  if (instanceTable) {
    getFormatProperties = instanceTable->vkGetPhysicalDeviceFormatProperties2
                              ? instanceTable->vkGetPhysicalDeviceFormatProperties2
                              : instanceTable->vkGetPhysicalDeviceFormatProperties2KHR;
  }
  if (!getFormatProperties) {
    return aspects;
  }
  VkFormatProperties3 properties3{};
  properties3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3;
  VkFormatProperties2 properties{};
  properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
  properties.pNext = &properties3;
  getFormatProperties(device.PhysicalDevice, image.Format, &properties);
  const VkFormatFeatureFlags2 features =
      image.Tiling == VK_IMAGE_TILING_OPTIMAL  ? properties3.optimalTilingFeatures
      : image.Tiling == VK_IMAGE_TILING_LINEAR ? properties3.linearTilingFeatures
                                               : 0;
  const bool compute = queueFlags & VK_QUEUE_COMPUTE_BIT;
  const VkFormatFeatureFlags2 depthCopies =
      compute ? VK_FORMAT_FEATURE_2_DEPTH_COPY_ON_COMPUTE_QUEUE_BIT_KHR
              : VK_FORMAT_FEATURE_2_DEPTH_COPY_ON_TRANSFER_QUEUE_BIT_KHR;
  const VkFormatFeatureFlags2 stencilCopies =
      compute ? VK_FORMAT_FEATURE_2_STENCIL_COPY_ON_COMPUTE_QUEUE_BIT_KHR
              : VK_FORMAT_FEATURE_2_STENCIL_COPY_ON_TRANSFER_QUEUE_BIT_KHR;
  if (features & depthCopies) {
    aspects &= ~VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  if (features & stencilCopies) {
    aspects &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  return aspects;
}

std::string ResourcesDumpService::CommandContext(GITSKey commandKey) const {
  return "ResourcesDump: after command " + DumpKeyToString(commandKey);
}

std::optional<DumpTarget> ResourcesDumpService::BufferTarget(GITSKey commandKey,
                                                             VkBuffer handle,
                                                             const Buffer& buffer,
                                                             uint32_t queueFamilyIndex,
                                                             const ChangeLists& before) {
  if (!CanDump(CommandContext(commandKey), "buffer", buffer,
               ResolveOwner(handle, VK_NULL_HANDLE, buffer, before), queueFamilyIndex) ||
      buffer.Size == 0) {
    return std::nullopt;
  }
  DumpTarget target;
  target.Name = DumpName(commandKey, "buffer", buffer.Key);
  target.Buffer = handle;
  target.BufferSize = buffer.Size;
  return target;
}

std::optional<DumpTarget> ResourcesDumpService::ImageTarget(GITSKey commandKey,
                                                            VkImage handle,
                                                            const Image& image,
                                                            uint32_t queueFamilyIndex,
                                                            const ChangeLists& before) {
  DumpTarget target;
  target.Name = DumpName(commandKey, "image", image.Key);
  target.FileFormat = m_ResourcesFormat;
  target.Image = handle;
  target.Layouts = ResolveLayouts(handle, image, before);
  return MakeImageTarget(CommandContext(commandKey), std::move(target), image,
                         ResolveOwner(VK_NULL_HANDLE, handle, image, before), queueFamilyIndex,
                         false);
}

std::vector<DumpTarget> ResourcesDumpService::RenderTargetTargets(
    const CommandBufferState& state, const std::vector<RenderTarget>& renderTargets) {
  const std::string context = "RenderTargetsDump: in frame " +
                              std::to_string(state.RenderPassFrame) + " render pass " +
                              std::to_string(state.RenderPassNumber);
  // The aspects of an attachment sharing a name are dumped together.
  struct Entry {
    std::string Name;
    VkImage Handle{};
    DumpTarget Target;
  };
  const ChangeLists before = ChangesBefore(&state);
  std::vector<Entry> entries;
  for (const RenderTarget& renderTarget : renderTargets) {
    auto viewIt = m_Views.find(renderTarget.View);
    if (viewIt == m_Views.end()) {
      continue;
    }
    const VkImage handle = viewIt->second.Image;
    auto imageIt = m_Images.find(handle);
    if (imageIt == m_Images.end()) {
      continue;
    }
    const Image& image = imageIt->second;
    const std::string what = context + ", the render target " + renderTarget.Name + " (image O" +
                             DumpKeyToString(image.Key) + ")";
    if (image.Samples != VK_SAMPLE_COUNT_1_BIT) {
      LOG_INFO << what << " is multisampled and cannot be copied to a buffer, so it is not dumped";
      continue;
    }
    if (renderTarget.StoreOp != VK_ATTACHMENT_STORE_OP_STORE) {
      LOG_WARNING << what << " is not dumped, as its store operation "
                  << toStr(renderTarget.StoreOp) << " leaves the "
                  << AspectName(renderTarget.Aspect) << " contents undefined";
      continue;
    }
    const View& view = viewIt->second;
    std::vector<VkImageSubresourceRange> ranges = RenderedRanges(state, view, image);
    uint32_t baseSlice = view.BaseSlice;
    uint32_t sliceCount = view.SliceCount;
    if (image.Type == VK_IMAGE_TYPE_3D) {
      // The depth slices from the first to the last rendered one.
      const uint32_t depth = std::max(1u, image.Extent.depth >> view.Range.baseMipLevel);
      const std::vector<VkImageSubresourceRange> slices =
          RenderedLayers({view.Range.aspectMask, view.Range.baseMipLevel, 1, baseSlice, sliceCount},
                         depth, state.LayerCount, state.ViewMask);
      if (slices.empty()) {
        ranges.clear();
      } else {
        baseSlice = slices.front().baseArrayLayer;
        sliceCount = slices.back().baseArrayLayer + slices.back().layerCount - baseSlice;
      }
    }
    if (ranges.empty()) {
      continue;
    }
    auto entryIt = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
      return entry.Name == renderTarget.Name && entry.Handle == handle;
    });
    if (entryIt == entries.end()) {
      std::string format = toStr(image.Format);
      if (format.starts_with("VK_FORMAT_")) {
        format.erase(0, std::char_traits<char>::length("VK_FORMAT_"));
      }
      Entry entry{renderTarget.Name, handle};
      entry.Target.Name =
          m_RenderTargetsPath / ("renderpass_f_" + std::to_string(state.RenderPassFrame) + "_rp_" +
                                 std::to_string(state.RenderPassNumber) + "_" + renderTarget.Name +
                                 "_O" + DumpKeyToString(image.Key) + "_" + format);
      entry.Target.FileFormat = m_RenderTargetsFormat;
      entry.Target.Image = handle;
      entry.Target.Layouts = ResolveLayouts(handle, image, before);
      entry.Target.BaseSlice = baseSlice;
      entry.Target.SliceCount = sliceCount;
      entryIt = entries.insert(entries.end(), std::move(entry));
    }
    // The layout the render pass instance leaves the attachment in is the one its copies start
    // from, whatever layout changes the tracking missed.
    for (VkImageSubresourceRange& range : ranges) {
      range.aspectMask = renderTarget.Aspect;
      entryIt->Target.Layouts.Set(range, renderTarget.Layout);
      entryIt->Target.Selection.push_back(range);
    }
  }

  std::vector<DumpTarget> targets;
  for (Entry& entry : entries) {
    const Image& image = m_Images.at(entry.Handle);
    std::optional<DumpTarget> target = MakeImageTarget(
        context, std::move(entry.Target), image,
        ResolveOwner(VK_NULL_HANDLE, entry.Handle, image, before), state.QueueFamilyIndex, true);
    if (target) {
      targets.push_back(std::move(*target));
    }
  }
  return targets;
}

std::optional<DumpTarget> ResourcesDumpService::MakeImageTarget(const std::string& context,
                                                                DumpTarget target,
                                                                const Image& image,
                                                                const Ownership& owner,
                                                                uint32_t queueFamilyIndex,
                                                                bool renderTarget) {
  if (!CanDump(context, "image", image, owner, queueFamilyIndex)) {
    return std::nullopt;
  }
  const std::string what = context + ", the image O" + DumpKeyToString(image.Key);
  if (image.Samples != VK_SAMPLE_COUNT_1_BIT) {
    LOG_WARNING << what
                << " is multisampled and cannot be copied to a buffer, so it is not dumped. Dump "
                   "the image it is resolved to instead.";
    return std::nullopt;
  }
  if (!GetFormatBlockInfo(image.Format).Known) {
    LOG_WARNING << what << " has format " << toStr(image.Format)
                << ", which is not supported, so it is not dumped";
    return std::nullopt;
  }
  // Why some aspects of a depth/stencil image are not copied.
  std::string excluded;
  if (image.NoTransferSrcAspect) {
    excluded = "has no transfer source usage for its " +
               DepthStencilAspectNames(image.NoTransferSrcAspect);
  }
  auto deviceIt = m_Devices.find(image.Device);
  const VkImageAspectFlags withoutCopies = AspectsWithoutCopies(image, queueFamilyIndex);
  if (withoutCopies) {
    excluded += (excluded.empty() ? "" : ", and ") + std::string("is copied on queue family ") +
                std::to_string(queueFamilyIndex) +
                ", which has no graphics support and cannot copy the " +
                DepthStencilAspectNames(withoutCopies) + " of " + toStr(image.Format) +
                (deviceIt->second.NonGraphicsDepthStencilCopies ? "" : " without maintenance10");
  }
  const VkImageAspectFlags excludedAspects = image.NoTransferSrcAspect | withoutCopies;
  if (excludedAspects && excludedAspects == DepthStencilAspects(image.Format)) {
    LOG_WARNING << what << " " << excluded << ", so it is not dumped";
    return std::nullopt;
  }
  if (excludedAspects) {
    if (deviceIt == m_Devices.end() || !deviceIt->second.SeparateDepthStencilLayouts) {
      LOG_WARNING << what << " " << excluded
                  << ", and without separateDepthStencilLayouts the layout of the other aspect "
                     "cannot change alone, so it is not dumped";
      return std::nullopt;
    }
    target.ExcludedAspects = excludedAspects;
  }

  target.Format = image.Format;
  target.Extent = image.Extent;
  target.Disjoint = image.Disjoint;
  target.RenderTarget = renderTarget;
  std::vector<VkBufferImageCopy> regions;
  CopyRegions(target, regions);
  if (regions.empty() && target.ExcludedAspects) {
    LOG_WARNING << what << " " << excluded
                << ", and the contents of its other aspect are undefined, so it is not dumped";
    return std::nullopt;
  }
  if (regions.empty()) {
    LOG_WARNING << what << " has undefined contents (its layout is "
                << toStr(target.Layouts.Get(0, 0, 0)) << "), so it is not dumped";
    return std::nullopt;
  }
  if (target.ExcludedAspects) {
    LOG_INFO << what << " " << excluded << ", so that aspect is not dumped";
  }

  const ImageLayouts& layouts = target.Layouts;
  unsigned undefined = 0;
  unsigned total = 0;
  for (uint32_t slot = 0; slot < layouts.AspectSlots(); ++slot) {
    for (uint32_t mip = 0; mip < layouts.MipLevels(); ++mip) {
      for (uint32_t layer = 0; layer < layouts.ArrayLayers(); ++layer) {
        ++total;
        if (!IsContentDefined(layouts.Get(slot, mip, layer))) {
          ++undefined;
        }
      }
    }
  }
  if (undefined > 0 && !renderTarget) {
    LOG_INFO << what << " has " << undefined << " of " << total
             << " subresources with undefined contents, which are not dumped";
  }
  return target;
}

std::filesystem::path ResourcesDumpService::DumpName(GITSKey commandKey,
                                                     const std::string& kind,
                                                     GITSKey resourceKey) const {
  return m_ResourcesPath / ("command_" + DumpKeyToString(commandKey) + "_" + kind + "_O" +
                            DumpKeyToString(resourceKey));
}

} // namespace vulkan
} // namespace gits
