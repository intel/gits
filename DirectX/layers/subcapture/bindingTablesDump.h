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
    unsigned globalRootSignature;
    std::unordered_map<std::wstring, unsigned> exportToRootSignature;
  };
  struct DescriptorHeaps {
    unsigned viewDescriptorHeapKey;
    unsigned viewDescriptorHeapSize;
    unsigned samplerHeapKey;
    unsigned samplerHeapSize;
  };

public:
  BindingTablesDump(AnalyzerRaytracingService& raytracingService)
      : raytracingService_(raytracingService) {}
  void dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned offset,
                        unsigned size,
                        unsigned stride,
                        D3D12_RESOURCE_STATES state,
                        StateObjectInfo* stateObjectInfo,
                        DescriptorHeaps descriptorHeaps,
                        unsigned rootSignatureKey);

  std::unordered_set<unsigned>& getBindingTablesResources() {
    return bindingTablesResources_;
  }
  std::set<std::pair<unsigned, unsigned>>& getBindingTablesDescriptors() {
    return bindingTablesDescriptors_;
  }

private:
  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;
  unsigned align(unsigned value, unsigned alignment);

private:
  AnalyzerRaytracingService& raytracingService_;
  std::mutex mutex_;
  std::unordered_set<unsigned> bindingTablesResources_;
  std::set<std::pair<unsigned, unsigned>> bindingTablesDescriptors_;

  struct BindingTablesInfo : DumpInfo {
    unsigned stride{};
    StateObjectInfo* stateObjectInfo{};
    DescriptorHeaps descriptorHeaps{};
    unsigned rootSignatureKey{};
  };
};

} // namespace DirectX
} // namespace gits
