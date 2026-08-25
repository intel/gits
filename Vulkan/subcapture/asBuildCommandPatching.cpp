// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "asBuildCommandPatching.h"

#include "arguments.h"
#include "log.h"

#include <vector>

namespace gits {
namespace vulkan {

namespace {

// A micromap chained onto a triangle geometry is the only handle a build info's pNext can carry
size_t MicromapKeyCount(const void* pNext) {
  size_t count = 0;
  for (const auto* node = static_cast<const VkBaseInStructure*>(pNext); node; node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT ||
        node->sType ==
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_DISPLACEMENT_MICROMAP_NV) {
      ++count;
    }
  }
  return count;
}

// Key slots one info contributes after the leading pair block - mirrors CollectAsBuildPNextKeys
// (Vulkan/recorder/handleArgumentUpdatersCustom.cpp)
size_t AsBuildPNextKeyCount(const VkAccelerationStructureBuildGeometryInfoKHR& info) {
  size_t count = MicromapKeyCount(info.pNext);
  for (uint32_t i = 0; i < info.geometryCount; ++i) {
    const VkAccelerationStructureGeometryKHR* geometry = nullptr;
    if (info.pGeometries) {
      geometry = &info.pGeometries[i];
    } else if (info.ppGeometries) {
      geometry = info.ppGeometries[i];
    }
    if (geometry && geometry->geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR) {
      count += MicromapKeyCount(geometry->geometry.triangles.pNext);
    }
  }
  return count;
}

// True when the leading [src, dst] pairs plus each info's pNext key run account for exactly
// totalKeys, filling payloadCount with those runs. False means the keys are not in the layout
// the recorder writes, so one info's slice cannot be told from the next.
bool IsPayloadValid(const VkAccelerationStructureBuildGeometryInfoKHR* infos,
                    uint32_t infoCount,
                    size_t totalKeys,
                    std::vector<size_t>& payloadCount) {
  size_t expectedKeys = 2 * static_cast<size_t>(infoCount);
  if (totalKeys == expectedKeys) {
    return true; // no info chains a micromap, so every run is empty and none need walking
  }
  for (uint32_t i = 0; i < infoCount; ++i) {
    payloadCount[i] = AsBuildPNextKeyCount(infos[i]);
    expectedKeys += payloadCount[i];
  }
  return totalKeys == expectedKeys;
}

} // namespace

bool RemoveUnreferencedAsBuildInfos(vkCmdBuildAccelerationStructuresKHRCommand& cmd,
                                    const std::unordered_set<uint64_t>& keepDstAsKeys) {
  const uint32_t infoCount = cmd.m_infoCount.Value;
  if (infoCount == 0 || !cmd.m_pInfos.Value) {
    return true;
  }

  std::vector<GITSKey>& handleKeys = cmd.m_pInfos.HandleKeys;
  VkAccelerationStructureBuildGeometryInfoKHR* infos = cmd.m_pInfos.Value;

  // Verify the layout before inverting it - an unrecognized one is the caller's fatal
  std::vector<size_t> payloadCount(infoCount, 0);
  if (!IsPayloadValid(infos, infoCount, handleKeys.size(), payloadCount)) {
    return false;
  }
  // Decode sizes Data to infoCount. Anything else and the ranges cannot be kept in step with
  // the infos, so refuse rather than build a command whose two halves disagree.
  if (!cmd.m_ppBuildRangeInfos.Data.empty() && cmd.m_ppBuildRangeInfos.Data.size() != infoCount) {
    return false;
  }

  std::vector<bool> keep(infoCount, false);
  uint32_t keptCount = 0;
  for (uint32_t i = 0; i < infoCount; ++i) {
    if (keepDstAsKeys.count(handleKeys[AsBuildDstKeyIndex(i)])) {
      keep[i] = true;
      ++keptCount;
    }
  }
  if (keptCount == infoCount) {
    return true; // nothing to drop - leave the command byte for byte as recorded
  }

  // Pair block first, then the kept payload runs, so the result has the same layout the
  // recorder wrote and the player's two pass resolve still lines up.
  std::vector<GITSKey> newKeys;
  newKeys.reserve(handleKeys.size());
  for (uint32_t i = 0; i < infoCount; ++i) {
    if (keep[i]) {
      newKeys.push_back(handleKeys[AsBuildSrcKeyIndex(i)]);
      newKeys.push_back(handleKeys[AsBuildDstKeyIndex(i)]);
    }
  }
  size_t payloadOffset = 2 * static_cast<size_t>(infoCount);
  for (uint32_t i = 0; i < infoCount; ++i) {
    if (keep[i]) {
      newKeys.insert(newKeys.end(), handleKeys.begin() + payloadOffset,
                     handleKeys.begin() + payloadOffset + payloadCount[i]);
    }
    payloadOffset += payloadCount[i];
  }

  // Encode walks Size structs from Value and only then follows each one's geometries, so
  // closing the gaps in place is enough - a dropped info's geometry data is never visited
  uint32_t write = 0;
  for (uint32_t read = 0; read < infoCount; ++read) {
    if (!keep[read]) {
      continue;
    }
    if (write != read) {
      infos[write] = infos[read];
    }
    ++write;
  }

  auto& ranges = cmd.m_ppBuildRangeInfos;
  if (!ranges.Data.empty()) {
    std::vector<std::vector<VkAccelerationStructureBuildRangeInfoKHR>> newData;
    newData.reserve(keptCount);
    for (uint32_t i = 0; i < infoCount; ++i) {
      if (keep[i]) {
        newData.push_back(std::move(ranges.Data[i]));
      }
    }
    ranges.Data = std::move(newData);
    ranges.Pointers.resize(ranges.Data.size());
    for (size_t i = 0; i < ranges.Data.size(); ++i) {
      ranges.Pointers[i] = ranges.Data[i].data();
    }
    ranges.Size = static_cast<uint32_t>(ranges.Data.size());
    // Encode treats a null Value as "argument not provided", so it must keep pointing at the
    // (possibly reallocated) pointer table.
    ranges.Value = const_cast<VkAccelerationStructureBuildRangeInfoKHR**>(ranges.Pointers.data());
  }

  cmd.m_pInfos.Size = keptCount;
  cmd.m_infoCount.Value = keptCount;
  handleKeys = std::move(newKeys);
  LOG_TRACE << "Vulkan subcapture: acceleration structure build command key=" << cmd.m_Key
            << " replays " << keptCount << " of " << infoCount
            << " recorded infos - the rest write structures this restore does not need";
  return true;
}

} // namespace vulkan
} // namespace gits
