// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  std::string CacheFile = "rtas_cache.dat";
  bool Record = false;
  bool StateRestoreOnly = true;
  bool DumpCacheInfoFile = false;
};

class RtasCacheLayer : public Layer {
public:
  RtasCacheLayer(const RtasCacheConfig& cfg);
  ~RtasCacheLayer();

  void Pre(StateRestoreBeginCommand& command) override;
  void Pre(StateRestoreEndCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12CommandQueueWaitCommand& command) override;
  void Post(ID3D12CommandQueueSignalCommand& command) override;
  void Post(ID3D12FenceSignalCommand& command) override;
  void Post(ID3D12DeviceCreateFenceCommand& command) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& command) override;

private:
  bool Record() const {
    return m_Cfg.Record && (m_Cfg.StateRestoreOnly ? m_StateRestore : true);
  }
  bool Replay() const {
    return !m_Cfg.Record && m_IsValid && (m_Cfg.StateRestoreOnly ? m_StateRestore : true);
  }

  RtasCacheConfig m_Cfg;
  RtasSerializer m_Serializer;
  RtasDeserializer m_Deserializer;
  bool m_StateRestore{false};
  bool m_IsValid{true};
  unsigned m_CachedBlasCount{0};
  unsigned m_BlasCount{0};
};

} // namespace DirectX
} // namespace gits
