// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLog.h"
#include "vulkanStateTracking.h"

gits::Vulkan::CBinaryResourceData::CBinaryResourceData(TResourceType _,
                                                       const void* data,
                                                       size_t size)
    : _data((uint8_t*)data, (uint8_t*)data + size) {}

gits::Vulkan::CBinaryResourceData::PointerProxy gits::Vulkan::CBinaryResourceData::Data() const {
  return PointerProxy(_data.data(), _data.size());
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkGenericArgumentData::CVkGenericArgumentData(const void* pVkGenericArgumentData)
    : _argument(nullptr) {
  pVkGenericArgumentData = ignoreLoaderSpecificStructureTypes(pVkGenericArgumentData);

  if (pVkGenericArgumentData) {
    switch (*(VkStructureType*)pVkGenericArgumentData) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = std::make_unique<C##structure##Data>((structure*)pVkGenericArgumentData);          \
    break;

#include "vulkanPNextWrappers.inl"

    default:
      LOG_ERROR << "Unknown enum value: " << *(VkStructureType*)pVkGenericArgumentData
                << " for CVkGenericArgumentData";
      const void* pNext = ((const VkBaseInStructure*)pVkGenericArgumentData)->pNext;
      while (pNext != nullptr) {
        LOG_WARNING << "Additional structure pointed to by pNext: " << *(VkStructureType*)pNext;
        pNext = ((const VkBaseInStructure*)pNext)->pNext;
      }
      break;
    }
  }
}

const void* gits::Vulkan::CVkGenericArgumentData::Value() {
  if (!_argument) {
    return nullptr;
  } else {
    return _argument->GetPtrType();
  }
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkClearColorValueData::CVkClearColorValueData(
    const VkClearColorValue* clearcolorvalue)
    : _ClearColorValue(nullptr), _isNullPtr(clearcolorvalue == nullptr) {
  if (!*_isNullPtr) {
    _uint32 = std::make_unique<Cuint32_tDataArray>(4, clearcolorvalue->uint32);
  } else {
    _uint32 = nullptr;
  }
}

VkClearColorValue* gits::Vulkan::CVkClearColorValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearColorValue == nullptr) {
    _ClearColorValue = std::make_unique<VkClearColorValue>();
    auto uint32Values = **_uint32;
    if (uint32Values != nullptr) {
      for (int i = 0; i < 4; i++) {
        _ClearColorValue->uint32[i] = uint32Values[i];
      }
    } else {
      LOG_ERROR << "The pointer to array **_uint32 is null.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return _ClearColorValue.get();
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkClearValueData::CVkClearValueData(const VkClearValue* clearvalue)
    : _ClearValue(nullptr), _isNullPtr(clearvalue == nullptr) {
  if (!*_isNullPtr) {
    _color = std::make_unique<CVkClearColorValueData>(&clearvalue->color);
  } else {
    _color = nullptr;
  }
}

VkClearValue* gits::Vulkan::CVkClearValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearValue == nullptr) {
    _ClearValue = std::make_unique<VkClearValue>();
    _ClearValue->color = **_color;
  }
  return _ClearValue.get();
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CBufferDeviceAddressObjectData::CBufferDeviceAddressObjectData(
    VkDeviceAddress deviceAddress)
    : _originalDeviceAddress(deviceAddress), _buffer(VK_NULL_HANDLE), _offset(0) {}

gits::Vulkan::CBufferDeviceAddressObjectData& gits::Vulkan::CBufferDeviceAddressObjectData::
operator=(CBufferDeviceAddressObjectData&& other) noexcept {
  if (this != &other) {
    _originalDeviceAddress = other._originalDeviceAddress;
    _buffer = other._buffer;
    _offset = other._offset;
  }

  return *this;
}

std::set<uint64_t> gits::Vulkan::CBufferDeviceAddressObjectData::GetMappedPointers() {
  std::set<uint64_t> returnMap;
  if (_buffer) {
    returnMap = SD()._bufferstates[_buffer]->GetMappedPointers();
  }
  return returnMap;
}

// ------------------------------------------------------------------------------------------------

void gits::Vulkan::COnQueueSubmitEndDataStorage::GatherDataOnQueueSubmitEnd(
    CCommandBufferState& commandBufferState,
    OnQueueSubmitEndFunctionType function,
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size) {
  _onQueueSubmitEnd = function;
  _device = device;
  _offset = offset;
  _dataSize = size;
  _dataMemory = memory;
  commandBufferState.queueSubmitEndMessageReceivers.push_back(this);
}

void gits::Vulkan::COnQueueSubmitEndDataStorage::OnQueueSubmitEnd() {
  if (_onQueueSubmitEnd) {
    _onQueueSubmitEnd(_device, _dataMemory, _offset, _dataSize, _data);
  }
}

// ------------------------------------------------------------------------------------------------

VkDeviceOrHostAddressConstKHR* gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::Value() {
  if (!_DeviceOrHostAddressConst) {
    _DeviceOrHostAddressConst = std::make_unique<VkDeviceOrHostAddressConstKHR>();
    _DeviceOrHostAddressConst->deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
  }
  return _DeviceOrHostAddressConst.get();
}

std::set<uint64_t> gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::GetMappedPointers() {
  return _bufferDeviceAddress.GetMappedPointers();
}

// ------------------------------------------------------------------------------------------------
// No const, mainly scratch data

gits::Vulkan::CVkDeviceOrHostAddressKHRData::CVkDeviceOrHostAddressKHRData(
    const VkDeviceOrHostAddressKHR deviceorhostaddress)
    : _bufferDeviceAddress(), _DeviceOrHostAddress(nullptr) {
  if (deviceorhostaddress.deviceAddress == 0) {
    return;
  }
  // Store original device address
  _bufferDeviceAddress = CBufferDeviceAddressObjectData(deviceorhostaddress.deviceAddress);
}

VkDeviceOrHostAddressKHR* gits::Vulkan::CVkDeviceOrHostAddressKHRData::Value() {
  if (!_DeviceOrHostAddress) {
    _DeviceOrHostAddress = std::make_unique<VkDeviceOrHostAddressKHR>();
    _DeviceOrHostAddress->deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
  }
  return _DeviceOrHostAddress.get();
}

std::set<uint64_t> gits::Vulkan::CVkDeviceOrHostAddressKHRData::GetMappedPointers() {
  return _bufferDeviceAddress.GetMappedPointers();
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::
    CVkAccelerationStructureGeometryDataKHRData(
        VkGeometryTypeKHR geometryType,
        const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr)
    : _AccelerationStructureGeometryDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometrydatakhr == nullptr) {
  if (*_isNullPtr) {
    return;
  }

  _geometryType = std::make_unique<CVkGeometryTypeKHRData>(geometryType);
  switch (geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
    _triangles = std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHRData>(
        &accelerationstructuregeometrydatakhr->triangles);
    break;
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    _aabbs = std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHRData>(
        &accelerationstructuregeometrydatakhr->aabbs);
    break;
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    _instances = std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHRData>(
        &accelerationstructuregeometrydatakhr->instances);
    break;
  default:
    throw std::runtime_error("Unknown geometry type provided!");
    break;
  }
}

VkAccelerationStructureGeometryDataKHR* gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::
    Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryDataKHR == nullptr) {
    _AccelerationStructureGeometryDataKHR =
        std::make_unique<VkAccelerationStructureGeometryDataKHR>();
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _AccelerationStructureGeometryDataKHR->triangles = **_triangles;
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _AccelerationStructureGeometryDataKHR->aabbs = **_aabbs;
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _AccelerationStructureGeometryDataKHR->instances = ***_instances;
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
  return _AccelerationStructureGeometryDataKHR.get();
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::GetMappedPointers() {
  switch (**_geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
    return _triangles->GetMappedPointers();
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    return _aabbs->GetMappedPointers();
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    return _instances->GetMappedPointers();
  default:
    throw std::runtime_error("Unknown geometry type provided!");
  }
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    CVkAccelerationStructureGeometryInstancesDataKHRData(
        const VkAccelerationStructureGeometryInstancesDataKHR*
            accelerationstructuregeometryinstancesdatakhr)
    : _baseIn(),
      _AccelerationStructureGeometryInstancesDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometryinstancesdatakhr == nullptr) {
  if (!*_isNullPtr) {
    _sType = std::make_unique<CVkStructureTypeData>(
        accelerationstructuregeometryinstancesdatakhr->sType);
    _pNext =
        std::make_unique<CpNextWrapperData>(accelerationstructuregeometryinstancesdatakhr->pNext);
    _arrayOfPointers = std::make_unique<Cuint32_tData>(
        accelerationstructuregeometryinstancesdatakhr->arrayOfPointers);
    _data = std::make_unique<CVkDeviceOrHostAddressConstKHRData>(
        &accelerationstructuregeometryinstancesdatakhr->data);
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _arrayOfPointers = nullptr;
    _data = nullptr;
  }
}

VkAccelerationStructureGeometryInstancesDataKHR* gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHRData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryInstancesDataKHR == nullptr) {
    // Pass this structure through pNext
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        **_pNext,                                      // const void* pNext;
        **_sType,                                      // VkStructureType sStructStorageType;
        this                                           // const void* pStructStorage;
    };

    _AccelerationStructureGeometryInstancesDataKHR =
        std::make_unique<VkAccelerationStructureGeometryInstancesDataKHR>();
    _AccelerationStructureGeometryInstancesDataKHR->sType = **_sType;
    _AccelerationStructureGeometryInstancesDataKHR->pNext = &_baseIn;
    _AccelerationStructureGeometryInstancesDataKHR->arrayOfPointers = **_arrayOfPointers;
    _AccelerationStructureGeometryInstancesDataKHR->data = **_data;
  }
  return _AccelerationStructureGeometryInstancesDataKHR.get();
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    GetMappedPointers() {
  return _data->GetMappedPointers();
}
