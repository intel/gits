// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateTracking.h"

// nothing extraordinary here; it's vulkanArguments counterpart's behavior that is changed
gits::Vulkan::CVkAccelerationStructureGeometryAabbsDataKHRData::
    CVkAccelerationStructureGeometryAabbsDataKHRData(
        const VkAccelerationStructureGeometryAabbsDataKHR*
            accelerationstructuregeometryaabbsdatakhr)
    : _AccelerationStructureGeometryAabbsDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometryaabbsdatakhr == nullptr) {
  if (!*_isNullPtr) {
    _sType =
        std::make_unique<CVkStructureTypeData>(accelerationstructuregeometryaabbsdatakhr->sType);
    _pNext = std::make_unique<CpNextWrapperData>(accelerationstructuregeometryaabbsdatakhr->pNext);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        &accelerationstructuregeometryaabbsdatakhr->data);
    _stride = std::make_unique<Cuint64_tData>(accelerationstructuregeometryaabbsdatakhr->stride);
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,    // VkStructureType sType;
        accelerationstructuregeometryaabbsdatakhr->pNext, // const void* pNext;
        accelerationstructuregeometryaabbsdatakhr->sType, // VkStructureType sStructStorageType;
        this                                              // const void* pStructStorage;
    };
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _data = nullptr;
    _stride = nullptr;
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,                    // VkStructureType sType;
        nullptr,                                                          // const void* pNext;
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR, // VkStructureType sStructStorageType;
        nullptr // const void* pStructStorage;
    };
  }
}

// nothing extraordinary here; it's vulkanArguments counterpart's behavior that is changed
gits::Vulkan::CVkAccelerationStructureGeometryTrianglesDataKHRData::
    CVkAccelerationStructureGeometryTrianglesDataKHRData(
        const VkAccelerationStructureGeometryTrianglesDataKHR*
            accelerationstructuregeometrytrianglesdatakhr)
    : _AccelerationStructureGeometryTrianglesDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometrytrianglesdatakhr == nullptr) {
  if (!*_isNullPtr) {
    _sType = std::make_unique<CVkStructureTypeData>(
        accelerationstructuregeometrytrianglesdatakhr->sType);
    _pNext =
        std::make_unique<CpNextWrapperData>(accelerationstructuregeometrytrianglesdatakhr->pNext);
    _vertexFormat = std::make_unique<CVkFormatData>(
        accelerationstructuregeometrytrianglesdatakhr->vertexFormat);
    _vertexData = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        &accelerationstructuregeometrytrianglesdatakhr->vertexData);
    _vertexStride = std::make_unique<Cuint64_tData>(
        accelerationstructuregeometrytrianglesdatakhr->vertexStride);
    _maxVertex =
        std::make_unique<Cuint32_tData>(accelerationstructuregeometrytrianglesdatakhr->maxVertex);
    _indexType = std::make_unique<CVkIndexTypeData>(
        accelerationstructuregeometrytrianglesdatakhr->indexType);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        &accelerationstructuregeometrytrianglesdatakhr->indexData);
    _transformData = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        &accelerationstructuregeometrytrianglesdatakhr->transformData);
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,        // VkStructureType sType;
        accelerationstructuregeometrytrianglesdatakhr->pNext, // const void* pNext;
        accelerationstructuregeometrytrianglesdatakhr->sType, // VkStructureType sStructStorageType;
        this                                                  // const void* pStructStorage;
    };
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
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        nullptr,                                       // const void* pNext;
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR, // VkStructureType sStructStorageType;
        nullptr // const void* pStructStorage;
    };
  }
}
