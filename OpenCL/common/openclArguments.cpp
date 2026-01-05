// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openclArguments.cpp
 *
 * @brief Implementation of OpenCL library function call argument wrappers.
 */

#include "openclArguments.h"
#include "include/openclArguments.h"
#include "openclStateDynamic.h"
#include "openclTools.h"

#include "wglArguments.h"
#include "mapping.h"

#include <iomanip>

std::vector<cl_context_properties> gits::OpenCL::MapContextProperties(
    const Ccl_context_properties::CSArray& props) {
  std::vector<cl_context_properties> properties(props.Vector());
  if (properties.empty()) {
    return properties;
  }
  for (size_t i = 0; props.Vector()[i] != 0; i += 2) {
    cl_context_properties type = props.Vector()[i];
    switch (type) {
    case CL_CONTEXT_PLATFORM: {
      cl_platform_id value = reinterpret_cast<cl_platform_id>(props.Vector()[i + 1]);
      properties[i + 1] =
          reinterpret_cast<cl_context_properties>(Ccl_platform_id::GetMapping(value));
      break;
    }
    case CL_CONTEXT_D3D11_DEVICE_KHR: {
      // Adobe After Effects requests this, but doesn't use DX11. Apps that
      // actually use DX11 will likely fail. Thus, we don't crash, but we do
      // warn the user.
      LOG_WARNING << "The application has requested DirectX 11 interoperability support, but this "
                     "feature is not supported in GITS. The playback will continue, but there may "
                     "be various problems, such as crashes or corruption. If you don't need "
                     "DirectX API sharing in your OpenCL stream, consider recording the stream "
                     "again with the RemoveDXSharing option set to true in the config file.";
      break;
    }
    case CL_CONTEXT_INTEROP_USER_SYNC:
    case CL_CONTEXT_FLAGS_INTEL: {
      break;
    }
    case CL_GL_CONTEXT_KHR: {
      try {
        const HGLRC& hglrc = reinterpret_cast<const HGLRC&>(props.Vector()[i + 1]);
        properties[i + 1] =
            reinterpret_cast<cl_context_properties>(gits::OpenGL::CHGLRC::GetMapping(hglrc));
      } catch (ENotFound&) {
        LOG_ERROR << "Could not map GL Context." << std::endl;
        throw;
      }
      break;
    }
    case CL_WGL_HDC_KHR: {
      try {
        const HDC& hdc = reinterpret_cast<const HDC&>(props.Vector()[i + 1]);
        properties[i + 1] =
            reinterpret_cast<cl_context_properties>(gits::OpenGL::CHDC::GetMapping(hdc));
      } catch (ENotFound&) {
        LOG_ERROR << "Could not map GL Context." << std::endl;
        throw;
      }
      break;
    }
    default:
      LOG_ERROR << cl_context_propertiesToString(type) + " is not supported yet.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  return properties;
}

std::vector<cl_mem_properties_intel> gits::OpenCL::MapMemPropertiesIntel(
    const Ccl_mem_properties_intel::CSArray& props) {
  std::vector<cl_mem_properties_intel> properties(props.Vector());
  if (properties.empty()) {
    return properties;
  }
  for (auto i = 0u; props.Vector()[i] != 0; i += 2) {
    const auto type = props.Vector()[i];
    if (type == CL_MEM_DEVICE_ID_INTEL) {
      auto value = reinterpret_cast<cl_device_id>(props.Vector()[i + 1]);
      properties[i + 1] =
          reinterpret_cast<cl_mem_properties_intel>(Ccl_device_id::GetMapping(value));
    }
  }
  return properties;
}

std::string gits::OpenCL::Ccl_buffer_region::ToString() const {
  std::stringstream result;
  result << "{ " << Value().origin << ", " << Value().size << " }";
  return result.str();
}

std::string gits::OpenCL::Ccl_device_partition_property::ToString() const {
  std::string text;
  MaskAppend(text, cl_device_partition_propertyToString(Value()));
  if (text.find("CL_") != std::string::npos) {
    return text;
  } else {
    MaskAppend(text, cl_device_affinity_domainToString(Value()));
  }
  return text;
}

std::string gits::OpenCL::Ccl_device_partition_property_ext::ToString() const {
  std::string text;
  MaskAppend(text, cl_device_partition_property_extToString(Value()));
  if (text.find("CL_") != std::string::npos) {
    return text;
  } else {
    MaskAppend(text, cl_affinity_domain_extToString(Value()));
  }
  return text;
}

std::string gits::OpenCL::Ccl_image_desc::ToString() const {
  std::stringstream result;
  result << "{ " << cl_mem_object_typeToString(Value().image_type) << ", " << Value().image_width
         << ", " << Value().image_height << ", " << Value().image_depth << ", "
         << Value().image_array_size << ", " << Value().image_row_pitch << ", "
         << Value().image_slice_pitch << ", " << Value().num_mip_levels << ", "
         << Value().num_samples << ", " << Value().mem_object << " }";
  return result.str();
}

std::string gits::OpenCL::Ccl_image_format::ToString() const {
  std::stringstream result;
  result << "{ " << cl_channel_orderToString(Value().image_channel_order) << ", "
         << cl_channel_typeToString(Value().image_channel_data_type) << " }";
  return result.str();
}

std::string gits::OpenCL::Ccl_GLenum::ToString() const {
  return OpenGL::CGLenum::EnumString(Value());
}

/******************** CALLBACKS ********************/

const char* gits::OpenCL::CCallbackContext::NAME =
    "void (CL_CALLBACK *)(const char *, const void *, size_t, void *)";

void gits::OpenCL::CCallbackContext::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

const char* gits::OpenCL::CCallbackProgram::NAME =
    "void (CL_CALLBACK *)(cl_program program, void * user_data)";

void gits::OpenCL::CCallbackProgram::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  // if we provide callback to clCompile/LinkProgram they will be asynchronous
  // we don't have support for that.
  Value() = nullptr;
}

const char* gits::OpenCL::CCallbackEvent::NAME = "void (CL_CALLBACK *)(cl_event, cl_int, void *)";

void gits::OpenCL::CCallbackEvent::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

const char* gits::OpenCL::CCallbackMem::NAME = "void (CL_CALLBACK *)(cl_mem, void*)";

void gits::OpenCL::CCallbackMem::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

const char* gits::OpenCL::CCallbackSVM::NAME =
    "void (CL_CALLBACK *)(cl_command_queue, cl_uint , void **, void*)";

void gits::OpenCL::CCallbackSVM::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

/***************** CPROGRAMSOURCE ******************/

unsigned gits::OpenCL::CProgramSource::_programSourceIdx = 0;
unsigned gits::OpenCL::CProgramSource::_binarySourceIdx = 0;

std::string gits::OpenCL::CProgramSource::GetFileName(ProgramType type) {
  std::stringstream stream;
  stream << "clPrograms/kernel_";
  if (type == PROGRAM_SOURCE) {
    stream << "source_" << std::setfill('0') << std::setw(2) << _programSourceIdx++ << ".cl";
  } else if (type == PROGRAM_BINARY) {
    stream << "binary_" << std::setfill('0') << std::setw(2) << _binarySourceIdx++ << ".bin";
  }
  return stream.str();
}

std::string gits::OpenCL::CProgramSource::GetProgramSource(const char** strings,
                                                           const cl_uint count,
                                                           const size_t* lengths) {
  std::string shaderSource;
  for (unsigned i = 0; i < count; ++i) {
    if (lengths != nullptr && lengths[i] > 0) {
      shaderSource.append(strings[i], lengths[i]);
    } else {
      shaderSource.append(strings[i]);
    }
    if (!shaderSource.empty() && shaderSource.back() == '\0') {
      shaderSource.pop_back();
    }
  }
  shaderSource = RemoveDoubleDotHeaderSyntax(shaderSource);
  return shaderSource;
}

std::string gits::OpenCL::CProgramSource::GetProgramBinary(const unsigned char* binary,
                                                           const size_t length) {
  std::string shaderSource(binary, binary + length);
  return shaderSource;
}

gits::OpenCL::CProgramSource::CProgramSource(cl_uint count,
                                             const char** strings,
                                             const size_t* lengths)
    : CArgumentFileText(GetFileName(PROGRAM_SOURCE), GetProgramSource(strings, count, lengths)) {}

gits::OpenCL::CProgramSource::CProgramSource(const unsigned char* binary, const size_t length)
    : CArgumentFileText(GetFileName(PROGRAM_BINARY), GetProgramBinary(binary, length)) {}

gits::OpenCL::CProgramSource::CProgramSource(const void* binary, const size_t length)
    : CArgumentFileText(GetFileName(PROGRAM_BINARY),
                        GetProgramBinary(static_cast<const unsigned char*>(binary), length)) {}

const char** gits::OpenCL::CProgramSource::Value() {
  text_cstr = Text().c_str();
  return &text_cstr;
}

size_t* gits::OpenCL::CProgramSource::Length() {
  text_length = Text().size();
  return &text_length;
}

/***************** CBINARIESARRAY ******************/

gits::OpenCL::CBinariesArray::CBinariesArray(const cl_uint count,
                                             const unsigned char** binaries,
                                             const size_t* lengths)
    : _binaries(0), _text(0) {
  _binaries.reserve(count);
  for (cl_uint i = 0; i < count; i++) {
    _binaries.push_back(
        std::unique_ptr<CProgramSource>(new CProgramSource(binaries[i], lengths[i])));
  }
}

void gits::OpenCL::CBinariesArray::Write(CBinOStream& stream) const {
  uint32_t size = ensure_unsigned32bit_representible<size_t>(_binaries.size());
  if (!Configurator::Get().common.recorder.nullIO) {
    stream.write((char*)&size, sizeof(size));
  }

  if (size != 0) {
    for (unsigned idx = 0; idx < size; idx++) {
      _binaries[idx]->Write(stream);
    }
  }
}

void gits::OpenCL::CBinariesArray::Read(CBinIStream& stream) {
  uint32_t size = 0;
  const auto ret = stream.read(reinterpret_cast<char*>(&size), sizeof(size));
  if (size > 0U && size <= UINT32_MAX && ret) {
    _binaries.resize(size);
    for (auto idx = 0U; idx < size; idx++) {
      _binaries[idx] = std::make_unique<CProgramSource>();
      _binaries[idx]->Read(stream);
    }
  }
}

const unsigned char** gits::OpenCL::CBinariesArray::Value() {
  _text.clear();
  for (auto const& binary : _binaries) {
    _text.push_back(**binary);
  }
  return _text.data();
}

uint64_t gits::OpenCL::CBinariesArray::Size() const {
  uint64_t total = sizeof(uint32_t);
  for (const auto& bin : _binaries) {
    if (bin) {
      total += bin->Size();
    }
  }
  return total;
}

/***************** CBINARIESARRAY_V1 ******************/

gits::OpenCL::CBinariesArray_V1::CBinariesArray_V1(const cl_uint& count,
                                                   const uint8_t** binaries,
                                                   const size_t* lengths) {
  const auto programHashes = HashBinaryData(count, binaries, lengths);
  for (const auto& state : SD()._programStates) {
    for (const auto& stateHash : state.second->GetBinaryHash()) {
      for (const auto& programHash : programHashes) {
        if (stateHash == programHash) {
          _programOriginal = state.first;
          _linkMode = ProgramBinaryLink::program;
          return;
        }
      }
    }
  }
  if (_linkMode == ProgramBinaryLink::binary) {
    _binaries.reserve(count);
    for (auto i = 0U; i < count; i++) {
      _binaries.push_back(std::make_unique<CProgramSource>(binaries[i], lengths[i]));
    }
  }
}

std::vector<std::string> gits::OpenCL::CBinariesArray_V1::FileNames() const {
  std::vector<std::string> fileNames;
  if (_linkMode == ProgramBinaryLink::program) {
    auto program = _programOriginal;
    if (Configurator::IsPlayer()) {
      program = Ccl_program::GetMapping(_programOriginal);
    }
    const auto& fileName = SD().GetProgramState(program, EXCEPTION_MESSAGE).fileName;
    fileNames.push_back(fileName);
  } else {
    for (const auto& binary : _binaries) {
      fileNames.push_back(binary->FileName());
    }
  }
  return fileNames;
}

uint64_t gits::OpenCL::CBinariesArray_V1::Size() const {
  uint64_t total = sizeof(_linkMode);
  if (_linkMode == ProgramBinaryLink::program) {
    total += sizeof(_programOriginal);
  } else {
    total += sizeof(uint32_t);
    for (const auto& bin : _binaries) {
      if (bin) {
        total += bin->Size();
      }
    }
  }
  return total;
}

void gits::OpenCL::CBinariesArray_V1::Write(CBinOStream& stream) const {
  stream << CBuffer(&_linkMode, sizeof(_linkMode));
  if (_linkMode == ProgramBinaryLink::program) {
    stream << CBuffer(&_programOriginal, sizeof(_programOriginal));
  } else {
    const auto size = ensure_unsigned32bit_representible<size_t>(_binaries.size());
    if (!Configurator::Get().common.recorder.nullIO) {
      stream.write((char*)&size, sizeof(size));
    }
    if (size != 0U) {
      for (auto i = 0U; i < size; i++) {
        _binaries.at(i)->Write(stream);
      }
    }
  }
}

void gits::OpenCL::CBinariesArray_V1::Read(CBinIStream& stream) {
  stream >> CBuffer(&_linkMode, sizeof(_linkMode));
  if (_linkMode == ProgramBinaryLink::program) {
    stream >> CBuffer(&_programOriginal, sizeof(_programOriginal));
  } else {
    uint32_t size = 0;
    const auto ret = stream.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (size > 0U && size <= UINT32_MAX && ret) {
      _binaries.resize(size);
      for (auto idx = 0U; idx < size; idx++) {
        _binaries.at(idx) = std::make_unique<CProgramSource>();
        _binaries.at(idx)->Read(stream);
      }
    }
  }
}

const unsigned char** gits::OpenCL::CBinariesArray_V1::Value() {
  if (_linkMode == ProgramBinaryLink::program) {
    if (_binariesArray.empty()) {
      cl_program program = Ccl_program::GetMapping(_programOriginal);
      auto& programState = SD().GetProgramState(program, EXCEPTION_MESSAGE);
      const auto* bPointers = programState.Binaries();
      const auto* bLengths = programState.BinarySizes();
      const auto size = programState.BinariesCount();
      _binariesArray.resize(size);
      for (auto i = 0U; i < size; i++) {
        _binariesArray.at(i).reserve(bLengths[i]);
        _binariesArray.at(i).assign(bPointers[i], bPointers[i] + bLengths[i]);
      }
    }
    _text.clear();
    for (auto const& binary : _binariesArray) {
      _text.push_back(binary.data());
    }
    return _text.data();
  }
  _text.clear();
  for (auto const& binary : _binaries) {
    _text.push_back(**binary);
  }
  return _text.data();
}

/******************** CVOIDPTR ********************/

const char* gits::OpenCL::CvoidPtr::NAME = "void *";

const char* gits::OpenCL::CCLMappedPtr::NAME = "void*";

char gits::OpenCL::CCLUserData::_buffer = 0x55;

void gits::OpenCL::CCLUserData::Read(CBinIStream& stream) {
  CvoidPtr::Read(stream);
  // Callback is replaced with empty function during playback, so we don't
  // need to pass original user data, we still want to pass something
  // to the driver though.
  if (Value()) {
    Value() = &_buffer;
  }
}

gits::OpenCL::CCLMappedPtr::CCLMappedPtr(CLType value, bool onlyMap)
    : CCLArgObj(value), _hasData(!onlyMap && value) {
  if (_hasData) {
    auto& mappedBufferStates = SD()._mappedBufferStates;
    _data.reset(RESOURCE_DATA_RAW, mappedBufferStates[value].back().buffer.data(),
                mappedBufferStates[value].back().size);
    mappedBufferStates[value].pop_back();
    if (mappedBufferStates[value].empty()) {
      mappedBufferStates.erase(value);
    }
  }
}

void gits::OpenCL::CCLMappedPtr::Write(CBinOStream& stream) const {
  CCLArgObj::Write(stream);
  stream << CBuffer(_hasData);
  if (_hasData) {
    stream << _data;
  }
}

void gits::OpenCL::CCLMappedPtr::Read(CBinIStream& stream) {
  CCLArgObj::Read(stream);
  stream >> CBuffer(_hasData);
  if (_hasData) {
    stream >> _data;
  }
}

void gits::OpenCL::CCLMappedPtr::SyncBuffer() {
  if (_hasData) {
    std::copy_n((const char*)_data.Data(), _data.Data().Size(), (char*)Value());
  }
}

uint64_t gits::OpenCL::CCLMappedPtr::Size() const {
  return CCLArgObj::Size() + sizeof(_hasData) + (_hasData ? _data.Size() : 0);
}

/******************** CCLKernelExecInfo ********************/

const char* gits::OpenCL::CCLKernelExecInfo::NAME = "void *";

gits::OpenCL::CCLKernelExecInfo::CCLKernelExecInfo(cl_kernel_exec_info param_name,
                                                   const void* param_value,
                                                   size_t param_value_size) {
  switch (param_name) {
  case CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL:
  case CL_KERNEL_EXEC_INFO_SVM_PTRS:
    _svmPtrs = std::unique_ptr<CCLMappedPtr::CSArray>(new CCLMappedPtr::CSArray(
        param_value_size / sizeof(void*), static_cast<void**>(const_cast<void*>(param_value))));
    break;
  case CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM:
    *_fineGrainSystemParam = *static_cast<cl_bool*>(const_cast<void*>(param_value));
    break;
  default:
    throw ENotImplemented("CCLKernelExecInfo param_name unsupported.");
  }
}

void* gits::OpenCL::CCLKernelExecInfo::operator*() {
  if (_svmPtrs != nullptr) {
    return **_svmPtrs;
  } else {
    return static_cast<void*>(&*_fineGrainSystemParam);
  }
}

void gits::OpenCL::CCLKernelExecInfo::Write(CBinOStream& stream) const {
  bool isSVMPtrArray = _svmPtrs == nullptr ? false : true;
  stream << CBuffer(&isSVMPtrArray, sizeof(isSVMPtrArray));
  if (isSVMPtrArray) {
    _svmPtrs->Write(stream);
  } else {
    _fineGrainSystemParam.Write(stream);
  }
}

void gits::OpenCL::CCLKernelExecInfo::Read(CBinIStream& stream) {
  bool isSVMPtrArray;
  stream >> CBuffer(&isSVMPtrArray, sizeof(isSVMPtrArray));
  if (isSVMPtrArray) {
    auto ptrs = new CCLMappedPtr::CSArray();
    ptrs->Read(stream);
    _svmPtrs = std::unique_ptr<CCLMappedPtr::CSArray>(ptrs);
  } else {
    _fineGrainSystemParam.Read(stream);
  }
}

uint64_t gits::OpenCL::CCLKernelExecInfo::Size() const {
  uint64_t total = sizeof(bool);
  if (_svmPtrs) {
    total += _svmPtrs->Size();
  } else {
    total += _fineGrainSystemParam.Size();
  }
  return total;
}

/******************** CCLKernelExecInfo_V1 ********************/

const char* gits::OpenCL::CCLKernelExecInfo_V1::NAME = "void *";

gits::OpenCL::CCLKernelExecInfo_V1::CCLKernelExecInfo_V1(cl_kernel_exec_info param_name,
                                                         const void* param_value,
                                                         size_t param_value_size) {
  switch (param_name) {
  case CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL:
  case CL_KERNEL_EXEC_INFO_SVM_PTRS:
    param_ptrs = std::unique_ptr<CCLMappedPtr::CSArray>(new CCLMappedPtr::CSArray(
        param_value_size / sizeof(void*), static_cast<void**>(const_cast<void*>(param_value))));
    type = KernelExecInfoType::pointers;
    break;
  case CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL:
  case CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM:
    *param_bool = *static_cast<cl_bool*>(const_cast<void*>(param_value));
    type = KernelExecInfoType::boolean;
    break;
  case CL_KERNEL_EXEC_INFO_THREAD_ARBITRATION_POLICY_INTEL:
  case CL_KERNEL_EXEC_INFO_KERNEL_TYPE_INTEL:
    *param_uint = *static_cast<cl_uint*>(const_cast<void*>(param_value));
    type = KernelExecInfoType::uint;
    break;
  default:
    throw ENotImplemented("CCLKernelExecInfo_V1: cl_kernel_exec_info param_name unsupported.");
  }
}

void* gits::OpenCL::CCLKernelExecInfo_V1::operator*() {
  switch (type) {
  case KernelExecInfoType::pointers:
    return **param_ptrs;
  case KernelExecInfoType::boolean:
    return static_cast<void*>(&*param_bool);
  case KernelExecInfoType::uint:
    return static_cast<void*>(&*param_uint);
  default:
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void gits::OpenCL::CCLKernelExecInfo_V1::Write(CBinOStream& stream) const {
  stream << CBuffer(&type, sizeof(type));
  switch (type) {
  case KernelExecInfoType::pointers:
    param_ptrs->Write(stream);
    break;
  case KernelExecInfoType::boolean:
    param_bool.Write(stream);
    break;
  case KernelExecInfoType::uint:
    param_uint.Write(stream);
    break;
  default:
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void gits::OpenCL::CCLKernelExecInfo_V1::Read(CBinIStream& stream) {
  stream >> CBuffer(&type, sizeof(type));
  switch (type) {
  case KernelExecInfoType::pointers: {
    auto ptrs = new CCLMappedPtr::CSArray();
    ptrs->Read(stream);
    param_ptrs = std::unique_ptr<CCLMappedPtr::CSArray>(ptrs);
    break;
  }
  case KernelExecInfoType::boolean:
    param_bool.Read(stream);
    break;
  case KernelExecInfoType::uint:
    param_uint.Read(stream);
    break;
  default:
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

uint64_t gits::OpenCL::CCLKernelExecInfo_V1::Size() const {
  // Serialized as: type + depending on type either bool, uint or array
  uint64_t total = sizeof(type);
  switch (type) {
  case KernelExecInfoType::boolean:
    total += param_bool.Size();
    break;
  case KernelExecInfoType::uint:
    total += param_uint.Size();
    break;
  case KernelExecInfoType::pointers:
    if (param_ptrs) {
      total += param_ptrs->Size();
    }
    break;
  }
  return total;
}

unsigned gits::OpenCL::CBinaryData::_lastId = 0;

gits::OpenCL::CBinaryData::CBinaryData(const size_t size, const void* buffer)
    : _size(ensure_unsigned32bit_representible<size_t>(size)),
      _ptr((void*)buffer),
      _buffer(0, '\0') {
  if (_size && _ptr) {
    _buffer.resize(_size);
    std::copy_n((const char*)buffer, _size, _buffer.begin());
  }
}

gits::OpenCL::CBinaryData::~CBinaryData() {}

void gits::OpenCL::CBinaryData::Write(CBinOStream& stream) const {
  stream << CBuffer(&_size, sizeof(_size));
  // needed for negative scenarios
  stream << CBuffer(&_ptr, sizeof(_ptr));
  if (_size && !_buffer.empty()) {
    _resource.reset(RESOURCE_DATA_RAW, _buffer.data(), _size);
    stream << _resource;
  }
}

void gits::OpenCL::CBinaryData::Read(CBinIStream& stream) {
  stream >> CBuffer(&_size, sizeof(_size));
  // needed for negative scenarios
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  if (_size && _ptr) {
    stream >> _resource;
    _buffer.resize(_resource.Data().Size());
    std::copy_n((const char*)_resource.Data(), _resource.Data().Size(), _buffer.begin());
  }
}

void gits::OpenCL::CBinaryData::Deallocate() {
  std::vector<char>().swap(_buffer);
}

uint64_t gits::OpenCL::CBinaryData::Size() const {
  uint64_t total = sizeof(_size) + sizeof(_ptr);
  if (!_buffer.empty()) {
    total += _resource.Size();
  }
  return total;
}

/******************** CKERNELARGVALUE ********************/

gits::OpenCL::CKernelArgValue::CKernelArgValue(const size_t len, const void* buffer)
    : CBinaryData(len, buffer), _obj(nullptr) {}

const void* gits::OpenCL::CKernelArgValue::operator*() {
  if (Value() != nullptr) {
    if (Length() == sizeof(cl_mem)) {
      cl_mem handle = *static_cast<const cl_mem*>(Value());
      if (Ccl_mem::CheckMapping(handle)) {
        _obj = static_cast<const void*>(Ccl_mem::GetMapping(handle));
        return &_obj;
      }
    }
    if (Length() == sizeof(cl_sampler)) {
      cl_sampler handle = *static_cast<const cl_sampler*>(Value());
      if (Ccl_sampler::CheckMapping(handle)) {
        _obj = static_cast<const void*>(Ccl_sampler::GetMapping(handle));
        return &_obj;
      }
    }
  }
  return Value();
}

/******************** CKERNELARGVALUE_V1 ********************/

gits::OpenCL::CKernelArgValue_V1::CKernelArgValue_V1(const size_t len, const void* buffer)
    : CBinaryData(len, buffer), _obj(nullptr) {}

void gits::OpenCL::CKernelArgValue_V1::Write(CBinOStream& stream) const {
  stream << CBuffer(&_size, sizeof(_size));
  stream << CBuffer(&_ptr, sizeof(_ptr));
  if (_size && !_buffer.empty()) {
    stream << CBuffer(_buffer.data(), _size);
  }
}

void gits::OpenCL::CKernelArgValue_V1::Read(CBinIStream& stream) {
  stream >> CBuffer(&_size, sizeof(_size));
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  if (_size) {
    _buffer.resize(_size);
    stream >> CBuffer(_buffer.data(), _size);
  }
}

const void* gits::OpenCL::CKernelArgValue_V1::operator*() {
  if (Value() != nullptr) {
    if (Length() == sizeof(cl_mem)) {
      cl_mem handle = *static_cast<const cl_mem*>(Value());
      if (Ccl_mem::CheckMapping(handle)) {
        _obj = static_cast<const void*>(Ccl_mem::GetMapping(handle));
        return &_obj;
      }
    }
    if (Length() == sizeof(cl_sampler)) {
      cl_sampler handle = *static_cast<const cl_sampler*>(Value());
      if (Ccl_sampler::CheckMapping(handle)) {
        _obj = static_cast<const void*>(Ccl_sampler::GetMapping(handle));
        return &_obj;
      }
    }
    if (Length() == 0 || Length() == sizeof(void*)) {
      const auto allocInfo = GetOriginalMappedPtrFromRegion(Value());
      if (allocInfo.first != nullptr) {
        _obj = GetOffsetPointer(allocInfo.first, allocInfo.second);
        return &_obj;
      }
    } else if (Length() > sizeof(void*)) {
      uintptr_t potentialPointer = 0U;
      size_t i = 0U;
      while (_size > i && _size - i > sizeof(void*)) {
        std::memcpy(&potentialPointer, _buffer.data() + i, sizeof(uintptr_t));
        const auto allocPair =
            GetOriginalMappedPtrFromRegion(reinterpret_cast<void*>(potentialPointer));
        if (allocPair.first != nullptr) {
          potentialPointer =
              reinterpret_cast<uintptr_t>(GetOffsetPointer(allocPair.first, allocPair.second));
          std::memcpy(_buffer.data() + i, &potentialPointer, sizeof(uintptr_t));
          LOG_TRACEV << "Fetching pointer("
                     << ToStringHelper(reinterpret_cast<void*>(potentialPointer)) << ") on offset "
                     << std::to_string(i) << " inside local memory.";
          i += sizeof(void*) - 1;
        }
        i++;
      }
    }
  }
  return Value();
}

/******************** CASYNCCLBUFFER ********************/

gits::OpenCL::CAsyncBinaryData::CAsyncBinaryData(size_t len, const void* appPtr, bool read)
    : _appPtr(appPtr), _len(len), _resource() {
  if (read == false) {
    _resource.reset(RESOURCE_DATA_RAW, _appPtr, _len);
  }
}

gits::OpenCL::CAsyncBinaryData::CAsyncBinaryData(const cl_image_format imageFormat,
                                                 const cl_image_desc imageDesc,
                                                 void* appPtr)
    : _appPtr(appPtr), _len(0) {
  if (appPtr) {
    _len = CountImageSize(imageFormat, imageDesc);
  }
  _resource.reset(RESOURCE_DATA_RAW, _appPtr, _len);
}

gits::OpenCL::CAsyncBinaryData::CAsyncBinaryData(const cl_image_format imageFormat,
                                                 size_t imageWidth,
                                                 size_t imageHeight,
                                                 size_t imageRowPitch,
                                                 void* appPtr)
    : _appPtr(appPtr), _len(0) {
  if (appPtr) {
    _len = CountImageSize(imageFormat, imageWidth, imageHeight, imageRowPitch);
  }
  _resource.reset(RESOURCE_DATA_RAW, _appPtr, _len);
}

gits::OpenCL::CAsyncBinaryData::CAsyncBinaryData(const cl_image_format imageFormat,
                                                 size_t imageWidth,
                                                 size_t imageHeight,
                                                 size_t imageDepth,
                                                 size_t imageRowPitch,
                                                 size_t imageSlicePitch,
                                                 void* appPtr)
    : _appPtr(appPtr), _len(0) {
  if (appPtr) {
    _len = CountImageSize(imageFormat, imageWidth, imageHeight, imageDepth, imageRowPitch,
                          imageSlicePitch);
  }
  _resource.reset(RESOURCE_DATA_RAW, _appPtr, _len);
}

gits::CBinaryResource::PointerProxy gits::OpenCL::CAsyncBinaryData::Value() {
  if (Configurator::IsRecorder()) {
    return gits::CBinaryResource::PointerProxy(_appPtr);
  } else {
    return _resource.Data();
  }
}

void gits::OpenCL::CAsyncBinaryData::Write(CBinOStream& stream) const {
  stream << CBuffer(&_len, sizeof(_len));
  if (_len) {
    stream << CBuffer(&_appPtr, sizeof(_appPtr));
    stream << _resource;
  }
}

void gits::OpenCL::CAsyncBinaryData::Read(CBinIStream& stream) {
  stream >> CBuffer(&_len, sizeof(_len));
  if (_len) {
    stream >> CBuffer(&_appPtr, sizeof(_appPtr));
    stream >> _resource;
  }
}

uint64_t gits::OpenCL::CAsyncBinaryData::Size() const {
  // Serialized via CBinaryResource plus headers for length/appPtr
  uint64_t total = sizeof(_len);
  if (_len) {
    total += sizeof(_appPtr);
    total += _resource.Size();
  }
  return total;
}

/******************** CSVMPtr ********************/

const char* gits::OpenCL::CSVMPtr::NAME = "void *";

gits::OpenCL::CSVMPtr::CSVMPtr(void* ptr, size_t len, bool createdByCLSVMAlloc)
    : _createdByCLSVMAlloc(createdByCLSVMAlloc),
      _mappedPtr(createdByCLSVMAlloc ? ptr : nullptr),
      _hostPtr(createdByCLSVMAlloc ? 0 : len, createdByCLSVMAlloc ? nullptr : ptr){};

void* gits::OpenCL::CSVMPtr::operator*() {
  if (_createdByCLSVMAlloc) {
    return *_mappedPtr;
  } else {
    return const_cast<void*>((const void*)*_hostPtr);
  }
}

void gits::OpenCL::CSVMPtr::Write(CBinOStream& stream) const {
  stream << CBuffer(&_createdByCLSVMAlloc, sizeof(_createdByCLSVMAlloc));
  if (_createdByCLSVMAlloc) {
    _mappedPtr.Write(stream);
  } else {
    _hostPtr.Write(stream);
  }
}

void gits::OpenCL::CSVMPtr::Read(CBinIStream& stream) {
  stream >> CBuffer(&_createdByCLSVMAlloc, sizeof(_createdByCLSVMAlloc));
  if (_createdByCLSVMAlloc) {
    _mappedPtr.Read(stream);
  } else {
    _hostPtr.Read(stream);
  }
}

uint64_t gits::OpenCL::CSVMPtr::Size() const {
  // Mode flag + either mapped ptr size or host resource size
  return sizeof(_createdByCLSVMAlloc) +
         (_createdByCLSVMAlloc ? _mappedPtr.Size() : _hostPtr.Size());
}

/******************** CSVMPtr_V1 ********************/

const char* gits::OpenCL::CSVMPtr_V1::NAME = "void *";

gits::OpenCL::CSVMPtr_V1::CSVMPtr_V1(void* ptr, size_t len)
    : _createdByCLSVMAlloc(SD().CheckIfSVMAllocExists(GetSvmPtrFromRegion(ptr).first)),
      _mappedPtr(_createdByCLSVMAlloc ? ptr : nullptr),
      _hostPtr(_createdByCLSVMAlloc ? 0 : len, _createdByCLSVMAlloc ? nullptr : ptr),
      _offset(0) {
  const auto ptrValue = reinterpret_cast<uintptr_t>(ptr);
  for (const auto& it : SD()._svmAllocStates) {
    const auto ptrBegin = reinterpret_cast<uintptr_t>(it.first);
    if (ptrValue < ptrBegin + it.second->size && ptrValue > ptrBegin) {
      _offset = ptrValue - ptrBegin;
      _mappedPtr.Reset(reinterpret_cast<void*>(ptrValue - _offset));
    }
  }
};

void* gits::OpenCL::CSVMPtr_V1::operator*() {
  if (_createdByCLSVMAlloc) {
    if (_mappedPtr.CheckMapping()) {
      return reinterpret_cast<void*>((uintptr_t)*_mappedPtr + _offset);
    } else {
      LOG_WARNING << "Given pointer is out of bound";
      return nullptr;
    }
  } else {
    return *_hostPtr;
  }
}

void gits::OpenCL::CSVMPtr_V1::Write(CBinOStream& stream) const {
  stream << CBuffer(&_createdByCLSVMAlloc, sizeof(_createdByCLSVMAlloc));
  stream << CBuffer(&_offset, sizeof(_offset));
  if (_createdByCLSVMAlloc) {
    _mappedPtr.Write(stream);
  } else {
    _hostPtr.Write(stream);
  }
}

void gits::OpenCL::CSVMPtr_V1::Read(CBinIStream& stream) {
  stream >> CBuffer(&_createdByCLSVMAlloc, sizeof(_createdByCLSVMAlloc));
  stream >> CBuffer(&_offset, sizeof(_offset));
  if (_createdByCLSVMAlloc) {
    _mappedPtr.Read(stream);
  } else {
    _hostPtr.Read(stream);
  }
}
uint64_t gits::OpenCL::CSVMPtr_V1::Size() const {
  // Mode flag + offset + either mapped ptr size or host buffer size
  return sizeof(_createdByCLSVMAlloc) + sizeof(_offset) +
         (_createdByCLSVMAlloc ? _mappedPtr.Size() : _hostPtr.Size());
}
/******************** CBuildOptions ********************/

gits::OpenCL::CGetContextInfoOutArgument::CGetContextInfoOutArgument(const size_t size,
                                                                     const void* buffer,
                                                                     cl_context_info param_info)
    : CBinaryData(size, buffer) {}

/******************** CUSMPtr ********************/
const char* gits::OpenCL::CUSMPtr::NAME = "void *";

gits::OpenCL::CUSMPtr::CUSMPtr(void* ptr, size_t len)
    : _createdByCLUSMAlloc(SD().CheckIfUSMAllocExists(GetUsmPtrFromRegion(ptr).first)),
      _hostPtr(_createdByCLUSMAlloc ? 0 : len, _createdByCLUSMAlloc ? nullptr : ptr),
      _mappedPtr(_createdByCLUSMAlloc ? ptr : nullptr),
      _offset(0) {
  SetMappedOffset(ptr);
};

gits::OpenCL::CUSMPtr::CUSMPtr(const void* ptr)
    : _createdByCLUSMAlloc(true),
      _hostPtr(0, nullptr),
      _mappedPtr(const_cast<void*>(ptr)),
      _offset(0) {
  SetMappedOffset(const_cast<void*>(ptr));
}

void gits::OpenCL::CUSMPtr::SetMappedOffset(void* ptr) {
  const auto ptrValue = reinterpret_cast<uintptr_t>(ptr);
  for (const auto& it : SD()._usmAllocStates) {
    const auto ptrBegin = reinterpret_cast<uintptr_t>(it.first);
    if (ptrValue < ptrBegin + it.second->size && ptrValue > ptrBegin) {
      _offset = ptrValue - ptrBegin;
      _mappedPtr.Reset(reinterpret_cast<void*>(ptrValue - _offset));
      return;
    }
  }
  for (const auto& it : SD()._svmAllocStates) {
    const auto ptrBegin = reinterpret_cast<uintptr_t>(it.first);
    if (ptrValue < ptrBegin + it.second->size && ptrValue > ptrBegin) {
      _offset = ptrValue - ptrBegin;
      _mappedPtr.Reset(reinterpret_cast<void*>(ptrValue - _offset));
      return;
    }
  }
}

void gits::OpenCL::CUSMPtr::Write(CBinOStream& stream) const {
  stream << CBuffer(&_createdByCLUSMAlloc, sizeof(_createdByCLUSMAlloc));
  stream << CBuffer(&_offset, sizeof(_offset));
  if (_createdByCLUSMAlloc) {
    _mappedPtr.Write(stream);
  } else {
    _hostPtr.Write(stream);
  }
}

void gits::OpenCL::CUSMPtr::Read(CBinIStream& stream) {
  stream >> CBuffer(&_createdByCLUSMAlloc, sizeof(_createdByCLUSMAlloc));
  stream >> CBuffer(&_offset, sizeof(_offset));
  if (_createdByCLUSMAlloc) {
    _mappedPtr.Read(stream);
  } else {
    _hostPtr.Read(stream);
  }
}

uint64_t gits::OpenCL::CUSMPtr::Size() const {
  // Mode flag + offset + either mapped ptr size or host buffer size
  return sizeof(_createdByCLUSMAlloc) + sizeof(_offset) +
         (_createdByCLUSMAlloc ? _mappedPtr.Size() : _hostPtr.Size());
}

void* gits::OpenCL::CUSMPtr::Value() {
  if (_createdByCLUSMAlloc) {
    if (_mappedPtr.CheckMapping()) {
      return reinterpret_cast<void*>((uintptr_t)*_mappedPtr + _offset);
    } else {
      LOG_WARNING << "Given pointer is out of bound";
      return nullptr;
    }
  }
  return *_hostPtr;
}

/******************** Ccl_resource_barrier_descriptor_intel ********************/

const char* gits::OpenCL::Ccl_resource_barrier_descriptor_intel::NAME =
    "cl_resource_barrier_descriptor_intel";

std::string gits::OpenCL::Ccl_resource_barrier_descriptor_intel::ToString() const {
  std::stringstream result;
  result << "{ " << _struct.svm_allocation_pointer << ", " << _struct.mem_object << ", "
         << cl_resource_barrier_typeToString(_struct.type) << " }"
         << cl_resource_memory_scopeToString(_struct.scope) << ", ";
  return result.str();
}

uint64_t gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Size() const {
  // Struct fields: svm allocation mapped ptr + mem object + type + scope
  return _svm_allocation_pointer.Size() + _mem_object.Size() + _type.Size() + _scope.Size();
}

cl_resource_barrier_descriptor_intel* gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Ptr() {
  auto memHandle = *_mem_object;
  if (Ccl_mem::CheckMapping(memHandle)) {
    memHandle = Ccl_mem::GetMapping(memHandle);
  }
  _struct.mem_object = memHandle;
  auto svmHandle = *_svm_allocation_pointer;
  if (CCLMappedPtr::CheckMapping(svmHandle)) {
    svmHandle = CCLMappedPtr::GetMapping(svmHandle);
  }
  _struct.svm_allocation_pointer = svmHandle;
  _struct.type = *_type;
  _struct.scope = *_scope;
  return &_struct;
}

gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Ccl_resource_barrier_descriptor_intel(
    const CLType& value)
    : _svm_allocation_pointer(value.svm_allocation_pointer),
      _mem_object(value.mem_object),
      _type(value.type),
      _scope(value.scope) {}

void gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Write(CBinOStream& stream) const {
  _svm_allocation_pointer.Write(stream);
  _mem_object.Write(stream);
  _type.Write(stream);
  _scope.Write(stream);
}

void gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Read(CBinIStream& stream) {
  _svm_allocation_pointer.Read(stream);
  _mem_object.Read(stream);
  _type.Read(stream);
  _scope.Read(stream);
}
