// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "apis_iface.h"
#include "openclTools.h"
#include "openclStateDynamic.h"

namespace gits {
namespace OpenCL {
class OpenCLApi : public ApisIface::ApiCompute {
public:
  OpenCLApi() : ApiCompute(ApisIface::OpenCL) {}
  virtual bool CfgRec_IsAllMode() const {
    return Configurator::Get().opencl.recorder.mode == TOpenCLRecorderMode::ALL;
  }
  virtual bool CfgRec_IsSingleKernelMode() const {
    return Configurator::Get().opencl.recorder.mode == TOpenCLRecorderMode::SINGLE_KERNEL;
  }
  virtual int CfgRec_SingleKernel() const {
    return Configurator::Get().opencl.recorder.oclSingleKernel.number;
  }
  virtual bool CfgRec_IsKernelsRangeMode() const {
    return Configurator::Get().opencl.recorder.mode == TOpenCLRecorderMode::KERNELS_RANGE;
  }
  virtual int CfgRec_StartKernel() const {
    return Configurator::Get().opencl.recorder.oclKernelsRange.startKernel;
  }
  virtual int CfgRec_StopKernel() const {
    return Configurator::Get().opencl.recorder.oclKernelsRange.stopKernel;
  }
  virtual void MemorySnifferUninstall() const {
    if (!IsMemorySnifferInstalled()) {
      return;
    }
    for (auto& state : SD()._usmAllocStates) {
      auto& handle = state.second->sniffedRegionHandle;
      if (handle != nullptr) {
        MemorySnifferUnProtect(handle);
        MemorySniffer::Get().RemoveRegion(handle);
        handle = nullptr;
      }
    }
    for (auto& state : SD()._svmAllocStates) {
      auto& handle = state.second->sniffedRegionHandle;
      if (handle != nullptr) {
        MemorySnifferUnProtect(handle);
        MemorySniffer::Get().RemoveRegion(handle);
        handle = nullptr;
      }
    }
    MemorySniffer::UnInstall();
  }
  virtual bool VerifyAllocation(void* address) const {
    const auto allocInfo = GetSvmOrUsmFromRegion(address);
    return allocInfo.first != nullptr;
  }
  virtual bool VerifyAllocationShared(void* address) const {
    auto allocInfo = GetSvmPtrFromRegion(address);
    auto& sd = SD();
    if (sd.CheckIfSVMAllocExists(allocInfo.first)) {
      return (sd.GetSVMAllocState(allocInfo.first, EXCEPTION_MESSAGE).flags &
              CL_MEM_SVM_FINE_GRAIN_BUFFER) != 0;
    }
    allocInfo = GetUsmPtrFromRegion(address);
    if (sd.CheckIfUSMAllocExists(allocInfo.first)) {
      return sd.GetUSMAllocState(allocInfo.first, EXCEPTION_MESSAGE).type ==
             UnifiedMemoryType::shared;
    }
    return false;
  }
  virtual ~OpenCLApi() = default; // Fixes the -Wdelete-non-virtual-dtor warning.
};
} // namespace OpenCL
} // namespace gits
