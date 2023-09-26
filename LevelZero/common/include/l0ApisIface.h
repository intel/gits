// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "apis_iface.h"
#include "config.h"
#include "l0StateDynamic.h"
#include "gits.h"

namespace gits {
namespace l0 {
class Api : public ApisIface::ApiCompute {
public:
  Api() : ApiCompute(ApisIface::LevelZero) {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().recorder.levelZero.capture.mode.find("All") != std::string::npos;
  }
  virtual bool CfgRec_IsKernelsRangeMode() const {
    return Config::Get().recorder.levelZero.capture.mode.find("Kernel") != std::string::npos;
  }
  virtual bool CfgRec_IsSingleKernelMode() const {
    return Config::Get().recorder.levelZero.capture.kernel.singleCapture;
  }
  virtual int CfgRec_StartKernel() const {
    return Config::Get().recorder.levelZero.capture.kernel.startKernel;
  }
  virtual int CfgRec_StopKernel() const {
    return Config::Get().recorder.levelZero.capture.kernel.stopKernel;
  }
  virtual bool CfgRec_IsStartQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.minQueueSubmitNumber ==
           CGits::Instance().CurrentCommandQueueExecCount();
  }
  virtual bool CfgRec_IsStopQueueSubmit() const {
    return Config::Get().recorder.levelZero.capture.kernel.maxQueueSubmitNumber ==
           CGits::Instance().CurrentCommandQueueExecCount();
  }
  virtual bool CfgRec_IsObjectToRecord() const {
    return Config::Get()
        .recorder.levelZero.capture.kernel
        .queueRange[CGits::Instance().CurrentCommandQueueExecCount()];
  }
  virtual bool CfgRec_IsCommandListToRecord(uint32_t cmdListNumber) const {
    return Config::Get().recorder.levelZero.capture.kernel.cmdListRange[cmdListNumber];
  }
  virtual bool CfgRec_IsKernelToRecord(uint32_t kernelNumber) const {
    return Config::Get().recorder.levelZero.capture.kernel.kernelRange[kernelNumber];
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
  virtual ~Api() = default; // Fixes the -Wdelete-non-virtual-dtor warning.
};
} // namespace l0
} // namespace gits
