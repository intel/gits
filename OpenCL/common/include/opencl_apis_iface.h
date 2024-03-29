// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "apis_iface.h"
#include "config.h"
#include "openclTools.h"
#include "openclStateDynamic.h"

namespace gits {
namespace OpenCL {
class OpenCLApi : public ApisIface::ApiCompute {
public:
  OpenCLApi() : ApiCompute(ApisIface::OpenCL) {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().recorder.openCL.capture.mode.find("All") != std::string::npos;
  }
  virtual bool CfgRec_IsSingleKernelMode() const {
    return Config::Get().recorder.openCL.capture.mode.find("OclSingleKernel") != std::string::npos;
  }
  virtual int CfgRec_SingleKernel() const {
    return Config::Get().recorder.openCL.capture.oclSingleKernel.number;
  }
  virtual bool CfgRec_IsKernelsRangeMode() const {
    return Config::Get().recorder.openCL.capture.mode.find("OclKernelsRange") != std::string::npos;
  }
  virtual int CfgRec_StartKernel() const {
    return Config::Get().recorder.openCL.capture.oclKernelsRange.startKernel;
  }
  virtual int CfgRec_StopKernel() const {
    return Config::Get().recorder.openCL.capture.oclKernelsRange.stopKernel;
  }
  virtual bool CfgRec_IsBenchmark() const {
    return false;
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
  virtual ~OpenCLApi() = default; // Fixes the -Wdelete-non-virtual-dtor warning.
};
} // namespace OpenCL
} // namespace gits
