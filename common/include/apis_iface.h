// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   apis_iface.h
 *
 * @brief Declaration of classes that passes generic API data between lower, generic layers and specialized APIs layers.
 *
 */

#pragma once
#include "exception.h"
#include "tools.h"
#include "MemorySniffer.h"

namespace gits {
class ApisIface {
public:
  enum TApi {
    OpenGL,
    Vulkan,
    OpenCL,
    LevelZero,
  };
  enum TApiType {
    A3D,
    ACompute
  };

  class ApiIface {
    TApi _api;
    TApiType _type;

  public:
    ApiIface(TApi api) : _api(api) {
      switch (api) {
      case OpenGL:
      case Vulkan:
        _type = A3D;
        break;
      case OpenCL:
      case LevelZero:
        _type = ACompute;
        break;
      default:
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
    TApi Api() const {
      return _api;
    }
    TApiType Type() const {
      return _type;
    }
  };

  class Api3d : public ApiIface {
  public:
    Api3d(TApi api) : ApiIface(api) {}
    virtual bool CfgRec_IsAllMode() const {
      return true;
    }
    virtual unsigned CfgRec_ExitFrame() const {
      return 10000000;
    }
    virtual bool CfgRec_IsFramesMode() const {
      return false;
    }
    virtual int CfgRec_StartFrame() const {
      return 1;
    }
    virtual int CfgRec_StopFrame() const {
      return 1;
    }
    virtual const std::vector<unsigned>& CfgRec_StartKeys() const {
      static std::vector<unsigned> vec;
      return vec;
    }
    virtual bool CfgRec_IsSingleDrawMode() const {
      return false;
    }
    virtual int CfgRec_SingleDraw() const {
      return 1;
    }
    virtual bool CfgRec_IsDrawsRangeMode() const {
      return false;
    }
    virtual bool CfgRec_IsBlitRangeMode() const {
      return false;
    }
    virtual bool CfgRec_IsDispatchRangeMode() const {
      return false;
    }
    virtual bool CfgRec_IsEncodersRangeMode() const {
      return false;
    }
    virtual bool CfgRec_IsSubEncodersRangeMode() const {
      return false;
    }
    virtual int CfgRec_StartDraw() const {
      return 1;
    }
    virtual int CfgRec_StopDraw() const {
      return 1;
    }
    virtual int CfgRec_Frame() const {
      return 0;
    }
    virtual unsigned CfgRec_EndFrameSleep() const {
      return 0;
    }
    virtual bool CfgRec_IsRenderPassMode() const {
      return false;
    }
    virtual bool CfgRec_IsCmdBufferMode() const {
      return false;
    }
    virtual bool CfgRec_IsQueueSubmitMode() const {
      return false;
    }
    virtual bool CfgRec_IsObjectToRecord(std::vector<uint64_t>& /* counter */) const {
      return false;
    }
    virtual bool CfgRec_IsObjectToRecord() const {
      return false;
    };
    virtual bool CfgRec_IsSubcapture() const {
      return false;
    };
    virtual bool CfgRec_IsSubFrameMode() const {
      return false;
    };
    virtual void Play_SwapAfterPrepare() const {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void Play_StateRestoreBegin() const {}
    virtual void Play_StateRestoreEnd() const {}
    virtual void Rec_StateRestoreFinished() const {}
  };

  class ApiCompute : public ApiIface {
  public:
    ApiCompute(TApi api) : ApiIface(api) {}
    virtual bool CfgRec_IsAllMode() const {
      return true;
    }
    virtual bool CfgRec_IsSubcapture() const {
      return false;
    }
    virtual bool CfgRec_IsSingleKernelMode() const {
      return false;
    }
    virtual int CfgRec_SingleKernel() const {
      return 1;
    }
    virtual bool CfgRec_IsKernelsRangeMode() const {
      return false;
    }
    virtual int CfgRec_StartKernel() const {
      return 1;
    }
    virtual int CfgRec_StopKernel() const {
      return 1;
    }
    virtual bool CfgRec_IsStartQueueSubmit() const {
      return false;
    }
    virtual bool CfgRec_IsStopQueueSubmit() const {
      return false;
    }
    virtual unsigned int CfgRec_StartCommandList() const {
      return 1;
    }
    virtual unsigned int CfgRec_StopCommandList() const {
      return 1;
    }
    virtual unsigned int CfgRec_StartCommandQueueSubmit() const {
      return 1;
    }
    virtual unsigned int CfgRec_StopCommandQueueSubmit() const {
      return 1;
    }
    virtual bool CfgRec_IsStopOfSubcapture([[maybe_unused]] const uint32_t cmdListNumber,
                                           [[maybe_unused]] const uint32_t kernelNumber) const {
      return false;
    }
    virtual bool CfgRec_IsObjectToRecord() {
      return false;
    }
    virtual bool MemorySnifferInstall() const;
    virtual void MemorySnifferUninstall() const {
      return;
    };
    virtual bool VerifyAllocation([[maybe_unused]] void* address) const {
      return false;
    }
    virtual bool VerifyAllocationShared([[maybe_unused]] void* address) const {
      return false;
    }
    virtual void UpdateConditionMemoryProtection() const {
      return;
    }
    virtual void MemorySnifferProtect(PagedMemoryRegionHandle& handle) const;
    virtual void MemorySnifferUnProtect(PagedMemoryRegionHandle& handle) const;
    virtual void EnableMemorySnifferForPointer(void* ptr,
                                               const size_t& size,
                                               PagedMemoryRegionHandle& handle) const;
    virtual bool IsMemorySnifferInstalled() const {
      return MemorySniffer::Get().IsInstalled();
    }
    virtual void PrintMaxLocalMemoryUsage() const;
    virtual void Play_StateRestoreEnd() const {}
    virtual void Rec_StateRestoreFinished() const {}
  };

private:
  std::shared_ptr<Api3d> _3d;
  std::shared_ptr<ApiCompute> _compute;

public:
  ApisIface() {}
  void UseApi3dIface(std::shared_ptr<Api3d> iface);
  void UseApiComputeIface(std::shared_ptr<ApiCompute> iface);

  bool Has3D() const {
    return (_3d.get() != 0);
  }
  bool HasCompute() const {
    return (_compute.get() != 0);
  }
  const Api3d& Iface3D() const {
    if (_3d.get() == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    return *_3d;
  }
  const ApiCompute& IfaceCompute() const {
    if (_compute.get() == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    return *_compute;
  }
};
} // namespace gits
