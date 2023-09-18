// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanArgumentsAuto.h"
#include "vulkanTools.h"
#include "vulkanLog.h"

template <>
const char* gits::Vulkan::CVulkanObj<HWND, gits::Vulkan::HWNDTypeTag>::NAME = "HWND";

template <>
const char* gits::Vulkan::CVulkanObj<HINSTANCE, gits::Vulkan::HINSTANCETypeTag>::NAME = "HINSTANCE";

template <>
const char* gits::Vulkan::CVulkanObj<HMONITOR, gits::Vulkan::HMONITORTypeTag>::NAME = "HMONITOR";

template <>
const char*
    gits::Vulkan::CVulkanObj<xcb_connection_t*, gits::Vulkan::xcb_connection_t_TypeTag>::NAME =
        "xcb_connection_t*";

template <>
const char* gits::Vulkan::CVulkanObj<Display*, gits::Vulkan::VkDisplay_TypeTag>::NAME = "Display*";

template <>
const char* gits::Vulkan::CVulkanObj<void*, gits::Vulkan::VkWindow_TypeTag>::NAME = "Window";

bool gits::Vulkan::CDeclaredBinaryResource::DeclarationNeeded() const {
  if (_resource_hash != 0) {
    return true;
  } else {
    return false; // Because we will just print nullptr, no need for a variable.
  }
}

void gits::Vulkan::CDeclaredBinaryResource::Declare(CCodeOStream& stream) const {
  stream.Register(ScopeKey(), "res", true);
  stream.Indent() << "Resource " << stream.VariableName(ScopeKey()) << "(" << _resource_hash
                  << ");\n";
}

void gits::Vulkan::CDeclaredBinaryResource::Write(CCodeOStream& stream) const {
  if (_resource_hash != 0) {
    stream << stream.VariableName(ScopeKey());
  } else {
    stream << "nullptr";
  }
}

gits::Vulkan::CVkClearColorValue::CVkClearColorValue()
    : _uint32(new Cuint32_t::CSArray()),
      _ClearColorValue(nullptr),
      _ClearColorValueOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkClearColorValue::~CVkClearColorValue() {
  delete _uint32;
  delete _ClearColorValue;
  delete _ClearColorValueOriginal;
}

gits::Vulkan::CVkClearColorValue::CVkClearColorValue(const VkClearColorValue* clearcolorvalue)
    : _ClearColorValue(nullptr),
      _ClearColorValueOriginal(nullptr),
      _isNullPtr(clearcolorvalue == nullptr) {
  if (!*_isNullPtr) {
    _uint32 = new Cuint32_t::CSArray(4, clearcolorvalue->uint32);
  } else {
    _uint32 = nullptr;
  }
}

VkClearColorValue* gits::Vulkan::CVkClearColorValue::Value() {
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

gits::PtrConverter<VkClearColorValue> gits::Vulkan::CVkClearColorValue::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkClearColorValue>(nullptr);
  }
  if (_ClearColorValueOriginal == nullptr) {
    _ClearColorValueOriginal = new VkClearColorValue;
    auto uint32ValuesOriginal = _uint32->Original();
    if (uint32ValuesOriginal != nullptr) {
      for (int i = 0; i < 4; i++) {
        _ClearColorValueOriginal->uint32[i] = uint32ValuesOriginal[i];
      }
    } else {
      Log(ERR) << "The pointer to array _uint32->Original() is null.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return PtrConverter<VkClearColorValue>(_ClearColorValueOriginal);
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

void gits::Vulkan::CVkClearColorValue::Declare(CCodeOStream& stream) const {
  // TODO: print floats, ints or uints, depending on image/attachment format.
  stream.Register(ScopeKey(), "clearColor", true);
  stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = ";
  stream << "MakeVkClearColorValue(";
  auto& componentVec = _uint32->Vector();
  for (size_t i = 0; i < 4; i++) {
    if (i != 0) {
      stream << ", ";
    }
    stream << componentVec[i];
  }
  stream << ");" << std::endl;
  ;
}

void gits::Vulkan::CVkClearColorValue::Write(CCodeOStream& stream) const {
  if (!*_isNullPtr) {
    stream << stream.VariableName(ScopeKey());
  } else {
    stream << "nullptr";
  }
}

bool gits::Vulkan::CVkClearColorValue::AmpersandNeeded() const {
  return !*_isNullPtr;
}

bool gits::Vulkan::CVkClearColorValue::DeclarationNeeded() const {
  return !*_isNullPtr;
}

gits::Vulkan::CVkClearValue::CVkClearValue()
    : _color(new CVkClearColorValue()),
      _ClearValue(nullptr),
      _ClearValueOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkClearValue::~CVkClearValue() {
  delete _color;
  delete _ClearValue;
  delete _ClearValueOriginal;
}

gits::Vulkan::CVkClearValue::CVkClearValue(const VkClearValue* clearvalue)
    : _ClearValue(nullptr), _ClearValueOriginal(nullptr), _isNullPtr(clearvalue == nullptr) {
  if (!*_isNullPtr) {
    _color = new CVkClearColorValue(&clearvalue->color);
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
    _ClearValue = new VkClearValue;
    _ClearValue->color = **_color;
  }
  return _ClearValue;
}

gits::PtrConverter<VkClearValue> gits::Vulkan::CVkClearValue::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkClearValue>(nullptr);
  }
  if (_ClearValueOriginal == nullptr) {
    _ClearValueOriginal = new VkClearValue;
    _ClearValueOriginal->color = _color->Original();
  }
  return PtrConverter<VkClearValue>(_ClearValueOriginal);
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

void gits::Vulkan::CVkClearValue::Write(CCodeOStream& stream) const {
  // TODO: print color or depthStencil, depending on the attachment.
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    _color->Write(stream);
  }
}

void gits::Vulkan::CVkClearValue::Declare(CCodeOStream& stream) const {
  _color->Declare(stream);
}

bool gits::Vulkan::CVkClearValue::DeclarationNeeded() const {
  return !*_isNullPtr && _color->DeclarationNeeded();
}

gits::Vulkan::CVkGenericArgument::CVkGenericArgument()
    : _sType(new CVkStructureType()),
      _argument(nullptr),
      _skipped(new Cbool()),
      _isNullPtr(false) {}

gits::Vulkan::CVkGenericArgument::~CVkGenericArgument() {
  delete _sType;
  delete _argument;
  delete _skipped;
}

void gits::Vulkan::CVkGenericArgument::InitArgument(uint32_t type) {
  switch (type) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = new C##structure##__VA_ARGS__;                                                     \
    break;

#include "vulkanPNextWrappers.inl"

  default:
    _skipped = new Cbool(true);
    Log(ERR) << "Unknown enum value: " << type << " for CVkGenericArgument";
    break;
  }
}

void gits::Vulkan::CVkGenericArgument::CreateArgument(uint32_t type,
                                                      const void* vkgenericargument) {
  switch (type) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = new C##structure##__VA_ARGS__((structure*)vkgenericargument);                      \
    break;

#include "vulkanPNextWrappers.inl"

  default:
    _skipped = new Cbool(true);
    Log(ERR) << "Unknown enum value: " << type << " for CVkGenericArgument";
    break;
  }
}

gits::Vulkan::CVkGenericArgument::CVkGenericArgument(const void* vkgenericargument)
    : _skipped(nullptr) {
  vkgenericargument = ignoreLoaderSpecificStructureTypes(vkgenericargument);
  _isNullPtr = (vkgenericargument == nullptr);

  if (!*_isNullPtr) {
    _sType = new CVkStructureType(*(uint32_t*)vkgenericargument);
    CreateArgument(**_sType, vkgenericargument);
    if (!_skipped) {
      _skipped = new Cbool(false);
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
      VkLog(ERR) << "Due to unknown enum value: " << **_sType
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

void gits::Vulkan::CVkGenericArgument::Write(CCodeOStream& stream) const {
  if (!*_isNullPtr && !**_skipped) {
    stream << "&";
    _argument->Write(stream);
  } else {
    stream << "nullptr";
  }
}

bool gits::Vulkan::CVkGenericArgument::AmpersandNeeded() const {
  return !*_isNullPtr && !**_skipped;
}

bool gits::Vulkan::CVkGenericArgument::DeclarationNeeded() const {
  return !*_isNullPtr && !**_skipped;
}

void gits::Vulkan::CVkGenericArgument::Declare(CCodeOStream& stream) const {
  if (!*_isNullPtr && !**_skipped) {
    _argument->Declare(stream);
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
      _cgenericargsDict.push_back(obj);
    }
  }
  _size = new Cuint64_t(max_length);
}

gits::Vulkan::CUpdateDescriptorSetWithTemplateArray::~CUpdateDescriptorSetWithTemplateArray() {
  delete _size;
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
  for (unsigned i = 0; i < dictSize; i++) {
    auto keyArgPtr = std::make_shared<CDescriptorUpdateTemplateObject>();
    keyArgPtr->Read(stream);
    _cgenericargsDict.push_back(keyArgPtr);
  }
}

gits::Vulkan::CDescriptorUpdateTemplateObject::CDescriptorUpdateTemplateObject(
    VkDescriptorType descType, const void* pData, std::uint64_t offset)
    : _descType(new CVkDescriptorType(descType)), _offset(new Cuint64_t(offset)) {

  switch (descType) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    _size = new Cuint64_t(sizeof(VkDescriptorBufferInfo));
    _argument = new CVkDescriptorBufferInfo((VkDescriptorBufferInfo*)((char*)pData + offset));
    break;
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    _size = new Cuint64_t(sizeof(VkBufferView));
    _argument = new CVkBufferView((VkBufferView*)((char*)pData + offset));
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    _size = new Cuint64_t(sizeof(VkDescriptorImageInfo));
    _argument =
        new CVkDescriptorImageInfo((VkDescriptorImageInfo*)((char*)pData + offset), descType);
    break;
  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    _size = new Cuint64_t(sizeof(VkAccelerationStructureKHR));
    _argument =
        new CVkAccelerationStructureKHR((VkAccelerationStructureKHR*)((char*)pData + offset));
    break;
  default:
    VkLog(WARN) << "Unknown descriptor type " << descType
                << " in CDescriptorUpdateTemplateObject constructor!";
    _size = nullptr;
    _argument = nullptr;
  }
}

gits::Vulkan::CDescriptorUpdateTemplateObject::~CDescriptorUpdateTemplateObject() {
  delete _argument;
  delete _descType;
  delete _offset;
  delete _size;
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
    _argument = new CVkDescriptorBufferInfo();
    break;
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    _argument = new CVkBufferView();
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    _argument = new CVkDescriptorImageInfo();
    break;
  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    _argument = new CVkAccelerationStructureKHR();
    break;
  default:
    Log(TRACE) << "Not handled CDescriptorUpdateTemplateObject enumeration: " +
                      std::string(_descType->Name());
    break;
  }
  _argument->Read(stream);
}

gits::Vulkan::CVkPipelineCacheCreateInfo_V1::CVkPipelineCacheCreateInfo_V1()
    : _sType(new CVkStructureType()),
      _pNext(new CpNextWrapper()),
      _flags(new Cuint32_t()),
      _initialDataSize(new Csize_t()),
      _pInitialData(new CDeclaredBinaryResource()),
      _PipelineCacheCreateInfo(nullptr),
      _PipelineCacheCreateInfoOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkPipelineCacheCreateInfo_V1::~CVkPipelineCacheCreateInfo_V1() {
  delete _sType;
  delete _pNext;
  delete _flags;
  delete _initialDataSize;
  delete _pInitialData;
  delete _PipelineCacheCreateInfo;
  delete _PipelineCacheCreateInfoOriginal;
}

gits::Vulkan::CVkPipelineCacheCreateInfo_V1::CVkPipelineCacheCreateInfo_V1(
    const VkPipelineCacheCreateInfo* pipelinecachecreateinfo)
    : _PipelineCacheCreateInfo(nullptr),
      _PipelineCacheCreateInfoOriginal(nullptr),
      _isNullPtr(pipelinecachecreateinfo == nullptr) {
  if (!*_isNullPtr) {
    _sType = new CVkStructureType(pipelinecachecreateinfo->sType);
    _pNext = new CpNextWrapper(pipelinecachecreateinfo->pNext);
    _flags = new Cuint32_t(pipelinecachecreateinfo->flags);
    _initialDataSize = new Csize_t(pipelinecachecreateinfo->initialDataSize);
    _pInitialData =
        new CDeclaredBinaryResource(RESOURCE_DATA_RAW, pipelinecachecreateinfo->pInitialData,
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
    _PipelineCacheCreateInfo = new VkPipelineCacheCreateInfo;
    _PipelineCacheCreateInfo->sType = **_sType;
    _PipelineCacheCreateInfo->pNext = **_pNext;
    _PipelineCacheCreateInfo->flags = **_flags;
    _PipelineCacheCreateInfo->initialDataSize = **_initialDataSize;
    _initialData.resize(**_initialDataSize);
    memcpy(_initialData.data(), **_pInitialData, **_initialDataSize);
    _PipelineCacheCreateInfo->pInitialData = _initialData.data();
  }
  return _PipelineCacheCreateInfo;
}

gits::PtrConverter<VkPipelineCacheCreateInfo> gits::Vulkan::CVkPipelineCacheCreateInfo_V1::
    Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkPipelineCacheCreateInfo>(nullptr);
  }
  if (_PipelineCacheCreateInfoOriginal == nullptr) {
    _PipelineCacheCreateInfoOriginal = new VkPipelineCacheCreateInfo;
    _PipelineCacheCreateInfoOriginal->sType = _sType->Original();
    _PipelineCacheCreateInfoOriginal->pNext = _pNext->Original();
    _PipelineCacheCreateInfoOriginal->flags = _flags->Original();
    _PipelineCacheCreateInfoOriginal->initialDataSize = _initialDataSize->Original();
    _initialData.resize(**_initialDataSize);
    memcpy(_initialData.data(), **_pInitialData, **_initialDataSize);
    _PipelineCacheCreateInfoOriginal->pInitialData = _initialData.data();
  }
  return PtrConverter<VkPipelineCacheCreateInfo>(_PipelineCacheCreateInfoOriginal);
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

void gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << getVarName("vkPipelineCacheCreateInfo_", this);
  }
}

bool gits::Vulkan::CVkPipelineCacheCreateInfo_V1::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkPipelineCacheCreateInfo_V1::Declare(CCodeOStream& stream) const {
  if (!*_isNullPtr) {
    _pInitialData->VariableNameRegister(stream, false);
    _pInitialData->Declare(stream);

    stream.Indent() << Name() << " " << getVarName("vkPipelineCacheCreateInfo_", this) << " = {\n";
    stream.ScopeBegin();
    stream.Indent() << *(this->_sType) << ", // sType\n";
    stream.Indent() << *(this->_pNext) << ", // pNext\n";
    stream.Indent() << *(this->_flags) << ", // flags\n";
    stream.Indent() << *(this->_initialDataSize) << ", // initialDataSize\n";
    stream.Indent() << *_pInitialData << ", // pInitialData\n";
    stream.ScopeEnd();
    stream.Indent() << "};\n";
  }
}

gits::Vulkan::CBufferDeviceAddressObject::CBufferDeviceAddressObject(
    VkDeviceAddress deviceAddress) {
  VkBuffer buffer = VK_NULL_HANDLE;
  int64_t offset = 0;

  if (deviceAddress != 0) {
    buffer = findBufferFromDeviceAddress(deviceAddress);
    if (buffer) {
      offset = deviceAddress - SD()._bufferstates[buffer]->deviceAddress;
    }
  }

  _originalDeviceAddress = new Cuint64_t(deviceAddress);
  _buffer = new CVkBuffer(buffer);
  _offset = new Cint64_t(offset);
}

gits::Vulkan::CBufferDeviceAddressObject::CBufferDeviceAddressObject(
    gits::Vulkan::CBufferDeviceAddressObjectData& bufferDeviceAddressObject) {
  _originalDeviceAddress = new Cuint64_t(bufferDeviceAddressObject._originalDeviceAddress);
  _buffer = new CVkBuffer(bufferDeviceAddressObject._buffer);
  _offset = new Cint64_t(bufferDeviceAddressObject._offset);
}

gits::Vulkan::CBufferDeviceAddressObject::~CBufferDeviceAddressObject() {
  delete _originalDeviceAddress;
  delete _buffer;
  delete _offset;
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
  std::set<uint64_t> returnMap;
  for (auto obj : _buffer->GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  return returnMap;
}

gits::Vulkan::CVkDeviceOrHostAddressConstKHR::CVkDeviceOrHostAddressConstKHR(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    uint32_t offset,
    uint64_t stride,
    uint32_t count,
    const VkAccelerationStructureBuildControlDataGITS& controlData) {

  _commandExecutionSideGITS =
      std::make_unique<CVkCommandExecutionSideGITS>(controlData.executionSide);

  if (deviceorhostaddress.deviceAddress == 0) {
    _bufferDeviceAddress = std::make_unique<CBufferDeviceAddressObject>();
    _dataSize = std::make_unique<Csize_t>(0);
    return;
  }

  auto hash = prepareStateTrackingHash(controlData, deviceorhostaddress.deviceAddress, offset,
                                       stride, count);

  CVkDeviceOrHostAddressConstKHRData* deviceOrHostAddressData;
  if (!getStructStorageFromHash(hash, controlData.accelerationStructure,
                                (void**)(&deviceOrHostAddressData))) {
    _bufferDeviceAddress = std::make_unique<CBufferDeviceAddressObject>();
    _dataSize = std::make_unique<Csize_t>(0);
    return;
  }

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    _bufferDeviceAddress =
        std::make_unique<CBufferDeviceAddressObject>(deviceOrHostAddressData->_bufferDeviceAddress);
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    _dataSize = std::make_unique<Csize_t>(deviceOrHostAddressData->_dataSize);
    _data.resize(deviceOrHostAddressData->_dataSize);
    memcpy(_data.data(), deviceOrHostAddressData->_inputData.data(),
           deviceOrHostAddressData->_dataSize);
    _resource = std::make_unique<CDeclaredBinaryResource>(hash);
    break;
  }
}

VkDeviceOrHostAddressConstKHR* gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Value() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (!_DeviceOrHostAddress) {
    _DeviceOrHostAddress = std::make_unique<VkDeviceOrHostAddressConstKHR>();

    switch (**_commandExecutionSideGITS) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddress->deviceAddress = _bufferDeviceAddress->Value();
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      auto dataSize = **_dataSize;

      if (dataSize != 0) {
        if (_data.size() == 0) {
          _data.resize(dataSize);
          memcpy(_data.data(), **_resource, dataSize);
        }
      }

      _DeviceOrHostAddress->hostAddress = _data.data();
      break;
    }
  }
  return _DeviceOrHostAddress.get();
}

gits::Vulkan::CVkDeviceOrHostAddressConstKHR::PtrConverter gits::Vulkan::
    CVkDeviceOrHostAddressConstKHR::Original() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
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

  return PtrConverter(_DeviceOrHostAddressOriginal.get());
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHR::Write(CBinOStream& stream) const {
  _commandExecutionSideGITS->Write(stream);

  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress->Write(stream);
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    _dataSize->Write(stream);
    if (**_dataSize) {
      _resource->Write(stream);
      CGits::Instance().ResourceManager().put(RESOURCE_DATA_RAW, _data.data(), **_dataSize,
                                              _resource->GetResourceHash());
    }
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
    _dataSize->Read(stream);
    if (**_dataSize) {
      _resource->Read(stream);
    }
    break;
  }
}

gits::Vulkan::CVkDependencyInfo::CVkDependencyInfo()
    : _sType(new CVkStructureType()),
      _pNext(new CpNextWrapper()),
      _dependencyFlags(new Cuint32_t()),
      _memoryBarrierCount(new Cuint32_t()),
      _pMemoryBarriers(new CVkMemoryBarrier2Array()),
      _bufferMemoryBarrierCount(new Cuint32_t()),
      _pBufferMemoryBarriers(new CVkBufferMemoryBarrier2Array()),
      _imageMemoryBarrierCount(new Cuint32_t()),
      _pImageMemoryBarriers(new CVkImageMemoryBarrier2Array()),
      _DependencyInfo(nullptr),
      _DependencyInfoOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkDependencyInfo::~CVkDependencyInfo() {
  delete _sType;
  delete _pNext;
  delete _dependencyFlags;
  delete _memoryBarrierCount;
  delete _pMemoryBarriers;
  delete _bufferMemoryBarrierCount;
  delete _pBufferMemoryBarriers;
  delete _imageMemoryBarrierCount;
  delete _pImageMemoryBarriers;
  delete _DependencyInfo;
  delete _DependencyInfoOriginal;
}

gits::Vulkan::CVkDependencyInfo::CVkDependencyInfo(const VkDependencyInfo* dependencyinfo)
    : _DependencyInfo(nullptr),
      _DependencyInfoOriginal(nullptr),
      _isNullPtr(dependencyinfo == nullptr) {
  if (!*_isNullPtr) {
    _sType = new CVkStructureType(dependencyinfo->sType);
    _pNext = new CpNextWrapper(dependencyinfo->pNext);
    _dependencyFlags = new Cuint32_t(dependencyinfo->dependencyFlags);
    _memoryBarrierCount = new Cuint32_t(dependencyinfo->memoryBarrierCount);
    _pMemoryBarriers = new CVkMemoryBarrier2Array(dependencyinfo->memoryBarrierCount,
                                                  dependencyinfo->pMemoryBarriers);
    _bufferMemoryBarrierCount = new Cuint32_t(dependencyinfo->bufferMemoryBarrierCount);
    _pBufferMemoryBarriers = new CVkBufferMemoryBarrier2Array(
        dependencyinfo->bufferMemoryBarrierCount, dependencyinfo->pBufferMemoryBarriers);
    _imageMemoryBarrierCount = new Cuint32_t(dependencyinfo->imageMemoryBarrierCount);
    _pImageMemoryBarriers = new CVkImageMemoryBarrier2Array(dependencyinfo->imageMemoryBarrierCount,
                                                            dependencyinfo->pImageMemoryBarriers);
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
    _DependencyInfo = new VkDependencyInfo;
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
  return _DependencyInfo;
}

gits::PtrConverter<VkDependencyInfo> gits::Vulkan::CVkDependencyInfo::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkDependencyInfo>(nullptr);
  }
  if (_DependencyInfoOriginal == nullptr) {
    _DependencyInfoOriginal = new VkDependencyInfo;
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
  return PtrConverter<VkDependencyInfo>(_DependencyInfoOriginal);
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

void gits::Vulkan::CVkDependencyInfo::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << stream.VariableName(ScopeKey());
  }
}

bool gits::Vulkan::CVkDependencyInfo::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkDependencyInfo::Declare(CCodeOStream& stream) const {
  if (!*_isNullPtr) {
    _pNext->VariableNameRegister(stream, false);
    _pNext->Declare(stream);
    _pMemoryBarriers->VariableNameRegister(stream, false);
    _pMemoryBarriers->Declare(stream);
    _pBufferMemoryBarriers->VariableNameRegister(stream, false);
    _pBufferMemoryBarriers->Declare(stream);
    _pImageMemoryBarriers->VariableNameRegister(stream, false);
    _pImageMemoryBarriers->Declare(stream);

    stream.Register(ScopeKey(), "vkDependencyInfo", true);
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = {\n";
    stream.ScopeBegin();
    stream.Indent() << *(this->_sType) << ", // sType\n";
    stream.Indent() << *_pNext << ", // pNext\n";
    stream.Indent() << *(this->_dependencyFlags) << ", // dependencyFlags\n";
    stream.Indent() << *(this->_memoryBarrierCount) << ", // memoryBarrierCount\n";
    stream.Indent() << *_pMemoryBarriers << ", // pMemoryBarriers\n";
    stream.Indent() << *(this->_bufferMemoryBarrierCount) << ", // bufferMemoryBarrierCount\n";
    stream.Indent() << *_pBufferMemoryBarriers << ", // pBufferMemoryBarriers\n";
    stream.Indent() << *(this->_imageMemoryBarrierCount) << ", // imageMemoryBarrierCount\n";
    stream.Indent() << *_pImageMemoryBarriers << ", // pImageMemoryBarriers\n";
    stream.ScopeEnd();
    stream.Indent() << "};\n";
  }
}

void gits::Vulkan::CVkDependencyInfo::Declare(CCodeOStream& stream,
                                              size_t memoryBarrierStart,
                                              size_t memoryBarrierEnd,
                                              size_t bufferMemoryBarrierStart,
                                              size_t bufferMemoryBarrierEnd,
                                              size_t imageMemoryBarrierStart,
                                              size_t imageMemoryBarrierEnd) const {

  if (!*_isNullPtr) {
    _pNext->VariableNameRegister(stream, false);
    _pNext->Declare(stream);
    _pMemoryBarriers->VariableNameRegister(stream, false);
    _pMemoryBarriers->Declare(stream, memoryBarrierStart, memoryBarrierEnd);
    _pBufferMemoryBarriers->VariableNameRegister(stream, false);
    _pBufferMemoryBarriers->Declare(stream, bufferMemoryBarrierStart, bufferMemoryBarrierEnd);
    _pImageMemoryBarriers->VariableNameRegister(stream, false);
    _pImageMemoryBarriers->Declare(stream, imageMemoryBarrierStart, imageMemoryBarrierEnd);

    size_t memoryBarrierSize = memoryBarrierEnd - memoryBarrierStart;
    size_t bufferMemoryBarrierSize = bufferMemoryBarrierEnd - bufferMemoryBarrierStart;
    size_t imageMemoryBarrierSize = imageMemoryBarrierEnd - imageMemoryBarrierStart;

    stream.Register(ScopeKey(), "vkDependencyInfo", true);
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = {\n";
    stream.ScopeBegin();
    stream.Indent() << *(this->_sType) << ", // sType\n";
    stream.Indent() << *_pNext << ", // pNext\n";
    stream.Indent() << *(this->_dependencyFlags) << ", // dependencyFlags\n";
    stream.Indent() << memoryBarrierSize << ", // memoryBarrierCount\n";
    stream.Indent() << *_pMemoryBarriers << ", // pMemoryBarriers\n";
    stream.Indent() << bufferMemoryBarrierSize << ", // bufferMemoryBarrierCount\n";
    stream.Indent() << *_pBufferMemoryBarriers << ", // pBufferMemoryBarriers\n";
    stream.Indent() << imageMemoryBarrierSize << ", // imageMemoryBarrierCount\n";
    stream.Indent() << *_pImageMemoryBarriers << ", // pImageMemoryBarriers\n";
    stream.ScopeEnd();
    stream.Indent() << "};\n";
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
    _data = new CVkGenericArgument(ptr);
  } else {
    _data = nullptr;
  }
}

gits::Vulkan::CpNextWrapper::~CpNextWrapper() {
  delete _data;
}

gits::Vulkan::CVkTransformMatrixKHR::CVkTransformMatrixKHR(
    const VkTransformMatrixKHR* transformmatrixkhr)
    : _TransformMatrixKHR(nullptr),
      _TransformMatrixKHROriginal(nullptr),
      _isNullPtr(transformmatrixkhr == nullptr) {
  if (!*_isNullPtr) {
    _matrix = std::make_unique<Cfloat::CSArray>(12, &transformmatrixkhr->matrix[0][0]);
  }
}

VkTransformMatrixKHR* gits::Vulkan::CVkTransformMatrixKHR::Value() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_TransformMatrixKHR == nullptr) {
    _TransformMatrixKHR = std::make_unique<VkTransformMatrixKHR>();
    float* ptr = &_TransformMatrixKHR->matrix[0][0];
    for (uint32_t i = 0; i < 12; ++i) {
      ptr[i] = (**_matrix)[i];
    }
  }
  return _TransformMatrixKHR.get();
}

gits::Vulkan::CVkTransformMatrixKHR::PtrConverter gits::Vulkan::CVkTransformMatrixKHR::Original() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (*_isNullPtr) {
    return PtrConverter(nullptr);
  }
  if (_TransformMatrixKHROriginal == nullptr) {
    _TransformMatrixKHROriginal = std::make_unique<VkTransformMatrixKHR>();
    float* ptr = &_TransformMatrixKHROriginal->matrix[0][0];
    for (uint32_t i = 0; i < 12; i++) {
      ptr[i] = (_matrix->Original())[i];
    }
  }
  return PtrConverter(_TransformMatrixKHROriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkTransformMatrixKHR::GetMappedPointers() {

  std::set<uint64_t> returnMap;
  return returnMap;
}

void gits::Vulkan::CVkTransformMatrixKHR::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _matrix->Write(stream);
  }
}

void gits::Vulkan::CVkTransformMatrixKHR::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _matrix->Read(stream);
  }
}

void gits::Vulkan::CVkTransformMatrixKHR::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << getVarName("vkTransformMatrixKHR_", this);
  }
}

bool gits::Vulkan::CVkTransformMatrixKHR::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkTransformMatrixKHR::Declare(CCodeOStream& stream) const {
  if (!*_isNullPtr) {

    TODO("Implement CCode support!")
    
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

  auto hash = prepareStateTrackingHash(
      prepareAccelerationStructureControlData(controlData,
                                              accelerationstructuregeometryinstancesdatakhr->sType),
      accelerationstructuregeometryinstancesdatakhr->data.deviceAddress,
      buildRangeInfo.primitiveOffset, sizeof(VkAccelerationStructureInstanceKHR),
      buildRangeInfo.primitiveCount);

  CVkAccelerationStructureGeometryInstancesDataKHRData* geometryInstancesData;
  if (!getStructStorageFromHash(hash, controlData.accelerationStructure,
                                (void**)(&geometryInstancesData))) {
    _isNullPtr = true;
    return;
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
    //_count = std::make_unique<Cuint32_t>(geometryInstancesData->_inputData.size());
    //_data.resize(deviceOrHostAddressData->_dataSize);
    //memcpy(_data.data(), deviceOrHostAddressData->_inputData.data(),
    //       deviceOrHostAddressData->_dataSize);
    //_resource = std::make_unique<CDeclaredBinaryResource>(hash);
    break;
  }
}

const char* gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::NAME =
    "VkAccelerationStructureGeometryInstancesDataKHR";

VkAccelerationStructureGeometryInstancesDataKHR* gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHR::Value() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
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

gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::PtrConverter gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHR::Original() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
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
  return PtrConverter(_AccelerationStructureGeometryInstancesDataKHROriginal.get());
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::
    GetMappedPointers() {
  switch (**_commandExecutionSideGITS) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    return _bufferDeviceAddress->GetMappedPointers();
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
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

void gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::Write(
    CCodeOStream& stream) const {
}

bool gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHR::Declare(
    CCodeOStream& stream) const {
}

gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::~CVkAccelerationStructureGeometryDataKHR() {}

gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::CVkAccelerationStructureGeometryDataKHR(
    VkGeometryTypeKHR geometryType,
    const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr,
    const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
    VkAccelerationStructureBuildControlDataGITS controlData)
    : _geometryType(nullptr),
      _triangles(nullptr),
      _aabbs(nullptr),
      _instances(nullptr),
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
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryDataKHR == nullptr) {
    _AccelerationStructureGeometryDataKHR = new VkAccelerationStructureGeometryDataKHR();
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
  return _AccelerationStructureGeometryDataKHR;
}

gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::PtrConverter gits::Vulkan::
    CVkAccelerationStructureGeometryDataKHR::Original() {
  if (Config::Get().common.mode != Config::MODE_PLAYER) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (*_isNullPtr) {
    return PtrConverter(nullptr);
  }
  if (_AccelerationStructureGeometryDataKHROriginal == nullptr) {
    _AccelerationStructureGeometryDataKHROriginal = new VkAccelerationStructureGeometryDataKHR();
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _AccelerationStructureGeometryDataKHROriginal->triangles = **_triangles;
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _AccelerationStructureGeometryDataKHROriginal->aabbs = **_aabbs;
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _AccelerationStructureGeometryDataKHROriginal->instances = **_instances;
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
  return PtrConverter(_AccelerationStructureGeometryDataKHROriginal);
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::GetMappedPointers() {

  std::set<uint64_t> returnMap;
  switch (**_geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
    for (auto obj : _triangles->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    break;
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    for (auto obj : _aabbs->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    break;
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    for (auto obj : _instances->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    break;
  default:
    throw std::runtime_error("Unknown geometry type provided!");
    break;
  }
  return returnMap;
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

void gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << getVarName("vkAccelerationStructureGeometryDataKHR_", this);
  }
}

bool gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkAccelerationStructureGeometryDataKHR::Declare(CCodeOStream& stream) const {

  TODO("Implement proper handling in CCode - important for ray tracing")

}
