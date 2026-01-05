// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "apis_iface.h"

namespace gits {
namespace DirectX {

class DirectXApiIfaceRecorder : public ApisIface::Api3d {
public:
  DirectXApiIfaceRecorder() : Api3d(ApisIface::DirectX) {}
  virtual bool CfgRec_IsAllMode() const {
    return true;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return false;
  };
  virtual int CfgRec_StartFrame() const {
    return 1;
  }
  virtual int CfgRec_StopFrame() const {
    return INT_MAX;
  }
};

} // namespace DirectX
} // namespace gits
