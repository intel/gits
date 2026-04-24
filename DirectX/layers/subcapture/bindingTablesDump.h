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
#include <unordered_set>
#include <unordered_map>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService;

class BindingTablesDump : public ResourceDump {
public:
  struct StateObjectInfo {
    unsigned GlobalRootSignature;
    std::unordered_map<std::wstring, unsigned> ExportToRootSignature;
  };
  struct DescriptorHeaps {
    unsigned ViewDescriptorHeapKey;
    unsigned ViewDescriptorHeapSize;
    unsigned SamplerHeapKey;
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
                        D3D12_RESOURCE_STATES state,
                        StateObjectInfo* stateObjectInfo,
                        DescriptorHeaps descriptorHeaps,
                        unsigned RootSignatureKey);

  std::unordered_set<unsigned>& GetBindingTablesResources() {
    return m_BindingTablesResources;
  }
  std::set<std::pair<unsigned, unsigned>>& GetBindingTablesDescriptors() {
    return m_BindingTablesDescriptors;
  }

private:
  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;
  unsigned Align(unsigned value, unsigned alignment);

private:
  AnalyzerRaytracingService& m_RaytracingService;
  std::mutex m_Mutex;
  std::unordered_set<unsigned> m_BindingTablesResources;
  std::set<std::pair<unsigned, unsigned>> m_BindingTablesDescriptors;

  struct BindingTablesInfo : DumpInfo {
    unsigned stride{};
    StateObjectInfo* stateObjectInfo{};
    DescriptorHeaps descriptorHeaps{};
    unsigned RootSignatureKey{};
  };
};

} // namespace DirectX
} // namespace gits
