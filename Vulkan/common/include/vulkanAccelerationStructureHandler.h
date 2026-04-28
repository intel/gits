// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateTracking.h
*
* @brief Functions for tracking current state of Vulkan resources
*
*/

#include "vulkanTools.h"

#pragma once

namespace gits {

namespace Vulkan {

namespace {

inline bool isRecorder() {
  static bool isRecorder = Configurator::IsRecorder();
  return isRecorder;
}

inline bool isSubcaptureBeforeRestorationPhase() {
  static bool isSubcapture = CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture();
  if (!isSubcapture) {
    return false;
  }
  return !SD().stateRestoreFinished;
}

inline bool updateOnlyUsedMemory() {
  static bool updateOnlyUsedMemory =
      TMemoryUpdateStates::ONLY_USED == Configurator::Get().vulkan.recorder.memoryUpdateState;
  return updateOnlyUsedMemory;
}

inline bool captureRenderPasses() {
  static bool captureRenderPasses =
      !Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanDraws.empty() ||
      !Configurator::Get().vulkan.recorder.dumpSubmits.empty();
  return captureRenderPasses;
}

inline bool captureRenderPassesResources() {
  static bool captureRenderPassesResources =
      !Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanResources.empty();
  return captureRenderPassesResources;
}

inline bool crossPlatformStateRestoration() {
  static bool crossPlatformStateRestoration =
      Configurator::Get().vulkan.recorder.crossPlatformStateRestoration.images;
  return crossPlatformStateRestoration;
}
#ifdef GITS_PLATFORM_WINDOWS
inline bool usePresentSrcLayoutTransitionAsAFrameBoundary() {
  static bool usePresentSrcLayoutTransitionAsAFrameBoundary =
      Configurator::Get().vulkan.recorder.usePresentSrcLayoutTransitionAsAFrameBoundary;
  return usePresentSrcLayoutTransitionAsAFrameBoundary;
}
#endif

inline bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures() {
  static bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures =
      Configurator::Get()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
  return useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
}

inline bool isUseExternalMemoryExtensionUsed() {
#ifdef GITS_PLATFORM_WINDOWS
  return Configurator::Get().vulkan.recorder.useExternalMemoryExtension;
#else
  return false;
#endif
}

inline bool isGitsRecorderAttached() {
  if (drvVk.GetGlobalDispatchTable().vkIAmRecorderGITS) {
    return true;
  }
  return false;
}

template <class STATE_CONTAINER, class KEY, class DST_CONTAINER>
inline void insertStateIfFound(STATE_CONTAINER& state, KEY key, DST_CONTAINER& dst) {
  const auto it = state.find(key);
  if (it != state.end()) {
    dst.insert(it->second);
  }
}

template <>
inline void insertStateIfFound(gits::Vulkan::CStateDynamic::TAccelerationStructureKHRStates& state,
                               VkAccelerationStructureKHR key,
                               std::unordered_set<std::shared_ptr<CBufferState>>& dst) {
  const auto it = state.find(key);
  if (it != state.end()) {
    dst.insert(it->second->bufferStateStore);
  }
}

template <>
inline void insertStateIfFound(gits::Vulkan::CStateDynamic::TMicromapEXTStates& state,
                               VkMicromapEXT key,
                               std::unordered_set<std::shared_ptr<CBufferState>>& dst) {
  const auto it = state.find(key);
  if (it != state.end()) {
    dst.insert(it->second->bufferStateStore);
  }
}

} // namespace

inline void handleAccelerationStructureBuild(
    VkCommandBuffer cmdBuf,
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
  auto& cmdBufState = SD()._commandbufferstates[cmdBuf];
  auto device = cmdBufState->commandPoolStateStore->deviceStateStore->deviceHandle;

  for (uint32_t bgi = 0; bgi < infoCount; ++bgi) {
    // Struct storage data is going to be injected into original structures via pNext
    // that's why a pointer to original, app-provided structures is needed here.
    auto* pBuildGeometryInfo = &pInfos[bgi];
    auto* pBuildRangeInfos = ppBuildRangeInfos[bgi];
    auto& accelerationStructureState =
        SD()._accelerationstructurekhrstates[pBuildGeometryInfo->dstAccelerationStructure];

    std::shared_ptr<CAccelerationStructureKHRState::CBuildInfo> stateTrackInfo;

    // Full acceleration structure build
    if (pBuildGeometryInfo->mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR) {
      stateTrackInfo = std::make_shared<CAccelerationStructureKHRState::CBuildInfo>(
          pBuildGeometryInfo, pBuildRangeInfos);

      accelerationStructureState->buildInfo = stateTrackInfo;
      accelerationStructureState->updateInfo.reset();
      accelerationStructureState->copyInfo.reset();
    }
    // Acceleration structure update (full build info left untouched)
    else if (pBuildGeometryInfo->mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR) {
      const auto& srcAccelerationStructureState =
          SD()._accelerationstructurekhrstates[pBuildGeometryInfo->srcAccelerationStructure];
      stateTrackInfo = std::make_shared<CAccelerationStructureKHRState::CBuildInfo>(
          pBuildGeometryInfo, pBuildRangeInfos, srcAccelerationStructureState);

      accelerationStructureState->updateInfo = stateTrackInfo;
    }

    std::vector<uint32_t> primitivesCount(pBuildGeometryInfo->geometryCount);

    for (uint32_t g = 0; g < pBuildGeometryInfo->geometryCount; ++g) {
      auto& buildRangeInfo = pBuildRangeInfos[g];

      primitivesCount[g] = buildRangeInfo.primitiveCount;
      //auto* pStateTrackedGeometry =
      //    pBuildGeometryInfo->pGeometries
      //        ? stateTrackInfo->buildGeometryInfoData._pGeometries->Vector()[g].get()
      //        : stateTrackInfo->buildGeometryInfoData._ppGeometries->Vector()[g][0].get();
      //const VkAccelerationStructureGeometryKHR* pGeometry =
      //    pBuildGeometryInfo->pGeometries ? &pBuildGeometryInfo->pGeometries[g]
      //                                    : pBuildGeometryInfo->ppGeometries[g];
      //
      //switch (pGeometry->geometryType) {
      //case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
      //  auto& triangles = pGeometry->geometry.triangles;
      //  auto& stateTrackedTriangles = pStateTrackedGeometry->_geometry->_triangles;
      //
      //  // Non-indexed geometry
      //  if (triangles.indexType == VK_INDEX_TYPE_NONE_KHR) {
      //    stateTrackedTriangles->_vertexData->_bufferDeviceAddress = CBufferDeviceAddressObjectData(
      //        triangles.vertexData.deviceAddress,
      //        buildRangeInfo.primitiveOffset + buildRangeInfo.firstVertex * triangles.vertexStride);
      //  }
      //  // Indexed geometry
      //  else {
      //    stateTrackedTriangles->_vertexData->_bufferDeviceAddress = CBufferDeviceAddressObjectData(
      //        triangles.vertexData.deviceAddress,
      //        std::max(0u, triangles.maxVertex - 1) * triangles.vertexStride);
      //    stateTrackedTriangles->_indexData->_bufferDeviceAddress = CBufferDeviceAddressObjectData(
      //        triangles.indexData.deviceAddress, buildRangeInfo.primitiveOffset);
      //  }
      //
      //  stateTrackedTriangles->_transformData->_bufferDeviceAddress =
      //      CBufferDeviceAddressObjectData(triangles.transformData.deviceAddress,
      //                                     buildRangeInfo.transformOffset);
      //
      //  if (isSubcaptureBeforeRestorationPhase()) {
      //    //stateTrackedTriangles->_vertexData->PrepareStateRestore(cmdBufState.get(), )
      //    //stateTrackedTriangles->_indexData
      //  }
      //  break;
      //}
      //case VK_GEOMETRY_TYPE_AABBS_KHR: {
      //  break;
      //}
      //case VK_GEOMETRY_TYPE_INSTANCES_KHR: {
      //  break;
      //}
      //default: {
      //  throw std::runtime_error("Unknown geometry type provided!");
      //}
      //}
    }

    if (isSubcaptureBeforeRestorationPhase()) {
      drvVk.vkGetAccelerationStructureBuildSizesKHR(
          device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, pBuildGeometryInfo,
          primitivesCount.data(), &accelerationStructureState->buildSizeInfo);
      cmdBufState->touchedResources.emplace_back(
          (uint64_t)pBuildGeometryInfo->dstAccelerationStructure,
          ResourceType::ACCELERATION_STRUCTURE);
    }
  }
}

} // namespace Vulkan
} // namespace gits
