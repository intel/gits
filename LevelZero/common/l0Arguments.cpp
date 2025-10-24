// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Arguments.h"
#include "l0ArgumentsAuto.h"

namespace gits {
namespace l0 {
const char* Cze_host_pfn_t::NAME = "ze_host_pfn_t";

void Cze_host_pfn_t::Read(CBinIStream& stream) {
  CArg::Read(stream);
  if (Value()) {
    Value() = Callback;
  }
}

const char* CvoidPtr::NAME = "void *";

CKernelArgValue::CKernelArgValue(const size_t len, const void* buffer)
    : _size(ensure_unsigned32bit_representible<size_t>(len)),
      _ptr((void*)buffer),
      _buffer(0, '\0') {
  if (_size && _ptr) {
    _buffer.resize(_size);
    std::copy_n((const char*)buffer, _size, _buffer.begin());
  }
}

void CKernelArgValue::Write(CBinOStream& stream) const {
  stream << CBuffer(&_size, sizeof(_size));
  stream << CBuffer(&_ptr, sizeof(_ptr));
  write_to_stream(stream, _buffer.empty());
  if (_size && !_buffer.empty()) {
    stream << CBuffer(_buffer.data(), _size);
  }
}

void CKernelArgValue::Read(CBinIStream& stream) {
  bool emptyBuffer = false;
  stream >> CBuffer(&_size, sizeof(_size));
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  read_from_stream(stream, emptyBuffer);
  if (_size && !emptyBuffer) {
    _buffer.resize(_size);
    stream >> CBuffer(_buffer.data(), _size);
  }
}

const void* CKernelArgValue::operator*() {
  if (_obj != nullptr) {
    return &_obj;
  }
  if (Value() != nullptr) {
    if (Length() == sizeof(void*)) {
      uintptr_t handle = *static_cast<const uintptr_t*>(Value());
      const auto allocPair = GetAllocFromOriginalPtr(reinterpret_cast<void*>(handle), SD());
      if (allocPair.first != nullptr) {
        _obj = static_cast<const void*>(GetOffsetPointer(allocPair.first, allocPair.second));
        return &_obj;
      }
      ze_image_handle_t h_img = reinterpret_cast<ze_image_handle_t>(handle);
      if (Cze_image_handle_t::CheckMapping(h_img)) {
        _obj = Cze_image_handle_t::GetMapping(h_img);
        return &_obj;
      }
    }
    if (Length() >= sizeof(void*) && !_buffer.empty() && !_localMemoryScanned) {
      uintptr_t potentialPointer = 0U;
      size_t i = 0U;
      while (_size > i && _size - i >= sizeof(void*)) {
        std::memcpy(&potentialPointer, _buffer.data() + i, sizeof(uintptr_t));
        const auto allocPair =
            GetAllocFromOriginalPtr(reinterpret_cast<void*>(potentialPointer), SD());
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
      _localMemoryScanned = true;
    }
  }
  return Value();
}

const char* CMappedPtr::NAME = "void*";

void CUSMPtr::Write(CBinOStream& stream) const {
  stream << CBuffer(&_size, sizeof(_size));
  stream << CBuffer(&_ptr, sizeof(_ptr));
  stream << _resource;
}

void CUSMPtr::Read(CBinIStream& stream) {
  stream >> CBuffer(&_size, sizeof(_size));
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  stream >> _resource;
}

void CUSMPtr::FreeHostMemory() {
  DeallocateVector(_temp_buffer);
  _resource.Deallocate();
}

uint32_t SourceFileInfo::_programSourceIdx = 0;
uint32_t SourceFileInfo::_binarySourceIdx = 0;

std::string SourceFileInfo::CreateFileName(ze_module_format_t format) {
  std::stringstream stream;
  stream << "l0Programs/kernel_";
  if (format == ZE_MODULE_FORMAT_IL_SPIRV) {
    stream << "source_" << std::setfill('0') << std::setw(2) << _programSourceIdx++ << ".spv";
  } else if (format == ZE_MODULE_FORMAT_NATIVE) {
    stream << "binary_" << std::setfill('0') << std::setw(2) << _binarySourceIdx++ << ".bin";
  } else {
    LOG_ERROR << "Module format " << ToStringHelper<ze_module_format_t>(format)
              << " is not supported";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  return stream.str();
}

SourceFileInfo::SourceFileInfo(const size_t _size, const uint8_t* _data, ze_module_format_t format)
    : SourceFileInfo(_size, _data, CreateFileName(format)) {}

SourceFileInfo::SourceFileInfo(const size_t _size, const uint8_t* _data, std::string _filename)
    : data(const_cast<uint8_t*>(_data), const_cast<uint8_t*>(_data) + _size),
      filename(std::move(_filename)) {}

/***************** CProgramSource ******************/
CProgramSource::CProgramSource(const uint8_t* text, size_t size, ze_module_format_t format)
    : sourceFile(*Length(), reinterpret_cast<const uint8_t*>(Text().c_str()), format) {
  if (size > 0) {
    _fileName = sourceFile.filename;
    _text = std::string(reinterpret_cast<const char*>(text), size);
    init(_fileName.c_str(), _text.c_str(), size);
  }
}

CProgramSource::CProgramSource(SourceFileInfo sourceFile) : sourceFile(sourceFile) {
  if (sourceFile.data.size() > 0) {
    _fileName = sourceFile.filename;
    _text =
        std::string(reinterpret_cast<const char*>(sourceFile.data.data()), sourceFile.data.size());
    init(_fileName.c_str(), _text.c_str(), sourceFile.data.size());
  }
}

const char** CProgramSource::Value() {
  if (*Length() == 0) {
    return nullptr;
  }
  textCstr = Text().c_str();
  return &textCstr;
}

size_t* CProgramSource::Length() {
  textLength = Text().size();
  return &textLength;
}

void CProgramSource::Write(CBinOStream& stream) const {
  if (Text().size() > 0) {
    CArgumentFileText::Write(stream);
  } else {
    stream << '"' << emptyFileName << '"';
  }
}

void CProgramSource::Read(CBinIStream& stream) {
  stream.get_delimited_string(_fileName, '"');
  if (_fileName != emptyFileName) {
    LoadTextFromFile();
    sourceFile =
        SourceFileInfo(*Length(), reinterpret_cast<const uint8_t*>(Text().c_str()), _fileName);
  }
}

SourceFileInfo CProgramSource::Original() {
  return sourceFile;
}

CProgramSourceArray::CProgramSourceArray(const size_t count,
                                         const uint8_t** data,
                                         const size_t* sizes,
                                         const ze_module_format_t format)
    : CArgumentSizedArray(ConvertToArray(count, data, sizes, format)) {}

std::vector<SourceFileInfo> CProgramSourceArray::ConvertToArray(const size_t count,
                                                                const uint8_t** data,
                                                                const size_t* sizes,
                                                                const ze_module_format_t format) {
  std::vector<SourceFileInfo> ret;
  for (size_t i = 0; i < count; i++) {
    ret.push_back(SourceFileInfo(sizes[i], data[i], format));
  }
  return ret;
}

const uint8_t** CProgramSourceArray::Value() {
  dataVector.clear();
  for (const auto& sourceInfo : Vector()) {
    dataVector.push_back(const_cast<uint8_t*>(sourceInfo.data.data()));
  }
  return const_cast<const uint8_t**>(dataVector.data());
}

CBinaryData::CBinaryData(const size_t size, const void* buffer)
    : _size(size), _ptr((void*)buffer), _buffer(0, '\0') {
  if (_size && _ptr) {
    _buffer.resize(_size);
    std::copy_n((const char*)buffer, _size, _buffer.begin());
  }
}

void CBinaryData::Write(CBinOStream& stream) const {
  stream << CBuffer(&_size, sizeof(_size));
  stream << CBuffer(&_ptr, sizeof(_ptr));
  if (_size && !_buffer.empty()) {
    _resource.reset(RESOURCE_DATA_RAW, _buffer.data(), _size);
    stream << _resource;
  }
}

void CBinaryData::Read(CBinIStream& stream) {
  stream >> CBuffer(&_size, sizeof(_size));
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  if (_size && _ptr) {
    stream >> _resource;
    _buffer.resize(_resource.Data().Size());
    std::copy_n((const char*)_resource.Data(), _resource.Data().Size(), _buffer.begin());
  }
}
} // namespace l0
} // namespace gits
