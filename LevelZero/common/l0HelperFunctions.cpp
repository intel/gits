// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0HelperFunctions.h"
#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"

gits::CArgument& gits::l0::CGitsL0MemoryUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _usmPtr, _resource);
}

gits::l0::CGitsL0MemoryUpdate::CGitsL0MemoryUpdate(const void* usmPtr) {
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(const_cast<void*>(usmPtr), sd);
  _usmPtr = allocInfo.first;
  auto& memoryState = sd.Get<CAllocState>(_usmPtr, EXCEPTION_MESSAGE);
  _resource.reset(RESOURCE_DATA_RAW, (const char*)_usmPtr, memoryState.size);
  auto& handle = memoryState.sniffedRegionHandle;
  (**handle).Reset();
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (!(**handle).Protected()) {
    // do nothing
  } else {
    l0IFace.MemorySnifferProtect(handle);
  }
}

void gits::l0::CGitsL0MemoryUpdate::Write(CCodeOStream& /*stream*/) const {
  throw ENotImplemented("CGitsL0MemoryUpdate not implemented for CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
}

void gits::l0::CGitsL0MemoryUpdate::Run() {
  if (_resource.Data()) {
    char* pointerToData = (char*)CMappedPtr::GetMapping(_usmPtr);
    std::memcpy(pointerToData, _resource.Data(),
                SD().Get<CAllocState>(CMappedPtr::GetMapping(_usmPtr), EXCEPTION_MESSAGE).size);
  }
}

void gits::l0::CGitsL0MemoryUpdate::Write(CBinOStream& stream) const {
  stream << CBuffer(&_usmPtr, sizeof(_usmPtr));
  _resource.Write(stream);
}

void gits::l0::CGitsL0MemoryUpdate::Read(CBinIStream& stream) {
  stream >> CBuffer(&_usmPtr, sizeof(_usmPtr));
  _resource.Read(stream);
}

gits::CArgument& gits::l0::CGitsL0MemoryRestore::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _usmPtr, _resource);
}

gits::l0::CGitsL0MemoryRestore::CGitsL0MemoryRestore(const void* usmPtr, const size_t& size)
    : _usmPtr(const_cast<void*>(usmPtr)) {
  _resource.reset(RESOURCE_DATA_RAW, (const char*)usmPtr, size);
}

gits::l0::CGitsL0MemoryRestore::CGitsL0MemoryRestore(const void* usmPtr,
                                                     const void* resourcePtr,
                                                     const size_t& size)
    : _usmPtr(const_cast<void*>(usmPtr)) {
  _resource.reset(RESOURCE_DATA_RAW, (const char*)resourcePtr, size);
}

gits::l0::CGitsL0MemoryRestore::CGitsL0MemoryRestore(const void* globalPointer,
                                                     const std::vector<char>& globalPtrAllocation)
    : _usmPtr(const_cast<void*>(globalPointer)) {
  _resource.reset(RESOURCE_DATA_RAW, globalPtrAllocation.data(), globalPtrAllocation.size());
}

void gits::l0::CGitsL0MemoryRestore::Write(CCodeOStream& /*stream*/) const {
  throw ENotImplemented("CGitsL0MemoryRestore not implemented for CCode.\nSet CCode to False in "
                        "gits_config.txt to skip CCode dumping.");
}

void gits::l0::CGitsL0MemoryRestore::Run() {
  if (_resource.Data()) {
    auto& sd = SD();
    auto allocInfo = GetAllocFromOriginalPtr(_usmPtr, sd);
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    if (allocState.allocType == AllocStateType::global_pointer) {
      allocState.originalGlobalPtrAllocation.resize(_resource.Data().Size());
      std::memcpy(allocState.originalGlobalPtrAllocation.data(), _resource.Data(),
                  _resource.Data().Size());
    } else {
      std::memcpy((char*)GetOffsetPointer(allocInfo.first, allocInfo.second), _resource.Data(),
                  _resource.Data().Size());
    }
  }
}

void gits::l0::CGitsL0MemoryRestore::Write(CBinOStream& stream) const {
  stream << CBuffer(&_usmPtr, sizeof(_usmPtr));
  _resource.Write(stream);
}

void gits::l0::CGitsL0MemoryRestore::Read(CBinIStream& stream) {
  stream >> CBuffer(&_usmPtr, sizeof(_usmPtr));
  _resource.Read(stream);
}

gits::CArgument& gits::l0::CGitsL0TokenMakeCurrentThread::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _threadId);
}
gits::l0::CGitsL0TokenMakeCurrentThread::CGitsL0TokenMakeCurrentThread(const int& threadId)
    : _threadId(threadId) {
  CALL_ONCE[] {
    Log(INFO) << "Recorded Application uses multiple threads.";
    Log(WARN) << "Multithreaded applications have to be "
                 "recorded from beginning. Subcapturing from stream is "
                 "possible without the --faithfulThreading option.";
    if (Config::Get().recorder.basic.dumpCCode && CGits::Instance().MultithreadedApp()) {
      Log(ERR) << "CCodeDump is not possible for multithreaded application. Please "
                  "record binary stream first and then recapture it to CCode";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  };
  Log(TRACE) << "Current thread: " << threadId;
  CGits::Instance().CurrentThreadId(threadId);
}

void gits::l0::CGitsL0TokenMakeCurrentThread::Write(CBinOStream& stream) const {
  write_to_stream(stream, _threadId);
}

void gits::l0::CGitsL0TokenMakeCurrentThread::Read(CBinIStream& stream) {
  read_from_stream(stream, _threadId);
}

void gits::l0::CGitsL0TokenMakeCurrentThread::Run() {
  CALL_ONCE[] {
    Log(INFO) << "Multithreaded stream";
  };
  CGits::Instance().CurrentThreadId(_threadId);
  Log(TRACE) << "Make current thread: " << _threadId;
}

gits::CArgument& gits::l0::CGitsL0OriginalQueueFamilyInfo::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDevice, _stype, _cqGroupProperties);
}

gits::l0::CGitsL0OriginalQueueFamilyInfo::CGitsL0OriginalQueueFamilyInfo(
    ze_device_handle_t hDevice, const std::vector<ze_command_queue_group_properties_t>& props)
    : _hDevice(hDevice),
      _stype(ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES),
      _cqGroupProperties(static_cast<uint32_t>(props.size()), props.data()) {}

void gits::l0::CGitsL0OriginalQueueFamilyInfo::Write(CBinOStream& stream) const {
  _hDevice.Write(stream);
  _stype.Write(stream);
  _cqGroupProperties.Write(stream);
}

void gits::l0::CGitsL0OriginalQueueFamilyInfo::Read(CBinIStream& stream) {
  _hDevice.Read(stream);
  _stype.Read(stream);
  _cqGroupProperties.Read(stream);
}

void gits::l0::CGitsL0OriginalQueueFamilyInfo::Run() {
  Log(TRACE) << "Updating original command queue group properties...";
  auto& originalQueueGroupProps =
      SD().Get<CDeviceState>(_hDevice.Value(), EXCEPTION_MESSAGE).originalQueueGroupProperties;
  const auto size = _cqGroupProperties.Vector().size();
  for (auto i = 0U; i < size; i++) {
    originalQueueGroupProps.push_back(_cqGroupProperties.Value()[i]);
  }
}
