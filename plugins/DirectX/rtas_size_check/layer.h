// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "messageBus.h"
#include <d3d12.h>

namespace gits {
namespace DirectX {

class RtasSizeCheckLayer final : public Layer {
public:
  RtasSizeCheckLayer(MessageBus* msgBus);
  ~RtasSizeCheckLayer() = default;

  // Capture time addresses and output
  void pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  // Playback time addresses and output
  void post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;

private:
  MessageBus* msgBus_;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO lastCaptureTimePrebuildInfo_;
};

} // namespace DirectX
} // namespace gits
