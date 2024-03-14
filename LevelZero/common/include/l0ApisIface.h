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
#include "l0StateDynamic.h"
#include "gits.h"
#include "l0Tools.h"

namespace gits {
namespace l0 {
class Api : public ApisIface::ApiCompute {
public:
  Api() : ApiCompute(ApisIface::LevelZero) {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().recorder.levelZero.capture.mode.find("All") != std::string::npos;
  }
  virtual bool CfgRec_IsSubcapture() const {
    return Config::Get().recorder.levelZero.capture.mode.find("Kernel") != std::string::npos;
  }
  virtual bool CfgRec_IsKernelsRangeMode() const {
    return CfgRec_IsSubcapture() && !Config::Get().recorder.levelZero.capture.kernel.singleCapture;
  }
  virtual bool CfgRec_IsSingleKernelMode() const {
    return CfgRec_IsSubcapture() && Config::Get().recorder.levelZero.capture.kernel.singleCapture;
  }
  virtual int CfgRec_StartKernel() const {
    return Config::Get().recorder.levelZero.capture.kernel.startKernel;
  }
  virtual int CfgRec_StopKernel() const {
    return Config::Get().recorder.levelZero.capture.kernel.stopKernel;
  }
  virtual unsigned int CfgRec_StartCommandList() const {
    return Config::Get().recorder.levelZero.capture.kernel.startCommandList;
  }
  virtual unsigned int CfgRec_StopCommandList() const {
    return Config::Get().recorder.levelZero.capture.kernel.stopCommandList;
  }
  virtual unsigned int CfgRec_StartCommandQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.startCommandQueueSubmit;
  }
  virtual unsigned int CfgRec_StopCommandQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.stopCommandQueueSubmit;
  }
  virtual bool CfgRec_IsStartQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.startCommandQueueSubmit ==
           CGits::Instance().CurrentCommandQueueExecCount();
  }
  virtual bool CfgRec_IsStopQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.stopCommandQueueSubmit ==
           CGits::Instance().CurrentCommandQueueExecCount();
  }
  virtual bool CfgRec_IsStopOfSubcapture(const uint32_t cmdListNumber,
                                         const uint32_t kernelNumber) const {
    return CfgRec_IsStopQueueSubmit() && CfgRec_StopCommandList() == cmdListNumber &&
           static_cast<uint32_t>(CfgRec_StopKernel()) == kernelNumber;
  }
  virtual void MemorySnifferUninstall() const {
    if (!IsMemorySnifferInstalled()) {
      return;
    }
    for (auto& state : SD().Map<CAllocState>()) {
      auto& handle = state.second->sniffedRegionHandle;
      if (handle != nullptr) {
        MemorySnifferUnProtect(handle);
        MemorySniffer::Get().RemoveRegion(handle);
        handle = nullptr;
      }
    }
    MemorySniffer::UnInstall();
  }
  virtual void Rec_StateRestoreFinished() const {
    SD().stateRestoreFinished = true;
  }
  virtual bool VerifyAllocation(void* address) const {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(address, sd);
    const auto isComputeAllocation = allocInfo.first != nullptr;
    return isComputeAllocation;
  }
  virtual ~Api() = default; // Fixes the -Wdelete-non-virtual-dtor warning.
};
} // namespace l0
} // namespace gits
