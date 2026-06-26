// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace DirectX {

struct AdapterSpoofConfig {
  std::string Description;
  unsigned VendorId = 0;
  unsigned DeviceId = 0;
};

class AdapterSpoofLayer : public Layer {
public:
  AdapterSpoofLayer(const AdapterSpoofConfig& cfg);
  ~AdapterSpoofLayer() = default;

  void Post(IDXGIAdapterGetDescCommand& command) override;
  void Post(IDXGIAdapter1GetDesc1Command& command) override;
  void Post(IDXGIAdapter2GetDesc2Command& command) override;
  void Post(IDXGIAdapter4GetDesc3Command& command) override;

private:
  AdapterSpoofConfig m_Cfg;
};

} // namespace DirectX
} // namespace gits
