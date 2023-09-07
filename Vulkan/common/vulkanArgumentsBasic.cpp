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

gits::Vulkan::CVkDescriptorImageInfo::CVkDescriptorImageInfo()
    : _sampler(new CVkSampler()),
      _imageView(new CVkImageView()),
      _imageLayout(new CVkImageLayout()),
      _DescriptorImageInfo(nullptr),
      _DescriptorImageInfoOriginal(nullptr),
      _isNullPtr(false) {}

gits::Vulkan::CVkDescriptorImageInfo::~CVkDescriptorImageInfo() {
  delete _sampler;
  delete _imageView;
  delete _imageLayout;
  delete _DescriptorImageInfo;
  delete _DescriptorImageInfoOriginal;
}

gits::Vulkan::CVkDescriptorImageInfo::CVkDescriptorImageInfo(
    const VkDescriptorImageInfo* descriptorimageinfo, VkDescriptorType descriptorType)
    : _DescriptorImageInfo(nullptr),
      _DescriptorImageInfoOriginal(nullptr),
      _isNullPtr(descriptorimageinfo == nullptr) {
  if (!*_isNullPtr) {
    _sampler = new CVkSampler(((descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                               (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))
                                  ? descriptorimageinfo->sampler
                                  : VK_NULL_HANDLE);
    _imageView = new CVkImageView(((descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                                   (descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                                   (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
                                   (descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT))
                                      ? descriptorimageinfo->imageView
                                      : VK_NULL_HANDLE);
    _imageLayout =
        new CVkImageLayout(((descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                            (descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                            (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
                            (descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT))
                               ? descriptorimageinfo->imageLayout
                               : 0);
  } else {
    _sampler = nullptr;
    _imageView = nullptr;
    _imageLayout = nullptr;
  }
}

const char* gits::Vulkan::CVkDescriptorImageInfo::NAME = "VkDescriptorImageInfo";

VkDescriptorImageInfo* gits::Vulkan::CVkDescriptorImageInfo::Value() {
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

gits::PtrConverter<VkDescriptorImageInfo> gits::Vulkan::CVkDescriptorImageInfo::Original() {
  if (*_isNullPtr) {
    return PtrConverter<VkDescriptorImageInfo>(nullptr);
  }
  if (_DescriptorImageInfoOriginal == nullptr) {
    _DescriptorImageInfoOriginal = new VkDescriptorImageInfo;
    _DescriptorImageInfoOriginal->sampler = _sampler->Original();
    _DescriptorImageInfoOriginal->imageView = _imageView->Original();
    _DescriptorImageInfoOriginal->imageLayout = _imageLayout->Original();
  }
  return PtrConverter<VkDescriptorImageInfo>(_DescriptorImageInfoOriginal);
}

void gits::Vulkan::CVkDescriptorImageInfo::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
    _sampler->Write(stream);
    _imageView->Write(stream);
    _imageLayout->Write(stream);
  }
}

void gits::Vulkan::CVkDescriptorImageInfo::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
    _sampler->Read(stream);
    _imageView->Read(stream);
    _imageLayout->Read(stream);
  }
}

void gits::Vulkan::CVkDescriptorImageInfo::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << stream.VariableName(ScopeKey());
  }
}

bool gits::Vulkan::CVkDescriptorImageInfo::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::CVkDescriptorImageInfo::Declare(CCodeOStream& stream) const {
  if (!*_isNullPtr) {
    stream.Register(ScopeKey(), "vkDescriptorImageInfo", true);
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = {\n";
    stream.ScopeBegin();
    stream.Indent() << *(_sampler) << ", // sampler\n";
    stream.Indent() << *(_imageView) << ", // imageView\n";
    stream.Indent() << *(_imageLayout) << ", // imageLayout\n";
    stream.ScopeEnd();
    stream.Indent() << "};\n";
  }
}

bool gits::Vulkan::CVkDescriptorImageInfo::DeclarationNeeded() const {
  // Because if it was a nullptr, we'll just print "nullptr", no need for a variable.
  return !*_isNullPtr;
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
  auto bufferState = findBufferStateFromDeviceAddress(deviceAddress);

  _buffer = new CVkBuffer(bufferState->bufferHandle);
  _originalDeviceAddress = new Cuint64_t(deviceAddress);
  _offset = new Cuint64_t(deviceAddress - bufferState->deviceAddress);
}

gits::Vulkan::CBufferDeviceAddressObject::~CBufferDeviceAddressObject() {
  delete _buffer;
  delete _originalDeviceAddress;
  delete _offset;
}

VkDeviceAddress gits::Vulkan::CBufferDeviceAddressObject::Value() {
  return SD()._bufferstates[**_buffer]->deviceAddress + **_offset;
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
