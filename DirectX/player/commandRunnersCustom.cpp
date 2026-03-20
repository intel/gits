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
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void StateRestoreEndRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void FrameEndRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void MarkerUInt64Runner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

#pragma endregion

#pragma region IUnknown

void IUnknownQueryInterfaceRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value) {
    IUnknown* ppvObject = nullptr;
    command.result_.value = command.object_.value->QueryInterface(
        command.riid_.value, reinterpret_cast<void**>(&ppvObject));

    if (command.result_.value == S_OK) {
      command.ppvObject_.data = ppvObject;
      command.ppvObject_.value = &command.ppvObject_.data;
      if (command.ppvObject_.key != command.object_.key) {
        updateOutputInterface(manager, command.ppvObject_);
      }
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void IUnknownAddRefRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value) {
    command.result_.value = command.object_.value->AddRef();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void IUnknownReleaseRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value) {
    command.result_.value = command.object_.value->Release();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

#pragma endregion

#pragma region MetaCommands

void CreateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    HWND hWnd = manager.getWindowService().createWindow(command.hWnd_.value, command.width_.value,
                                                        command.height_.value);
    command.hWnd_.value = hWnd;
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void MappedDataMetaRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    char* currentAddress = static_cast<char*>(
        manager.getMapTrackingService().getCurrentAddress(command.mappedAddress_.value));
    memcpy(currentAddress + command.offset_.value, command.data_.value, command.data_.size);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void CreateHeapAllocationMetaRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    manager.getHeapAllocationService().createHeapAllocation(
        command.heap_.key, command.address_.value, command.data_.value, command.data_.size);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

#pragma endregion

void WaitForFenceSignaledRunner::Run() {
  auto& manager = PlayerManager::get();
  updateInterface(manager, command.fence_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  // handled in ReplayCustomizationLayer

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void DllContainerMetaRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  // handled in DllOverrideUseLayer

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_GetSupportedVersionsRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = INTC_D3D12_GetSupportedVersions(
        command.pDevice_.value, command.pSupportedExtVersions_.value,
        command.pSupportedExtVersionsCount_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateDeviceExtensionContextRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      command.result_.value = INTC_D3D12_CreateDeviceExtensionContext(
          command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
          command.pExtensionAppInfo_.value);
    }

    if (command.result_.value == S_OK) {
      manager.getIntelExtensionsContextMap().setContext(
          command.ppExtensionContext_.key,
          reinterpret_cast<std::uintptr_t>(*command.ppExtensionContext_.value));
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateDeviceExtensionContext1Runner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      command.result_.value = INTC_D3D12_CreateDeviceExtensionContext1(
          command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
          command.pExtensionAppInfo_.value);
    }

    if (command.result_.value == S_OK) {
      manager.getIntelExtensionsContextMap().setContext(
          command.ppExtensionContext_.key,
          reinterpret_cast<std::uintptr_t>(*command.ppExtensionContext_.value));
    }
  }

  // The current implementation of INTC_D3D12_CreateDeviceExtensionContext1 is changing the value of pExtensionAppInfo with invalid data
  // Once the issue is fixed, this workaround won't be necessary (since the pointers will be preserved)
  command.pExtensionAppInfo_.value->pApplicationName = command.pExtensionAppInfo_.pApplicationName;
  command.pExtensionAppInfo_.value->pEngineName = command.pExtensionAppInfo_.pEngineName;

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_SetApplicationInfoRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = INTC_D3D12_SetApplicationInfo(command.pExtensionAppInfo_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_DestroyDeviceExtensionContextRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      command.result_.value = INTC_DestroyDeviceExtensionContext(command.ppExtensionContext_.value);
    }

    if (command.result_.value == S_OK) {
      manager.getIntelExtensionsContextMap().removeContext(command.ppExtensionContext_.key);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CheckFeatureSupportRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_CheckFeatureSupport(context, command.Feature_.value,
                                                           command.pFeatureSupportData_.value,
                                                           command.FeatureSupportDataSize_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_SetFeatureSupportRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_SetFeatureSupport(context, command.pFeature_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_GetResourceAllocationInfoRunner::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_GetResourceAllocationInfo(
        context, command.visibleMask_.value, command.numResourceDescs_.value,
        command.pResourceDescs_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateComputePipelineStateRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDesc_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
      command.result_.value = INTC_D3D12_CreateComputePipelineState(
          context, command.pDesc_.value, command.riid_.value, command.ppPipelineState_.value);
    }

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppPipelineState_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreatePlacedResourceRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pHeap_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
      command.result_.value = INTC_D3D12_CreatePlacedResource(
          context, command.pHeap_.value, command.HeapOffset_.value, command.pDesc_.value,
          command.InitialState_.value, command.pOptimizedClearValue_.value, command.riid_.value,
          command.ppvResource_.value);
    }

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppvResource_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateCommittedResourceRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
      command.result_.value = INTC_D3D12_CreateCommittedResource(
          context, command.pHeapProperties_.value, command.HeapFlags_.value, command.pDesc_.value,
          command.InitialResourceState_.value, command.pOptimizedClearValue_.value,
          command.riidResource_.value, command.ppvResource_.value);
    }

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppvResource_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateHeapRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands()) {
    if (!command.skip) {
      auto* context = reinterpret_cast<INTCExtensionContext*>(
          manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
      command.result_.value = INTC_D3D12_CreateHeap(context, command.pDesc_.value,
                                                    command.riid_.value, command.ppvHeap_.value);
    }

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppvHeap_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}
#pragma endregion

#pragma region NVAPI

void NvAPI_InitializeRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_Initialize();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_UnloadRunner::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_Unload();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_SetCreatePipelineStateOptionsRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value =
        NvAPI_D3D12_SetCreatePipelineStateOptions(command.pDevice_.value, command.pState_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpaceRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDev_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_D3D12_SetNvShaderExtnSlotSpace(
        command.pDev_.value, command.uavSlot_.value, command.uavSpace_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDev_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread(
        command.pDev_.value, command.uavSlot_.value, command.uavSpace_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingAccelerationStructureExRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_D3D12_BuildRaytracingAccelerationStructureEx(
        command.pCommandList_.value, command.pParams.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_D3D12_BuildRaytracingOpacityMicromapArray(
        command.pCommandList_.value, command.pParams.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationRunner::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {
    command.result_.value = NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation(
        command.pCommandList_.value, command.pParams.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
