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
    m_Mode = Mode::Store;
  } else {
    m_Mode = Mode::Use;
  }

  LOG_INFO << "CpuPatch - Mode: " << (m_Mode == Mode::Store ? "Store" : "Use");

  m_AddressService.EnablePlayerAddressLookup();
  if (m_Mode == Mode::Store) {
    m_InstancesDump = std::make_unique<InstancesDump>(gitsConfig);
    m_BindingTableDump = std::make_unique<BindingTableDump>(gitsConfig);
    m_ExecuteIndirectDump = std::make_unique<ExecuteIndirectDump>(
        gitsConfig, m_ResourceStateTracker, m_AddressService, m_ResourceByKey);
  } else {
    m_PatchService = std::make_unique<PatchService>(gitsConfig, m_AddressService);
  }
}

void CpuPatchLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreBuildRTAS(command);
  } else if (m_Mode == Mode::Store) {
    if (command.m_pDesc.Value->Inputs.Type !=
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
        !command.m_pDesc.Value->Inputs.NumDescs) {
      return;
    }

    if (command.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      unsigned instanceDescsKey = command.m_pDesc.InputKeys[0];
      ID3D12Resource* instanceDescs = m_ResourceByKey[instanceDescsKey];
      GITS_ASSERT(instanceDescs);

      unsigned offset = command.m_pDesc.InputOffsets[0];
      unsigned size =
          command.m_pDesc.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

      D3D12_RESOURCE_STATES state =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, command.m_Object.Value,
                                  instanceDescs, offset, instanceDescsKey,
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
              .State;

      m_InstancesDump->BuildTlas(command.m_Object.Value, instanceDescs, offset, size, state,
                                 command.Key);
    }
  }
}

void CpuPatchLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreDispatchRays(command);
  } else if (m_Mode == Mode::Store) {
    auto dumpBindingTable = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride,
                                std::string type) {
      if (!resourceKey) {
        return;
      }

      ID3D12Resource* resource = m_ResourceByKey[resourceKey];
      GITS_ASSERT(resource);

      D3D12_RESOURCE_STATES state =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, command.m_Object.Value,
                                  resource, offset, resourceKey,
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
              .State;

      if (stride == 0) {
        stride = size;
      }

      m_BindingTableDump->DispatchRays(command.m_Object.Value, resource, offset, size, state,
                                       command.Key, type);
    };

    dumpBindingTable(command.m_pDesc.RayGenerationShaderRecordKey,
                     command.m_pDesc.RayGenerationShaderRecordOffset,
                     command.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes,
                     command.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes, "RayGeneration");
    dumpBindingTable(command.m_pDesc.MissShaderTableKey, command.m_pDesc.MissShaderTableOffset,
                     command.m_pDesc.Value->MissShaderTable.SizeInBytes,
                     command.m_pDesc.Value->MissShaderTable.StrideInBytes, "Miss");
    dumpBindingTable(command.m_pDesc.HitGroupTableKey, command.m_pDesc.HitGroupTableOffset,
                     command.m_pDesc.Value->HitGroupTable.SizeInBytes,
                     command.m_pDesc.Value->HitGroupTable.StrideInBytes, "HitGroup");
    dumpBindingTable(command.m_pDesc.CallableShaderTableKey,
                     command.m_pDesc.CallableShaderTableOffset,
                     command.m_pDesc.Value->CallableShaderTable.SizeInBytes,
                     command.m_pDesc.Value->CallableShaderTable.StrideInBytes, "Callable");
  }
}

void CpuPatchLayer::Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreExecuteIndirect(command);
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PostExecuteIndirect(command);
  } else if (m_Mode == Mode::Store) {
    // Dumping in Post, because in Pre ArgumentBufferOffset is changed by GpuPatch
    m_ExecuteIndirectDump->ExecuteIndirect(command);
  }
}

void CpuPatchLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreExecute(command);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  ++m_CommandListExecutionCount;
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.ExecuteCommandLists(
        reinterpret_cast<ID3D12GraphicsCommandList**>(command.m_ppCommandLists.Value),
        command.m_NumCommandLists.Value);
    m_InstancesDump->ExecuteCommandLists(
        command.Key, command.m_Object.Key, command.m_Object.Value, command.m_ppCommandLists.Value,
        command.m_NumCommandLists.Value, m_FrameNumber, m_CommandListExecutionCount);
    m_BindingTableDump->ExecuteCommandLists(
        command.Key, command.m_Object.Key, command.m_Object.Value, command.m_ppCommandLists.Value,
        command.m_NumCommandLists.Value, m_FrameNumber, m_CommandListExecutionCount);
    m_ExecuteIndirectDump->ExecuteCommandLists(
        command.Key, command.m_Object.Key, command.m_Object.Value, command.m_ppCommandLists.Value,
        command.m_NumCommandLists.Value, m_FrameNumber, m_CommandListExecutionCount);
  } else if (m_Mode == Mode::Use) {
    m_PatchService->PostExecute(command);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueWaitCommand& command) {
  if (m_Mode == Mode::Store) {
    m_InstancesDump->CommandQueueWait(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                      command.m_Value.Value);
    m_BindingTableDump->CommandQueueWait(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                         command.m_Value.Value);
    m_ExecuteIndirectDump->CommandQueueWait(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                            command.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12CommandQueueSignalCommand& command) {
  if (m_Mode == Mode::Store) {
    m_InstancesDump->CommandQueueSignal(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                        command.m_Value.Value);
    m_BindingTableDump->CommandQueueSignal(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                           command.m_Value.Value);
    m_ExecuteIndirectDump->CommandQueueSignal(command.Key, command.m_Object.Key,
                                              command.m_pFence.Key, command.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12FenceSignalCommand& command) {
  if (m_Mode == Mode::Store) {
    m_InstancesDump->FenceSignal(command.Key, command.m_Object.Key, command.m_Value.Value);
    m_BindingTableDump->FenceSignal(command.Key, command.m_Object.Key, command.m_Value.Value);
    m_ExecuteIndirectDump->FenceSignal(command.Key, command.m_Object.Key, command.m_Value.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateFenceCommand& command) {
  if (m_Mode == Mode::Store) {
    m_InstancesDump->FenceSignal(command.Key, command.m_ppFence.Key, command.m_InitialValue.Value);
    m_BindingTableDump->FenceSignal(command.Key, command.m_ppFence.Key,
                                    command.m_InitialValue.Value);
    m_ExecuteIndirectDump->FenceSignal(command.Key, command.m_ppFence.Key,
                                       command.m_InitialValue.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& command) {
  if (m_Mode == Mode::Store) {
    m_InstancesDump->FenceSignal(command.Key, command.m_pFenceToSignal.Key,
                                 command.m_FenceValueToSignal.Value);
    m_BindingTableDump->FenceSignal(command.Key, command.m_pFenceToSignal.Key,
                                    command.m_FenceValueToSignal.Value);
    m_ExecuteIndirectDump->FenceSignal(command.Key, command.m_pFenceToSignal.Key,
                                       command.m_FenceValueToSignal.Value);
  }
}

void CpuPatchLayer::Post(IDXGISwapChainPresentCommand& command) {
  if (command.m_Flags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(command.Key)) {
    return;
  }
  ++m_FrameNumber;
  m_CommandListExecutionCount = 0;
}

void CpuPatchLayer::Post(IDXGISwapChain1Present1Command& command) {
  if (command.m_PresentFlags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(command.Key)) {
    return;
  }
  ++m_FrameNumber;
  m_CommandListExecutionCount = 0;
}

void CpuPatchLayer::Pre(IUnknownReleaseCommand& command) {
  if (command.m_Result.Value == 0) {
    m_AddressService.DestroyInterface(command.m_Object.Key);
    m_ResourceByKey.erase(command.m_Object.Key);
  }
  if (m_Mode == Mode::Use) {
    m_PatchService->ReleaseObject(command);
  }
}

void CpuPatchLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  m_ResourceByKey[command.m_Object.Key] = command.m_Object.Value;
  D3D12_RESOURCE_DESC desc = command.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_AddressService.AddGpuCaptureAddress(command.m_Object.Value, command.m_Object.Key, desc.Width,
                                          command.m_Result.Value);
  }
}

void CpuPatchLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  D3D12_RESOURCE_DESC desc = command.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_AddressService.AddGpuPlayerAddress(command.m_Object.Value, command.m_Object.Key, desc.Width,
                                         command.m_Result.Value);
  }
}

void CpuPatchLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreGetShaderId(command);
  }
}

void CpuPatchLayer::Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PostGetShaderId(command);
  }
}

void CpuPatchLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PreGetDescriptorHandle(command);
  }
}

void CpuPatchLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_Mode == Mode::Use) {
    m_PatchService->PostGetDescriptorHandle(command);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialResourceState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device4CreateCommittedResource1Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialResourceState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device8CreateCommittedResource2Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialResourceState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreateCommittedResource3Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialLayout.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialResourceState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  m_AddressService.CreatePlacedResource(command.m_pHeap.Key, command.m_ppvResource.Key,
                                        command.m_pDesc.Value->Flags);
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device8CreatePlacedResource1Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  m_AddressService.CreatePlacedResource(command.m_pHeap.Key, command.m_ppvResource.Key,
                                        command.m_pDesc.Value->Flags);
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreatePlacedResource2Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  m_AddressService.CreatePlacedResource(command.m_pHeap.Key, command.m_ppvResource.Key,
                                        command.m_pDesc.Value->Flags);
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialLayout.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  m_AddressService.CreatePlacedResource(command.m_pHeap.Key, command.m_ppvResource.Key,
                                        command.m_pDesc.Value->pD3D12Desc->Flags);
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateReservedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device4CreateReservedResource1Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12Device10CreateReservedResource2Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialLayout.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.AddResource(
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value), command.m_ppvResource.Key,
        command.m_InitialState.Value);
    m_ResourceByKey[command.m_ppvResource.Key] =
        reinterpret_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  }
}

void CpuPatchLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& command) {
  if (m_Mode == Mode::Store) {
    m_ExecuteIndirectDump->CreateCommandSignature(command);
  } else if (m_Mode == Mode::Use) {
    m_PatchService->CreateCommandSignature(command.m_ppvCommandSignature.Key, command.m_pDesc);
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& command) {
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.ResourceBarrier(command.m_Object.Value, command.m_pBarriers.Value,
                                           command.m_NumBarriers.Value,
                                           command.m_pBarriers.ResourceKeys.data());
  }
}

void CpuPatchLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& command) {
  if (m_Mode == Mode::Store) {
    m_ResourceStateTracker.ResourceBarrier(command.m_Object.Value, command.m_pBarrierGroups.Value,
                                           command.m_NumBarrierGroups.Value,
                                           command.m_pBarrierGroups.ResourceKeys.data());
  }
}

} // namespace DirectX
} // namespace gits
