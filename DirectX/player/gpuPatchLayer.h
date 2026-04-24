// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "capturePlayerGpuAddressService.h"
#include "raytracingShaderPatchService.h"
#include "executeIndirectShaderPatchService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "gpuPatchCommandListService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "gpuPatchDumpService.h"
#include "resourceStateTracker.h"

#include <d3d12.h>
#include <map>
#include <unordered_map>
#include <set>
#include <array>
#include <vector>

namespace gits {
namespace DirectX {

class PlayerManager;

class GpuPatchLayer : public Layer {
public:
  GpuPatchLayer(PlayerManager& manager);
  ~GpuPatchLayer();

  void Pre(IUnknownReleaseCommand& command) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;
  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) override;
  void Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) override;
  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) override;
  void Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Post(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void Post(ID3D12DeviceCreateCommandListCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void Post(ID3D12GraphicsCommandListResetCommand& command) override;
  void Pre(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12CommandQueueWaitCommand& command) override;
  void Post(ID3D12CommandQueueSignalCommand& command) override;
  void Post(ID3D12FenceSignalCommand& command) override;
  void Post(ID3D12DeviceCreateFenceCommand& command) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& command) override;
  void Post(ID3D12DeviceCreateCommandSignatureCommand& command) override;
  void Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& command) override;

private:
  void Initialize(ID3D12GraphicsCommandList* commandList);
  void InitializeInstancesAoP(ID3D12GraphicsCommandList* commandList);
  void AddPatchBuffer(ID3D12GraphicsCommandList* commandList, unsigned patchBufferSize);
  void CreateOrReplacePatchBufferObjects(ID3D12Device* device,
                                         unsigned patchBufferIndex,
                                         unsigned patchBufferSize);
  void AddMappingBuffer(ID3D12GraphicsCommandList* commandList);
  void CreateMappingBufferObjects(ID3D12Device* device, unsigned mappingBufferIndex);
  unsigned GetMappingBufferIndex(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);
  unsigned GetPatchBufferIndex(unsigned commandListKey,
                               ID3D12GraphicsCommandList* commandList,
                               size_t size);
  unsigned GetInstancesAoPPatchBufferIndex(unsigned commandListKey);
  unsigned GetInstancesAoPStagingBufferIndex(unsigned commandListKey);
  void GetPatchOffsets(const D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                       std::vector<unsigned>& patchOffsets);
  void LoadExecuteIndirectDispatchRays();
  void LoadInstancesArraysOfPointers();
  void PatchDispatchRays(ID3D12GraphicsCommandList* commandList,
                         D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc,
                         unsigned patchBufferIndex,
                         unsigned mappingBufferIndex,
                         unsigned callKey);
  size_t GetDispatchRaysPatchSize(const D3D12_DISPATCH_RAYS_DESC& desc) const;
  void WaitForFence(ID3D12Fence* fence, unsigned fenceValue);

  PlayerManager& m_Manager;
  bool m_UseAddressPinning{};
  HANDLE m_WaitForFenceEvent{};

  static constexpr unsigned PATCH_BUFFER_INITIAL_POOL_SIZE = 32;
  unsigned m_PatchBufferPoolSize{};
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_PatchBuffers;
  static constexpr unsigned PATCH_BUFFER_INITIAL_SIZE = 0x1000000;
  static constexpr float PATCH_BUFFER_SIZE_MULTIPLIER = 1.1f;

  static constexpr unsigned INSTANCES_AOP_PATCH_BUFFER_POOL_SIZE = 8;
  std::array<ID3D12Resource*, INSTANCES_AOP_PATCH_BUFFER_POOL_SIZE> m_InstancesAopPatchBuffers{};
  static constexpr unsigned INSTANCES_AOP_PATCH_BUFFER_SIZE = 0x10000000;

  static constexpr unsigned INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE = 8;
  std::array<ID3D12Resource*, INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE>
      m_InstancesAopStagingBuffers{};
  static constexpr unsigned INSTANCES_AOP_STAGING_BUFFER_SIZE = 0x80000;

  std::array<ID3D12Resource*, INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE>
      m_InstancesAopPatchOffsetsBuffers{};
  std::array<ID3D12Resource*, INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE>
      m_InstancesAopPatchOffsetsStagingBuffers{};
  static constexpr unsigned INSTANCES_AOP_PATCH_OFFSETS_BUFFER_SIZE = 0x40000;

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_PatchOffsetsBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_PatchOffsetsStagingBuffers;
  static constexpr unsigned PATCH_OFFSETS_BUFFER_SIZE = 0x20000;

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ExecuteIndirectRaytracingPatchBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>
      m_ExecuteIndirectRaytracingPatchStagingBuffers;
  static constexpr unsigned EXECUTE_INDIRECT_RAYTRACING_PATCH_BUFFER_SIZE = 0x100;

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ExecuteIndirectCountBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ExecuteIndirectCountStagingBuffers;
  static constexpr unsigned EXECUTE_INDIRECT_COUNT_BUFFER_SIZE = 0x10;

  static constexpr unsigned MAPPING_BUFFER_INITIAL_POOL_SIZE = 24;
  unsigned m_MappingBufferPoolSize{};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_GpuAddressBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_GpuAddressStagingBuffers;
  unsigned m_GpuAddressBufferSize{sizeof(CapturePlayerGpuAddressService::GpuAddressMapping) *
                                  0x10000};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ShaderIdentifierBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ShaderIdentifierStagingBuffers;
  unsigned m_ShaderIdentifierBufferSize{
      sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping) * 0x1000};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ViewDescriptorBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_ViewDescriptorStagingBuffers;
  unsigned m_ViewDescriptorBufferSize{
      sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) * 0x10000};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SampleDescriptorBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SampleDescriptorStagingBuffers;
  unsigned m_SampleDescriptorBufferSize{
      sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) * 0x1000};

  struct MappingCount {
    unsigned GpuAddressCount;
    unsigned ShaderIdentifierCount;
    unsigned ViewDescriptorCount;
    unsigned SampleDescriptorCount;
  };
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_MappingCountBuffers;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_MappingCountStagingBuffers;
  unsigned m_MappingCountBufferSize{sizeof(MappingCount)};

  struct FenceInfo {
    Microsoft::WRL::ComPtr<ID3D12Fence> D3d12Fence{};
    UINT64 FenceValue{};
    bool WaitingForExecute{};
  };
  struct PatchBufferInfo {
    FenceInfo Fence{};
    size_t Size{};
  };
  std::vector<FenceInfo> m_MappingFences;
  std::unordered_map<unsigned, unsigned> m_CurrentMappingsByCommandList;

  std::vector<PatchBufferInfo> m_PatchBufferInfos;
  std::unordered_map<unsigned, std::vector<unsigned>> m_CurrentPatchBuffersByCommandList;

  std::array<FenceInfo, INSTANCES_AOP_PATCH_BUFFER_POOL_SIZE> m_InstancesAopPatchBufferFences{};
  std::unordered_map<unsigned, std::vector<unsigned>>
      m_CurrentInstancesAopPatchBuffersByCommandList;

  std::array<FenceInfo, INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE> m_InstancesAopStagingBufferFences{};
  std::unordered_map<unsigned, std::vector<unsigned>>
      m_CurrentInstancesAopStagingBuffersByCommandList;

  bool m_Initialized{};
  bool m_InitializedInstancesAop{};

  CapturePlayerGpuAddressService m_AddressService;
  CapturePlayerShaderIdentifierService m_ShaderIdentifierService;
  CapturePlayerDescriptorHandleService m_DescriptorHandleService;
  RaytracingShaderPatchService m_RaytracingShaderPatchService;
  ExecuteIndirectShaderPatchService m_ExecuteIndirectShaderPatchService;
  GpuPatchCommandListService m_CommandListService;
  GpuPatchDumpService m_DumpService;
  ResourceStateTracker m_ResourceStateTracker;

  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      m_CommandSignatures;
  std::unordered_map<unsigned, D3D12_DISPATCH_RAYS_DESC> m_ExecuteIndirectDispatchRays;
  std::unordered_map<unsigned, std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> m_InstancesArraysOfPointers;

  std::unordered_map<unsigned, ID3D12Resource*> m_ResourceByKey;

  UINT64 m_ExecuteIndirectLastArgumentBufferOffset{};
};

} // namespace DirectX
} // namespace gits
