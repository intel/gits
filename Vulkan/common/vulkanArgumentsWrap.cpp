// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStateTracking.h"

gits::Vulkan::CVkAccelerationStructureGeometryAabbsDataKHR::
    CVkAccelerationStructureGeometryAabbsDataKHR(const VkAccelerationStructureGeometryAabbsDataKHR*
                                                     accelerationstructuregeometryaabbsdatakhr)
    : _AccelerationStructureGeometryAabbsDataKHR(nullptr),
      _AccelerationStructureGeometryAabbsDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometryaabbsdatakhr == nullptr) {
  if (*_isNullPtr) {
    _sType = nullptr;
    _pNext = nullptr;
    _data = nullptr;
    _stride = nullptr;
    return;
  }

  auto* pStructStorage = (CVkAccelerationStructureGeometryAabbsDataKHRData*)getStructStoragePointer(
      accelerationstructuregeometryaabbsdatakhr->pNext,
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR);

  _sType = std::make_unique<CVkStructureType>(accelerationstructuregeometryaabbsdatakhr->sType);
  _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometryaabbsdatakhr->pNext);
  _stride = std::make_unique<Cuint64_t>(accelerationstructuregeometryaabbsdatakhr->stride);

  if (pStructStorage) {
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHR>(*pStructStorage->_data);
  } else {
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        accelerationstructuregeometryaabbsdatakhr->data);
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryTrianglesDataKHR::
    CVkAccelerationStructureGeometryTrianglesDataKHR(
        const VkAccelerationStructureGeometryTrianglesDataKHR*
            accelerationstructuregeometrytrianglesdatakhr)
    : _AccelerationStructureGeometryTrianglesDataKHR(nullptr),
      _AccelerationStructureGeometryTrianglesDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometrytrianglesdatakhr == nullptr) {
  if (*_isNullPtr) {
    _sType = nullptr;
    _pNext = nullptr;
    _vertexFormat = nullptr;
    _vertexData = nullptr;
    _vertexStride = nullptr;
    _maxVertex = nullptr;
    _indexType = nullptr;
    _indexData = nullptr;
    _transformData = nullptr;
    return;
  }

  auto* pStructStorage =
      (CVkAccelerationStructureGeometryTrianglesDataKHRData*)getStructStoragePointer(
          accelerationstructuregeometrytrianglesdatakhr->pNext,
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR);

  _sType = std::make_unique<CVkStructureType>(accelerationstructuregeometrytrianglesdatakhr->sType);
  _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometrytrianglesdatakhr->pNext);
  _vertexFormat =
      std::make_unique<CVkFormat>(accelerationstructuregeometrytrianglesdatakhr->vertexFormat);
  _vertexStride =
      std::make_unique<Cuint64_t>(accelerationstructuregeometrytrianglesdatakhr->vertexStride);
  _maxVertex =
      std::make_unique<Cuint32_t>(accelerationstructuregeometrytrianglesdatakhr->maxVertex);
  _indexType =
      std::make_unique<CVkIndexType>(accelerationstructuregeometrytrianglesdatakhr->indexType);

  if (pStructStorage) {
    _vertexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(*pStructStorage->_vertexData);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(*pStructStorage->_indexData);
    _transformData =
        std::make_unique<CVkDeviceOrHostAddressConstKHR>(*pStructStorage->_transformData);
  } else {
    _vertexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        &accelerationstructuregeometrytrianglesdatakhr->vertexData);
    _indexData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        &accelerationstructuregeometrytrianglesdatakhr->indexData);
    _transformData = std::make_unique<CVkDeviceOrHostAddressConstKHR>(
        &accelerationstructuregeometrytrianglesdatakhr->transformData);
  }
}
