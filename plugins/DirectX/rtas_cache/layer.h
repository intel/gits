// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "rtasSerializer.h"
#include "rtasDeserializer.h"

namespace gits {
namespace DirectX {

struct RtasCacheConfig {
  std::string cacheFile = "rtas_cache.dat";
  bool record = false;
  bool stateRestoreOnly = true;
  bool dumpCacheInfoFile = false;
};

class RtasCacheLayer : public Layer {
public:
  RtasCacheLayer(const RtasCacheConfig& cfg);
  ~RtasCacheLayer();

  void pre(StateRestoreBeginCommand& c) override;
  void pre(StateRestoreEndCommand& c) override;
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
    return !cfg_.record && isValid_ && (cfg_.stateRestoreOnly ? stateRestore_ : true);
  }

private:
  RtasCacheConfig cfg_;
  RtasSerializer serializer_;
  RtasDeserializer deserializer_;
  bool stateRestore_{false};
  bool isValid_{true};
  unsigned cachedBlasCount_{0};
  unsigned blasCount_{0};
};

} // namespace DirectX
} // namespace gits
