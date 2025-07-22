// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include <d3d12.h>

namespace gits {

class CGits;

namespace DirectX {

class RtasSizeCheckLayer final : public Layer {
public:
  RtasSizeCheckLayer(CGits& gits);
  ~RtasSizeCheckLayer() = default;

  // Capture time addresses and output
  void pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  // Playback time addresses and output
  void post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;

private:
  CGits& gits_;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO lastCaptureTimePrebuildInfo_;
};

} // namespace DirectX
} // namespace gits
