// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "objectLifetimeSynchronizationLayer.h"

namespace gits {
namespace DirectX {

thread_local std::vector<bool> ObjectLifetimeSynchronizationLayer::m_ReleaseLocked;

// The reference count is only a snapshot. If two threads release the last two references
// concurrently, both can observe 2, skip the lock, and the object is destroyed unlocked.
// Locking every release of an address-tracked object would close this, but would put the
// exclusive lock's process-wide barrier and reader drain on a hot path.
void ObjectLifetimeSynchronizationLayer::Pre(IUnknownReleaseCommand& command) {
  IUnknown* object = command.m_Object.Value;
  object->AddRef();
  const bool lock = object->Release() == 1 && IsTrackedByAddress(object);
  if (lock) {
    m_Mutex.lock();
  }
  m_ReleaseLocked.push_back(lock);
}

bool ObjectLifetimeSynchronizationLayer::IsTrackedByAddress(IUnknown* object) {
  for (const IID& iid : {IID_ID3D12Resource, IID_ID3D12Heap, IID_ID3D12DescriptorHeap}) {
    IUnknown* queried = nullptr;
    if (SUCCEEDED(object->QueryInterface(iid, reinterpret_cast<void**>(&queried)))) {
      queried->Release();
      return true;
    }
  }
  return false;
}

void ObjectLifetimeSynchronizationLayer::Post(IUnknownReleaseCommand& command) {
  const bool locked = m_ReleaseLocked.back();
  m_ReleaseLocked.pop_back();
  if (locked) {
    m_Mutex.unlock();
  }
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateCommittedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device4CreateCommittedResource1Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device4CreateCommittedResource1Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device8CreateCommittedResource2Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device8CreateCommittedResource2Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device10CreateCommittedResource3Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device10CreateCommittedResource3Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device16CreateCommittedResource4Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device16CreateCommittedResource4Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateReservedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateReservedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device4CreateReservedResource1Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device4CreateReservedResource1Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device10CreateReservedResource2Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device10CreateReservedResource2Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreatePlacedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device8CreatePlacedResource1Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device8CreatePlacedResource1Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device10CreatePlacedResource2Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device10CreatePlacedResource2Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device16CreatePlacedResource3Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device16CreatePlacedResource3Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateHeapCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateHeapCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device4CreateHeap1Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device4CreateHeap1Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateDescriptorHeapCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateRenderTargetViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateRenderTargetViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateRenderTargetViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateShaderResourceViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12DeviceCreateShaderResourceViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateShaderResourceViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateShaderResourceViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12DeviceCreateUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12DeviceCreateUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateDepthStencilViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateDepthStencilViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateDepthStencilViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateSamplerFeedbackUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateSamplerFeedbackUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateSamplerCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCreateSamplerCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device11CreateSampler2Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device11CreateSampler2Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12Device15TryCreateSampler2Command& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12Device15TryCreateSampler2Command& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCreateConstantBufferViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12DeviceCreateConstantBufferViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device15TryCreateConstantBufferViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device15TryCreateConstantBufferViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCopyDescriptorsCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCopyDescriptorsCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12CommandListGpuCopyDescriptorsCopyDescriptorsCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12CommandListGpuCopyDescriptorsCopyDescriptorsCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListIASetIndexBufferCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListIASetIndexBufferCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListSOSetTargetsCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListSOSetTargetsCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList4BeginRenderPassCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList4BeginRenderPassCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(ID3D12GraphicsCommandList7BarrierCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList11WaitBarrierCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList11WaitBarrierCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList11SignalBarrierCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList11SignalBarrierCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    ID3D12CommandListRaytracing2ExecuteIndirectRTASOperationsCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    ID3D12CommandListRaytracing2ExecuteIndirectRTASOperationsCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(IDMLDeviceCreateBindingTableCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(IDMLDeviceCreateBindingTableCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(IDMLBindingTableResetCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(IDMLBindingTableResetCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_Mutex.unlock_shared();
}

void ObjectLifetimeSynchronizationLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_Mutex.lock_shared();
}

void ObjectLifetimeSynchronizationLayer::Post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_Mutex.unlock_shared();
}

} // namespace DirectX
} // namespace gits
