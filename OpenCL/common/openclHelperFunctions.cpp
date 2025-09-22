// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclHelperFunctions.h"
#include "MemorySniffer.h"
#include "exception.h"
#include "openclTools.h"
#include "openclArgumentsAuto.h"
#include "openclStateDynamic.h"
#include "openclArguments.h"

gits::CArgument& gits::OpenCL::CGitsClMemoryUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _ptr, _resource);
}

gits::OpenCL::CGitsClMemoryUpdate::CGitsClMemoryUpdate(void* ptr) {
  std::vector<char> buffer;
  _ptr = GetSvmOrUsmFromRegion(ptr).first;
  PagedMemoryRegionHandle handle = nullptr;
  size_t size = 0;
  if (SD().CheckIfUSMAllocExists(_ptr)) {
    auto& allocState = SD().GetUSMAllocState(_ptr, EXCEPTION_MESSAGE);
    size = allocState.size;
    handle = allocState.sniffedRegionHandle;
  } else if (SD().CheckIfSVMAllocExists(_ptr)) {
    auto& allocState = SD().GetSVMAllocState(_ptr, EXCEPTION_MESSAGE);
    size = allocState.size;
    handle = allocState.sniffedRegionHandle;
  } else {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
  buffer = std::vector<char>(size, 0);
  std::memcpy(buffer.data(), _ptr, buffer.size());
  _resource.reset(RESOURCE_DATA_RAW, (const char*)buffer.data(), buffer.size());
  (**handle).Reset();
  if (!(**handle).Protected()) {
    // do nothing
  } else {
    oclIFace.MemorySnifferProtect(handle);
  }
}

void gits::OpenCL::CGitsClMemoryUpdate::Write(CCodeOStream& stream) const {
  throw ENotImplemented("CGitsClMemoryUpdate not implemented for CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
}

void gits::OpenCL::CGitsClMemoryUpdate::Run() {
  if (_resource.Data()) {
    char* pointerToData = (char*)CCLMappedPtr::GetMapping(_ptr);
    if (SD().CheckIfSVMAllocExists(pointerToData)) {
      std::memcpy(pointerToData, _resource.Data(),
                  SD().GetSVMAllocState(pointerToData, EXCEPTION_MESSAGE).size);
    } else {
      std::memcpy(pointerToData, _resource.Data(),
                  SD().GetUSMAllocState(pointerToData, EXCEPTION_MESSAGE).size);
    }
  }
}

void gits::OpenCL::CGitsClMemoryUpdate::Write(CBinOStream& stream) const {
  stream << CBuffer(&_ptr, sizeof(_ptr));
  _resource.Write(stream);
}

void gits::OpenCL::CGitsClMemoryUpdate::Read(CBinIStream& stream) {
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  _resource.Read(stream);
}

gits::CArgument& gits::OpenCL::CGitsClMemoryRestore::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _ptr, _length, _resource);
}

gits::OpenCL::CGitsClMemoryRestore::CGitsClMemoryRestore(void* ptr) : _ptr(ptr) {
  if (SD().CheckIfUSMAllocExists(ptr)) {
    _length = SD().GetUSMAllocState(ptr, EXCEPTION_MESSAGE).size;
  } else {
    _length = SD().GetSVMAllocState(ptr, EXCEPTION_MESSAGE).size;
  }
  _resource.reset(RESOURCE_DATA_RAW, (char*)ptr, _length);
}

gits::OpenCL::CGitsClMemoryRestore::CGitsClMemoryRestore(void* ptr, const size_t& size)
    : _ptr(ptr), _length(static_cast<uint64_t>(size)) {
  _resource.reset(RESOURCE_DATA_RAW, (char*)ptr, _length);
}

void gits::OpenCL::CGitsClMemoryRestore::Run() {
  if (_resource.Data()) {
    std::memcpy((char*)CCLMappedPtr::GetMapping(_ptr), _resource.Data(), _length);
  }
}

void gits::OpenCL::CGitsClMemoryRestore::Write(CBinOStream& stream) const {
  stream << CBuffer(&_ptr, sizeof(_ptr));
  stream << CBuffer(&_length, sizeof(_length));
  _resource.Write(stream);
}

void gits::OpenCL::CGitsClMemoryRestore::Read(CBinIStream& stream) {
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  stream >> CBuffer(&_length, sizeof(_length));
  _resource.Read(stream);
}

gits::CArgument& gits::OpenCL::CGitsClMemoryRegionRestore::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _ptr, _length, _offset, _resource);
}

gits::OpenCL::CGitsClMemoryRegionRestore::CGitsClMemoryRegionRestore(void* ptr,
                                                                     const uint64_t& offset,
                                                                     const uint64_t& size)
    : _ptr(ptr), _length(size), _offset(offset) {
  const char* offset_ptr = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(ptr) + offset);
  _resource.reset(RESOURCE_DATA_RAW, offset_ptr, _length);
}

void gits::OpenCL::CGitsClMemoryRegionRestore::Run() {
  if (_resource.Data()) {
    auto ptr = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(CCLMappedPtr::GetMapping(_ptr)) +
                                       _offset);
    std::memcpy(ptr, _resource.Data(), _length);
  }
}

void gits::OpenCL::CGitsClMemoryRegionRestore::Write(CBinOStream& stream) const {
  stream << CBuffer(&_ptr, sizeof(_ptr));
  stream << CBuffer(&_length, sizeof(_length));
  stream << CBuffer(&_offset, sizeof(_offset));
  _resource.Write(stream);
}

void gits::OpenCL::CGitsClMemoryRegionRestore::Read(CBinIStream& stream) {
  stream >> CBuffer(&_ptr, sizeof(_ptr));
  stream >> CBuffer(&_length, sizeof(_length));
  stream >> CBuffer(&_offset, sizeof(_offset));
  _resource.Read(stream);
}

gits::CArgument& gits::OpenCL::CGitsClTokenMakeCurrentThread::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _threadId);
}
gits::OpenCL::CGitsClTokenMakeCurrentThread::CGitsClTokenMakeCurrentThread(const int& threadId)
    : _threadId(threadId) {
  CALL_ONCE[] {
    LOG_INFO << "Recorded Application uses multiple threads.";
    LOG_WARNING << "Multithreaded applications have to be "
                   "recorded from beginning. Subcapturing from stream is "
                   "possible without the --faithfulThreading option.";
    if (Configurator::DumpCCode() && CGits::Instance().MultithreadedApp()) {
      LOG_ERROR << "CCodeDump is not possible for multithreaded application. Please "
                   "record binary stream first and then recapture it to CCode";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  };
  LOG_TRACE << "Current thread: " << threadId;
  CGits::Instance().CurrentThreadId(threadId);
}

void gits::OpenCL::CGitsClTokenMakeCurrentThread::Write(CBinOStream& stream) const {
  write_to_stream(stream, _threadId);
}

void gits::OpenCL::CGitsClTokenMakeCurrentThread::Read(CBinIStream& stream) {
  read_from_stream(stream, _threadId);
}

void gits::OpenCL::CGitsClTokenMakeCurrentThread::Run() {
  CALL_ONCE[] {
    LOG_INFO << "Multithreaded stream";
  };
  CGits::Instance().CurrentThreadId(_threadId);
  LOG_TRACE << "Make current thread: " << _threadId;
}
