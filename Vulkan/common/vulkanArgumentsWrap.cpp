// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateTracking.h"

gits::Vulkan::CVkAccelerationStructureGeometryAabbsDataKHR::
    CVkAccelerationStructureGeometryAabbsDataKHR(
        const VkAccelerationStructureGeometryAabbsDataKHR*
            accelerationstructuregeometryaabbsdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _AccelerationStructureGeometryAabbsDataKHR(nullptr),
      _AccelerationStructureGeometryAabbsDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometryaabbsdatakhr == nullptr) {
  if (!*_isNullPtr) {
    auto* pStructStorage =
        (CVkAccelerationStructureGeometryAabbsDataKHRData*)getStructStoragePointer(
            accelerationstructuregeometryaabbsdatakhr->pNext,
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR);
    if (!pStructStorage) {
      _isNullPtr = true;
      return;
    }

    // Revert pNext pointer to the original value (see CVkAccelerationStructureGeometryTrianglesDataKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureGeometryAabbsDataKHR*>(
          accelerationstructuregeometryaabbsdatakhr)
          ->pNext = pStructStorage->_baseIn.pNext;
    }

    _sType = std::make_unique<CVkStructureType>(accelerationstructuregeometryaabbsdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometryaabbsdatakhr->pNext);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometryaabbsdatakhr->data, *pStructStorage->_data);
    _stride = std::make_unique<Cuint64_t>(accelerationstructuregeometryaabbsdatakhr->stride);
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _data = nullptr;
    _stride = nullptr;
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryTrianglesDataKHR::
    CVkAccelerationStructureGeometryTrianglesDataKHR(
        const VkAccelerationStructureGeometryTrianglesDataKHR*
            accelerationstructuregeometrytrianglesdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _AccelerationStructureGeometryTrianglesDataKHR(nullptr),
      _AccelerationStructureGeometryTrianglesDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometrytrianglesdatakhr == nullptr) {
  if (!*_isNullPtr) {
    auto* pStructStorage =
        (CVkAccelerationStructureGeometryTrianglesDataKHRData*)getStructStoragePointer(
            accelerationstructuregeometrytrianglesdatakhr->pNext,
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR);
    if (!pStructStorage) {
      _isNullPtr = true;
      return;
    }

    // Revert pNext pointer to the original value (see CVkAccelerationStructureGeometryTrianglesDataKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureGeometryTrianglesDataKHR*>(
          accelerationstructuregeometrytrianglesdatakhr)
          ->pNext = pStructStorage->_baseIn.pNext;
    }

    _sType =
        std::make_unique<CVkStructureType>(accelerationstructuregeometrytrianglesdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometrytrianglesdatakhr->pNext);
    _vertexFormat =
        std::make_unique<CVkFormat>(accelerationstructuregeometrytrianglesdatakhr->vertexFormat);
    _vertexData = std::make_unique<CDeviceOrHostAddressAccelerationStructureVertexDataGITS>(
        accelerationstructuregeometrytrianglesdatakhr->vertexData, *pStructStorage->_vertexData);
    _vertexStride =
        std::make_unique<Cuint64_t>(accelerationstructuregeometrytrianglesdatakhr->vertexStride);
    _maxVertex =
        std::make_unique<Cuint32_t>(accelerationstructuregeometrytrianglesdatakhr->maxVertex);
    _indexType =
        std::make_unique<CVkIndexType>(accelerationstructuregeometrytrianglesdatakhr->indexType);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometrytrianglesdatakhr->indexData, *pStructStorage->_indexData);
    _transformData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometrytrianglesdatakhr->transformData,
        *pStructStorage->_transformData);
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

gits::Vulkan::CVkAccelerationStructureBuildGeometryInfoKHR::
    CVkAccelerationStructureBuildGeometryInfoKHR(
        const VkAccelerationStructureBuildGeometryInfoKHR*
            accelerationstructurebuildgeometryinfokhr,
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos,
        VkAccelerationStructureBuildControlDataGITS controlData)
    : _AccelerationStructureBuildGeometryInfoKHR(nullptr),
      _AccelerationStructureBuildGeometryInfoKHROriginal(nullptr),
      _isNullPtr(accelerationstructurebuildgeometryinfokhr == nullptr) {
  if (!*_isNullPtr) {
    auto* pStructStorage =
        (CVkAccelerationStructureBuildGeometryInfoKHRData*)getStructStoragePointer(
            accelerationstructurebuildgeometryinfokhr->pNext,
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR);
    if (!pStructStorage) {
      _isNullPtr = true;
      return;
    }

    // Revert pNext pointer to the original value (see CVkAccelerationStructureBuildGeometryInfoKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(
          accelerationstructurebuildgeometryinfokhr)
          ->pNext = pStructStorage->_baseIn.pNext;
    }

    _sType = std::make_unique<CVkStructureType>(accelerationstructurebuildgeometryinfokhr->sType);
    _pNext = std::make_unique<CpNextWrapper>(accelerationstructurebuildgeometryinfokhr->pNext);
    _type = std::make_unique<CVkAccelerationStructureTypeKHR>(
        accelerationstructurebuildgeometryinfokhr->type);
    _flags = std::make_unique<Cuint32_t>(accelerationstructurebuildgeometryinfokhr->flags);
    _mode = std::make_unique<CVkBuildAccelerationStructureModeKHR>(
        accelerationstructurebuildgeometryinfokhr->mode);
    _srcAccelerationStructure = std::make_unique<CVkAccelerationStructureKHR>(
        accelerationstructurebuildgeometryinfokhr->srcAccelerationStructure);
    _dstAccelerationStructure = std::make_unique<CVkAccelerationStructureKHR>(
        accelerationstructurebuildgeometryinfokhr->dstAccelerationStructure);
    _geometryCount =
        std::make_unique<Cuint32_t>(accelerationstructurebuildgeometryinfokhr->geometryCount);
    _pGeometries = std::make_unique<CVkAccelerationStructureGeometryKHRArray>(
        accelerationstructurebuildgeometryinfokhr->geometryCount,
        accelerationstructurebuildgeometryinfokhr->pGeometries, pBuildRangeInfos, controlData);
    _ppGeometries = std::make_unique<CVkAccelerationStructureGeometryKHRArrayOfArrays>(
        accelerationstructurebuildgeometryinfokhr->geometryCount,
        accelerationstructurebuildgeometryinfokhr->ppGeometries, pBuildRangeInfos, controlData);
    _scratchData = std::make_unique<CVkDeviceOrHostAddressKHR>(
        accelerationstructurebuildgeometryinfokhr->scratchData, *pStructStorage->_scratchData);
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

gits::Vulkan::CVkMicromapBuildInfoEXT::CVkMicromapBuildInfoEXT(
    const VkMicromapBuildInfoEXT* micromapbuildinfoext,
    VkAccelerationStructureBuildControlDataGITS controlData)
    : _MicromapBuildInfoEXT(nullptr),
      _MicromapBuildInfoEXTOriginal(nullptr),
      _isNullPtr(micromapbuildinfoext == nullptr) {
  if (!*_isNullPtr) {
    auto* pStructStorage = (CVkMicromapBuildInfoEXTData*)getStructStoragePointer(
        micromapbuildinfoext->pNext, VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT);
    if (!pStructStorage) {
      _isNullPtr = true;
      return;
    }

    // Revert pNext pointer to the original value (see CVkMicromapBuildInfoEXTData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkMicromapBuildInfoEXT*>(micromapbuildinfoext)->pNext =
          pStructStorage->_baseIn.pNext;
    }

    _sType = std::make_unique<CVkStructureType>(micromapbuildinfoext->sType);
    _pNext = std::make_unique<CpNextWrapper>(micromapbuildinfoext->pNext);
    _type = std::make_unique<CVkMicromapTypeEXT>(micromapbuildinfoext->type);
    _flags = std::make_unique<Cuint32_t>(micromapbuildinfoext->flags);
    _mode = std::make_unique<CVkBuildMicromapModeEXT>(micromapbuildinfoext->mode);
    _dstMicromap = std::make_unique<CVkMicromapEXT>(micromapbuildinfoext->dstMicromap);
    _usageCountsCount = std::make_unique<Cuint32_t>(micromapbuildinfoext->usageCountsCount);
    _pUsageCounts = std::make_unique<CVkMicromapUsageEXTArray>(
        micromapbuildinfoext->usageCountsCount, micromapbuildinfoext->pUsageCounts);
    _ppUsageCounts = std::make_unique<CVkMicromapUsageEXTArrayOfArrays>(
        micromapbuildinfoext->usageCountsCount, micromapbuildinfoext->ppUsageCounts);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHR>(micromapbuildinfoext->data,
                                                             *pStructStorage->_data);
    _scratchData = std::make_unique<CVkDeviceOrHostAddressKHR>(micromapbuildinfoext->scratchData,
                                                               *pStructStorage->_scratchData);
    _triangleArray = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        micromapbuildinfoext->triangleArray, *pStructStorage->_triangleArray);
    _triangleArrayStride = std::make_unique<Cuint64_t>(micromapbuildinfoext->triangleArrayStride);
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

gits::Vulkan::CVkAccelerationStructureTrianglesOpacityMicromapEXT::
    CVkAccelerationStructureTrianglesOpacityMicromapEXT(
        const VkAccelerationStructureTrianglesOpacityMicromapEXT*
            accelerationstructuretrianglesopacitymicromapext,
        const void* pCustomData)
    : _AccelerationStructureTrianglesOpacityMicromapEXT(nullptr),
      _AccelerationStructureTrianglesOpacityMicromapEXTOriginal(nullptr),
      _isNullPtr(accelerationstructuretrianglesopacitymicromapext == nullptr) {
  if (!*_isNullPtr) {
    auto* pStructStorage =
        (CVkAccelerationStructureTrianglesOpacityMicromapEXTData*)getStructStoragePointer(
            accelerationstructuretrianglesopacitymicromapext->pNext,
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT);
    if (!pStructStorage) {
      _isNullPtr = true;
      return;
    }

    // Revert pNext pointer to the original value (see CVkAccelerationStructureBuildGeometryInfoKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureTrianglesOpacityMicromapEXT*>(
          accelerationstructuretrianglesopacitymicromapext)
          ->pNext = pStructStorage->_baseIn.pNext;
    }

    _sType =
        std::make_unique<CVkStructureType>(accelerationstructuretrianglesopacitymicromapext->sType);
    _pNext =
        std::make_unique<CpNextWrapper>(accelerationstructuretrianglesopacitymicromapext->pNext);
    _indexType =
        std::make_unique<CVkIndexType>(accelerationstructuretrianglesopacitymicromapext->indexType);
    _indexBuffer = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuretrianglesopacitymicromapext->indexBuffer,
        *pStructStorage->_indexBuffer);
    _indexStride =
        std::make_unique<Cuint64_t>(accelerationstructuretrianglesopacitymicromapext->indexStride);
    _baseTriangle =
        std::make_unique<Cuint32_t>(accelerationstructuretrianglesopacitymicromapext->baseTriangle);
    _usageCountsCount = std::make_unique<Cuint32_t>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount);
    _pUsageCounts = std::make_unique<CVkMicromapUsageEXTArray>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount,
        accelerationstructuretrianglesopacitymicromapext->pUsageCounts);
    _ppUsageCounts = std::make_unique<CVkMicromapUsageEXTArrayOfArrays>(
        accelerationstructuretrianglesopacitymicromapext->usageCountsCount,
        accelerationstructuretrianglesopacitymicromapext->ppUsageCounts);
    _micromap = std::make_unique<CVkMicromapEXT>(
        accelerationstructuretrianglesopacitymicromapext->micromap);
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
