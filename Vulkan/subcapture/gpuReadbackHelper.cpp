// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuReadbackHelper.h"
#include "playerManager.h"
#include "handleMapService.h"
#include "log.h"

#include <cstring>

namespace gits {
namespace vulkan {

GpuReadbackHelper::GpuReadbackHelper(PlayerManager& playerManager) : m_Player(playerManager) {}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::IsHostVisible(uint64_t physDevKey, uint32_t memoryTypeIndex) {
  if (memoryTypeIndex == UINT32_MAX) {
    return false;
  }
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return false;
  }
  VkPhysicalDeviceMemoryProperties props{};
  m_Player.GetInstanceDispatchTable(physDevice)
      .vkGetPhysicalDeviceMemoryProperties(physDevice, &props);
  if (memoryTypeIndex >= props.memoryTypeCount) {
    return false;
  }
  return (props.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) !=
         0;
}

uint32_t GpuReadbackHelper::FindStagingMemoryTypeForPhysDevice(VkPhysicalDevice physDevice,
                                                               uint32_t memoryTypeBits) {
  VkPhysicalDeviceMemoryProperties props{};
  m_Player.GetInstanceDispatchTable(physDevice)
      .vkGetPhysicalDeviceMemoryProperties(physDevice, &props);

  constexpr VkMemoryPropertyFlags kRequired =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((memoryTypeBits & (1u << i)) &&
        (props.memoryTypes[i].propertyFlags & kRequired) == kRequired) {
      return i;
    }
  }
  // Fall back: HOST_VISIBLE without HOST_COHERENT (will need manual flush)
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((memoryTypeBits & (1u << i)) &&
        (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      return i;
    }
  }
  return UINT32_MAX;
}

uint32_t GpuReadbackHelper::FindStagingMemoryType(uint64_t physDevKey, uint32_t memoryTypeBits) {
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return UINT32_MAX;
  }
  return FindStagingMemoryTypeForPhysDevice(physDevice, memoryTypeBits);
}

// ---------------------------------------------------------------------------
// QueryStagingBufferRequirements
//
// Create a throwaway VkBuffer with the requested size+usage, query its memory
// requirements via vkGetBufferMemoryRequirements, then immediately destroy the
// buffer.  The reported req.size (alignment-rounded) and req.memoryTypeBits
// (driver-allowed types) feed into the staging-buffer commands the recorder
// emits into the stream, so the second player's vkAllocateMemory +
// vkBindBufferMemory satisfy the actual driver requirements rather than
// guessing from the raw data length.
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::QueryStagingBufferRequirements(uint64_t deviceKey,
                                                       VkDeviceSize size,
                                                       VkBufferUsageFlags usage,
                                                       VkMemoryRequirements& outReq) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: QueryStagingBufferRequirements: invalid device key="
                << deviceKey;
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer tempBuf = VK_NULL_HANDLE;
  if (dt.vkCreateBuffer(device, &bci, nullptr, &tempBuf) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: QueryStagingBufferRequirements: vkCreateBuffer failed";
    return false;
  }
  outReq = {};
  dt.vkGetBufferMemoryRequirements(device, tempBuf, &outReq);
  dt.vkDestroyBuffer(device, tempBuf, nullptr);
  return true;
}

// ---------------------------------------------------------------------------
// AllocateStagingBuffer
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::AllocateStagingBuffer(VkDevice device,
                                              VkPhysicalDevice physDevKey,
                                              VkDeviceSize size,
                                              VkBuffer& outBuf,
                                              VkDeviceMemory& outMem,
                                              void*& outMapped) {
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size;
  bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (dt.vkCreateBuffer(device, &bci, nullptr, &outBuf) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkCreateBuffer for staging failed";
    return false;
  }

  VkMemoryRequirements req{};
  dt.vkGetBufferMemoryRequirements(device, outBuf, &req);

  uint32_t memType = FindStagingMemoryTypeForPhysDevice(physDevKey, req.memoryTypeBits);
  if (memType == UINT32_MAX) {
    LOG_WARNING << "GpuReadbackHelper: no HOST_VISIBLE memory type for staging buffer";
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    return false;
  }

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = memType;

  if (dt.vkAllocateMemory(device, &mai, nullptr, &outMem) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkAllocateMemory for staging failed";
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    return false;
  }

  if (dt.vkBindBufferMemory(device, outBuf, outMem, 0) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkBindBufferMemory for staging failed";
    dt.vkFreeMemory(device, outMem, nullptr);
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    outMem = VK_NULL_HANDLE;
    return false;
  }

  if (dt.vkMapMemory(device, outMem, 0, VK_WHOLE_SIZE, 0, &outMapped) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkMapMemory for staging failed";
    dt.vkFreeMemory(device, outMem, nullptr);
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    outMem = VK_NULL_HANDLE;
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// SubmitOneShot
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::SubmitOneShot(VkDevice device,
                                      VkQueue queue,
                                      VkCommandPool pool,
                                      std::function<void(VkCommandBuffer)> recordFn) {
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = pool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;

  VkCommandBuffer cb = VK_NULL_HANDLE;
  if (dt.vkAllocateCommandBuffers(device, &ai, &cb) != VK_SUCCESS) {
    return false;
  }

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  dt.vkBeginCommandBuffer(cb, &bi);

  recordFn(cb);

  dt.vkEndCommandBuffer(cb);

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cb;
  VkResult result = dt.vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
  dt.vkQueueWaitIdle(queue);
  dt.vkFreeCommandBuffers(device, pool, 1, &cb);

  return result == VK_SUCCESS;
}

// ---------------------------------------------------------------------------
// ReadBuffer
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::ReadBuffer(uint64_t deviceKey,
                                   uint64_t physDevKey,
                                   uint64_t queueKey,
                                   uint64_t commandPoolKey,
                                   uint64_t bufferKey,
                                   VkDeviceSize size,
                                   std::vector<uint8_t>& outData) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));
  auto buffer = reinterpret_cast<VkBuffer>(hms.TryGetHandle(bufferKey));

  if (!device || !physDevice || !queue || !pool || !buffer || size == 0) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;

  if (!AllocateStagingBuffer(device, physDevice, size, stagingBuf, stagingMem, mappedPtr)) {
    return false;
  }

  bool ok = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    // Barrier: ensure all prior writes to 'buffer' are visible for transfer read.
    VkBufferMemoryBarrier srcBarrier{};
    srcBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    srcBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.buffer = buffer;
    srcBarrier.offset = 0;
    srcBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 1, &srcBarrier, 0, nullptr);

    VkBufferCopy region{0, 0, size};
    dt.vkCmdCopyBuffer(cb, buffer, stagingBuf, 1, &region);

    // Barrier: staging ? host-read.
    VkBufferMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.buffer = stagingBuf;
    dstBarrier.offset = 0;
    dstBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                            nullptr, 1, &dstBarrier, 0, nullptr);
  });

  if (ok) {
    // Invalidate non-coherent memory before reading.
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingMem;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    dt.vkInvalidateMappedMemoryRanges(device, 1, &range);

    outData.resize(static_cast<size_t>(size));
    std::memcpy(outData.data(), mappedPtr, static_cast<size_t>(size));
  }

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  return ok;
}

// ---------------------------------------------------------------------------
// ReadImage
// ---------------------------------------------------------------------------

namespace {

struct FormatBlockInfo {
  uint32_t BlockWidth{1};
  uint32_t BlockHeight{1};
  uint32_t BytesPerBlock{4};
  bool IsDepthStencil{false};
  uint32_t DepthBytes{0};   // 0 = no depth
  uint32_t StencilBytes{0}; // 0 = no stencil
  // Multi-planar YCbCr formats: copy requires one region per plane with
  // VK_IMAGE_ASPECT_PLANE_*_BIT and per-plane extent/stride.
  bool IsMultiPlanar{false};
  uint8_t PlaneCount{0};
  struct PlaneDesc {
    uint32_t WidthDivisor{1};  // plane width  = image width  / WidthDivisor
    uint32_t HeightDivisor{1}; // plane height = image height / HeightDivisor
    uint32_t BytesPerPixel{1};
  } Planes[3]{};
};

static FormatBlockInfo GetFormatBlockInfo(VkFormat fmt) {
  switch (fmt) {
  // ---- Single-channel ----
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8_SRGB:
    return {1, 1, 1};
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16_SFLOAT:
    return {1, 1, 2};
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32_SFLOAT:
    return {1, 1, 4};
  // ---- Two-channel ----
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8_SRGB:
    return {1, 1, 2};
  case VK_FORMAT_R16G16_SFLOAT:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16_SINT:
    return {1, 1, 4};
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32_SINT:
    return {1, 1, 8};
  // ---- Four-channel (most common) ----
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8G8B8A8_SRGB:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    return {1, 1, 4};
  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return {1, 1, 8};
  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return {1, 1, 16};
  // ---- BC compressed ----
  case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
  case VK_FORMAT_BC4_UNORM_BLOCK:
  case VK_FORMAT_BC4_SNORM_BLOCK:
    return {4, 4, 8};
  case VK_FORMAT_BC2_UNORM_BLOCK:
  case VK_FORMAT_BC2_SRGB_BLOCK:
  case VK_FORMAT_BC3_UNORM_BLOCK:
  case VK_FORMAT_BC3_SRGB_BLOCK:
  case VK_FORMAT_BC5_UNORM_BLOCK:
  case VK_FORMAT_BC5_SNORM_BLOCK:
  case VK_FORMAT_BC6H_UFLOAT_BLOCK:
  case VK_FORMAT_BC6H_SFLOAT_BLOCK:
  case VK_FORMAT_BC7_UNORM_BLOCK:
  case VK_FORMAT_BC7_SRGB_BLOCK:
    return {4, 4, 16};
  // ---- YCbCr packed single-plane ----
  // 4 bytes cover a 2-wide texel pair (macropixel).
  case VK_FORMAT_G8B8G8R8_422_UNORM:
  case VK_FORMAT_B8G8R8G8_422_UNORM:
    return {2, 1, 4};
  // ---- YCbCr multi-planar ----
  // 2-plane 4:2:0 (NV12 / P010 / P012 / P016):
  //   plane 0 = luma  (Y),    full resolution, bytesPerPixel=1 (8b) or 2 (10/12/16b)
  //   plane 1 = chroma (CbCr), half width+height, bytesPerPixel=2 or 4
  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 4};
    return fi;
  }
  // 3-plane 4:2:0 (I420 / YV12 variants):
  //   plane 0 = Y (full), plane 1 = Cb (quarter area), plane 2 = Cr (quarter area)
  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 1};
    fi.Planes[2] = {2, 2, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 2};
    fi.Planes[2] = {2, 2, 2};
    return fi;
  }
  // 2-plane 4:2:2: chroma is half-width but full-height
  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 4};
    return fi;
  }
  // 3-plane 4:2:2: chroma is half-width, full-height, per-channel
  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 1};
    fi.Planes[2] = {2, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 2};
    fi.Planes[2] = {2, 1, 2};
    return fi;
  }
  // 3-plane 4:4:4: no chroma subsampling, all planes full resolution
  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {1, 1, 1};
    fi.Planes[2] = {1, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {1, 1, 2};
    fi.Planes[2] = {1, 1, 2};
    return fi;
  }
  // ---- Depth / stencil ----
  case VK_FORMAT_D16_UNORM:
    return {1, 1, 0, true, 2, 0};
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D32_SFLOAT:
    return {1, 1, 0, true, 4, 0};
  case VK_FORMAT_S8_UINT:
    return {1, 1, 0, true, 0, 1};
  case VK_FORMAT_D16_UNORM_S8_UINT:
    return {1, 1, 0, true, 2, 1};
  case VK_FORMAT_D24_UNORM_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  default:
    LOG_WARNING << "GpuReadbackHelper: unknown format " << static_cast<uint32_t>(fmt)
                << " for image readback, assuming 4 bytes/pixel";
    return {1, 1, 4};
  }
}

// Returns total staging bytes needed and populates outRegions (one per subresource).
static VkDeviceSize ComputeImageStagingLayout(VkFormat format,
                                              const VkExtent3D& extent,
                                              uint32_t mipLevels,
                                              uint32_t arrayLayers,
                                              std::vector<VkBufferImageCopy>& outRegions) {
  const auto fi = GetFormatBlockInfo(format);
  VkDeviceSize offset = 0;

  auto addRegion = [&](VkImageAspectFlags aspect, uint32_t layer, uint32_t mip, uint32_t w,
                       uint32_t h, uint32_t d, uint32_t bw, uint32_t bh, uint32_t bytesPerBlock) {
    // Buffer offsets must be aligned to at least 4 bytes (spec requirement).
    const VkDeviceSize align = std::max(4u, bytesPerBlock);
    offset = (offset + align - 1) & ~(align - 1);

    VkBufferImageCopy region{};
    region.bufferOffset = offset;
    region.bufferRowLength = 0; // tight packing
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = aspect;
    region.imageSubresource.mipLevel = mip;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {w, h, d};
    outRegions.push_back(region);

    uint32_t blocksX = (w + bw - 1) / bw;
    uint32_t blocksY = (h + bh - 1) / bh;
    offset += static_cast<VkDeviceSize>(blocksX) * blocksY * d * bytesPerBlock;
  };

  for (uint32_t layer = 0; layer < arrayLayers; ++layer) {
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
      uint32_t w = std::max(1u, extent.width >> mip);
      uint32_t h = std::max(1u, extent.height >> mip);
      uint32_t d = std::max(1u, extent.depth >> mip);

      if (fi.IsMultiPlanar) {
        static constexpr VkImageAspectFlags kPlaneAspect[3] = {
            VK_IMAGE_ASPECT_PLANE_0_BIT,
            VK_IMAGE_ASPECT_PLANE_1_BIT,
            VK_IMAGE_ASPECT_PLANE_2_BIT,
        };
        for (uint8_t p = 0; p < fi.PlaneCount; ++p) {
          uint32_t pw = std::max(1u, w / fi.Planes[p].WidthDivisor);
          uint32_t ph = std::max(1u, h / fi.Planes[p].HeightDivisor);
          addRegion(kPlaneAspect[p], layer, mip, pw, ph, d, 1, 1, fi.Planes[p].BytesPerPixel);
        }
      } else if (!fi.IsDepthStencil) {
        addRegion(VK_IMAGE_ASPECT_COLOR_BIT, layer, mip, w, h, d, fi.BlockWidth, fi.BlockHeight,
                  fi.BytesPerBlock);
      } else {
        if (fi.DepthBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_DEPTH_BIT, layer, mip, w, h, d, 1, 1, fi.DepthBytes);
        }
        if (fi.StencilBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_STENCIL_BIT, layer, mip, w, h, d, 1, 1, fi.StencilBytes);
        }
      }
    }
  }
  return offset;
}

} // anonymous namespace

bool GpuReadbackHelper::ReadImage(uint64_t deviceKey,
                                  uint64_t physDevKey,
                                  uint64_t queueKey,
                                  uint64_t commandPoolKey,
                                  uint64_t imageKey,
                                  VkFormat format,
                                  const VkExtent3D& extent,
                                  uint32_t mipLevels,
                                  uint32_t arrayLayers,
                                  VkSampleCountFlagBits samples,
                                  VkImageLayout currentLayout,
                                  std::vector<uint8_t>& outData,
                                  std::vector<VkBufferImageCopy>& outRegions) {
  // Multisampled images cannot be copied with vkCmdCopyImageToBuffer.
  if (samples != VK_SAMPLE_COUNT_1_BIT) {
    return false;
  }
  // Zero-size images have nothing to copy.
  if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
    return false;
  }

  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));
  auto image = reinterpret_cast<VkImage>(hms.TryGetHandle(imageKey));

  if (!device || !physDevice || !queue || !pool || !image) {
    return false;
  }

  // Compute staging buffer layout.
  VkDeviceSize stagingSize =
      ComputeImageStagingLayout(format, extent, mipLevels, arrayLayers, outRegions);
  if (stagingSize == 0 || outRegions.empty()) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;

  if (!AllocateStagingBuffer(device, physDevice, stagingSize, stagingBuf, stagingMem, mappedPtr)) {
    outRegions.clear();
    return false;
  }

  // Determine the aspect mask for layout transitions.
  const auto fi = GetFormatBlockInfo(format);
  VkImageAspectFlags transitionAspect = VK_IMAGE_ASPECT_COLOR_BIT;
  if (fi.IsDepthStencil) {
    transitionAspect = 0;
    if (fi.DepthBytes > 0) {
      transitionAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (fi.StencilBytes > 0) {
      transitionAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else if (fi.IsMultiPlanar) {
    // Multi-planar images in video-decode layouts (VIDEO_DECODE_DST_KHR,
    // VIDEO_DECODE_DPB_KHR) require per-plane aspect bits in barriers;
    // VK_IMAGE_ASPECT_COLOR_BIT is only valid for GENERAL / UNDEFINED /
    // TRANSFER_* layouts (VUID-VkImageMemoryBarrier-image-01672).
    static constexpr VkImageAspectFlags kPlaneAspect[3] = {
        VK_IMAGE_ASPECT_PLANE_0_BIT,
        VK_IMAGE_ASPECT_PLANE_1_BIT,
        VK_IMAGE_ASPECT_PLANE_2_BIT,
    };
    transitionAspect = 0;
    for (uint8_t p = 0; p < fi.PlaneCount; ++p) {
      transitionAspect |= kPlaneAspect[p];
    }
  }

  bool ok = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    // Transition image: currentLayout ? TRANSFER_SRC_OPTIMAL.
    VkImageMemoryBarrier toSrc{};
    toSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toSrc.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    toSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toSrc.oldLayout = currentLayout;
    toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.image = image;
    toSrc.subresourceRange = {transitionAspect, 0, VK_REMAINING_MIP_LEVELS, 0,
                              VK_REMAINING_ARRAY_LAYERS};
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &toSrc);

    // Copy all subresources to staging buffer.
    dt.vkCmdCopyImageToBuffer(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuf,
                              static_cast<uint32_t>(outRegions.size()), outRegions.data());

    // Transition image back: TRANSFER_SRC_OPTIMAL ? currentLayout.
    VkImageMemoryBarrier restore{};
    restore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    restore.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    restore.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    restore.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    restore.newLayout = currentLayout;
    restore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    restore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    restore.image = image;
    restore.subresourceRange = {transitionAspect, 0, VK_REMAINING_MIP_LEVELS, 0,
                                VK_REMAINING_ARRAY_LAYERS};
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &restore);

    // Staging buffer barrier: TRANSFER_WRITE ? HOST_READ.
    VkBufferMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.buffer = stagingBuf;
    dstBarrier.offset = 0;
    dstBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                            nullptr, 1, &dstBarrier, 0, nullptr);
  });

  if (ok) {
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingMem;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    dt.vkInvalidateMappedMemoryRanges(device, 1, &range);

    outData.resize(static_cast<size_t>(stagingSize));
    std::memcpy(outData.data(), mappedPtr, static_cast<size_t>(stagingSize));
  } else {
    outRegions.clear();
  }

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  return ok;
}

} // namespace vulkan
} // namespace gits
