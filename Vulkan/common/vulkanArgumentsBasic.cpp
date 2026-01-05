// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLog.h"
#include "vulkanStateTracking.h"

template <>
const char* gits::Vulkan::CVulkanObj<HWND, gits::Vulkan::HWNDTypeTag>::NAME = "HWND";

template <>
const char* gits::Vulkan::CVulkanObj<HINSTANCE, gits::Vulkan::HINSTANCETypeTag>::NAME = "HINSTANCE";

template <>
const char* gits::Vulkan::CVulkanObj<HMONITOR, gits::Vulkan::HMONITORTypeTag>::NAME = "HMONITOR";

template <>
const char* gits::Vulkan::CVulkanObj<D3DKMT_HANDLE, gits::Vulkan::D3DKMT_HANDLETypeTag>::NAME =
    "D3DKMT_HANDLE";

template <>
const char*
    gits::Vulkan::CVulkanObj<xcb_connection_t*, gits::Vulkan::xcb_connection_t_TypeTag>::NAME =
        "xcb_connection_t*";

template <>
const char* gits::Vulkan::CVulkanObj<Display*, gits::Vulkan::VkDisplay_TypeTag>::NAME = "Display*";

template <>
const char* gits::Vulkan::CVulkanObj<void*, gits::Vulkan::VkWindow_TypeTag>::NAME = "Window";

gits::Vulkan::CVkClearColorValue::CVkClearColorValue()
    : _uint32(std::make_unique<Cuint32_t::CSArray>()),
      _ClearColorValue(nullptr),
      _ClearColorValueOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkClearColorValue::CVkClearColorValue(const VkClearColorValue* clearcolorvalue)
    : _ClearColorValue(nullptr),
      _ClearColorValueOriginal(nullptr),
      _isNullPtr(clearcolorvalue == nullptr) {
  if (!*_isNullPtr) {
    _uint32 = std::make_unique<Cuint32_t::CSArray>(4, clearcolorvalue->uint32);
  } else {
    _uint32 = nullptr;
  }
}

VkClearColorValue* gits::Vulkan::CVkClearColorValue::Value() {
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

gits::PtrConverter<VkClearColorValue> gits::Vulkan::CVkClearColorValue::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkClearColorValue>(nullptr);
  }
  if (_ClearColorValueOriginal == nullptr) {
    _ClearColorValueOriginal = std::make_unique<VkClearColorValue>();
    auto uint32ValuesOriginal = _uint32->Original();
    if (uint32ValuesOriginal != nullptr) {
      for (int i = 0; i < 4; i++) {
        _ClearColorValueOriginal->uint32[i] = uint32ValuesOriginal[i];
      }
    } else {
      LOG_ERROR << "The pointer to array _uint32->Original() is null.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return PtrConverter<VkClearColorValue>(_ClearColorValueOriginal.get());
}

void gits::Vulkan::CVkClearColorValue::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _uint32->Write(stream);
  }
}

void gits::Vulkan::CVkClearColorValue::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _uint32->Read(stream);
  }
}

gits::Vulkan::CVkClearValue::CVkClearValue()
    : _color(std::make_unique<CVkClearColorValue>()),
      _ClearValue(nullptr),
      _ClearValueOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkClearValue::CVkClearValue(const VkClearValue* clearvalue)
    : _ClearValue(nullptr), _ClearValueOriginal(nullptr), _isNullPtr(clearvalue == nullptr) {
  if (!*_isNullPtr) {
    _color = std::make_unique<CVkClearColorValue>(&clearvalue->color);
  } else {
    _color = nullptr;
  }
}

const char* gits::Vulkan::CVkClearValue::NAME = "VkClearValue";

VkClearValue* gits::Vulkan::CVkClearValue::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearValue == nullptr) {
    _ClearValue = std::make_unique<VkClearValue>();
    _ClearValue->color = **_color;
  }
  return _ClearValue.get();
}

gits::PtrConverter<VkClearValue> gits::Vulkan::CVkClearValue::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkClearValue>(nullptr);
  }
  if (_ClearValueOriginal == nullptr) {
    _ClearValueOriginal = std::make_unique<VkClearValue>();
    _ClearValueOriginal->color = _color->Original();
  }
  return PtrConverter<VkClearValue>(_ClearValueOriginal.get());
}

void gits::Vulkan::CVkClearValue::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _color->Write(stream);
  }
}

void gits::Vulkan::CVkClearValue::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _color->Read(stream);
  }
}

gits::Vulkan::CVkGenericArgument::CVkGenericArgument()
    : _sType(std::make_unique<CVkStructureType>()),
      _argument(nullptr),
      _skipped(std::make_unique<Cbool>()),
      _isNullPtr(false) {}

void gits::Vulkan::CVkGenericArgument::InitArgument(uint32_t type) {
  switch (type) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = std::make_unique<C##structure##__VA_ARGS__>();                                     \
    break;

#define PNEXT_EXTENDED_WRAPPER(STRUCTURE_TYPE, structure, ...)                                     \
  case STRUCTURE_TYPE:                                                                             \
    _argument = std::make_unique<C##structure##__VA_ARGS__>();                                     \
    break;

#include "vulkanPNextWrappers.inl"

  default:
    _skipped = std::make_unique<Cbool>(true);
    LOG_ERROR << "Unknown enum value: " << type << " for CVkGenericArgument";
    break;
  }
}

void gits::Vulkan::CVkGenericArgument::CreateArgument(const void* pVkGenericArgument,
                                                      const void* pCustomData) {
  _sType = std::make_unique<CVkStructureType>(*(uint32_t*)pVkGenericArgument);
  switch (**_sType) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = std::make_unique<C##structure##__VA_ARGS__>((structure*)pVkGenericArgument);       \
    break;

#define PNEXT_EXTENDED_WRAPPER(STRUCTURE_TYPE, structure, ...)                                     \
  case STRUCTURE_TYPE:                                                                             \
    _argument =                                                                                    \
        std::make_unique<C##structure##__VA_ARGS__>((structure*)pVkGenericArgument, pCustomData);  \
    break;

#include "vulkanPNextWrappers.inl"

  default:
    _skipped = std::make_unique<Cbool>(true);
    LOG_ERROR << "Unknown enum value: " << **_sType << " for CVkGenericArgument";
    break;
  }
}

gits::Vulkan::CVkGenericArgument::CVkGenericArgument(const void* vkgenericargument)
    : _skipped(nullptr), _isNullPtr(vkgenericargument == nullptr) {
  vkgenericargument = ignoreLoaderSpecificStructureTypes(vkgenericargument);

  if (!*_isNullPtr) {
    CreateArgument(vkgenericargument);
    if (!_skipped) {
      _skipped = std::make_unique<Cbool>(false);
    }
  } else {
    _sType = nullptr;
    _argument = nullptr;
  }
}

const void* gits::Vulkan::CVkGenericArgument::Value() {
  if (*_isNullPtr || **_skipped) {
    return nullptr;
  }

  return _argument->GetPtrType();
}

void gits::Vulkan::CVkGenericArgument::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _sType->Write(stream);
    _skipped->Write(stream);
    if (!**_skipped) {
      _argument->Write(stream);
    }
  }
}

void gits::Vulkan::CVkGenericArgument::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _sType->Read(stream);
    _skipped->Read(stream);
    if (!**_skipped) {
      InitArgument(**_sType);
      _argument->Read(stream);
    } else {
      LOG_ERROR << "Due to unknown enum value: " << **_sType
                << " structure was not recorded into CVkGenericArgument.";
    }
  }
}

const char* gits::Vulkan::CVkGenericArgument::Name() const {
  if (!*_isNullPtr) {
    return _argument->Name();
  } else {
    return "";
  }
}

gits::Vulkan::CUpdateDescriptorSetWithTemplateArray::CUpdateDescriptorSetWithTemplateArray(
    VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData) {
  if (pData == nullptr) {
    return;
  }

  auto& descriptorUpdateTemplateSD = SD()._descriptorupdatetemplatestates[descriptorUpdateTemplate];
  auto pDescriptorUpdateTemplateCreateInfo =
      descriptorUpdateTemplateSD->descriptorUpdateTemplateCreateInfoData.Value();
  std::uint64_t max_length = 0;
  for (unsigned i = 0; i < pDescriptorUpdateTemplateCreateInfo->descriptorUpdateEntryCount; i++) {
    auto& descriptorUpdateEntry = pDescriptorUpdateTemplateCreateInfo->pDescriptorUpdateEntries[i];
    for (unsigned j = 0; j < descriptorUpdateEntry.descriptorCount; j++) {
      auto obj = std::make_shared<CDescriptorUpdateTemplateObject>(
          descriptorUpdateEntry.descriptorType, pData,
          descriptorUpdateEntry.offset + descriptorUpdateEntry.stride * j);
      if (obj->GetOffset() + obj->GetSize() > max_length) {
        max_length = obj->GetOffset() + obj->GetSize();
      }
      _cgenericargsDict.push_back(std::move(obj));
    }
  }
  _size = std::make_unique<Cuint64_t>(max_length);
}

const void* gits::Vulkan::CUpdateDescriptorSetWithTemplateArray::Value() {
  if (_cgenericargsDict.size() == 0) {
    return nullptr;
  }
  _data.clear();
  _data.resize((size_t) * *_size);
  for (auto& arg : _cgenericargsDict) {
    memcpy(&_data[0] + arg->GetOffset(), arg->Value(), (size_t)arg->GetSize());
  }
  return &_data[0];
}

void gits::Vulkan::CUpdateDescriptorSetWithTemplateArray::Read(CBinIStream& stream) {
  unsigned dictSize = 0;
  read_name_from_stream(stream, dictSize);
  _size->Read(stream);
  if (dictSize <= _cgenericargsDict.max_size()) {
    for (unsigned i = 0; i < dictSize; i++) {
      auto keyArgPtr = std::make_shared<CDescriptorUpdateTemplateObject>();
      keyArgPtr->Read(stream);
      _cgenericargsDict.push_back(std::move(keyArgPtr));
    }
  } else {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}

gits::Vulkan::CDescriptorUpdateTemplateObject::CDescriptorUpdateTemplateObject(
    VkDescriptorType descType, const void* pData, std::uint64_t offset)
    : _descType(std::make_unique<CVkDescriptorType>(descType)),
      _offset(std::make_unique<Cuint64_t>(offset)) {

  switch (descType) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    _size = std::make_unique<Cuint64_t>(sizeof(VkDescriptorBufferInfo));
    _argument =
        std::make_unique<CVkDescriptorBufferInfo>((VkDescriptorBufferInfo*)((char*)pData + offset));
    break;
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    _size = std::make_unique<Cuint64_t>(sizeof(VkBufferView));
    _argument = std::make_unique<CVkBufferView>((VkBufferView*)((char*)pData + offset));
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    _size = std::make_unique<Cuint64_t>(sizeof(VkDescriptorImageInfo));
    _argument = std::make_unique<CVkDescriptorImageInfo>(
        (VkDescriptorImageInfo*)((char*)pData + offset), descType);
    break;
  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    _size = std::make_unique<Cuint64_t>(sizeof(VkAccelerationStructureKHR));
    _argument = std::make_unique<CVkAccelerationStructureKHR>(
        (VkAccelerationStructureKHR*)((char*)pData + offset));
    break;
  default:
    LOG_WARNING << "Unknown descriptor type " << descType
                << " in CDescriptorUpdateTemplateObject constructor!";
    _size = nullptr;
    _argument = nullptr;
  }
}

void gits::Vulkan::CDescriptorUpdateTemplateObject::Read(CBinIStream& stream) {
  _descType->Read(stream);
  _offset->Read(stream);
  _size->Read(stream);
  switch (**_descType) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    _argument = std::make_unique<CVkDescriptorBufferInfo>();
    break;
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    _argument = std::make_unique<CVkBufferView>();
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    _argument = std::make_unique<CVkDescriptorImageInfo>();
    break;
  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    _argument = std::make_unique<CVkAccelerationStructureKHR>();
    break;
  default:
    LOG_TRACE << "Not handled CDescriptorUpdateTemplateObject enumeration: " +
                     std::string(_descType->Name());
    break;
  }
  _argument->Read(stream);
}

gits::Vulkan::CVkPipelineCacheCreateInfo_V1::CVkPipelineCacheCreateInfo_V1()
    : _sType(std::make_unique<CVkStructureType>()),
      _pNext(std::make_unique<CpNextWrapper>()),
      _flags(std::make_unique<Cuint32_t>()),
      _initialDataSize(std::make_unique<Csize_t>()),
      _pInitialData(std::make_unique<CBinaryResource>()),
      _PipelineCacheCreateInfo(nullptr),
      _PipelineCacheCreateInfoOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkPipelineCacheCreateInfo_V1::CVkPipelineCacheCreateInfo_V1(
    const VkPipelineCacheCreateInfo* pipelinecachecreateinfo)
    : _PipelineCacheCreateInfo(nullptr),
      _PipelineCacheCreateInfoOriginal(nullptr),
      _isNullPtr(pipelinecachecreateinfo == nullptr) {
  if (!*_isNullPtr) {
    _sType = std::make_unique<CVkStructureType>(pipelinecachecreateinfo->sType);
    _pNext = std::make_unique<CpNextWrapper>(pipelinecachecreateinfo->pNext);
    _flags = std::make_unique<Cuint32_t>(pipelinecachecreateinfo->flags);
    _initialDataSize = std::make_unique<Csize_t>(pipelinecachecreateinfo->initialDataSize);
    _pInitialData =
        std::make_unique<CBinaryResource>(RESOURCE_DATA_RAW, pipelinecachecreateinfo->pInitialData,
                                          pipelinecachecreateinfo->initialDataSize);
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _flags = nullptr;
    _initialDataSize = nullptr;
    _pInitialData = nullptr;
  }
}

const char* gits::Vulkan::CVkPipelineCacheCreateInfo_V1::NAME = "VkPipelineCacheCreateInfo";

VkPipelineCacheCreateInfo* gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_PipelineCacheCreateInfo == nullptr) {
    _PipelineCacheCreateInfo = std::make_unique<VkPipelineCacheCreateInfo>();
    _PipelineCacheCreateInfo->sType = **_sType;
    _PipelineCacheCreateInfo->pNext = **_pNext;
    _PipelineCacheCreateInfo->flags = **_flags;
    _PipelineCacheCreateInfo->initialDataSize = **_initialDataSize;
    _initialData.resize(**_initialDataSize);
    memcpy(_initialData.data(), **_pInitialData, **_initialDataSize);
    _PipelineCacheCreateInfo->pInitialData = _initialData.data();
  }
  return _PipelineCacheCreateInfo.get();
}

gits::PtrConverter<VkPipelineCacheCreateInfo> gits::Vulkan::CVkPipelineCacheCreateInfo_V1::
    Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkPipelineCacheCreateInfo>(nullptr);
  }
  if (_PipelineCacheCreateInfoOriginal == nullptr) {
    _PipelineCacheCreateInfoOriginal = std::make_unique<VkPipelineCacheCreateInfo>();
    _PipelineCacheCreateInfoOriginal->sType = _sType->Original();
    _PipelineCacheCreateInfoOriginal->pNext = _pNext->Original();
    _PipelineCacheCreateInfoOriginal->flags = _flags->Original();
    _PipelineCacheCreateInfoOriginal->initialDataSize = _initialDataSize->Original();
    _initialData.resize(**_initialDataSize);
    memcpy(_initialData.data(), **_pInitialData, **_initialDataSize);
    _PipelineCacheCreateInfoOriginal->pInitialData = _initialData.data();
  }
  return PtrConverter<VkPipelineCacheCreateInfo>(_PipelineCacheCreateInfoOriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkPipelineCacheCreateInfo_V1::GetMappedPointers() {

  std::set<uint64_t> returnMap;
  return returnMap;
}

void gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _sType->Write(stream);
    _pNext->Write(stream);
    _flags->Write(stream);
    _initialDataSize->Write(stream);
    _pInitialData->Write(stream);
  }
}

void gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _sType->Read(stream);
    _pNext->Read(stream);
    _flags->Read(stream);
    _initialDataSize->Read(stream);
    _pInitialData->Read(stream);
  }
}

gits::Vulkan::CBufferDeviceAddressObject::CBufferDeviceAddressObject(VkDeviceAddress deviceAddress)
    : _deviceAddress(0) {
  VkBuffer buffer = VK_NULL_HANDLE;
  int64_t offset = 0;

  if (deviceAddress != 0) {
    buffer = findBufferFromDeviceAddress(deviceAddress);
    if (buffer) {
      offset = deviceAddress - SD()._bufferstates[buffer]->deviceAddress;
    }
  }

  _originalDeviceAddress = std::make_unique<Cuint64_t>(deviceAddress);
  _buffer = std::make_unique<CVkBuffer>(buffer);
  _offset = std::make_unique<Cint64_t>(offset);
}

gits::Vulkan::CBufferDeviceAddressObject::CBufferDeviceAddressObject(
    const gits::Vulkan::CBufferDeviceAddressObjectData& bufferDeviceAddressObject)
    : _deviceAddress(0) {
  _originalDeviceAddress =
      std::make_unique<Cuint64_t>(bufferDeviceAddressObject._originalDeviceAddress);
  _buffer = std::make_unique<CVkBuffer>(bufferDeviceAddressObject._buffer);
  _offset = std::make_unique<Cint64_t>(bufferDeviceAddressObject._offset);
}

VkDeviceAddress gits::Vulkan::CBufferDeviceAddressObject::Value() {
  if (!_deviceAddress) {
    _deviceAddress = **_originalDeviceAddress;

    if (_deviceAddress != 0) {
      auto buffer = **_buffer;

      if (buffer) {
        _deviceAddress = SD()._bufferstates[buffer]->deviceAddress + **_offset;
      }
    }
  }

  return _deviceAddress;
}

std::set<uint64_t> gits::Vulkan::CBufferDeviceAddressObject::GetMappedPointers() {
  return _buffer->GetMappedPointers();
}

gits::Vulkan::CVkDeviceOrHostAddressConstKHR::CVkDeviceOrHostAddressConstKHR(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    const CVkDeviceOrHostAddressConstKHRData& deviceOrHostAddressData)
    : _commandExecutionSideGITS(std::make_unique<CVkCommandExecutionSideGITS>(
          deviceOrHostAddressData._controlData.executionSide)),
      _bufferDeviceAddress(std::make_unique<CBufferDeviceAddressObject>()),
      _dataSize(std::make_unique<Csize_t>(0)) {
  if (deviceorhostaddress.deviceAddress == 0) {
    return;
  }

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    _bufferDeviceAddress =
        std::make_unique<CBufferDeviceAddressObject>(deviceOrHostAddressData._bufferDeviceAddress);
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    // Host operations are not tested due to lack of applications using them.
    // Below code will be uncommented as soon as there are any applications
    // which use host operations.

    //_dataSize = std::make_unique<Csize_t>(deviceOrHostAddressData->_dataSize);
    //_data.resize(deviceOrHostAddressData->_dataSize);
    //memcpy(_data.data(), deviceOrHostAddressData->_inputData.data(),
    //       deviceOrHostAddressData->_dataSize);
    //_resource = std::make_unique<CBinaryResource>(hash);
    break;
  }
}

VkDeviceOrHostAddressConstKHR* gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Value() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (!_DeviceOrHostAddress) {
    _DeviceOrHostAddress = std::make_unique<VkDeviceOrHostAddressConstKHR>();

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddress->deviceAddress = _bufferDeviceAddress->Value();
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      // Host operations are not tested due to lack of applications using them.
      // Below code will be uncommented as soon as there are any applications
      // which use host operations.

      //auto dataSize = **_dataSize;
      //
      //if (dataSize != 0) {
      //  if (_data.size() == 0) {
      //    _data.resize(dataSize);
      //    memcpy(_data.data(), **_resource, dataSize);
      //  }
      //}
      //
      //_DeviceOrHostAddress->hostAddress = _data.data();
      break;
    }
  }
  return _DeviceOrHostAddress.get();
}

gits::PtrConverter<VkDeviceOrHostAddressConstKHR> gits::Vulkan::CVkDeviceOrHostAddressConstKHR::
    Original() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (!_DeviceOrHostAddressOriginal) {
    _DeviceOrHostAddressOriginal = std::make_unique<VkDeviceOrHostAddressConstKHR>();

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
      _DeviceOrHostAddressOriginal->deviceAddress = _bufferDeviceAddress->Original();
      break;
    }
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      _DeviceOrHostAddressOriginal->hostAddress = _data.data();
      break;
    }
  }

  return PtrConverter<VkDeviceOrHostAddressConstKHR>(_DeviceOrHostAddressOriginal.get());
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Write(CBinOStream& stream) const {
  _commandExecutionSideGITS->Write(stream);

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress->Write(stream);
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    // Host operations are not tested due to lack of applications using them.
    // Below code will be uncommented as soon as there are any applications
    // which use host operations.

    //_dataSize->Write(stream);
    //if (**_dataSize) {
    //  _resource->Write(stream);
    //  CGits::Instance().ResourceManager().put(RESOURCE_DATA_RAW, _data.data(), **_dataSize,
    //                                          _resource->GetResourceHash());
    //}
    break;
  }
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Read(CBinIStream& stream) {
  _commandExecutionSideGITS->Read(stream);

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress->Read(stream);
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    // Host operations are not tested due to lack of applications using them.
    // Below code will be uncommented as soon as there are any applications
    // which use host operations.

    //_dataSize->Read(stream);
    //if (**_dataSize) {
    //  _resource->Read(stream);
    //}
    break;
  }
}

// CVkDeviceOrHostAddressKHR

gits::Vulkan::CVkDeviceOrHostAddressKHR::CVkDeviceOrHostAddressKHR(
    const VkDeviceOrHostAddressKHR deviceorhostaddress,
    const CVkDeviceOrHostAddressKHRData& deviceOrHostAddressData)
    : _commandExecutionSideGITS(std::make_unique<CVkCommandExecutionSideGITS>(
          deviceOrHostAddressData._controlData.executionSide)),
      _bufferDeviceAddress(std::make_unique<CBufferDeviceAddressObject>()) {
  if (deviceorhostaddress.deviceAddress == 0) {
    return;
  }

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    _bufferDeviceAddress =
        std::make_unique<CBufferDeviceAddressObject>(deviceOrHostAddressData._bufferDeviceAddress);
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    throw std::runtime_error("Ray tracing operations on host are not yet supported!");
    break;
  }
}

VkDeviceOrHostAddressKHR* gits::Vulkan::CVkDeviceOrHostAddressKHR::Value() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (!_DeviceOrHostAddress) {
    _DeviceOrHostAddress = std::make_unique<VkDeviceOrHostAddressKHR>();

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddress->deviceAddress = _bufferDeviceAddress->Value();
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      throw std::runtime_error("Ray tracing operations on host are not yet supported!");
      break;
    }
  }
  return _DeviceOrHostAddress.get();
}

gits::PtrConverter<VkDeviceOrHostAddressKHR> gits::Vulkan::CVkDeviceOrHostAddressKHR::Original() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (!_DeviceOrHostAddressOriginal) {
    _DeviceOrHostAddressOriginal = std::make_unique<VkDeviceOrHostAddressKHR>();

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
      _DeviceOrHostAddressOriginal->deviceAddress = _bufferDeviceAddress->Original();
      break;
    }
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      throw std::runtime_error("Ray tracing operations on host are not yet supported!");
      break;
    }
  }

  return PtrConverter<VkDeviceOrHostAddressKHR>(_DeviceOrHostAddressOriginal.get());
}

void gits::Vulkan::CVkDeviceOrHostAddressKHR::Write(CBinOStream& stream) const {
  _commandExecutionSideGITS->Write(stream);

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress->Write(stream);
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    throw std::runtime_error("Ray tracing operations on host are not yet supported!");
    break;
  }
}

void gits::Vulkan::CVkDeviceOrHostAddressKHR::Read(CBinIStream& stream) {
  _commandExecutionSideGITS->Read(stream);

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress->Read(stream);
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    throw std::runtime_error("Ray tracing operations on host are not yet supported!");
    break;
  }
}

gits::Vulkan::CVkDependencyInfo::CVkDependencyInfo()
    : _sType(std::make_unique<CVkStructureType>()),
      _pNext(std::make_unique<CpNextWrapper>()),
      _dependencyFlags(std::make_unique<Cuint32_t>()),
      _memoryBarrierCount(std::make_unique<Cuint32_t>()),
      _pMemoryBarriers(std::make_unique<CVkMemoryBarrier2Array>()),
      _bufferMemoryBarrierCount(std::make_unique<Cuint32_t>()),
      _pBufferMemoryBarriers(std::make_unique<CVkBufferMemoryBarrier2Array>()),
      _imageMemoryBarrierCount(std::make_unique<Cuint32_t>()),
      _pImageMemoryBarriers(std::make_unique<CVkImageMemoryBarrier2Array>()),
      _DependencyInfo(nullptr),
      _DependencyInfoOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkDependencyInfo::CVkDependencyInfo(const VkDependencyInfo* dependencyinfo)
    : _DependencyInfo(nullptr),
      _DependencyInfoOriginal(nullptr),
      _isNullPtr(dependencyinfo == nullptr) {
  if (!*_isNullPtr) {
    _sType = std::make_unique<CVkStructureType>(dependencyinfo->sType);
    _pNext = std::make_unique<CpNextWrapper>(dependencyinfo->pNext);
    _dependencyFlags = std::make_unique<Cuint32_t>(dependencyinfo->dependencyFlags);
    _memoryBarrierCount = std::make_unique<Cuint32_t>(dependencyinfo->memoryBarrierCount);
    _pMemoryBarriers = std::make_unique<CVkMemoryBarrier2Array>(dependencyinfo->memoryBarrierCount,
                                                                dependencyinfo->pMemoryBarriers);
    _bufferMemoryBarrierCount =
        std::make_unique<Cuint32_t>(dependencyinfo->bufferMemoryBarrierCount);
    _pBufferMemoryBarriers = std::make_unique<CVkBufferMemoryBarrier2Array>(
        dependencyinfo->bufferMemoryBarrierCount, dependencyinfo->pBufferMemoryBarriers);
    _imageMemoryBarrierCount = std::make_unique<Cuint32_t>(dependencyinfo->imageMemoryBarrierCount);
    _pImageMemoryBarriers = std::make_unique<CVkImageMemoryBarrier2Array>(
        dependencyinfo->imageMemoryBarrierCount, dependencyinfo->pImageMemoryBarriers);
  } else {
    _sType = nullptr;
    _pNext = nullptr;
    _dependencyFlags = nullptr;
    _memoryBarrierCount = nullptr;
    _pMemoryBarriers = nullptr;
    _bufferMemoryBarrierCount = nullptr;
    _pBufferMemoryBarriers = nullptr;
    _imageMemoryBarrierCount = nullptr;
    _pImageMemoryBarriers = nullptr;
  }
}

const char* gits::Vulkan::CVkDependencyInfo::NAME = "VkDependencyInfo";

VkDependencyInfo* gits::Vulkan::CVkDependencyInfo::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_DependencyInfo == nullptr) {
    _DependencyInfo = std::make_unique<VkDependencyInfo>();
    _DependencyInfo->sType = **_sType;
    _DependencyInfo->pNext = **_pNext;
    _DependencyInfo->dependencyFlags = **_dependencyFlags;
    _DependencyInfo->memoryBarrierCount = **_memoryBarrierCount;
    _DependencyInfo->pMemoryBarriers = **_pMemoryBarriers;
    _DependencyInfo->bufferMemoryBarrierCount = **_bufferMemoryBarrierCount;
    _DependencyInfo->pBufferMemoryBarriers = **_pBufferMemoryBarriers;
    _DependencyInfo->imageMemoryBarrierCount = **_imageMemoryBarrierCount;
    _DependencyInfo->pImageMemoryBarriers = **_pImageMemoryBarriers;
  }
  return _DependencyInfo.get();
}

gits::PtrConverter<VkDependencyInfo> gits::Vulkan::CVkDependencyInfo::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkDependencyInfo>(nullptr);
  }
  if (_DependencyInfoOriginal == nullptr) {
    _DependencyInfoOriginal = std::make_unique<VkDependencyInfo>();
    _DependencyInfoOriginal->sType = _sType->Original();
    _DependencyInfoOriginal->pNext = _pNext->Original();
    _DependencyInfoOriginal->dependencyFlags = _dependencyFlags->Original();
    _DependencyInfoOriginal->memoryBarrierCount = _memoryBarrierCount->Original();
    _DependencyInfoOriginal->pMemoryBarriers = _pMemoryBarriers->Original();
    _DependencyInfoOriginal->bufferMemoryBarrierCount = _bufferMemoryBarrierCount->Original();
    _DependencyInfoOriginal->pBufferMemoryBarriers = _pBufferMemoryBarriers->Original();
    _DependencyInfoOriginal->imageMemoryBarrierCount = _imageMemoryBarrierCount->Original();
    _DependencyInfoOriginal->pImageMemoryBarriers = _pImageMemoryBarriers->Original();
  }
  return PtrConverter<VkDependencyInfo>(_DependencyInfoOriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkDependencyInfo::GetMappedPointers() {

  std::set<uint64_t> returnMap;
  for (auto obj : _pMemoryBarriers->GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _pBufferMemoryBarriers->GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _pImageMemoryBarriers->GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  return returnMap;
}

void gits::Vulkan::CVkDependencyInfo::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _sType->Write(stream);
    _pNext->Write(stream);
    _dependencyFlags->Write(stream);
    _memoryBarrierCount->Write(stream);
    _pMemoryBarriers->Write(stream);
    _bufferMemoryBarrierCount->Write(stream);
    _pBufferMemoryBarriers->Write(stream);
    _imageMemoryBarrierCount->Write(stream);
    _pImageMemoryBarriers->Write(stream);
  }
}

void gits::Vulkan::CVkDependencyInfo::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _sType->Read(stream);
    _pNext->Read(stream);
    _dependencyFlags->Read(stream);
    _memoryBarrierCount->Read(stream);
    _pMemoryBarriers->Read(stream);
    _bufferMemoryBarrierCount->Read(stream);
    _pBufferMemoryBarriers->Read(stream);
    _imageMemoryBarrierCount->Read(stream);
    _pImageMemoryBarriers->Read(stream);
  }
}

uint32_t gits::Vulkan::CVkDependencyInfo::GetMemoryBarrierCount() const {
  return **_memoryBarrierCount;
}

uint32_t gits::Vulkan::CVkDependencyInfo::GetBufferMemoryBarrierCount() const {
  return **_bufferMemoryBarrierCount;
}

uint32_t gits::Vulkan::CVkDependencyInfo::GetImageMemoryBarrierCount() const {
  return **_imageMemoryBarrierCount;
}

gits::Vulkan::CpNextWrapper::CpNextWrapper(const void* ptr) : _ptr(0) {
  ptr = ignoreLoaderSpecificStructureTypes(ptr);
  if (ptr) {
    _ptr = (std::uint64_t)ptr;
    _data = std::make_unique<CVkGenericArgument>(ptr);
  } else {
    _data = nullptr;
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::
    CVkAccelerationStructureGeometryInstancesDataKHR(
        const VkAccelerationStructureGeometryInstancesDataKHR*
            accelerationstructuregeometryinstancesdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _AccelerationStructureGeometryInstancesDataKHR(nullptr),
      _AccelerationStructureGeometryInstancesDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometryinstancesdatakhr == nullptr) {
  if (*_isNullPtr) {
    return;
  }

  if (buildRangeInfo.primitiveCount == 0) {
    _isNullPtr = true;
    return;
  }

  auto* structStoragePointer = (VkStructStoragePointerGITS*)getPNextStructure(
      accelerationstructuregeometryinstancesdatakhr->pNext,
      VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS);
  if (!structStoragePointer || !structStoragePointer->pStructStorage) {
    _isNullPtr = true;
    return;
  }

  auto* geometryInstancesData =
      (CVkAccelerationStructureGeometryInstancesDataKHRData*)structStoragePointer->pStructStorage;

  // Revert pNext pointer to the original value (see CVkAccelerationStructureGeometryInstancesDataKHRData() constructor)
  if (!isSubcaptureBeforeRestorationPhase()) {
    const_cast<VkAccelerationStructureGeometryInstancesDataKHR*>(
        accelerationstructuregeometryinstancesdatakhr)
        ->pNext = structStoragePointer->pNext;
  }

  _sType = std::make_unique<CVkStructureType>(accelerationstructuregeometryinstancesdatakhr->sType);
  _pNext = std::make_unique<CpNextWrapper>(accelerationstructuregeometryinstancesdatakhr->pNext);
  _commandExecutionSideGITS =
      std::make_unique<CVkCommandExecutionSideGITS>(controlData.executionSide);
  _arrayOfPointers =
      std::make_unique<Cuint32_t>(accelerationstructuregeometryinstancesdatakhr->arrayOfPointers);

  switch (controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    _bufferDeviceAddress =
        std::make_unique<CBufferDeviceAddressObject>(geometryInstancesData->_bufferDeviceAddress);
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    throw std::runtime_error("Ray tracing operations on host are not yet supported!");
    break;
  }
}

VkAccelerationStructureGeometryInstancesDataKHR* gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHR::Value() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (_AccelerationStructureGeometryInstancesDataKHR == nullptr) {
    VkDeviceOrHostAddressConstKHR address = {0};

    if (!*_isNullPtr) {
      switch (**_commandExecutionSideGITS) {
      case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
        address.deviceAddress = **_bufferDeviceAddress;
        break;
      case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
        throw std::runtime_error("Ray tracing operations on host are not yet supported!");
        break;
      }
    }

    _AccelerationStructureGeometryInstancesDataKHR =
        std::make_unique<VkAccelerationStructureGeometryInstancesDataKHR>();
    _AccelerationStructureGeometryInstancesDataKHR->sType = **_sType;
    _AccelerationStructureGeometryInstancesDataKHR->pNext = **_pNext;
    _AccelerationStructureGeometryInstancesDataKHR->arrayOfPointers = **_arrayOfPointers;
    _AccelerationStructureGeometryInstancesDataKHR->data = address;
  }
  return _AccelerationStructureGeometryInstancesDataKHR.get();
}

gits::PtrConverter<VkAccelerationStructureGeometryInstancesDataKHR> gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHR::Original() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (_AccelerationStructureGeometryInstancesDataKHROriginal == nullptr) {
    VkDeviceOrHostAddressConstKHR address = {0};

    if (!*_isNullPtr) {
      switch (**_commandExecutionSideGITS) {
      case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
        address.deviceAddress = _bufferDeviceAddress->Original();
        break;
      case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
        throw std::runtime_error("Ray tracing operations on host are not yet supported!");
        break;
      }
    }

    _AccelerationStructureGeometryInstancesDataKHROriginal =
        std::make_unique<VkAccelerationStructureGeometryInstancesDataKHR>();
    _AccelerationStructureGeometryInstancesDataKHROriginal->sType = _sType->Original();
    _AccelerationStructureGeometryInstancesDataKHROriginal->pNext = _pNext->Original();
    _AccelerationStructureGeometryInstancesDataKHROriginal->arrayOfPointers =
        _arrayOfPointers->Original();
    _AccelerationStructureGeometryInstancesDataKHROriginal->data = address;
  }
  return PtrConverter<VkAccelerationStructureGeometryInstancesDataKHR>(
      _AccelerationStructureGeometryInstancesDataKHROriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::
    GetMappedPointers() {
  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    return _bufferDeviceAddress->GetMappedPointers();
  default:
    return {};
  }
}

void gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::Write(
    CBinOStream& stream) const {
  _isNullPtr.Write(stream);

  if (!*_isNullPtr) {
    _sType->Write(stream);
    _pNext->Write(stream);
    _commandExecutionSideGITS->Write(stream);
    _arrayOfPointers->Write(stream);

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _bufferDeviceAddress->Write(stream);
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      throw std::runtime_error("Ray tracing operations on host are not yet supported!");
      break;
    }
  }
}

void gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);

  if (!*_isNullPtr) {
    _sType->Read(stream);
    _pNext->Read(stream);
    _commandExecutionSideGITS->Read(stream);
    _arrayOfPointers->Read(stream);

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _bufferDeviceAddress->Read(stream);
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      throw std::runtime_error("Ray tracing operations on host are not yet supported!");
      break;
    }
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::CVkAccelerationStructureGeometryDataKHR(
    VkGeometryTypeKHR geometryType,
    const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr,
    const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
    VkAccelerationStructureBuildControlDataGITS controlData)
    : _geometryType(nullptr),
      _triangles(nullptr),
      _aabbs(nullptr),
      _instances(nullptr),
      _AccelerationStructureGeometryDataKHR(nullptr),
      _AccelerationStructureGeometryDataKHROriginal(nullptr),
      _isNullPtr(accelerationstructuregeometrydatakhr == nullptr) {
  if (!*_isNullPtr) {
    _geometryType = std::make_unique<CVkGeometryTypeKHR>(geometryType);
    switch (geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _triangles = std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHR>(
          &accelerationstructuregeometrydatakhr->triangles, buildRangeInfo, controlData);
      _aabbs = std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHR>();
      _instances = std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHR>();
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _triangles = std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHR>();
      _aabbs = std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHR>(
          &accelerationstructuregeometrydatakhr->aabbs, buildRangeInfo, controlData);
      _instances = std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHR>();
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _triangles = std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHR>();
      _aabbs = std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHR>();
      _instances = std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHR>(
          &accelerationstructuregeometrydatakhr->instances, buildRangeInfo, controlData);
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
}

VkAccelerationStructureGeometryDataKHR* gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::
    Value() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
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
      _AccelerationStructureGeometryDataKHR->instances = **_instances;
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
  return _AccelerationStructureGeometryDataKHR.get();
}

gits::PtrConverter<VkAccelerationStructureGeometryDataKHR> gits::Vulkan::
    CVkAccelerationStructureGeometryDataKHR::Original() {
  if (Configurator::Get().common.mode != GITSMode::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (*_isNullPtr) {
    return PtrConverter<VkAccelerationStructureGeometryDataKHR>(nullptr);
  }
  if (_AccelerationStructureGeometryDataKHROriginal == nullptr) {
    _AccelerationStructureGeometryDataKHROriginal =
        std::make_unique<VkAccelerationStructureGeometryDataKHR>();
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _AccelerationStructureGeometryDataKHROriginal->triangles = _triangles->Original();
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _AccelerationStructureGeometryDataKHROriginal->aabbs = _aabbs->Original();
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _AccelerationStructureGeometryDataKHROriginal->instances = _instances->Original();
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
  return PtrConverter<VkAccelerationStructureGeometryDataKHR>(
      _AccelerationStructureGeometryDataKHROriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::GetMappedPointers() {
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

void gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _geometryType->Write(stream);
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _triangles->Write(stream);
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _aabbs->Write(stream);
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _instances->Write(stream);
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
}

void gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _geometryType->Read(stream);
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _triangles->Read(stream);
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _aabbs->Read(stream);
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _instances->Read(stream);
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
}

uint64_t gits::Vulkan::CVkClearColorValue::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    // Cuint32_t::CSArray writes count(uint32) + elements (4x uint32)
    sz += sizeof(uint32_t) + sizeof(uint32_t) * 4;
  }
  return sz;
}

uint64_t gits::Vulkan::CVkClearValue::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr && _color) {
    sz += _color->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CVkGenericArgument::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    sz += sizeof(int32_t); // sType enum
    sz += sizeof(bool);    // skipped flag
    if (!**_skipped && _argument) {
      sz += _argument->Size();
    }
  }
  return sz;
}

uint64_t gits::Vulkan::CVkGenericArgumentArray::Size() const {
  uint64_t sz = sizeof(uint32_t);
  for (auto& arg : _cgenericargsDict) {
    sz += arg->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CpNextWrapper::Size() const {
  uint64_t sz = sizeof(uint64_t);
  if (_ptr && _data) {
    sz += _data->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CDescriptorUpdateTemplateObject::Size() const {
  uint64_t sz = 0;
  sz += sizeof(int32_t);  // descriptor type enum
  sz += sizeof(uint64_t); // offset
  sz += sizeof(uint64_t); // size
  if (_argument) {
    sz += _argument->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CUpdateDescriptorSetWithTemplateArray::Size() const {
  uint64_t sz = sizeof(uint32_t); // count
  sz += sizeof(uint64_t);         // total size field
  for (auto& arg : _cgenericargsDict) {
    sz += arg->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    sz += sizeof(int32_t);       // sType
    sz += _pNext->Size();        // pNext wrapper
    sz += sizeof(uint32_t);      // flags
    sz += sizeof(size_t);        // initialDataSize
    sz += _pInitialData->Size(); // resource hash
  }
  return sz;
}

uint64_t gits::Vulkan::CBufferDeviceAddressObject::Size() const {
  return sizeof(uint64_t)   // original device address
         + sizeof(void*)    // buffer handle id serialization
         + sizeof(int64_t); // offset
}

uint64_t gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Size() const {
  uint64_t sz = sizeof(int32_t); // execution side enum
  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    sz += _bufferDeviceAddress->Size();
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    // currently no payload written (code commented out)
    break;
  }
  return sz;
}

uint64_t gits::Vulkan::CVkDeviceOrHostAddressKHR::Size() const {
  uint64_t sz = sizeof(int32_t); // execution side enum
  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    sz += _bufferDeviceAddress->Size();
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    // throws; no payload
    break;
  }
  return sz;
}

uint64_t gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    sz += sizeof(int32_t);  // sType
    sz += _pNext->Size();   // pNext
    sz += sizeof(int32_t);  // execution side enum
    sz += sizeof(uint32_t); // arrayOfPointers
    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      sz += _bufferDeviceAddress->Size();
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      // not supported
      break;
    }
  }
  return sz;
}

uint64_t gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    sz += sizeof(int32_t); // geometryType
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      sz += _triangles->Size();
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      sz += _aabbs->Size();
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      sz += _instances->Size();
      break;
    default:
      break;
    }
  }
  return sz;
}

uint64_t gits::Vulkan::CVkDependencyInfo::Size() const {
  uint64_t sz = sizeof(bool);
  if (!*_isNullPtr) {
    sz += sizeof(int32_t);  // sType
    sz += _pNext->Size();   // pNext
    sz += sizeof(uint32_t); // dependencyFlags
    sz += sizeof(uint32_t); // memoryBarrierCount
    sz += _pMemoryBarriers->Size();
    sz += sizeof(uint32_t); // bufferMemoryBarrierCount
    sz += _pBufferMemoryBarriers->Size();
    sz += sizeof(uint32_t); // imageMemoryBarrierCount
    sz += _pImageMemoryBarriers->Size();
  }
  return sz;
}

uint64_t gits::Vulkan::CVkBool32::Size() const {
  return sizeof(uint32_t);
}

uint64_t gits::Vulkan::CNullWrapper::Size() const {
  return sizeof(uint64_t);
}

uint64_t gits::Vulkan::CVoidPtr::Size() const {
  return sizeof(uint64_t);
}

uint64_t gits::Vulkan::CVulkanShader::Size() const {
  return _data.Size();
}
