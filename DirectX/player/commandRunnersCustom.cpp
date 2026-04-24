// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandRunnersCustom.h"
#include "commandsCustom.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "interfaceArgumentUpdaters.h"

#include "streams.h"

namespace gits {
namespace DirectX {

#pragma region Common

void StateRestoreBeginRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void StateRestoreEndRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void FrameEndRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void MarkerUInt64Runner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

#pragma endregion

#pragma region IUnknown

void IUnknownQueryInterfaceRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_Object);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip && command.m_Object.Value) {
    IUnknown* ppvObject = nullptr;
    command.m_Result.Value = command.m_Object.Value->QueryInterface(
        command.m_riid.Value, reinterpret_cast<void**>(&ppvObject));

    if (command.m_Result.Value == S_OK) {
      command.m_ppvObject.Data = ppvObject;
      command.m_ppvObject.Value = &command.m_ppvObject.Data;
      if (command.m_ppvObject.Key != command.m_Object.Key) {
        UpdateOutputInterface(manager, command.m_ppvObject);
      }
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void IUnknownAddRefRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_Object);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip && command.m_Object.Value) {
    command.m_Result.Value = command.m_Object.Value->AddRef();
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void IUnknownReleaseRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_Object);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip && command.m_Object.Value) {
    command.m_Result.Value = command.m_Object.Value->Release();
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

#pragma endregion

#pragma region MetaCommands

void CreateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    HWND hWnd = manager.GetWindowService().CreatePlayerWindow(
        command.m_hWnd.Value, command.m_width.Value, command.m_height.Value);
    command.m_hWnd.Value = hWnd;
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void MappedDataMetaRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    char* currentAddress = static_cast<char*>(
        manager.GetMapTrackingService().GetCurrentAddress(command.m_mappedAddress.Value));
    memcpy(currentAddress + command.m_offset.Value, command.m_data.Value, command.m_data.Size);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void CreateHeapAllocationMetaRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    manager.GetHeapAllocationService().CreateHeapAllocation(
        command.m_heap.Key, command.m_address.Value, command.m_data.Value, command.m_data.Size);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

#pragma endregion

void WaitForFenceSignaledRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateInterface(manager, command.m_fence);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  // handled in ReplayCustomizationLayer

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void DllContainerMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  // handled in DllOverrideUseLayer

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_GetSupportedVersionsRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDevice);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = INTC_D3D12_GetSupportedVersions(
        command.m_pDevice.Value, command.m_pSupportedExtVersions.Value,
        command.m_pSupportedExtVersionsCount.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreateDeviceExtensionContextRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDevice);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      command.m_Result.Value = INTC_D3D12_CreateDeviceExtensionContext(
          command.m_pDevice.Value, command.m_ppExtensionContext.Value,
          command.m_pExtensionInfo.Value, command.m_pExtensionAppInfo.Value);
    }

    if (command.m_Result.Value == S_OK) {
      manager.GetIntelExtensionsContextMap().SetContext(
          command.m_ppExtensionContext.Key,
          reinterpret_cast<std::uintptr_t>(*command.m_ppExtensionContext.Value));
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreateDeviceExtensionContext1Runner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDevice);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      command.m_Result.Value = INTC_D3D12_CreateDeviceExtensionContext1(
          command.m_pDevice.Value, command.m_ppExtensionContext.Value,
          command.m_pExtensionInfo.Value, command.m_pExtensionAppInfo.Value);
    }

    if (command.m_Result.Value == S_OK) {
      manager.GetIntelExtensionsContextMap().SetContext(
          command.m_ppExtensionContext.Key,
          reinterpret_cast<std::uintptr_t>(*command.m_ppExtensionContext.Value));
    }
  }

  // The current implementation of INTC_D3D12_CreateDeviceExtensionContext1 is changing the value of pExtensionAppInfo with invalid data
  // Once the issue is fixed, this workaround won't be necessary (since the pointers will be preserved)
  command.m_pExtensionAppInfo.Value->pApplicationName = command.m_pExtensionAppInfo.ApplicationName;
  command.m_pExtensionAppInfo.Value->pEngineName = command.m_pExtensionAppInfo.EngineName;

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_SetApplicationInfoRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = INTC_D3D12_SetApplicationInfo(command.m_pExtensionAppInfo.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_DestroyDeviceExtensionContextRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      command.m_Result.Value =
          INTC_DestroyDeviceExtensionContext(command.m_ppExtensionContext.Value);
    }

    if (command.m_Result.Value == S_OK) {
      manager.GetIntelExtensionsContextMap().RemoveContext(command.m_ppExtensionContext.Key);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CheckFeatureSupportRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
    command.m_Result.Value = INTC_D3D12_CheckFeatureSupport(context, command.m_Feature.Value,
                                                            command.m_pFeatureSupportData.Value,
                                                            command.m_FeatureSupportDataSize.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_SetFeatureSupportRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
    command.m_Result.Value = INTC_D3D12_SetFeatureSupport(context, command.m_pFeature.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_GetResourceAllocationInfoRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
    command.m_Result.Value = INTC_D3D12_GetResourceAllocationInfo(
        context, command.m_visibleMask.Value, command.m_numResourceDescs.Value,
        command.m_pResourceDescs.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreateComputePipelineStateRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDesc);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
      command.m_Result.Value = INTC_D3D12_CreateComputePipelineState(
          context, command.m_pDesc.Value, command.m_riid.Value, command.m_ppPipelineState.Value);
    }

    if (command.m_Result.Value == S_OK) {
      UpdateOutputInterface(manager, command.m_ppPipelineState);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreatePlacedResourceRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pHeap);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
      command.m_Result.Value = INTC_D3D12_CreatePlacedResource(
          context, command.m_pHeap.Value, command.m_HeapOffset.Value, command.m_pDesc.Value,
          command.m_InitialState.Value, command.m_pOptimizedClearValue.Value, command.m_riid.Value,
          command.m_ppvResource.Value);
    }

    if (command.m_Result.Value == S_OK) {
      UpdateOutputInterface(manager, command.m_ppvResource);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreateCommittedResourceRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
      command.m_Result.Value = INTC_D3D12_CreateCommittedResource(
          context, command.m_pHeapProperties.Value, command.m_HeapFlags.Value,
          command.m_pDesc.Value, command.m_InitialResourceState.Value,
          command.m_pOptimizedClearValue.Value, command.m_riidResource.Value,
          command.m_ppvResource.Value);
    }

    if (command.m_Result.Value == S_OK) {
      UpdateOutputInterface(manager, command.m_ppvResource);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void INTC_D3D12_CreateHeapRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands()) {
    if (!command.Skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.GetIntelExtensionsContextMap().GetContext(command.m_pExtensionContext.Key));
      command.m_Result.Value = INTC_D3D12_CreateHeap(context, command.m_pDesc.Value,
                                                     command.m_riid.Value, command.m_ppvHeap.Value);
    }

    if (command.m_Result.Value == S_OK) {
      UpdateOutputInterface(manager, command.m_ppvHeap);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}
#pragma endregion

#pragma region NVAPI

void NvAPI_InitializeRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_Initialize();
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_UnloadRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_Unload();
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_SetCreatePipelineStateOptionsRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDevice);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value =
        NvAPI_D3D12_SetCreatePipelineStateOptions(command.m_pDevice.Value, command.m_pState.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpaceRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDev);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_D3D12_SetNvShaderExtnSlotSpace(
        command.m_pDev.Value, command.m_uavSlot.Value, command.m_uavSpace.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pDev);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread(
        command.m_pDev.Value, command.m_uavSlot.Value, command.m_uavSpace.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingAccelerationStructureExRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pCommandList);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_D3D12_BuildRaytracingAccelerationStructureEx(
        command.m_pCommandList.Value, command.m_pParams.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pCommandList);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_D3D12_BuildRaytracingOpacityMicromapArray(
        command.m_pCommandList.Value, command.m_pParams.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationRunner::Run() {
  auto& manager = PlayerManager::Get();

  UpdateInterface(manager, command.m_pCommandList);

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.Skip) {
    command.m_Result.Value = NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation(
        command.m_pCommandList.Value, command.m_pParams.Value);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
