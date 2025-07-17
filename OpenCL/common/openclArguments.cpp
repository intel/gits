// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      Log(WARN) << "The application has requested DirectX 11 interoperability support, but this "
                   "feature is not supported in GITS. The playback will continue, but there may be "
                   "various problems, such as crashes or corruption. If you don't need DirectX API "
                   "sharing in your OpenCL stream, consider recording the stream again with the "
                   "RemoveDXSharing option set to true in the config file.";
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
        Log(ERR) << "Could not map GL Context." << std::endl;
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
        Log(ERR) << "Could not map GL Context." << std::endl;
        throw;
      }
      break;
    }
    default:
      Log(ERR) << cl_context_propertiesToString(type) + " is not supported yet.";
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

void gits::OpenCL::Ccl_context_properties::Write(CCodeOStream& stream) const {
  std::string prop = ToString();
  if (prop[0] == 'C') {
    stream << prop;
  } else if (prop == "nullptr") {
    stream << "0";
  } else {
    stream << "(" << Name() << ") (";
    Ccl_platform_id((cl_platform_id)Value()).Write(stream);
    stream << ")";
  }
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

void gits::OpenCL::CCallbackContext::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "cl::CallbackContext";
  } else {
    stream << "(cl::FCallbackContext)"
           << "0";
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

void gits::OpenCL::CCallbackProgram::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "cl::CallbackProgram";
  } else {
    stream << "(cl::FCallbackProgram)"
           << "0";
  }
}

const char* gits::OpenCL::CCallbackEvent::NAME = "void (CL_CALLBACK *)(cl_event, cl_int, void *)";

void gits::OpenCL::CCallbackEvent::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

void gits::OpenCL::CCallbackEvent::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "cl::CallbackEvent";
  } else {
    stream << "(cl::FCallbackEvent)"
           << "0";
  }
}

const char* gits::OpenCL::CCallbackMem::NAME = "void (CL_CALLBACK *)(cl_mem, void*)";

void gits::OpenCL::CCallbackMem::Read(CBinIStream& stream) {
  CCLArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

void gits::OpenCL::CCallbackMem::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "cl::CallbackMem";
  } else {
    stream << "(cl::FCallbackMem)"
           << "0";
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

void gits::OpenCL::CCallbackSVM::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "cl::CallbackSVM";
  } else {
    stream << "(cl::FCallbackSVM)"
           << "0";
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

void gits::OpenCL::CBinariesArray::Declare(CCodeOStream& stream) const {
  // TextFiles have to be declared earlier to prevent them from going
  // out of scope (when declared directly in const char* array)
  if ((int)_binaries.size() != 0) {
    stream.Indent() << "TextFile " << stream.VariableName(ScopeKey()) << "_array[] = { ";
    for (auto iter = _binaries.begin(); iter != _binaries.end(); ++iter) {
      iter->get()->Write(stream);
      if (iter < _binaries.end() - 1) {
        stream << ", ";
      }
    }
    stream << " };\n";
  }

  stream.Indent() << Name();

  if ((int)_binaries.size() == 0) {
    stream << "*";
  }
  stream << " " << stream.VariableName(ScopeKey());
  if ((int)_binaries.size() != 0) {
    stream << "[]";
  }
  // initiate all elements in an array
  if ((int)_binaries.size() == 0) {
    stream << " = 0;\n";
  } else {
    // declare an array
    stream << " = { ";
    int idx = 0;
    for (auto iter = _binaries.begin(); iter != _binaries.end(); ++iter) {
      stream << stream.VariableName(ScopeKey()) << "_array[" << idx << "]";
      if (iter < _binaries.end() - 1) {
        stream << ", ";
      }
      idx++;
    }
    stream << " };\n";
  }
}

const unsigned char** gits::OpenCL::CBinariesArray::Value() {
  _text.clear();
  for (auto const& binary : _binaries) {
    _text.push_back(**binary);
  }
  return _text.data();
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

void gits::OpenCL::CBinariesArray_V1::Declare(CCodeOStream& stream) const {
  if (_linkMode == ProgramBinaryLink::program) {
    throw ENotImplemented("Program binary connection with built program is not "
                          "implemented for CCode.");
  }
  // TextFiles have to be declared earlier to prevent them from going
  // out of scope (when declared directly in const char* array)
  if (static_cast<int>(_binaries.size()) != 0) {
    stream.Indent() << "TextFile " << stream.VariableName(ScopeKey()) << "_array[] = { ";
    for (auto iter = _binaries.begin(); iter != _binaries.end(); ++iter) {
      iter->get()->Write(stream);
      if (iter < _binaries.end() - 1) {
        stream << ", ";
      }
    }
    stream << " };\n";
  }

  stream.Indent() << Name();

  if (_binaries.empty()) {
    stream << "*";
  }
  stream << " " << stream.VariableName(ScopeKey());
  if (!_binaries.empty()) {
    stream << "[]";
  }
  // initiate all elements in an array
  if (_binaries.empty()) {
    stream << " = 0;\n";
  } else {
    // declare an array
    stream << " = { ";
    int idx = 0;
    for (auto iter = _binaries.begin(); iter != _binaries.end(); ++iter) {
      stream << stream.VariableName(ScopeKey()) << "_array[" << idx << "]";
      if (iter < _binaries.end() - 1) {
        stream << ", ";
      }
      idx++;
    }
    stream << " };\n";
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

void gits::OpenCL::CvoidPtr::Write(CCodeOStream& stream) const {
  stream << "(void *)" << std::hex << (intptr_t)Value() << std::dec;
}

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

void gits::OpenCL::CCLUserData::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "&cl::userData";
  } else {
    stream << "(" << Name() << ")"
           << "0";
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

void gits::OpenCL::CCLMappedPtr::Write(CCodeOStream& stream) const {
  CCLArgObj::Write(stream);
}

void gits::OpenCL::CCLMappedPtr::SyncBuffer() {
  if (_hasData) {
    std::copy_n((const char*)_data.Data(), _data.Data().Size(), (char*)Value());
  }
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

void gits::OpenCL::CCLKernelExecInfo::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CCLKernelExecInfo not implemented for CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
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

void gits::OpenCL::CCLKernelExecInfo_V1::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CCLKernelExecInfo_V1 not implemented for CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
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

void gits::OpenCL::CBinaryData::Write(CCodeOStream& stream) const {
  stream << "(" << Name() << ")";
  if (_size && _ptr) {
    // use an array
    if (DeclarationNeeded()) {
      stream << stream.VariableName(ScopeKey());
      if (Length() > CODE_STREAM_INLINE_BUFFER_SIZE_MAX) {
        stream << ".data()";
      }
    } else {
      stream << "nullptr";
    }
  } else {
    stream << hex(_ptr);
    if (_ptr) {
      // negative scenario
      stream << " /* WARNING: Invalid buffer passed by the application */";
    }
  }
}

void gits::OpenCL::CBinaryData::Declare(CCodeOStream& stream) const {
  if (!Length() || !_ptr || _buffer.empty()) {
    return;
  }

  if (Length() > CODE_STREAM_INLINE_BUFFER_SIZE_MAX) {
    std::string varName = stream.VariableName(ScopeKey());
    stream.Indent() << "std::vector<char> " << varName << "(" << Length() << ");" << std::endl;
    if (_resource.GetResourceHash() != 0) {
      std::string resVarName = varName + "_res";
      stream.Indent() << "auto " << resVarName << " = " << _resource << ";" << std::endl;
      stream.Indent() << "std::copy((const char*)" << resVarName << " + 0, (const char*)"
                      << resVarName << " + " << resVarName << ".Size(), " << varName << ".begin());"
                      << std::endl;
    }
  } else {
    stream.Indent() << "unsigned char " << stream.VariableName(ScopeKey()) << "[" << Length()
                    << "]";

    // dump buffer inlined in a code
    stream << " = {" << std::endl;
    stream.ScopeBegin();
    stream.Indent();
    std::string asciiText;
    const unsigned CHARS_IN_ROW = 16;
    for (unsigned i = 0; i < Length(); i++) {
      unsigned char ch = (unsigned char)_buffer[i];
      stream << hex(ch);
      asciiText += std::isprint(ch) ? ch : '.';
      if (i != Length() - 1) {
        stream << ",";
        if ((i + 1) % CHARS_IN_ROW == 0) {
          stream << " // " << asciiText << std::endl;
          asciiText.clear();
          stream.Indent();
        }
      } else {
        stream << std::setw((CHARS_IN_ROW - (i % CHARS_IN_ROW) - 1) * 5 + 1) << " "
               << " // " << asciiText;
        asciiText.clear();
      }
    }
    stream << " " << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "};" << std::endl;
  }
}

void gits::OpenCL::CBinaryData::Deallocate() {
  std::vector<char>().swap(_buffer);
}

/******************** CKERNELARGVALUE ********************/

gits::OpenCL::CKernelArgValue::CKernelArgValue(const size_t len, const void* buffer)
    : CBinaryData(len, buffer), _obj(nullptr) {}

void gits::OpenCL::CKernelArgValue::Declare(CCodeOStream& stream) const {
  if (Length() == sizeof(void*)) {
    uintptr_t ptr = 0;
    memcpy(&ptr, Value(), sizeof(void*));
    stream.Indent() << "uintptr_t " << stream.VariableName(ScopeKey()) << " = " << hex(ptr) << ";"
                    << std::endl;
    stream.Indent() << "if(CCLArgObj::CheckMapping(" << hex(ptr) << "))" << std::endl;
    stream.ScopeBegin();
    stream.Indent() << stream.VariableName(ScopeKey()) << " = CCLArgObj::GetMapping(" << hex(ptr)
                    << ");" << std::endl;
    stream.ScopeEnd();
  } else {
    CBinaryData::Declare(stream);
  }
}

void gits::OpenCL::CKernelArgValue::Write(CCodeOStream& stream) const {
  stream << "(const void*) ";
  if (Length() == sizeof(void*)) {
    stream << "&" << stream.VariableName(ScopeKey());
  } else {
    if (Value() == nullptr) {
      stream << "nullptr";
    } else {
      stream << stream.VariableName(ScopeKey());
    }
  }
}

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

void gits::OpenCL::CKernelArgValue_V1::Declare(CCodeOStream& stream) const {
  if (_size == sizeof(void*)) {
    uintptr_t ptr = 0;
    memcpy(&ptr, Value(), sizeof(void*));
    stream.Indent() << "uintptr_t " << stream.VariableName(ScopeKey()) << " = " << hex(ptr) << ";"
                    << std::endl;
    stream.Indent() << "if(CCLArgObj::CheckMapping(" << hex(ptr) << "))" << std::endl;
    stream.ScopeBegin();
    stream.Indent() << stream.VariableName(ScopeKey()) << " = CCLArgObj::GetMapping(" << hex(ptr)
                    << ");" << std::endl;
    stream.ScopeEnd();
  } else {
    CBinaryData::Declare(stream);
  }
}

void gits::OpenCL::CKernelArgValue_V1::Write(CCodeOStream& stream) const {
  stream << "(const void*) ";
  if (_size == sizeof(void*)) {
    stream << "&" << stream.VariableName(ScopeKey());
  } else {
    if (Value() == nullptr) {
      stream << "nullptr";
    } else {
      stream << stream.VariableName(ScopeKey());
    }
  }
}

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
          Log(TRACEV) << "Fetching pointer("
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

void gits::OpenCL::CAsyncBinaryData::Write(CCodeOStream& stream) const {
  stream << "(" << Name() << ")";
  if (_len) {
    // use an array
    stream << stream.VariableName(ScopeKey()) << ".data()";
  } else {
    stream << "0";
  }
}

void gits::OpenCL::CAsyncBinaryData::Declare(CCodeOStream& stream) const {
  if (_len) {
    std::string varName = stream.VariableName(ScopeKey());
    stream.select(CCodeOStream::GITS_EXTERNS_H);
    stream.Indent() << "extern std::vector<char> " << varName << ";" << std::endl;
    stream.select(CCodeOStream::GITS_EXTERNS_CPP);
    stream.Indent() << "std::vector<char> " << varName << "(0);" << std::endl;
    stream.select(CCodeOStream::GITS_FRAMES_CPP);
    std::string resVarName = varName + "_res";
    if (_resource.GetResourceHash() != 0) {
      stream.Indent() << "auto " << resVarName << " = " << _resource << ";" << std::endl;
      stream.Indent() << "if (" << resVarName << ".Size() > " << varName << ".size())" << std::endl;
      stream.ScopeBegin(); // No real scope, just for indentation.
      stream.Indent() << varName << ".resize(" << resVarName << ".Size());" << std::endl;
      stream.ScopeEnd();
      stream.Indent() << "std::copy((const char*)" << resVarName << " + 0, (const char*)"
                      << resVarName << " + " << resVarName << ".Size(), " << varName << ".begin());"
                      << std::endl;
    } else {
      stream.Indent() << "if (" << _len << " > " << varName << ".size())" << std::endl;
      stream.Indent() << "  " << varName << ".resize(" << _len << ");" << std::endl;
    }
  }
}

void gits::OpenCL::CAsyncBinaryData::VariableNameRegister(CCodeOStream& stream,
                                                          bool returnValue) const {
  stream.Register(ScopeKey(), VariableNamePrefix(), true, GlobalScopeVariable());
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

void gits::OpenCL::CSVMPtr::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CSVMPtr not implemented for CCode.\nSet CCode to False in gits_config.txt "
                        "to skip CCode dumping.");
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
      Log(WARN) << "Given pointer is out of bound";
      return nullptr;
    }
  } else {
    return *_hostPtr;
  }
}

void gits::OpenCL::CSVMPtr_V1::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CSVMPtr_V1 not implemented for CCode.\nSet CCode to "
                        "False in gits_config.txt to skip CCode dumping.");
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
/******************** CBuildOptions ********************/

void gits::OpenCL::CBuildOptions::Declare(CCodeOStream& stream) const {
  VariableNameRegister(stream, false);
  stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = ";
  if (Cchar::CSArray::Vector().size() > 0) {
    stream << "\"" << Cchar::CSArray::ToString();
    if (_hasHeaders) {
      stream << " -I./gitsFiles";
    }
    stream << "\";\n";
  } else {
    stream << "{ 0 };\n";
  }
}

gits::OpenCL::CGetContextInfoOutArgument::CGetContextInfoOutArgument(const size_t size,
                                                                     const void* buffer,
                                                                     cl_context_info param_info)
    : CBinaryData(size, buffer) {
  _isPostActionNeeded = param_info == CL_CONTEXT_DEVICES;
}

void gits::OpenCL::CGetContextInfoOutArgument::PostAction(CCodeOStream& stream) const {
  if (_size == sizeof(void*)) {
    stream.Indent() << "CCLArgObj::AddMapping(" << stream.VariableName(ScopeKey())
                    << "_buffer.data(), " << stream.VariableName(ScopeKey()) << ", " << 1 << ");\n";
  }
}

void gits::OpenCL::CGetContextInfoOutArgument::Declare(CCodeOStream& stream) const {
  CBinaryData::Declare(stream);
  if (_size == sizeof(void*)) {
    stream.Indent() << "std::vector<unsigned char> " << stream.VariableName(ScopeKey()) + "_buffer"
                    << "(" << stream.VariableName(ScopeKey()) << ", "
                    << stream.VariableName(ScopeKey()) << " + " << Length() << ");" << std::endl;
  }
}

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

void gits::OpenCL::CUSMPtr::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CUSMPtr not implemented for CCode.\nSet CCode to False in gits_config.txt "
                        "to skip CCode dumping.");
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

void* gits::OpenCL::CUSMPtr::Value() {
  if (_createdByCLUSMAlloc) {
    if (_mappedPtr.CheckMapping()) {
      return reinterpret_cast<void*>((uintptr_t)*_mappedPtr + _offset);
    } else {
      Log(WARN) << "Given pointer is out of bound";
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

void gits::OpenCL::Ccl_resource_barrier_descriptor_intel::Write(CCodeOStream& stream) const {
  throw ENotImplemented("Experimental headers are not supported in CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
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
