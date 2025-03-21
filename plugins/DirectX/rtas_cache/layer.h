// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "accelerationStructuresSerializer.h"
#include "accelerationStructuresDeserializer.h"

namespace gits {
class CGits;
namespace DirectX {

struct RtasCacheConfig {
  bool record = false;
  bool stateRestoreOnly = true;
};

class RtasCacheLayer : public Layer {
public:
  RtasCacheLayer(CGits& gits, const RtasCacheConfig& cfg);
  ~RtasCacheLayer();

  void pre(D3D12CreateDeviceCommand& c) override;
  void pre(IDXGISwapChainPresentCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;

private:
  bool record() const {
    return cfg_.record && (cfg_.stateRestoreOnly ? stateRestore_ : true);
  }
  bool replay() const {
    return !cfg_.record && (cfg_.stateRestoreOnly ? stateRestore_ : true);
  }

private:
  CGits& gits_;
  RtasCacheConfig cfg_;
  AccelerationStructuresSerializer accelerationStructuresSerializer_;
  AccelerationStructuresDeserializer accelerationStructuresDeserializer_;
  bool stateRestore_{};
  unsigned blasCount_{};
};

} // namespace DirectX
} // namespace gits
