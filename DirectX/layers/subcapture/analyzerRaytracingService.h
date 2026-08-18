// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"
#include "raytracingInstancesDump.h"
#include "bindingTablesDump.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "descriptorService.h"
#include "descriptorRootSignatureService.h"
#include "resourceStateTracker.h"

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
                            DescriptorRootSignatureService& rootSignatureService,
                            ResourceStateTracker& resourceStateTracker);
  void CreateStateObject(ID3D12Device5CreateStateObjectCommand& c);
  void AddToStateObject(ID3D12Device7AddToStateObjectCommand& c);
  void SetPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c);

  struct DescriptorHeapInfo {
    GITSKey Key{};
    D3D12_DESCRIPTOR_HEAP_TYPE Type{};
    unsigned NumDescriptors{};
  };
  void SetDescriptorHeaps(GITSKey commandListKey, const std::vector<DescriptorHeapInfo>& infos);

  void BuildTlas(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void DispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        GITSKey commandListKey,
                        ID3D12Resource* resource,
                        GITSKey resourceKey,
                        unsigned offset,
                        UINT64 size,
                        UINT64 stride,
                        D3D12_GPU_VIRTUAL_ADDRESS address);
  void AddAccelerationStructureSource(GITSKey key, unsigned offset) {
    m_Sources.insert(std::make_pair(key, offset));
  }
  void Flush();
  void ExecuteCommandLists(GITSKey key,
                           GITSKey commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(GITSKey key, GITSKey commandQueueKey, GITSKey fenceKey, UINT64 fenceValue);
  void CommandQueueSignal(GITSKey key,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(GITSKey key, GITSKey fenceKey, UINT64 fenceValue);
  void GetGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c);

  CapturePlayerGpuAddressService& GetGpuAddressService() {
    return m_GpuAddressService;
  }
  CapturePlayerDescriptorHandleService& GetDescriptorHandleService() {
    return m_DescriptorHandleService;
  }
  CapturePlayerShaderIdentifierService& GetShaderIdentifierService() {
    return m_ShaderIdentifierService;
  }
  DescriptorService& GetDescriptorService() {
    return m_DescriptorService;
  }
  DescriptorRootSignatureService& GetRootSignatureService() {
    return m_RootSignatureService;
  }

  std::set<GITSKey> GetStateObjectAllSubobjects(GITSKey stateObjectKey);
  using KeyOffset = std::pair<GITSKey, unsigned>;
  std::vector<KeyOffset>& GetBlases(GITSKey tlasBuildKey) {
    return m_BlasesByTlas[tlasBuildKey];
  }
  std::set<KeyOffset>& GetSources() {
    return m_Sources;
  }
  std::unordered_set<GITSKey>& GetBindingTablesResources() {
    return m_BindingTablesDump.GetBindingTablesResources();
  }
  std::set<std::pair<GITSKey, unsigned>>& GetBindingTablesDescriptors() {
    return m_BindingTablesDump.GetBindingTablesDescriptors();
  }

  GITSKey FindTlas(const KeyOffset& tlas);
  void GetTlases(std::set<GITSKey>& tlases);

private:
  void FillStateObjectInfo(D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc,
                           BindingTablesDump::StateObjectInfo* info);
  void LoadInstancesArraysOfPointers();

private:
  CapturePlayerGpuAddressService& m_GpuAddressService;
  CapturePlayerDescriptorHandleService& m_DescriptorHandleService;
  CapturePlayerShaderIdentifierService& m_ShaderIdentifierService;
  DescriptorService& m_DescriptorService;
  AnalyzerCommandListService& m_CommandListService;
  DescriptorRootSignatureService& m_RootSignatureService;
  ResourceStateTracker& m_ResourceStateTracker;

  std::unordered_map<GITSKey, std::set<GITSKey>> m_StateObjectsDirectSubobjects;
  std::unordered_map<GITSKey, std::unique_ptr<BindingTablesDump::StateObjectInfo>>
      m_StateObjectInfos;
  std::unordered_map<GITSKey, GITSKey> m_StateObjectByComandList;
  std::unordered_map<GITSKey, BindingTablesDump::DescriptorHeaps> m_DescriptorHeapsByComandList;

  RaytracingInstancesDump m_InstancesDump;
  BindingTablesDump m_BindingTablesDump;
  std::unordered_map<GITSKey, std::vector<KeyOffset>> m_BlasesByTlas;
  std::map<KeyOffset, GITSKey> m_TlasBuildKeys;
  std::set<KeyOffset> m_Sources;
  std::unordered_map<GITSKey, ID3D12Resource*> m_ResourceByKey;
  std::unordered_map<GITSKey, std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> m_InstancesArraysOfPointers;
};

} // namespace DirectX
} // namespace gits
