// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  // Values are stored in stream headers; keep them unchanged.
  enum TApi {
    ApiNotSet = 0,
    OpenGL = 1,
    Vulkan = 2,    // no longer supported, identifies old streams
    OpenCL = 3,    // no longer supported, identifies old streams
    LevelZero = 4, // no longer supported, identifies old streams
    DirectX = 5,
  };

  static gits::ApiBool ToApiBool(const TApi& api) {
    switch (api) {
    case OpenGL:
      return gits::ApiBool::OGL;
    case Vulkan:
      return gits::ApiBool::VK;
    case DirectX:
      return gits::ApiBool::DX;
    case OpenCL:
      return gits::ApiBool::OCL;
    case LevelZero:
      return gits::ApiBool::L0;
    default:
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
  }

  class ApiIface {
    TApi _api;

  public:
    ApiIface(TApi api) : _api(api) {
      switch (api) {
      case OpenGL:
      case Vulkan:
      case DirectX:
        break;
      default:
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
    TApi Api() const {
      return _api;
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

private:
  std::shared_ptr<Api3d> _3d;

public:
  ApisIface() {}
  void UseApi3dIface(std::shared_ptr<Api3d> iface);

  bool Has3D() const {
    return (_3d.get() != 0);
  }
  const Api3d& Iface3D() const {
    if (_3d.get() == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    return *_3d;
  }
};
} // namespace gits
