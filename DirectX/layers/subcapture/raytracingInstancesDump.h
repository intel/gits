// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      : raytracingService_(raytracingService) {}
  void buildTlas(ID3D12GraphicsCommandList* commandList,
                 ID3D12Resource* resource,
                 unsigned offset,
                 unsigned size,
                 D3D12_RESOURCE_STATES state,
                 unsigned buildCall);
  void buildTlasArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                ID3D12Resource* resource,
                                unsigned offset,
                                unsigned size,
                                D3D12_RESOURCE_STATES state,
                                unsigned buildCall,
                                std::vector<unsigned>& arrayOfPointersOffsets);
  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  AnalyzerRaytracingService& raytracingService_;
  std::mutex mutex_;

  struct InstancesInfo : DumpInfo {
    unsigned buildCall{};
    std::vector<unsigned> arrayOfPointersOffsets;
  };
};

} // namespace DirectX
} // namespace gits
