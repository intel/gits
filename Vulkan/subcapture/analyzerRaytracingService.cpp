// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerRaytracingService.h"

#include "commandsAuto.h"
#include "log.h"

#include <algorithm>

namespace gits {
namespace vulkan {

namespace {
const std::vector<uint64_t> kEmptyBlasKeys;
} // namespace

void AnalyzerRaytracingService::StageTlasInstanceReadbacks(
    const vkCmdBuildAccelerationStructuresKHRCommand& command) {
  const uint32_t infoCount = command.m_infoCount.Value;
  if (infoCount == 0 || !command.m_pInfos.Value ||
      command.m_pInfos.HandleKeys.size() < 2 * static_cast<size_t>(infoCount)) {
    return;
  }

  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkAccelerationStructureBuildGeometryInfoKHR& info = command.m_pInfos.Value[i];
    if (info.type != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR || info.geometryCount == 0) {
      continue;
    }
    // TODO: 2 handles per info will not hold after we add pNext OMM handle
    const uint64_t dstKey = command.m_pInfos.HandleKeys[2 * i + 1];
    if (!dstKey) {
      continue;
    }

    const VkAccelerationStructureGeometryKHR* geometry = nullptr;
    if (info.pGeometries) {
      geometry = &info.pGeometries[0];
    } else if (info.ppGeometries && info.ppGeometries[0]) {
      geometry = info.ppGeometries[0];
    }
    if (!geometry || geometry->geometryType != VK_GEOMETRY_TYPE_INSTANCES_KHR) {
      continue;
    }

    const VkAccelerationStructureGeometryInstancesDataKHR& instancesData =
        geometry->geometry.instances;
    const bool isArrayOfPointers = instancesData.arrayOfPointers != VK_FALSE;
    const VkDeviceAddress instanceArrayBase = instancesData.data.deviceAddress;
    if (instanceArrayBase == 0) {
      continue;
    }

    uint32_t primitiveCount = 0;
    uint32_t primitiveOffset = 0;
    if (i < command.m_ppBuildRangeInfos.Data.size() &&
        !command.m_ppBuildRangeInfos.Data[i].empty()) {
      const VkAccelerationStructureBuildRangeInfoKHR& range =
          command.m_ppBuildRangeInfos.Data[i][0];
      primitiveCount = range.primitiveCount;
      primitiveOffset = range.primitiveOffset;
    }
    if (primitiveCount == 0) {
      continue;
    }

    m_PendingByCb[command.m_commandBuffer.Key].push_back(
        {dstKey, instanceArrayBase + primitiveOffset, primitiveCount, isArrayOfPointers});
  }
}

void AnalyzerRaytracingService::ReadStagedTlasInstances(uint64_t cbKey, uint64_t submitQueueKey) {
  auto it = m_PendingByCb.find(cbKey);
  if (it == m_PendingByCb.end()) {
    return;
  }
  // Read on the actual submit queue so the copy is GPU-ordered after the staged builds.
  uint64_t deviceKey = 0, physDevKey = 0;
  auto* cbState = m_StateTracking.GetState<CommandBufferState>(cbKey);
  auto* poolState =
      cbState ? m_StateTracking.GetState<CommandPoolState>(cbState->PoolKey) : nullptr;
  if (poolState) {
    deviceKey = poolState->ParentKey;
    if (auto* deviceState = m_StateTracking.GetState<DeviceState>(deviceKey)) {
      physDevKey = deviceState->ParentKey;
    }
  }
  if (deviceKey == 0 || physDevKey == 0 || submitQueueKey == 0) {
    LOG_WARNING << "Vulkan subcapture: cannot resolve device/queue for TLAS instance readback "
                   "(command buffer key="
                << cbKey << "); skipping TLAS->BLAS discovery";
    m_PendingByCb.erase(it);
    return;
  }
  const uint64_t poolKey = cbState->PoolKey;

  for (const PendingTlasReadback& pending : it->second) {
    std::vector<uint64_t> blasKeys = m_InstancesDump.ReadReferencedBlases(
        deviceKey, physDevKey, submitQueueKey, poolKey, pending.InstanceArrayAddress,
        pending.InstanceCount, pending.IsArrayOfPointers);
    if (blasKeys.empty()) {
      continue;
    }
    auto& dest = m_TlasToBlases[pending.TlasKey];
    for (uint64_t blasKey : blasKeys) {
      if (std::find(dest.begin(), dest.end(), blasKey) == dest.end()) {
        dest.push_back(blasKey);
      }
    }
  }
  // Can be erased because resubmit would re-stage nothing new
  m_PendingByCb.erase(it);
}

void AnalyzerRaytracingService::MergeSecondary(uint64_t primaryCbKey, uint64_t secondaryCbKey) {
  auto it = m_PendingByCb.find(secondaryCbKey);
  if (it == m_PendingByCb.end() || it->second.empty()) {
    return;
  }
  auto& dst = m_PendingByCb[primaryCbKey];
  dst.insert(dst.end(), it->second.begin(), it->second.end());
}

const std::vector<uint64_t>& AnalyzerRaytracingService::GetReferencedBlases(
    uint64_t tlasKey) const {
  auto it = m_TlasToBlases.find(tlasKey);
  if (it == m_TlasToBlases.end()) {
    return kEmptyBlasKeys;
  }
  return it->second;
}

uint64_t AnalyzerRaytracingService::ResolveBufferKeyForAddress(VkDeviceAddress address) const {
  if (address == 0) {
    return 0;
  }
  auto found = m_StateTracking.GetDeviceAddressTracking().FindContaining(address);
  return found ? found->first : 0;
}

} // namespace vulkan
} // namespace gits
