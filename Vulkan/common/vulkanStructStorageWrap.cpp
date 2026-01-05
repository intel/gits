// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateTracking.h"

gits::Vulkan::CVkAccelerationStructureGeometryAabbsDataKHRData::
    CVkAccelerationStructureGeometryAabbsDataKHRData(
        const VkAccelerationStructureGeometryAabbsDataKHR*
            accelerationstructuregeometryaabbsdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _baseIn(),
      _AccelerationStructureGeometryAabbsDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometryaabbsdatakhr == nullptr) {
  if (!*_isNullPtr) {
    _sType =
        std::make_unique<CVkStructureTypeData>(accelerationstructuregeometryaabbsdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapperData>(accelerationstructuregeometryaabbsdatakhr->pNext);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        accelerationstructuregeometryaabbsdatakhr->data, buildRangeInfo.primitiveOffset,
        accelerationstructuregeometryaabbsdatakhr->stride, buildRangeInfo.primitiveCount,
        controlData);
    _stride = std::make_unique<Cuint64_tData>(accelerationstructuregeometryaabbsdatakhr->stride);

    // Pass data to vulkan arguments class (see VkAccelerationStructureGeometryAabbsDataKHR() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,    // VkStructureType sType;
          accelerationstructuregeometryaabbsdatakhr->pNext, // const void* pNext;
          accelerationstructuregeometryaabbsdatakhr->sType, // VkStructureType sStructStorageType
          this                                              // const void* pStructStorage
      };
      const_cast<VkAccelerationStructureGeometryAabbsDataKHR*>(
          accelerationstructuregeometryaabbsdatakhr)
          ->pNext = &_baseIn;
    }
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _data = nullptr;
    _stride = nullptr;
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryTrianglesDataKHRData::
    CVkAccelerationStructureGeometryTrianglesDataKHRData(
        const VkAccelerationStructureGeometryTrianglesDataKHR*
            accelerationstructuregeometrytrianglesdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _baseIn(),
      _AccelerationStructureGeometryTrianglesDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometrytrianglesdatakhr == nullptr) {
  if (!*_isNullPtr) {
    VkOpacityMicromapCustomDataGITS ommCustomData = {buildRangeInfo.primitiveCount, controlData};

    _sType = std::make_unique<CVkStructureTypeData>(
        accelerationstructuregeometrytrianglesdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapperData>(
        accelerationstructuregeometrytrianglesdatakhr->pNext, &ommCustomData);
    _vertexFormat = std::make_unique<CVkFormatData>(
        accelerationstructuregeometrytrianglesdatakhr->vertexFormat);
    _vertexData = std::make_unique<CDeviceOrHostAddressAccelerationStructureVertexDataGITSData>(
        accelerationstructuregeometrytrianglesdatakhr->vertexData, buildRangeInfo.primitiveOffset,
        accelerationstructuregeometrytrianglesdatakhr->vertexStride,
        buildRangeInfo.primitiveCount * 3, buildRangeInfo.firstVertex,
        accelerationstructuregeometrytrianglesdatakhr->maxVertex,
        accelerationstructuregeometrytrianglesdatakhr->indexData,
        accelerationstructuregeometrytrianglesdatakhr->indexType, controlData);
    _vertexStride = std::make_unique<Cuint64_tData>(
        accelerationstructuregeometrytrianglesdatakhr->vertexStride);
    _maxVertex =
        std::make_unique<Cuint32_tData>(accelerationstructuregeometrytrianglesdatakhr->maxVertex);
    _indexType = std::make_unique<CVkIndexTypeData>(
        accelerationstructuregeometrytrianglesdatakhr->indexType);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        accelerationstructuregeometrytrianglesdatakhr->indexData, buildRangeInfo.primitiveOffset,
        getIndexElementSize(accelerationstructuregeometrytrianglesdatakhr->indexType),
        buildRangeInfo.primitiveCount * 3, controlData);
    _transformData = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        accelerationstructuregeometrytrianglesdatakhr->transformData,
        buildRangeInfo.transformOffset, sizeof(VkTransformMatrixKHR), 1, controlData);

    // Pass data to vulkan arguments class (see CVkAccelerationStructureGeometryTrianglesDataKHR() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,        // VkStructureType sType;
          accelerationstructuregeometrytrianglesdatakhr->pNext, // const void* pNext;
          accelerationstructuregeometrytrianglesdatakhr
              ->sType, // VkStructureType sStructStorageType
          this         // const void* pStructStorage;
      };
      const_cast<VkAccelerationStructureGeometryTrianglesDataKHR*>(
          accelerationstructuregeometrytrianglesdatakhr)
          ->pNext = &_baseIn;
    }
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _vertexFormat = nullptr;
    _vertexData = nullptr;
    _vertexStride = nullptr;
    _maxVertex = nullptr;
    _indexType = nullptr;
    _indexData = nullptr;
    _transformData = nullptr;
  }
}

gits::Vulkan::CVkAccelerationStructureBuildGeometryInfoKHRData::
    CVkAccelerationStructureBuildGeometryInfoKHRData(
        const VkAccelerationStructureBuildGeometryInfoKHR*
            accelerationstructurebuildgeometryinfokhr,
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos,
        VkAccelerationStructureBuildControlDataGITS controlData)
    : _baseIn(),
      _AccelerationStructureBuildGeometryInfoKHR(nullptr),
      _isNullPtr(accelerationstructurebuildgeometryinfokhr == nullptr) {
  if (!*_isNullPtr) {
    _sType =
        std::make_unique<CVkStructureTypeData>(accelerationstructurebuildgeometryinfokhr->sType);
    _pNext = std::make_unique<CpNextWrapperData>(accelerationstructurebuildgeometryinfokhr->pNext);
    _type = std::make_unique<CVkAccelerationStructureTypeKHRData>(
        accelerationstructurebuildgeometryinfokhr->type);
    _flags = std::make_unique<Cuint32_tData>(accelerationstructurebuildgeometryinfokhr->flags);
    _mode = std::make_unique<CVkBuildAccelerationStructureModeKHRData>(
        accelerationstructurebuildgeometryinfokhr->mode);
    _srcAccelerationStructure = std::make_unique<CVkAccelerationStructureKHRData>(
        accelerationstructurebuildgeometryinfokhr->srcAccelerationStructure);
    _dstAccelerationStructure = std::make_unique<CVkAccelerationStructureKHRData>(
        accelerationstructurebuildgeometryinfokhr->dstAccelerationStructure);
    _geometryCount =
        std::make_unique<Cuint32_tData>(accelerationstructurebuildgeometryinfokhr->geometryCount);
    _pGeometries = std::make_unique<CVkAccelerationStructureGeometryKHRDataArray>(
        accelerationstructurebuildgeometryinfokhr->geometryCount,
        accelerationstructurebuildgeometryinfokhr->pGeometries, pBuildRangeInfos, controlData);
    _ppGeometries = std::make_unique<CVkAccelerationStructureGeometryKHRDataArrayOfArrays>(
        accelerationstructurebuildgeometryinfokhr->geometryCount,
        accelerationstructurebuildgeometryinfokhr->ppGeometries, pBuildRangeInfos, controlData);
    _scratchData = std::make_unique<CVkDeviceOrHostAddressKHRData>(
        accelerationstructurebuildgeometryinfokhr->scratchData, controlData);

    // Pass data to vulkan arguments class (see CVkAccelerationStructureBuildGeometryInfoKHR() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,    // VkStructureType sType;
          accelerationstructurebuildgeometryinfokhr->pNext, // const void* pNext;
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, // VkStructureType sStructStorageType;
          this // const void* pStructStorage;
      };
      const_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(
          accelerationstructurebuildgeometryinfokhr)
          ->pNext = &_baseIn;
    }
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _type = nullptr;
    _flags = nullptr;
    _mode = nullptr;
    _srcAccelerationStructure = nullptr;
    _dstAccelerationStructure = nullptr;
    _geometryCount = nullptr;
    _pGeometries = nullptr;
    _ppGeometries = nullptr;
    _scratchData = nullptr;
  }
}

gits::Vulkan::CVkMicromapBuildInfoEXTData::CVkMicromapBuildInfoEXTData(
    const VkMicromapBuildInfoEXT* micromapbuildinfoext,
    VkAccelerationStructureBuildControlDataGITS controlData)
    : _MicromapBuildInfoEXT(nullptr), _isNullPtr(micromapbuildinfoext == nullptr) {
  if (!*_isNullPtr) {
    uint64_t triangleArrayCount = 0;
    if (micromapbuildinfoext->pUsageCounts) {
      for (uint32_t i = 0; i < micromapbuildinfoext->usageCountsCount; ++i) {
        triangleArrayCount += micromapbuildinfoext->pUsageCounts[i].count;
      }
    } else if (micromapbuildinfoext->ppUsageCounts) {
      for (uint32_t i = 0; i < micromapbuildinfoext->usageCountsCount; ++i) {
        triangleArrayCount += micromapbuildinfoext->ppUsageCounts[i]->count;
      }
    }
    _sType = std::make_unique<CVkStructureTypeData>(micromapbuildinfoext->sType);
    _pNext = std::make_unique<CpNextWrapperData>(micromapbuildinfoext->pNext);
    _type = std::make_unique<CVkMicromapTypeEXTData>(micromapbuildinfoext->type);
    _flags = std::make_unique<Cuint32_tData>(micromapbuildinfoext->flags);
    _mode = std::make_unique<CVkBuildMicromapModeEXTData>(micromapbuildinfoext->mode);
    _dstMicromap = std::make_unique<CVkMicromapEXTData>(micromapbuildinfoext->dstMicromap);
    _usageCountsCount = std::make_unique<Cuint32_tData>(micromapbuildinfoext->usageCountsCount);
    _pUsageCounts = std::make_unique<CVkMicromapUsageEXTDataArray>(
        micromapbuildinfoext->usageCountsCount, micromapbuildinfoext->pUsageCounts);
    _ppUsageCounts = std::make_unique<CVkMicromapUsageEXTDataArrayOfArrays>(
        micromapbuildinfoext->usageCountsCount, micromapbuildinfoext->ppUsageCounts);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(micromapbuildinfoext->data, 0, 1,
                                                                 1, controlData);
    _scratchData = std::make_unique<CVkDeviceOrHostAddressKHRData>(
        micromapbuildinfoext->scratchData, controlData);
    _triangleArray = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        micromapbuildinfoext->triangleArray, 0, micromapbuildinfoext->triangleArrayStride,
        triangleArrayCount, controlData);
    _triangleArrayStride =
        std::make_unique<Cuint64_tData>(micromapbuildinfoext->triangleArrayStride);

    // Pass data to vulkan arguments class (see CVkMicromapBuildInfoEXT() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
          micromapbuildinfoext->pNext,                   // const void* pNext;
          VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT,     // VkStructureType sStructStorageType;
          this                                           // const void* pStructStorage;
      };
      const_cast<VkMicromapBuildInfoEXT*>(micromapbuildinfoext)->pNext = &_baseIn;
    }
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _type = nullptr;
    _flags = nullptr;
    _mode = nullptr;
    _dstMicromap = nullptr;
    _usageCountsCount = nullptr;
    _pUsageCounts = nullptr;
    _ppUsageCounts = nullptr;
    _data = nullptr;
    _scratchData = nullptr;
    _triangleArray = nullptr;
    _triangleArrayStride = nullptr;
  }
}

gits::Vulkan::CVkAccelerationStructureTrianglesOpacityMicromapEXTData::
    CVkAccelerationStructureTrianglesOpacityMicromapEXTData(
        const VkAccelerationStructureTrianglesOpacityMicromapEXT*
            accelerationstructuretrianglesopacitymicromapext,
        const void* pCustomData)
    : _AccelerationStructureTrianglesOpacityMicromapEXT(nullptr),
      _isNullPtr(accelerationstructuretrianglesopacitymicromapext == nullptr) {
  if (!*_isNullPtr) {
    auto& ommCustomData = *(VkOpacityMicromapCustomDataGITS*)pCustomData;

    _sType = std::make_unique<CVkStructureTypeData>(
        accelerationstructuretrianglesopacitymicromapext->sType);
    _pNext = std::make_unique<CpNextWrapperData>(
        accelerationstructuretrianglesopacitymicromapext->pNext, &ommCustomData);
    _indexType = std::make_unique<CVkIndexTypeData>(
        accelerationstructuretrianglesopacitymicromapext->indexType);
    _indexBuffer = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        accelerationstructuretrianglesopacitymicromapext->indexBuffer, 0,
        accelerationstructuretrianglesopacitymicromapext->indexStride, ommCustomData.primitiveCount,
        ommCustomData.controlData);
    _indexStride = std::make_unique<Cuint64_tData>(
        accelerationstructuretrianglesopacitymicromapext->indexStride);
    _baseTriangle = std::make_unique<Cuint32_tData>(
        accelerationstructuretrianglesopacitymicromapext->baseTriangle);
    _usageCountsCount = std::make_unique<Cuint32_tData>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount);
    _pUsageCounts = std::make_unique<CVkMicromapUsageEXTDataArray>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount,
        accelerationstructuretrianglesopacitymicromapext->pUsageCounts);
    _ppUsageCounts = std::make_unique<CVkMicromapUsageEXTDataArrayOfArrays>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount,
        accelerationstructuretrianglesopacitymicromapext->ppUsageCounts);
    _micromap = std::make_unique<CVkMicromapEXTData>(
        accelerationstructuretrianglesopacitymicromapext->micromap);

    // Pass data to vulkan arguments class (see CVkAccelerationStructureBuildGeometryInfoKHR() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,           // VkStructureType sType;
          accelerationstructuretrianglesopacitymicromapext->pNext, // const void* pNext;
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT, // VkStructureType sStructStorageType;
          this // const void* pStructStorage;
      };
      const_cast<VkAccelerationStructureTrianglesOpacityMicromapEXT*>(
          accelerationstructuretrianglesopacitymicromapext)
          ->pNext = &_baseIn;
    }
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _indexType = nullptr;
    _indexBuffer = nullptr;
    _indexStride = nullptr;
    _baseTriangle = nullptr;
    _usageCountsCount = nullptr;
    _pUsageCounts = nullptr;
    _ppUsageCounts = nullptr;
    _micromap = nullptr;
  }
}
