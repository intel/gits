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

class RtasSizeCheckLayer final : public Layer {
public:
  RtasSizeCheckLayer(CGits& gits);
  ~RtasSizeCheckLayer() = default;

  void post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;

private:
  CGits& gits_;
};

} // namespace DirectX
} // namespace gits
