// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanStructStorageAuto.h"
#include "vulkanLog.h"
#include "vulkanTools_lite.h"
#include "gits.h"

gits::Vulkan::CBinaryResourceData::CBinaryResourceData(TResourceType _,
                                                       const void* data,
                                                       size_t size)
    : _data((uint8_t*)data, (uint8_t*)data + size) {}

gits::Vulkan::CBinaryResourceData::PointerProxy gits::Vulkan::CBinaryResourceData::Data() const {
  return PointerProxy(_data.data(), _data.size());
}

gits::Vulkan::CVkGenericArgumentData::~CVkGenericArgumentData() {
  delete _argument;
}

gits::Vulkan::CVkGenericArgumentData::CVkGenericArgumentData(const void* vkgenericargumentdata)
    : _argument(nullptr) {
  vkgenericargumentdata = ignoreLoaderSpecificStructureTypes(vkgenericargumentdata);

  if (vkgenericargumentdata) {
    switch (*(VkStructureType*)vkgenericargumentdata) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = new C##structure##Data((structure*)vkgenericargumentdata);                         \
    break;

#include "vulkanPNextWrappers.inl"

    default:
      VkLog(ERR) << "Unknown enum value: " << *(VkStructureType*)vkgenericargumentdata
                 << " for CVkGenericArgumentData";
      const void* pNext = ((const VkBaseInStructure*)vkgenericargumentdata)->pNext;
      while (pNext != nullptr) {
        VkLog(WARN) << "Additional structure pointed to by pNext: " << *(VkStructureType*)pNext;
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

gits::Vulkan::CVkClearColorValueData::CVkClearColorValueData(
    const VkClearColorValue* clearcolorvalue)
    : _ClearColorValue(nullptr), _isNullPtr(clearcolorvalue == nullptr) {
  if (!*_isNullPtr) {
    _uint32 = new Cuint32_tDataArray(4, clearcolorvalue->uint32);
  } else {
    _uint32 = nullptr;
  }
}
gits::Vulkan::CVkClearColorValueData::~CVkClearColorValueData() {
  delete _uint32;
  delete _ClearColorValue;
}
VkClearColorValue* gits::Vulkan::CVkClearColorValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearColorValue == nullptr) {
    _ClearColorValue = new VkClearColorValue;
    auto uint32Values = **_uint32;
    if (uint32Values != nullptr) {
      for (int i = 0; i < 4; i++) {
        _ClearColorValue->uint32[i] = uint32Values[i];
      }
    } else {
      Log(ERR) << "The pointer to array **_uint32 is null.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return _ClearColorValue;
}

gits::Vulkan::CVkClearValueData::CVkClearValueData(const VkClearValue* clearvalue)
    : _ClearValue(nullptr), _isNullPtr(clearvalue == nullptr) {
  if (!*_isNullPtr) {
    _color = new CVkClearColorValueData(&clearvalue->color);
  } else {
    _color = nullptr;
  }
}
gits::Vulkan::CVkClearValueData::~CVkClearValueData() {
  delete _color;
  delete _ClearValue;
}
VkClearValue* gits::Vulkan::CVkClearValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearValue == nullptr) {
    _ClearValue = new VkClearValue;
    _ClearValue->color = **_color;
  }
  return _ClearValue;
}

gits::Vulkan::CVkDescriptorImageInfoData::CVkDescriptorImageInfoData(
    const VkDescriptorImageInfo* descriptorimageinfo, const VkDescriptorType)
    : _DescriptorImageInfo(nullptr), _isNullPtr(descriptorimageinfo == nullptr) {
  if (!*_isNullPtr) {
    _sampler = new CVkSamplerData(descriptorimageinfo->sampler);
    _imageView = new CVkImageViewData(descriptorimageinfo->imageView);
    _imageLayout = new CVkImageLayoutData(descriptorimageinfo->imageLayout);
  } else {
    _sampler = nullptr;
    _imageView = nullptr;
    _imageLayout = nullptr;
  }
}
gits::Vulkan::CVkDescriptorImageInfoData::~CVkDescriptorImageInfoData() {
  delete _sampler;
  delete _imageView;
  delete _imageLayout;
  delete _DescriptorImageInfo;
}
VkDescriptorImageInfo* gits::Vulkan::CVkDescriptorImageInfoData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_DescriptorImageInfo == nullptr) {
    _DescriptorImageInfo = new VkDescriptorImageInfo;
    _DescriptorImageInfo->sampler = **_sampler;
    _DescriptorImageInfo->imageView = **_imageView;
    _DescriptorImageInfo->imageLayout = **_imageLayout;
  }
  return _DescriptorImageInfo;
}

gits::Vulkan::CBufferDeviceAddressObjectData::CBufferDeviceAddressObjectData(
    VkDeviceAddress deviceAddress) {
  _deviceAddress = new Cuint64_tData(deviceAddress);
}

gits::Vulkan::CBufferDeviceAddressObjectData::~CBufferDeviceAddressObjectData() {
  delete _deviceAddress;
}

VkDeviceAddress gits::Vulkan::CBufferDeviceAddressObjectData::Value() {
  return **_deviceAddress;
}

gits::Vulkan::CcharDataArray::~CcharDataArray() {
  delete stringData;
}
