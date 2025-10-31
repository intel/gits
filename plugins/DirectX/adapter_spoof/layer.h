// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {

class CGits;

namespace DirectX {

struct AdapterSpoofConfig {
  std::string description = "";
  unsigned vendorId = 0;
  unsigned deviceId = 0;
};

class AdapterSpoofLayer : public Layer {
public:
  AdapterSpoofLayer(CGits& gits, const AdapterSpoofConfig& cfg);
  ~AdapterSpoofLayer() = default;

  void post(IDXGIAdapterGetDescCommand& c) override;
  void post(IDXGIAdapter1GetDesc1Command& c) override;
  void post(IDXGIAdapter2GetDesc2Command& c) override;
  void post(IDXGIAdapter4GetDesc3Command& c) override;

private:
  CGits& gits_;
  AdapterSpoofConfig cfg_;
};

} // namespace DirectX
} // namespace gits
