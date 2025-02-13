// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateTracking.h"

bool gits::Vulkan::CVkDescriptorImageInfo::DeclarationNeeded() const {
  // Because if it was a nullptr, we'll just print "nullptr", no need for a variable.
  return !*_isNullPtr;
}

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
    auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
        accelerationstructuregeometryaabbsdatakhr->pNext,
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
    if (!structStoragePointer || !structStoragePointer->pStructStorage) {
      _isNullPtr = true;
      return;
    }

    auto* geometryAabbsData =
        (CVkAccelerationStructureGeometryAabbsDataKHRData*)structStoragePointer->pStructStorage;

    // Revert pNext pointer to the original value (see CVkAccelerationStructureGeometryTrianglesDataKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureGeometryAabbsDataKHR*>(
          accelerationstructuregeometryaabbsdatakhr)
          ->pNext = structStoragePointer->pNext;
    }

    _sType = std::make_unique<CVkStructureType>(accelerationstructuregeometryaabbsdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometryaabbsdatakhr->pNext);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometryaabbsdatakhr->data, *geometryAabbsData->_data);
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
    auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
        accelerationstructuregeometrytrianglesdatakhr->pNext,
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
    if (!structStoragePointer || !structStoragePointer->pStructStorage) {
      _isNullPtr = true;
      return;
    }

    auto* geometryTrianglesData =
        (CVkAccelerationStructureGeometryTrianglesDataKHRData*)structStoragePointer->pStructStorage;

    // Revert pNext pointer to the original value (see CVkAccelerationStructureGeometryTrianglesDataKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureGeometryTrianglesDataKHR*>(
          accelerationstructuregeometrytrianglesdatakhr)
          ->pNext = structStoragePointer->pNext;
    }

    _sType =
        std::make_unique<CVkStructureType>(accelerationstructuregeometrytrianglesdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometrytrianglesdatakhr->pNext);
    _vertexFormat =
        std::make_unique<CVkFormat>(accelerationstructuregeometrytrianglesdatakhr->vertexFormat);
    _vertexData = std::make_unique<CDeviceOrHostAddressAccelerationStructureVertexDataGITS>(
        accelerationstructuregeometrytrianglesdatakhr->vertexData,
        *geometryTrianglesData->_vertexData);
    _vertexStride =
        std::make_unique<Cuint64_t>(accelerationstructuregeometrytrianglesdatakhr->vertexStride);
    _maxVertex =
        std::make_unique<Cuint32_t>(accelerationstructuregeometrytrianglesdatakhr->maxVertex);
    _indexType =
        std::make_unique<CVkIndexType>(accelerationstructuregeometrytrianglesdatakhr->indexType);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometrytrianglesdatakhr->indexData,
        *geometryTrianglesData->_indexData);
    _transformData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometrytrianglesdatakhr->transformData,
        *geometryTrianglesData->_transformData);
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
    auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
        accelerationstructurebuildgeometryinfokhr->pNext,
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
    if (!structStoragePointer || !structStoragePointer->pStructStorage) {
      _isNullPtr = true;
      return;
    }

    auto* geometryInfoData =
        (CVkAccelerationStructureBuildGeometryInfoKHRData*)structStoragePointer->pStructStorage;

    // Revert pNext pointer to the original value (see CVkAccelerationStructureBuildGeometryInfoKHRData() constructor)
    if (!isSubcaptureBeforeRestorationPhase()) {
      const_cast<VkAccelerationStructureBuildGeometryInfoKHR*>(
          accelerationstructurebuildgeometryinfokhr)
          ->pNext = structStoragePointer->pNext;
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
        accelerationstructurebuildgeometryinfokhr->scratchData, *geometryInfoData->_scratchData);
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
