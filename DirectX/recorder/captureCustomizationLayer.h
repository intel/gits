// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "orderingRecorder.h"
#include "directStorageService.h"

namespace gits {
namespace DirectX {

class CaptureManager;

class CaptureCustomizationLayer : public Layer {
public:
  CaptureCustomizationLayer(CaptureManager& manager, stream::OrderingRecorder& recorder);
  void Post(IUnknownReleaseCommand& command) override;
  void Pre(IDXGIFactoryCreateSwapChainCommand& command) override;
  void Pre(IDXGIFactory2CreateSwapChainForHwndCommand& command) override;
  void Pre(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Pre(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Pre(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Pre(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Pre(ID3D12DeviceCreateHeapCommand& command) override;
  void Post(ID3D12DeviceCreateHeapCommand& command) override;
  void Pre(ID3D12Device4CreateHeap1Command& command) override;
  void Post(ID3D12Device4CreateHeap1Command& command) override;
  void Post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Post(ID3D12ResourceMapCommand& command) override;
  void Pre(ID3D12ResourceUnmapCommand& command) override;
  void Pre(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& command) override;
  void Pre(ID3D12DeviceCreateRenderTargetViewCommand& command) override;
  void Pre(ID3D12DeviceCreateShaderResourceViewCommand& command) override;
  void Pre(ID3D12DeviceCreateUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateDepthStencilViewCommand& command) override;
  void Pre(ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateSamplerCommand& command) override;
  void Pre(ID3D12Device11CreateSampler2Command& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& command) override;
  void Post(D3D12SerializeRootSignatureCommand& command) override;
  void Post(D3D12SerializeVersionedRootSignatureCommand& command) override;
  void Post(ID3DBlobGetBufferPointerCommand& command) override;
  void Post(ID3D12DeviceCreateRootSignatureCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& command) override;
  void Post(ID3D12GraphicsCommandListResetCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void Post(ID3D12FenceGetCompletedValueCommand& command) override;
  void Pre(ID3D12FenceSetEventOnCompletionCommand& command) override;
  void Pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& command) override;
  void Post(ID3D12DeviceOpenSharedHandleCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void Pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
               command) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Pre(ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void Pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void Pre(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Pre(IDMLDeviceCreateBindingTableCommand& command) override;
  void Pre(IDMLBindingTableResetCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void Post(IDStorageFactoryOpenFileCommand& c) override;
  void Pre(IDStorageQueueEnqueueRequestCommand& c) override;
  void Pre(xefgSwapChainD3D12InitFromSwapChainCommand& c) override;
  void Pre(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) override;

private:
  // Heaps (and resources) may require their D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to be updated (for writewatch)
  struct HeapInfo {
    // Pointer to the original data
    D3D12_HEAP_PROPERTIES* PropertiesPtr{nullptr};
    // Temporary values used during execution (potentially updated for writewatch)
    D3D12_HEAP_PROPERTIES Properties{D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                     D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
    D3D12_HEAP_FLAGS Flags{D3D12_HEAP_FLAG_NONE};

    HeapInfo() = default;
    HeapInfo(D3D12_HEAP_PROPERTIES* p, D3D12_HEAP_FLAGS f) {
      this->PropertiesPtr = p;
      this->Properties = *p;
      this->Flags = f;
    }
    HeapInfo(D3D12_HEAP_PROPERTIES& p, D3D12_HEAP_FLAGS f) {
      this->PropertiesPtr = &p;
      this->Properties = p;
      this->Flags = f;
    }
  };

  void fillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
  void fillGpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg,
                                       D3D12_DESCRIPTOR_HEAP_TYPE heapType);
  void fillCpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
                                       D3D12_DESCRIPTOR_HEAP_TYPE heapType);

  static thread_local HeapInfo m_HeapInfo;
  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
  DirectStorageService m_DirectStorageService;
};

} // namespace DirectX
} // namespace gits
