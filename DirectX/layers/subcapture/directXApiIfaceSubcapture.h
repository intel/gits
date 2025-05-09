// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "apis_iface.h"

namespace gits {
namespace DirectX {

class DirectXApiIfaceSubcapture : public ApisIface::Api3d {
public:
  DirectXApiIfaceSubcapture() : Api3d(ApisIface::DirectX) {}
  virtual bool CfgRec_IsAllMode() const {
    return !Configurator::Get().directx.features.subcapture.enabled;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return Configurator::Get().directx.features.subcapture.enabled;
  };
  virtual int CfgRec_StartFrame() const {
    auto& cfgDirectX = Configurator::Get().directx;
    if (cfgDirectX.features.subcapture.enabled) {
      const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
      size_t pos = frames.find("-");
      if (pos != std::string::npos) {
        return std::stoi(frames.substr(0, pos));
      } else {
        return std::stoi(frames);
      }
    } else {
      return 1;
    }
  }
  virtual int CfgRec_StopFrame() const {
    auto& cfgDirectX = Configurator::Get().directx;
    if (cfgDirectX.features.subcapture.enabled) {
      const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
      size_t pos = frames.find("-");
      if (pos != std::string::npos) {
        return std::stoi(frames.substr(pos + 1));
      } else {
        return std::stoi(frames);
      }
    } else {
      return INT_MAX;
    }
  }
};

} // namespace DirectX
} // namespace gits
