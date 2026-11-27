// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "asymmetricSharedMutex.h"

#include <vector>

namespace gits {
namespace DirectX {

// Makes destruction (Release reaching 0) of resources, heaps and descriptor heaps atomic across all
// layers with respect to commands which register or translate GPU virtual addresses and descriptor
// handles. Otherwise the driver can
// hand out a freed address range to a new object, or another thread can translate an address to the
// destroyed object, before the layers processing the destruction have unregistered it.
// Destruction callbacks run inside the real Release on the destroying thread, so commands they
// issue are not ordered after the destruction. A resource created there may get the freed address
// registered before the destroyed object is unregistered. A callback waiting for another thread
// that is recording a locked command deadlocks.
// Must be the first Pre layer and the last Post layer (only GlobalSynchronization may wrap it).
class ObjectLifetimeSynchronizationLayer : public Layer {
public:
  ObjectLifetimeSynchronizationLayer() : Layer("ObjectLifetimeSynchronization") {}

  void Pre(IUnknownReleaseCommand& command) override;
  void Post(IUnknownReleaseCommand& command) override;

  // Objects tracked by address
  void Pre(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Pre(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Pre(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Pre(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Pre(ID3D12Device16CreateCommittedResource4Command& command) override;
  void Post(ID3D12Device16CreateCommittedResource4Command& command) override;
  void Pre(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void Pre(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Pre(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Pre(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void Pre(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Pre(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Pre(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Pre(ID3D12Device16CreatePlacedResource3Command& command) override;
  void Post(ID3D12Device16CreatePlacedResource3Command& command) override;
  void Pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void Pre(ID3D12DeviceCreateHeapCommand& command) override;
  void Post(ID3D12DeviceCreateHeapCommand& command) override;
  void Pre(ID3D12Device4CreateHeap1Command& command) override;
  void Post(ID3D12Device4CreateHeap1Command& command) override;
  void Pre(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Pre(ID3D12DeviceCreateDescriptorHeapCommand& command) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& command) override;

  // Commands translating addresses or descriptor handles
  void Pre(ID3D12DeviceCreateRenderTargetViewCommand& command) override;
  void Post(ID3D12DeviceCreateRenderTargetViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateRenderTargetViewCommand& command) override;
  void Post(ID3D12Device15TryCreateRenderTargetViewCommand& command) override;
  void Pre(ID3D12DeviceCreateShaderResourceViewCommand& command) override;
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateShaderResourceViewCommand& command) override;
  void Post(ID3D12Device15TryCreateShaderResourceViewCommand& command) override;
  void Pre(ID3D12DeviceCreateUnorderedAccessViewCommand& command) override;
  void Post(ID3D12DeviceCreateUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateUnorderedAccessViewCommand& command) override;
  void Post(ID3D12Device15TryCreateUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateDepthStencilViewCommand& command) override;
  void Post(ID3D12DeviceCreateDepthStencilViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateDepthStencilViewCommand& command) override;
  void Post(ID3D12Device15TryCreateDepthStencilViewCommand& command) override;
  void Pre(ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Post(ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Post(ID3D12Device15TryCreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateSamplerCommand& command) override;
  void Post(ID3D12DeviceCreateSamplerCommand& command) override;
  void Pre(ID3D12Device11CreateSampler2Command& command) override;
  void Post(ID3D12Device11CreateSampler2Command& command) override;
  void Pre(ID3D12Device15TryCreateSampler2Command& command) override;
  void Post(ID3D12Device15TryCreateSampler2Command& command) override;
  void Pre(ID3D12DeviceCreateConstantBufferViewCommand& command) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& command) override;
  void Pre(ID3D12Device15TryCreateConstantBufferViewCommand& command) override;
  void Post(ID3D12Device15TryCreateConstantBufferViewCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsCommand& command) override;
  void Post(ID3D12DeviceCopyDescriptorsCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& command) override;
  void Post(ID3D12DeviceCopyDescriptorsSimpleCommand& command) override;
  void Pre(ID3D12CommandListGpuCopyDescriptorsCopyDescriptorsCommand& command) override;
  void Post(ID3D12CommandListGpuCopyDescriptorsCopyDescriptorsCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void Post(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void Post(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void Pre(ID3D12GraphicsCommandList7BarrierCommand& command) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& command) override;
  void Pre(ID3D12GraphicsCommandList11WaitBarrierCommand& command) override;
  void Post(ID3D12GraphicsCommandList11WaitBarrierCommand& command) override;
  void Pre(ID3D12GraphicsCommandList11SignalBarrierCommand& command) override;
  void Post(ID3D12GraphicsCommandList11SignalBarrierCommand& command) override;
  void Pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  void Post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
               command) override;
  void Post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
                command) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void Post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Post(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Pre(ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) override;
  void Post(ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) override;
  void Pre(ID3D12CommandListRaytracing2ExecuteIndirectRTASOperationsCommand& command) override;
  void Post(ID3D12CommandListRaytracing2ExecuteIndirectRTASOperationsCommand& command) override;
  void Pre(IDMLDeviceCreateBindingTableCommand& command) override;
  void Post(IDMLDeviceCreateBindingTableCommand& command) override;
  void Pre(IDMLBindingTableResetCommand& command) override;
  void Post(IDMLBindingTableResetCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

private:
  static bool IsTrackedByAddress(IUnknown* object);

  AsymmetricSharedMutex m_Mutex;
  // One entry per Release in progress on this thread, as destruction callbacks can nest them
  static thread_local std::vector<bool> m_ReleaseLocked;
};

} // namespace DirectX
} // namespace gits
