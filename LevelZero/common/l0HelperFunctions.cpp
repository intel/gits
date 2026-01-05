// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0HelperFunctions.h"
#include "MemorySniffer.h"
#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"
#include "l0Log.h"
#include "l0StateTracking.h"

gits::CArgument& gits::l0::CGitsL0MemoryUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _usmPtr, _resource);
}

gits::l0::CGitsL0MemoryUpdate::CGitsL0MemoryUpdate(const void* usmPtr) {
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(const_cast<void*>(usmPtr), sd);
  _usmPtr = allocInfo.first;
  LOG_TRACEV << "CGitsL0MemoryUpdate(" << ToStringHelper(_usmPtr) << ")";
  auto& allocState = sd.Get<CAllocState>(_usmPtr, EXCEPTION_MESSAGE);
  auto& handle = allocState.sniffedRegionHandle;
  const auto size = allocState.size;
  if (allocState.memType == UnifiedMemoryType::shared) {
    // Omit GPU migration
    std::vector<char> buffer(size);
    const auto cmdList = GetCommandListImmediate(sd, drv, allocState.hContext);
    drv.zeCommandListAppendMemoryCopy(cmdList, buffer.data(), _usmPtr, buffer.size(), nullptr, 0,
                                      nullptr);
    _resource.reset(RESOURCE_DATA_RAW, buffer.data(), size);
  } else {
    _resource.reset(RESOURCE_DATA_RAW, (const char*)_usmPtr, size);
  }
  (**handle).Reset();
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (!SD().isProtectionWrapper) {
    l0IFace.MemorySnifferProtect(handle);
  }
}

void gits::l0::CGitsL0MemoryUpdate::Run() {
  if (_resource.Data()) {
    void* pointerToData = CMappedPtr::GetMapping(_usmPtr);
    LOG_TRACEV << "CGitsL0MemoryUpdate(" << ToStringHelper(pointerToData) << ")";
    // CPU Write
    std::memcpy(pointerToData, _resource.Data(), _resource.Data().Size());
    // GPU Write
    auto& sd = SD();
    auto& allocState = sd.Get<CAllocState>(pointerToData, EXCEPTION_MESSAGE);
    if (allocState.memType == UnifiedMemoryType::shared) {
      auto cmdList = GetCommandListImmediate(sd, drv, allocState.hContext);
      drv.zeCommandListAppendMemoryCopy(cmdList, pointerToData, _resource.Data(),
                                        _resource.Data().Size(), nullptr, 0, nullptr);
    }
    TranslatePointerOffsets(sd, pointerToData, allocState.indirectPointersOffsets, true);
    _resource.Deallocate();
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
  _resource.reset(RESOURCE_DATA_RAW, usmPtr, size);
}

gits::l0::CGitsL0MemoryRestore::CGitsL0MemoryRestore(const void* usmPtr,
                                                     const void* resourcePtr,
                                                     const size_t& size)
    : _usmPtr(const_cast<void*>(usmPtr)) {
  _resource.reset(RESOURCE_DATA_RAW, resourcePtr, size);
}

gits::l0::CGitsL0MemoryRestore::CGitsL0MemoryRestore(const void* globalPointer,
                                                     const std::vector<char>& globalPtrAllocation)
    : _usmPtr(const_cast<void*>(globalPointer)) {
  _resource.reset(RESOURCE_DATA_RAW, globalPtrAllocation.data(), globalPtrAllocation.size());
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
    _resource.Deallocate();
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
    LOG_INFO << "Recorded Application uses multiple threads.";
    LOG_WARNING << "Multithreaded applications have to be "
                   "recorded from beginning. Subcapturing from stream is "
                   "possible without the --faithfulThreading option.";
  };
  LOG_TRACE << "Current thread: " << threadId;
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
    LOG_INFO << "Multithreaded stream";
  };
  CGits::Instance().CurrentThreadId(_threadId);
  LOG_TRACE << "Make current thread: " << _threadId;
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
  void* pFunctionAddress = nullptr;
  const auto drivers = GetDrivers(drv);
  for (const auto& driver : drivers) {
    if (drv.inject.zeDriverGetExtensionFunctionAddress(driver, "zeGitsOriginalQueueFamilyInfo",
                                                       &pFunctionAddress) == ZE_RESULT_SUCCESS) {
      drv.zeGitsOriginalQueueFamilyInfo(_hDevice.Value(),
                                        static_cast<uint32_t>(_cqGroupProperties.Vector().size()),
                                        _cqGroupProperties.Value());
      break;
    }
  }
  zeGitsOriginalQueueFamilyInfo_SD(ZE_RESULT_SUCCESS, _hDevice.Value(),
                                   static_cast<uint32_t>(_cqGroupProperties.Vector().size()),
                                   _cqGroupProperties.Value());
}
