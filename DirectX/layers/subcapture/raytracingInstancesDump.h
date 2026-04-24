// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

#include <mutex>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService;

class RaytracingInstancesDump : public ResourceDump {
public:
  RaytracingInstancesDump(AnalyzerRaytracingService& raytracingService)
      : m_RaytracingService(raytracingService) {}
  void BuildTlas(ID3D12GraphicsCommandList* commandList,
                 ID3D12Resource* resource,
                 unsigned offset,
                 unsigned size,
                 BarrierState state,
                 unsigned buildCall);
  void BuildTlasArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                ID3D12Resource* resource,
                                unsigned offset,
                                unsigned size,
                                BarrierState state,
                                unsigned buildCall,
                                std::vector<unsigned>& arrayOfPointersOffsets);

private:
  void DumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  AnalyzerRaytracingService& m_RaytracingService;
  std::mutex m_Mutex;

  struct InstancesInfo : DumpInfo {
    unsigned BuildCall{};
    std::vector<unsigned> ArrayOfPointersOffsets;
  };
};

} // namespace DirectX
} // namespace gits
