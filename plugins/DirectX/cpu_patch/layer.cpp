// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "keyUtils.h"
#include "log.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

CpuPatchLayer::CpuPatchLayer(IPluginContext context) : Layer("CpuPatch") {
  const auto& gitsConfig = *context.config;
  if (gitsConfig.directx.player.patchGpuBuffers) {
    mode_ = Mode::Store;
  } else {
    mode_ = Mode::Use;
  }

  LOG_INFO << "CpuPatch - Mode: " << (mode_ == Mode::Store ? "Store" : "Use");

  addressService_.EnablePlayerAddressLookup();
  if (mode_ == Mode::Store) {
    instancesDump_ = std::make_unique<InstancesDump>(gitsConfig);
    bindingTableDump_ = std::make_unique<BindingTableDump>(gitsConfig);
    executeIndirectDump_ = std::make_unique<ExecuteIndirectDump>(gitsConfig, resourceStateTracker_,
                                                                 addressService_, resourceByKey_);
  } else {
    patchService_ = std::make_unique<PatchService>(gitsConfig, addressService_);
  }
}

void CpuPatchLayer::Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preBuildRTAS(c);
  } else if (mode_ == Mode::Store) {
    if (c.m_pDesc.Value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
        !c.m_pDesc.Value->Inputs.NumDescs) {
      return;
    }

    if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      unsigned instanceDescsKey = c.m_pDesc.InputKeys[0];
      ID3D12Resource* instanceDescs = resourceByKey_[instanceDescsKey];
      GITS_ASSERT(instanceDescs);

      unsigned offset = c.m_pDesc.InputOffsets[0];
      unsigned size = c.m_pDesc.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

      D3D12_RESOURCE_STATES state =
          GetAdjustedCurrentState(resourceStateTracker_, addressService_, c.m_Object.Value,
                                  instanceDescs, offset, instanceDescsKey,
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
              .State;

      instancesDump_->buildTlas(c.m_Object.Value, instanceDescs, offset, size, state, c.Key);
    }
  }
}

void CpuPatchLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preDispatchRays(c);
  } else if (mode_ == Mode::Store) {
    auto dumpBindingTable = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride,
                                std::string type) {
      if (!resourceKey) {
        return;
      }

      ID3D12Resource* resource = resourceByKey_[resourceKey];
      GITS_ASSERT(resource);

      D3D12_RESOURCE_STATES state =
          GetAdjustedCurrentState(resourceStateTracker_, addressService_, c.m_Object.Value,
                                  resource, offset, resourceKey,
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
              .State;

      if (stride == 0) {
        stride = size;
      }

      bindingTableDump_->dispatchRays(c.m_Object.Value, resource, offset, size, state, c.Key, type);
    };

    dumpBindingTable(c.m_pDesc.RayGenerationShaderRecordKey,
                     c.m_pDesc.RayGenerationShaderRecordOffset,
                     c.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes,
                     c.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes, "RayGeneration");
    dumpBindingTable(c.m_pDesc.MissShaderTableKey, c.m_pDesc.MissShaderTableOffset,
                     c.m_pDesc.Value->MissShaderTable.SizeInBytes,
                     c.m_pDesc.Value->MissShaderTable.StrideInBytes, "Miss");
    dumpBindingTable(c.m_pDesc.HitGroupTableKey, c.m_pDesc.HitGroupTableOffset,
                     c.m_pDesc.Value->HitGroupTable.SizeInBytes,
                     c.m_pDesc.Value->HitGroupTable.StrideInBytes, "HitGroup");
    dumpBindingTable(c.m_pDesc.CallableShaderTableKey, c.m_pDesc.CallableShaderTableOffset,
                     c.m_pDesc.Value->CallableShaderTable.SizeInBytes,
                     c.m_pDesc.Value->CallableShaderTable.StrideInBytes, "Callable");
  }
}

void CpuPatchLayer::Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preExecuteIndirect(c);
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->postExecuteIndirect(c);
  } else if (mode_ == Mode::Store) {
    // Dumping in Post, because in Pre ArgumentBufferOffset is changed by GpuPatch
    executeIndirectDump_->executeIndirect(c);
  }
}

void CpuPatchLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preExecute(c);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  ++commandListExecutionCount_;
  if (mode_ == Mode::Store) {
    resourceStateTracker_.ExecuteCommandLists(
        reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
        c.m_NumCommandLists.Value);
    instancesDump_->executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                        c.m_ppCommandLists.Value, c.m_NumCommandLists.Value,
                                        frameNumber_, commandListExecutionCount_);
    bindingTableDump_->executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                           c.m_ppCommandLists.Value, c.m_NumCommandLists.Value,
                                           frameNumber_, commandListExecutionCount_);
    executeIndirectDump_->executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                              c.m_ppCommandLists.Value, c.m_NumCommandLists.Value,
                                              frameNumber_, commandListExecutionCount_);
  } else if (mode_ == Mode::Use) {
    patchService_->postExecute(c);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (mode_ == Mode::Store) {
    instancesDump_->commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    bindingTableDump_->commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    executeIndirectDump_->commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (mode_ == Mode::Store) {
    instancesDump_->commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    bindingTableDump_->commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    executeIndirectDump_->commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                             c.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12FenceSignalCommand& c) {
  if (mode_ == Mode::Store) {
    instancesDump_->fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
    bindingTableDump_->fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
    executeIndirectDump_->fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (mode_ == Mode::Store) {
    instancesDump_->fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
    bindingTableDump_->fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
    executeIndirectDump_->fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (mode_ == Mode::Store) {
    instancesDump_->fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
    bindingTableDump_->fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
    executeIndirectDump_->fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
  }
}

void CpuPatchLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (c.m_Flags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  ++frameNumber_;
  commandListExecutionCount_ = 0;
}

void CpuPatchLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (c.m_PresentFlags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  ++frameNumber_;
  commandListExecutionCount_ = 0;
}

void CpuPatchLayer::Pre(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    addressService_.DestroyInterface(c.m_Object.Key);
    resourceByKey_.erase(c.m_Object.Key);
  }
  if (mode_ == Mode::Use) {
    patchService_->releaseObject(c);
  }
}

void CpuPatchLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  resourceByKey_[c.m_Object.Key] = c.m_Object.Value;
  D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    addressService_.AddGpuCaptureAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                         c.m_Result.Value);
  }
}

void CpuPatchLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    addressService_.AddGpuPlayerAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                        c.m_Result.Value);
  }
}

void CpuPatchLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preGetShaderId(c);
  }
}

void CpuPatchLayer::Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->postGetShaderId(c);
  }
}

void CpuPatchLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->preGetDescriptorHandle(c);
  }
}

void CpuPatchLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (mode_ == Mode::Use) {
    patchService_->postGetDescriptorHandle(c);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialResourceState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialResourceState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialResourceState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialLayout.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialResourceState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  addressService_.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  addressService_.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  addressService_.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialLayout.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  addressService_.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                       c.m_pDesc.Value->pD3D12Desc->Flags);
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialLayout.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (mode_ == Mode::Store) {
    resourceStateTracker_.AddResource(reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                      c.m_ppvResource.Key, c.m_InitialState.Value);
    resourceByKey_[c.m_ppvResource.Key] = reinterpret_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (mode_ == Mode::Store) {
    executeIndirectDump_->createCommandSignature(c);
  } else if (mode_ == Mode::Use) {
    patchService_->createCommandSignature(c.m_ppvCommandSignature.Key, c.m_pDesc);
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (mode_ == Mode::Store) {
    resourceStateTracker_.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                          c.m_NumBarriers.Value, c.m_pBarriers.ResourceKeys.data());
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (mode_ == Mode::Store) {
    resourceStateTracker_.ResourceBarrier(c.m_Object.Value, c.m_pBarrierGroups.Value,
                                          c.m_NumBarrierGroups.Value,
                                          c.m_pBarrierGroups.ResourceKeys.data());
  }
}

} // namespace DirectX
} // namespace gits
