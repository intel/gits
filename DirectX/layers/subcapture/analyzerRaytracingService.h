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
#include "capturePlayerShaderIdentifierService.h"
#include "descriptorService.h"
#include "rootSignatureService.h"

#include <unordered_map>
#include <unordered_set>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerCommandListService;

class AnalyzerRaytracingService {
public:
  AnalyzerRaytracingService(DescriptorService& descriptorService,
                            CapturePlayerGpuAddressService& gpuAddressService,
                            CapturePlayerDescriptorHandleService& descriptorHandleService,
                            CapturePlayerShaderIdentifierService& shaderIdentifierService,
                            AnalyzerCommandListService& commandListService,
                            RootSignatureService& rootSignatureService);
  void createStateObject(ID3D12Device5CreateStateObjectCommand& c);
  void addToStateObject(ID3D12Device7AddToStateObjectCommand& c);
  void setPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void setDescriptorHeaps(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);
  void buildTlas(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        unsigned commandListKey,
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
  CapturePlayerShaderIdentifierService& getShaderIdentifierService() {
    return shaderIdentifierService_;
  }
  DescriptorService& getDescriptorService() {
    return descriptorService_;
  }
  RootSignatureService& getRootSignatureService() {
    return rootSignatureService_;
  }

  std::set<unsigned> getStateObjectAllSubobjects(unsigned stateObjectKey);
  using KeyOffset = std::pair<unsigned, unsigned>;
  std::vector<KeyOffset>& getBlases(unsigned tlasBuildKey) {
    return blasesByTlas_[tlasBuildKey];
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

  unsigned findTlas(KeyOffset& tlas);
  void getTlases(std::set<unsigned>& tlases);

private:
  void fillStateObjectInfo(D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc,
                           BindingTablesDump::StateObjectInfo* info);
  void loadInstancesArraysOfPointers();

private:
  CapturePlayerGpuAddressService& gpuAddressService_;
  CapturePlayerDescriptorHandleService& descriptorHandleService_;
  CapturePlayerShaderIdentifierService& shaderIdentifierService_;
  DescriptorService& descriptorService_;
  AnalyzerCommandListService& commandListService_;
  RootSignatureService& rootSignatureService_;

  std::unordered_map<unsigned, std::set<unsigned>> stateObjectsDirectSubobjects_;
  std::unordered_map<unsigned, std::unique_ptr<BindingTablesDump::StateObjectInfo>>
      stateObjectInfos_;
  std::unordered_map<unsigned, unsigned> stateObjectByComandList_;
  std::unordered_map<unsigned, BindingTablesDump::DescriptorHeaps> descriptorHeapsByComandList_;

  RaytracingInstancesDump instancesDump_;
  BindingTablesDump bindingTablesDump_;
  std::unordered_map<unsigned, std::vector<KeyOffset>> blasesByTlas_;
  std::map<KeyOffset, unsigned> tlasBuildKeys_;
  std::set<KeyOffset> sources_;
  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;
  std::unordered_set<unsigned> genericReadResources_;
  std::unordered_map<unsigned, std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> instancesArraysOfPointers_;
};

} // namespace DirectX
} // namespace gits
