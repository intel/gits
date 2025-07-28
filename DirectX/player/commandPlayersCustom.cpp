// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPlayersCustom.h"
#include "commandsCustom.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "interfaceArgumentUpdaters.h"

#include "streams.h"

namespace gits {
namespace DirectX {

#pragma region IUnknown

void IUnknownQueryInterfacePlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value &&
      command.result_.value == S_OK) {

    IUnknown* ppvObject = nullptr;
    command.result_.value = command.object_.value->QueryInterface(
        command.riid_.value, reinterpret_cast<void**>(&ppvObject));

    if (command.result_.value == S_OK) {
      command.ppvObject_.value = reinterpret_cast<void**>(&ppvObject);
      if (command.ppvObject_.key != command.object_.key) {
        updateOutputInterface(manager, command.ppvObject_);
      }
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void IUnknownAddRefPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value) {
    if (command.object_.value) {
      command.result_.value = command.object_.value->AddRef();
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void IUnknownReleasePlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.object_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip && command.object_.value) {
    if (command.object_.value) {
      command.result_.value = command.object_.value->Release();
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

#pragma endregion

#pragma region MetaCommands

void CreateWindowMetaPlayer::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && !command.skip) {

    manager.getWindowService().createWindow(command.hWnd_.value, command.width_.value,
                                            command.height_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void MappedDataMetaPlayer::Run() {
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

void CreateHeapAllocationMetaPlayer::Run() {
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

void WaitForFenceSignaledPlayer::Run() {
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

void INTC_D3D12_GetSupportedVersionsPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    command.result_.value = INTC_D3D12_GetSupportedVersions(
        command.pDevice_.value, command.pSupportedExtVersions_.value,
        command.pSupportedExtVersionsCount_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateDeviceExtensionContextPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    command.result_.value = INTC_D3D12_CreateDeviceExtensionContext(
        command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
        command.pExtensionAppInfo_.value);
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

void INTC_D3D12_CreateDeviceExtensionContext1Player::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDevice_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    command.result_.value = INTC_D3D12_CreateDeviceExtensionContext1(
        command.pDevice_.value, command.ppExtensionContext_.value, command.pExtensionInfo_.value,
        command.pExtensionAppInfo_.value);

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

void INTC_D3D12_SetApplicationInfoPlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    command.result_.value = INTC_D3D12_SetApplicationInfo(command.pExtensionAppInfo_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_DestroyDeviceExtensionContextPlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    command.result_.value = INTC_DestroyDeviceExtensionContext(command.ppExtensionContext_.value);

    if (command.result_.value == S_OK) {
      manager.getIntelExtensionsContextMap().removeContext(command.ppExtensionContext_.key);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CheckFeatureSupportPlayer::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
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

void INTC_D3D12_SetFeatureSupportPlayer::Run() {
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_SetFeatureSupport(context, command.pFeature_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_GetResourceAllocationInfoPlayer::Run() {
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

void INTC_D3D12_CreateComputePipelineStatePlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDesc_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_CreateComputePipelineState(
        context, command.pDesc_.value, command.riid_.value, command.ppPipelineState_.value);

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppPipelineState_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreatePlacedResourcePlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pHeap_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_CreatePlacedResource(
        context, command.pHeap_.value, command.HeapOffset_.value, command.pDesc_.value,
        command.InitialState_.value, command.pOptimizedClearValue_.value, command.riid_.value,
        command.ppvResource_.value);

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppvResource_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateCommittedResourcePlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_CreateCommittedResource(
        context, command.pHeapProperties_.value, command.HeapFlags_.value, command.pDesc_.value,
        command.InitialResourceState_.value, command.pOptimizedClearValue_.value,
        command.riidResource_.value, command.ppvResource_.value);

    if (command.result_.value == S_OK) {
      updateOutputInterface(manager, command.ppvResource_);
    }
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void INTC_D3D12_CreateHeapPlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == S_OK && !command.skip) {
    auto* context = reinterpret_cast<INTCExtensionContext*>(
        manager.getIntelExtensionsContextMap().getContext(command.pExtensionContext_.key));
    command.result_.value = INTC_D3D12_CreateHeap(context, command.pDesc_.value,
                                                  command.riid_.value, command.ppvHeap_.value);

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

void NvAPI_InitializePlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_Initialize();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_UnloadPlayer::Run() {
  auto& manager = PlayerManager::get();

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_Unload();
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpacePlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDev_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_D3D12_SetNvShaderExtnSlotSpace(
        command.pDev_.value, command.uavSlot_.value, command.uavSpace_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pDev_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread(
        command.pDev_.value, command.uavSlot_.value, command.uavSpace_.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingAccelerationStructureExPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_D3D12_BuildRaytracingAccelerationStructureEx(
        command.pCommandList_.value, command.pParams.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
    command.result_.value = NvAPI_D3D12_BuildRaytracingOpacityMicromapArray(
        command.pCommandList_.value, command.pParams.value);
  }

  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationPlayer::Run() {
  auto& manager = PlayerManager::get();

  updateInterface(manager, command.pCommandList_);

  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }

  if (manager.executeCommands() && command.result_.value == NVAPI_OK && !command.skip) {
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
