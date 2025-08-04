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
#include <unordered_set>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService;

class BindingTablesDump : public ResourceDump {
public:
  BindingTablesDump(AnalyzerRaytracingService& raytracingService)
      : raytracingService_(raytracingService) {}
  void dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned offset,
                        unsigned size,
                        unsigned stride,
                        D3D12_RESOURCE_STATES state,
                        unsigned callKey);
  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

  std::unordered_set<unsigned>& getBindingTablesResources() {
    return bindingTablesResources_;
  }
  std::set<std::pair<unsigned, unsigned>>& getBindingTablesDescriptors() {
    return bindingTablesDescriptors_;
  }

private:
  AnalyzerRaytracingService& raytracingService_;
  std::mutex mutex_;
  std::unordered_set<unsigned> bindingTablesResources_;
  std::set<std::pair<unsigned, unsigned>> bindingTablesDescriptors_;

  struct BindingTablesInfo : DumpInfo {
    unsigned dispatchCall{};
    unsigned stride{};
  };
};

} // namespace DirectX
} // namespace gits
