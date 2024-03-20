// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

void Cze_host_pfn_t::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "l0::host_pfn";
  } else {
    stream << "(ze_host_pfn_t) 0";
  }
}
const char* CvoidPtr::NAME = "void *";

void CvoidPtr::Write(CCodeOStream& stream) const {
  stream << "(void *)" << std::hex << (intptr_t)Value() << std::dec;
}

CKernelArgValue::CKernelArgValue(const size_t len, const void* buffer)
    : _size(ensure_unsigned32bit_representible<size_t>(len)),
      _ptr((void*)buffer),
      _buffer(0, '\0') {
  if (_size && _ptr) {
    _buffer.resize(_size);
    std::copy_n((const char*)buffer, _size, _buffer.begin());
  }
}

void CKernelArgValue::Declare(CCodeOStream& stream) const {
  if (_size == sizeof(void*)) {
    uintptr_t ptr = 0;
    memcpy(&ptr, Value(), sizeof(void*));
    stream.Indent() << "uintptr_t " << stream.VariableName(ScopeKey()) << " = " << hex(ptr) << ";"
                    << std::endl;
    stream.Indent() << "if(CArgHandle::CheckMapping(" << hex(ptr) << "))" << std::endl;
    stream.ScopeBegin();
    stream.Indent() << stream.VariableName(ScopeKey()) << " = CArgHandle::GetMapping(" << hex(ptr)
                    << ");" << std::endl;
    stream.ScopeEnd();
  } else {
    if (_size && _buffer.empty()) {
      return;
    }
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

void CKernelArgValue::Write(CCodeOStream& stream) const {
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
          Log(TRACEV) << "Fetching pointer("
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

void CUSMPtr::Declare(CCodeOStream& stream) const {
  stream.Indent() << "uintptr_t " << stream.VariableName(ScopeKey()) << " = " << hex(_ptr) << ";"
                  << std::endl;
  stream.Indent() << "if(CArgHandle::CheckMapping(" << hex(_ptr) << "))" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << stream.VariableName(ScopeKey()) << " = CArgHandle::GetMapping(" << hex(_ptr)
                  << ");" << std::endl;
  stream.ScopeEnd();
}

void CUSMPtr::Write(CCodeOStream& stream) const {
  stream << "(void*)" << stream.VariableName(ScopeKey());
}

/***************** CProgramSource ******************/
uint32_t CProgramSource::_programSourceIdx = 0;
uint32_t CProgramSource::_binarySourceIdx = 0;

std::string CProgramSource::GetFileName(ze_module_format_t format) {
  std::stringstream stream;
  stream << "l0Programs/kernel_";
  if (format == ZE_MODULE_FORMAT_IL_SPIRV) {
    stream << "source_" << std::setfill('0') << std::setw(2) << _programSourceIdx++ << ".spv";
  } else if (format == ZE_MODULE_FORMAT_NATIVE) {
    stream << "binary_" << std::setfill('0') << std::setw(2) << _binarySourceIdx++ << ".bin";
  } else {
    Log(ERR) << "Module format " << ToStringHelper<ze_module_format_t>(format)
             << " is not supported";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  return stream.str();
}

std::string CProgramSource::GetProgramBinary(const unsigned char* binary, const size_t length) {
  std::string shaderSource(binary, binary + length);
  return shaderSource;
}

CProgramSource::CProgramSource(const uint8_t* text, size_t size, ze_module_format_t format)
    : CArgumentFileText(GetFileName(format), GetProgramBinary(text, size)) {}

const char** CProgramSource::Value() {
  textCstr = Text().c_str();
  return &textCstr;
}

size_t* CProgramSource::Length() {
  textLength = Text().size();
  return &textLength;
}
} // namespace l0
} // namespace gits
