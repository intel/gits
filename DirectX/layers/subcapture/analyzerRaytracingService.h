// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "raytracingInstancesDump.h"
#include "bindingTablesDump.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "descriptorService.h"

#include <unordered_map>
#include <unordered_set>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerRaytracingService {
public:
  AnalyzerRaytracingService(DescriptorService& descriptorService,
                            CapturePlayerGpuAddressService& gpuAddressService,
                            CapturePlayerDescriptorHandleService& descriptorHandleService)
      : gpuAddressService_(gpuAddressService),
        descriptorHandleService_(descriptorHandleService),
        descriptorService_(descriptorService),
        instancesDump_(*this),
        bindingTablesDump_(*this) {}
  void createStateObject(ID3D12Device5CreateStateObjectCommand& c);
  void buildTlas(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned resourceKey,
                        unsigned offset,
                        UINT64 size,
                        UINT64 stride);
  void addAccelerationStructureSource(unsigned key, unsigned offset) {
    sources_.insert(std::make_pair(key, offset));
  }
  void flush();
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void commandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);
  void getGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c);
  void genericReadResource(unsigned resourceKey) {
    genericReadResources_.insert(resourceKey);
  }

  CapturePlayerGpuAddressService& getGpuAddressService() {
    return gpuAddressService_;
  }
  CapturePlayerDescriptorHandleService& getDescriptorHandleService() {
    return descriptorHandleService_;
  }
  DescriptorService& getDescriptorService() {
    return descriptorService_;
  }

  std::vector<unsigned>& getStateObjectSubobjects(unsigned stateObjectKey) {
    return stateObjectsSubobjects_[stateObjectKey];
  }
  using KeyOffset = std::pair<unsigned, unsigned>;
  std::vector<KeyOffset>& getBlasesForTlas(unsigned tlasBuildKey) {
    return blasesByTlas_[tlasBuildKey];
  }
  using BlasesByTlas = std::unordered_map<unsigned, std::vector<KeyOffset>>;
  BlasesByTlas& getBlases() {
    return blasesByTlas_;
  }
  std::set<KeyOffset>& getTlases() {
    return tlases_;
  }
  std::set<KeyOffset>& getSources() {
    return sources_;
  }
  std::unordered_set<unsigned>& getBindingTablesResources() {
    return bindingTablesDump_.getBindingTablesResources();
  }
  std::set<std::pair<unsigned, unsigned>>& getBindingTablesDescriptors() {
    return bindingTablesDump_.getBindingTablesDescriptors();
  }

private:
  CapturePlayerGpuAddressService& gpuAddressService_;
  CapturePlayerDescriptorHandleService& descriptorHandleService_;
  DescriptorService& descriptorService_;

  std::unordered_map<unsigned, std::vector<unsigned>> stateObjectsSubobjects_;
  RaytracingInstancesDump instancesDump_;
  BindingTablesDump bindingTablesDump_;
  BlasesByTlas blasesByTlas_;
  std::set<KeyOffset> tlases_;
  std::set<KeyOffset> sources_;
  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;
  std::unordered_set<unsigned> genericReadResources_;
};

} // namespace DirectX
} // namespace gits
