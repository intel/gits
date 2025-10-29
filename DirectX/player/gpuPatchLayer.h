// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(IUnknownReleaseCommand& c) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) override;
  void post(ID3D12DeviceCreateCommandListCommand& c) override;
  void post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) override;
  void post(ID3D12GraphicsCommandListResetCommand& c) override;
  void pre(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void post(ID3D12Device4CreateReservedResource1Command& c) override;
  void post(ID3D12Device10CreateReservedResource2Command& c) override;
  void post(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;

private:
  void initialize(ID3D12GraphicsCommandList* commandList);
  void initializeInstancesAoP(ID3D12GraphicsCommandList* commandList);
  void addPatchBuffer(ID3D12GraphicsCommandList* commandList);
  unsigned getMappingBufferIndex(unsigned commandListKey);
  unsigned getPatchBufferIndex(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);
  unsigned getInstancesAoPPatchBufferIndex(unsigned commandListKey);
  unsigned getInstancesAoPStagingBufferIndex(unsigned commandListKey);
  void getPatchOffsets(const D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                       std::vector<unsigned>& patchOffsets);
  void loadExecuteIndirectDispatchRays();
  void loadInstancesArraysOfPointers();
  void patchDispatchRays(ID3D12GraphicsCommandList* commandList,
                         D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc,
                         unsigned patchBufferIndex,
                         unsigned mappingBufferIndex,
                         unsigned callKey);

private:
  PlayerManager& manager_;
  bool useAddressPinning_{};

  static const unsigned patchBufferInitialPoolSize_{32};
  unsigned patchBufferPoolSize_{};
  std::vector<ID3D12Resource*> patchBuffers_{};
  const unsigned patchBufferSize_{0x1000000};

  static const unsigned instancesAoPPatchBufferPoolSize_{8};
  std::array<ID3D12Resource*, instancesAoPPatchBufferPoolSize_> instancesAoPPatchBuffers_{};
  const unsigned instancesAoPPatchBufferSize_{0x10000000};

  static const unsigned instancesAoPStagingBufferPoolSize_{8};
  std::array<ID3D12Resource*, instancesAoPStagingBufferPoolSize_> instancesAoPStagingBuffers_{};
  const unsigned instancesAoPStagingBufferSize_{0x80000};

  std::array<ID3D12Resource*, instancesAoPStagingBufferPoolSize_>
      instancesAoPPatchOffsetsBuffers_{};
  std::array<ID3D12Resource*, instancesAoPStagingBufferPoolSize_>
      instancesAoPPatchOffsetsStagingBuffers_{};
  const unsigned instancesAoPPatchOffsetsBufferSize_{0x40000};

  std::vector<ID3D12Resource*> patchOffsetsBuffers_{};
  std::vector<ID3D12Resource*> patchOffsetsStagingBuffers_{};
  const unsigned patchOffsetsBufferSize_{0x20000};

  std::vector<ID3D12Resource*> executeIndirectRaytracingPatchBuffers_{};
  std::vector<ID3D12Resource*> executeIndirectRaytracingPatchStagingBuffers_{};
  const unsigned executeIndirectRaytracingPatchBufferSize_{0x100};

  std::vector<ID3D12Resource*> executeIndirectCountBuffers_{};
  std::vector<ID3D12Resource*> executeIndirectCountStagingBuffers_{};
  const unsigned executeIndirectCountBufferSize_{0x10};

  static const unsigned mappingBufferPoolSize_{24};

  std::array<ID3D12Resource*, mappingBufferPoolSize_> gpuAddressBuffers_{};
  std::array<ID3D12Resource*, mappingBufferPoolSize_> gpuAddressStagingBuffers_{};
  unsigned gpuAddressBufferSize_{sizeof(CapturePlayerGpuAddressService::GpuAddressMapping) *
                                 0x10000};

  std::array<ID3D12Resource*, mappingBufferPoolSize_> shaderIdentifierBuffers_{};
  std::array<ID3D12Resource*, mappingBufferPoolSize_> shaderIdentifierStagingBuffers_{};
  unsigned shaderIdentifierBufferSize_{
      sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping) * 0x1000};

  std::array<ID3D12Resource*, mappingBufferPoolSize_> viewDescriptorBuffers_{};
  std::array<ID3D12Resource*, mappingBufferPoolSize_> viewDescriptorStagingBuffers_{};
  unsigned viewDescriptorBufferSize_{
      sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) * 0x10000};

  std::array<ID3D12Resource*, mappingBufferPoolSize_> sampleDescriptorBuffers_{};
  std::array<ID3D12Resource*, mappingBufferPoolSize_> sampleDescriptorStagingBuffers_{};
  unsigned sampleDescriptorBufferSize_{
      sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) * 0x1000};

  struct MappingCount {
    unsigned gpuAddressCount;
    unsigned shaderIdentiferCount;
    unsigned viewDescriptorCount;
    unsigned sampleDescriptorCount;
  };
  std::array<ID3D12Resource*, mappingBufferPoolSize_> mappingCountBuffers_{};
  std::array<ID3D12Resource*, mappingBufferPoolSize_> mappingCountStagingBuffers_{};
  unsigned mappingCountBufferSize_{sizeof(MappingCount)};

  struct FenceInfo {
    ID3D12Fence* fence{};
    UINT64 fenceValue{};
    bool waitingForExecute{};
  };
  std::array<FenceInfo, mappingBufferPoolSize_> mappingFences_{};
  std::unordered_map<unsigned, unsigned> currentMappingsByCommandList_;

  std::vector<FenceInfo> patchBufferFences_{};
  std::unordered_map<unsigned, std::vector<unsigned>> currentPatchBuffersByCommandList_;

  std::array<FenceInfo, instancesAoPPatchBufferPoolSize_> instancesAoPPatchBufferFences_{};
  std::unordered_map<unsigned, std::vector<unsigned>> currentInstancesAoPPatchBuffersByCommandList_;

  std::array<FenceInfo, instancesAoPStagingBufferPoolSize_> instancesAoPStagingBufferFences_{};
  std::unordered_map<unsigned, std::vector<unsigned>>
      currentInstancesAoPStagingBuffersByCommandList_;

  bool initialized_{};
  bool initializedInstancesAoP_{};

  CapturePlayerGpuAddressService addressService_;
  CapturePlayerShaderIdentifierService shaderIdentifierService_;
  CapturePlayerDescriptorHandleService descriptorHandleService_;
  RaytracingShaderPatchService raytracingShaderPatchService_;
  ExecuteIndirectShaderPatchService executeIndirectShaderPatchService_;
  GpuPatchCommandListService commandListService_;
  GpuPatchDumpService dumpService_;
  ResourceStateTracker resourceStateTracker_;

  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      commandSignatures_;
  std::unordered_map<unsigned, D3D12_DISPATCH_RAYS_DESC> executeIndirectDispatchRays_;
  std::unordered_map<unsigned, std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> instancesArraysOfPointers_;

  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;

  UINT64 executeIndirectLastArgumentBufferOffset_{};
};

} // namespace DirectX
} // namespace gits
