// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "buildInputCapture.h"
#include "asBuildCommandPatching.h"
#include "stateTrackingService.h"
#include "log.h"

#include <algorithm>

namespace gits {
namespace vulkan {

void ResolveAndTrackBufferAddress(StateTrackingService& stateTracking,
                                  VkDeviceAddress address,
                                  std::vector<uint64_t>& depKeys) {
  if (address == 0) {
    return;
  }
  auto found = stateTracking.GetDeviceAddressTracking().FindContaining(address);
  if (found) {
    depKeys.push_back(found->first);
  }
}

void CollectGeometryInputBufferKeys(StateTrackingService& stateTracking,
                                    const VkAccelerationStructureGeometryKHR& geometry,
                                    std::vector<uint64_t>& depKeys) {
  switch (geometry.geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
    const VkAccelerationStructureGeometryTrianglesDataKHR& tri = geometry.geometry.triangles;
    ResolveAndTrackBufferAddress(stateTracking, tri.vertexData.deviceAddress, depKeys);
    if (tri.indexType != VK_INDEX_TYPE_NONE_KHR) {
      ResolveAndTrackBufferAddress(stateTracking, tri.indexData.deviceAddress, depKeys);
    }
    ResolveAndTrackBufferAddress(stateTracking, tri.transformData.deviceAddress, depKeys);
    // Opacity micromap index buffer. Must stay in step with ComputeGeometryInputRegions.
    if (const auto* omm = FindOpacityMicromapGeometry(tri.pNext)) {
      if (omm->indexType != VK_INDEX_TYPE_NONE_KHR) {
        ResolveAndTrackBufferAddress(stateTracking, omm->indexBuffer.deviceAddress, depKeys);
      }
    }
    break;
  }
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    ResolveAndTrackBufferAddress(stateTracking, geometry.geometry.aabbs.data.deviceAddress,
                                 depKeys);
    break;
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    if (!geometry.geometry.instances.arrayOfPointers) {
      ResolveAndTrackBufferAddress(stateTracking, geometry.geometry.instances.data.deviceAddress,
                                   depKeys);
    }
    break;
  default:
    break;
  }
}

void ComputeGeometryInputRegions(StateTrackingService& stateTracking,
                                 const VkAccelerationStructureGeometryKHR& geometry,
                                 const VkAccelerationStructureBuildRangeInfoKHR& range,
                                 std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer) {
  auto addRegion = [&](VkDeviceAddress address, VkDeviceSize extraOffset, VkDeviceSize size) {
    if (address == 0 || size == 0) {
      return;
    }
    auto found = stateTracking.GetDeviceAddressTracking().FindContaining(address);
    if (!found) {
      return;
    }
    const VkDeviceSize start = found->second + extraOffset;
    regionsByBuffer[found->first].push_back({start, start + size});
  };

  switch (geometry.geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
    const VkAccelerationStructureGeometryTrianglesDataKHR& tri = geometry.geometry.triangles;
    if (tri.transformData.deviceAddress != 0) {
      addRegion(tri.transformData.deviceAddress, range.transformOffset,
                sizeof(VkTransformMatrixKHR));
    }
    if (tri.indexType != VK_INDEX_TYPE_NONE_KHR) {
      const VkDeviceSize elem = (tri.indexType == VK_INDEX_TYPE_UINT16) ? 2 : 4;
      addRegion(tri.indexData.deviceAddress, range.primitiveOffset,
                static_cast<VkDeviceSize>(range.primitiveCount) * 3 * elem);
      // Indexed: vertices are addressed at vertexData + stride*(firstVertex + index), where
      // maxVertex is already the highest *effective* index (VUID-...-10774), so firstVertex
      // must not be added on top of it. Span: [firstVertex, maxVertex + 1) strides.
      addRegion(tri.vertexData.deviceAddress,
                static_cast<VkDeviceSize>(range.firstVertex) * tri.vertexStride,
                (static_cast<VkDeviceSize>(tri.maxVertex) + 1 - range.firstVertex) *
                    tri.vertexStride);
    } else {
      // Non-indexed: vertices are addressed at
      // vertexData + primitiveOffset + stride*(firstVertex + i), i in [0, primitiveCount*3).
      addRegion(tri.vertexData.deviceAddress,
                range.primitiveOffset +
                    static_cast<VkDeviceSize>(range.firstVertex) * tri.vertexStride,
                static_cast<VkDeviceSize>(range.primitiveCount) * 3 * tri.vertexStride);
    }
    if (const auto* omm = FindOpacityMicromapGeometry(tri.pNext)) {
      if (omm->indexType != VK_INDEX_TYPE_NONE_KHR && omm->indexBuffer.deviceAddress != 0) {
        // VUID-vkCmdBuildAccelerationStructuresKHR-indexBuffer-11577 bounds the range at
        // indexBuffer + indexStride * primitiveCount, with no primitiveOffset unlike
        // tri.indexData. Spanning through the final index keeps a legal zero indexStride from
        // collapsing to an empty region. indexType here is only UINT16, UINT32 or NONE
        // (VUID-VkAccelerationStructureTrianglesOpacityMicromapEXT-indexType-10719).
        // baseTriangle is added to the fetched index to pick a triangle inside the micromap - it
        // is not an offset into indexBuffer.
        const VkDeviceSize indexSize =
            omm->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t);
        const VkDeviceSize indexSpan =
            range.primitiveCount == 0
                ? 0
                : (static_cast<VkDeviceSize>(range.primitiveCount) - 1) * omm->indexStride +
                      indexSize;
        addRegion(omm->indexBuffer.deviceAddress, /*extraOffset=*/0, indexSpan);
      }
    }
    break;
  }
  case VK_GEOMETRY_TYPE_AABBS_KHR: {
    const VkAccelerationStructureGeometryAabbsDataKHR& aabbs = geometry.geometry.aabbs;
    addRegion(aabbs.data.deviceAddress, range.primitiveOffset,
              static_cast<VkDeviceSize>(range.primitiveCount) * aabbs.stride);
    break;
  }
  case VK_GEOMETRY_TYPE_INSTANCES_KHR: {
    const VkAccelerationStructureGeometryInstancesDataKHR& inst = geometry.geometry.instances;
    if (!inst.arrayOfPointers) {
      addRegion(inst.data.deviceAddress, range.primitiveOffset,
                static_cast<VkDeviceSize>(range.primitiveCount) *
                    sizeof(VkAccelerationStructureInstanceKHR));
    }
    // arrayOfPointers instance data is not captured (parity with the analyzer's
    // TLAS->BLAS discovery, which also skips it).
    break;
  }
  default:
    break;
  }
}

std::vector<CapturedBuildInputRegion> MergeInputRegions(std::vector<RawInputRegion> regions,
                                                        VkDeviceSize bufferSize) {
  std::vector<CapturedBuildInputRegion> merged;
  if (regions.empty()) {
    return merged;
  }
  std::sort(regions.begin(), regions.end(),
            [](const RawInputRegion& a, const RawInputRegion& b) { return a.Start < b.Start; });
  VkDeviceSize curStart = regions[0].Start;
  VkDeviceSize curEnd = regions[0].End;
  // A region may run one trailing vertex-stride past a tightly-packed buffer. Clamping is
  // safe, since the GPU cannot read past the buffer end either.
  auto flush = [&]() {
    VkDeviceSize s = std::min(curStart, bufferSize);
    VkDeviceSize e = std::min(curEnd, bufferSize);
    if (e > s) {
      merged.push_back({s, e - s, 0});
    }
  };
  for (size_t i = 1; i < regions.size(); ++i) {
    if (regions[i].Start <= curEnd) {
      curEnd = std::max(curEnd, regions[i].End);
    } else {
      flush();
      curStart = regions[i].Start;
      curEnd = regions[i].End;
    }
  }
  flush();
  return merged;
}

void StageBuildInputReadbacks(StateTrackingService& stateTracking,
                              IGpuReadbackHelper& readbackHelper,
                              uint64_t deviceKey,
                              uint64_t physDevKey,
                              uint64_t cbKey,
                              std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer,
                              const char* what,
                              PendingAsInputReadback& pending) {
  for (auto& [bufKey, raw] : regionsByBuffer) {
    // A destroyed buffer's handle is dead, so a stale device-address hit means "no such buffer".
    auto* buf = stateTracking.GetState<BufferState>(bufKey);
    if (!buf || buf->Destroyed || buf->BufferSize == 0 || buf->BoundMemoryKey == 0) {
      continue;
    }
    auto* mem = stateTracking.GetState<DeviceMemoryState>(buf->BoundMemoryKey);
    if (!mem || mem->Destroyed) {
      continue;
    }
    CapturedBuildInputBuffer cbuf;
    cbuf.BufferKey = bufKey;
    cbuf.Size = buf->BufferSize;
    cbuf.BufferOpaqueCaptureAddress = buf->OpaqueCaptureAddress;
    cbuf.MemoryOpaqueCaptureAddress = mem->OpaqueCaptureAddress;
    cbuf.MemoryTypeIndex = mem->MemoryTypeIndex;
    cbuf.MemoryOffset = buf->MemoryOffset;
    cbuf.BaseDeviceAddress = buf->DeviceAddress;
    cbuf.Regions = MergeInputRegions(std::move(raw), buf->BufferSize);
    if (cbuf.Regions.empty()) {
      continue;
    }
    StagedInputReadback staging;
    if (!readbackHelper.StageBufferRegions(deviceKey, physDevKey, cbKey, bufKey, cbuf.Regions,
                                           staging)) {
      LOG_WARNING << "Vulkan subcapture: failed to stage " << what
                  << " build input copy (buffer key=" << bufKey
                  << ") - rebuild may be incomplete if this buffer is freed";
      continue;
    }
    pending.Buffers.push_back(std::move(cbuf));
    pending.Staging.push_back(staging);
  }
}

} // namespace vulkan
} // namespace gits
