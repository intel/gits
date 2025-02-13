// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
    _sType = std::make_unique<CVkStructureTypeData>(
        accelerationstructuregeometrytrianglesdatakhr->sType);
    _pNext =
        std::make_unique<CpNextWrapperData>(accelerationstructuregeometrytrianglesdatakhr->pNext);
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
          this                                                  // const void* pStructStorage;
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

    // Pass data to vulkan arguments class (see VkAccelerationStructureBuildGeometryInfoKHR() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      _baseIn = {
          VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,    // VkStructureType sType;
          accelerationstructurebuildgeometryinfokhr->pNext, // const void* pNext;
          this                                              // const void* pStructStorage;
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
