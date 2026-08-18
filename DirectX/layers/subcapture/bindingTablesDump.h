// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "resourceDump.h"

#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService;

class BindingTablesDump : public ResourceDump {
public:
  struct StateObjectInfo {
    GITSKey GlobalRootSignature{};
    std::unordered_map<std::wstring, GITSKey> ExportToRootSignature;
  };
  struct DescriptorHeaps {
    GITSKey ViewDescriptorHeapKey;
    unsigned ViewDescriptorHeapSize;
    GITSKey SamplerHeapKey;
    unsigned SamplerHeapSize;
  };

public:
  BindingTablesDump(AnalyzerRaytracingService& raytracingService)
      : m_RaytracingService(raytracingService) {}
  void DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned offset,
                        unsigned size,
                        unsigned stride,
                        BarrierState state,
                        StateObjectInfo* stateObjectInfo,
                        DescriptorHeaps descriptorHeaps,
                        GITSKey rootSignatureKey);

  std::unordered_set<GITSKey>& GetBindingTablesResources() {
    return m_BindingTablesResources;
  }
  std::set<std::pair<GITSKey, unsigned>>& GetBindingTablesDescriptors() {
    return m_BindingTablesDescriptors;
  }

private:
  void DumpBuffer(DumpInfo& dumpInfo, void* data) override;
  unsigned Align(unsigned value, unsigned alignment);

private:
  AnalyzerRaytracingService& m_RaytracingService;
  std::mutex m_Mutex;
  std::unordered_set<GITSKey> m_BindingTablesResources;
  std::set<std::pair<GITSKey, unsigned>> m_BindingTablesDescriptors;

  struct BindingTablesInfo : DumpInfo {
    unsigned Stride{};
    StateObjectInfo* StateObjectInfo{};
    DescriptorHeaps DescriptorHeaps{};
    GITSKey RootSignatureKey{};
  };
};

} // namespace DirectX
} // namespace gits
