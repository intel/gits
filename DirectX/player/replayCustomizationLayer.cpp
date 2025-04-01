// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"
#include "playerManager.h"
#include "interfaceArgumentUpdaters.h"
#include "gits.h"

#include <d3dx12.h>
#include <setupapi.h>
#include <ntddvdeo.h>
#include <igdext.h>

namespace gits {
namespace DirectX {

ReplayCustomizationLayer::ReplayCustomizationLayer(PlayerManager& manager)
    : Layer("ReplayCustomization"),
      manager_(manager),
      pipelineLibraryService_(manager.getPipelineLibraryService()) {}

void ReplayCustomizationLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    manager_.getDescriptorHandleService().destroyDescriptorHeap(c.object_.key);
    manager_.getMapTrackingService().destroyResource(c.object_.key);
    manager_.getGpuAddressService().destroyInterface(c.object_.key);
    manager_.removeObject(c.object_.key);
  }
  pipelineLibraryService_.releasePipelineState(c.object_.key, c.result_.value);
}

void ReplayCustomizationLayer::post(IUnknownAddRefCommand& c) {
  pipelineLibraryService_.addRefPipelineState(c.object_.key);
}

void ReplayCustomizationLayer::pre(D3D12CreateDeviceCommand& c) {

  overrideAdapter(c);

  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  if (c.pAdapter_.value) {
    c.pAdapter_.value->QueryInterface(IID_PPV_ARGS(&adapter));
  }
  if (!adapter) {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    GITS_ASSERT(hr == S_OK);
    hr = factory->EnumAdapters1(0, &adapter);
    GITS_ASSERT(hr == S_OK);
  }

  DXGI_ADAPTER_DESC1 desc{};
  HRESULT hr = adapter->GetDesc1(&desc);
  GITS_ASSERT(hr == S_OK);

  std::wstring descriptionW = desc.Description;
  std::string description(descriptionW.begin(), descriptionW.end());
  Log(INFO) << "D3D12CreateDevice - Using adapter: " << description;

  manager_.getIntelExtensionsService().loadIntelExtensions(desc.VendorId, desc.DeviceId);
  manager_.getIntelExtensionsService().setApplicationInfo();
}

void ReplayCustomizationLayer::pre(IDXGISwapChainSetFullscreenStateCommand& c) {
  if (c.Fullscreen_.value && gits::Config::Get().common.player.showWindowBorder) {
    c.Fullscreen_.value = false;
    Log(INFO) << "SetFullscreenState: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::pre(IDXGIFactoryCreateSwapChainCommand& c) {
  c.pDesc_.value->OutputWindow =
      manager_.getWindowService().getCurrentHwnd(c.pDesc_.value->OutputWindow);
  if (gits::Config::Get().common.player.showWindowBorder) {
    c.pDesc_.value->Windowed = true;
    Log(INFO) << "CreateSwapChain: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::pre(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  c.hWnd_.value = manager_.getWindowService().getCurrentHwnd(c.hWnd_.value);
  if (c.pFullscreenDesc_.value && gits::Config::Get().common.player.showWindowBorder) {
    c.pFullscreenDesc_.value->Windowed = true;
    Log(INFO) << "CreateSwapChainForHwnd: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::pre(IDXGIFactoryMakeWindowAssociationCommand& c) {
  c.WindowHandle_.value = manager_.getWindowService().getCurrentHwnd(c.WindowHandle_.value);
}

void ReplayCustomizationLayer::post(ID3D12ResourceMapCommand& c) {
  manager_.getMapTrackingService().mapResource(c.object_.key, c.Subresource_.value,
                                               c.ppData_.captureValue, c.ppData_.value);
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {

  if (c.result_.value != S_OK) {
    return;
  }

  ID3D12DescriptorHeap* descriptorHeap = static_cast<ID3D12DescriptorHeap*>(*c.ppvHeap_.value);

  manager_.getDescriptorHandleService().createDescriptorHeap(c.ppvHeap_.key, descriptorHeap,
                                                             c.pDescriptorHeapDesc_.value);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
  if (c.pDesc_.value &&
      c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    c.pDesc_.value->RaytracingAccelerationStructure.Location =
        manager_.getGpuAddressService().getGpuAddress(c.pDesc_.raytracingLocationKey,
                                                      c.pDesc_.raytracingLocationOffset);
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(
    ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateSamplerCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(ID3D12Device11CreateSampler2Command& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  fillGpuDescriptorHandleArgument(c.BaseDescriptor_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  fillGpuDescriptorHandleArgument(c.BaseDescriptor_);
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  if (c.pRenderTargetDescriptors_.value) {
    for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
      c.pRenderTargetDescriptors_.value[i].ptr =
          manager_.getDescriptorHandleService().getDescriptorHandle(
              c.pRenderTargetDescriptors_.interfaceKeys[i],
              ReplayDescriptorHandleService::HandleType::CpuHandle,
              c.pRenderTargetDescriptors_.indexes[i]);
    }
  }
  if (c.pDepthStencilDescriptor_.value) {
    c.pDepthStencilDescriptor_.value[0].ptr =
        manager_.getDescriptorHandleService().getDescriptorHandle(
            c.pDepthStencilDescriptor_.interfaceKeys[0],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.pDepthStencilDescriptor_.indexes[0]);
  }
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DepthStencilView_);
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.RenderTargetView_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  fillGpuDescriptorHandleArgument(c.ViewGPUHandleInCurrentHeap_);
  fillCpuDescriptorHandleArgument(c.ViewCPUHandle_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  fillGpuDescriptorHandleArgument(c.ViewGPUHandleInCurrentHeap_);
  fillCpuDescriptorHandleArgument(c.ViewCPUHandle_);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCopyDescriptorsCommand& c) {
  for (unsigned i = 0; i < c.NumDestDescriptorRanges_.value; ++i) {
    c.pDestDescriptorRangeStarts_.value[i].ptr =
        manager_.getDescriptorHandleService().getDescriptorHandle(
            c.pDestDescriptorRangeStarts_.interfaceKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.pDestDescriptorRangeStarts_.indexes[i]);
  }
  for (unsigned i = 0; i < c.NumSrcDescriptorRanges_.value; ++i) {
    c.pSrcDescriptorRangeStarts_.value[i].ptr =
        manager_.getDescriptorHandleService().getDescriptorHandle(
            c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.pSrcDescriptorRangeStarts_.indexes[i]);
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptorRangeStart_);
  fillCpuDescriptorHandleArgument(c.SrcDescriptorRangeStart_);
}

void ReplayCustomizationLayer::pre(ID3D12FenceSetEventOnCompletionCommand& c) {
  if (gits::Config::Get().directx.player.waitOnEventCompletion) {
    c.hEvent_.value = NULL;
  }
  if (c.hEvent_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& c) {
  if (gits::Config::Get().directx.player.waitOnEventCompletion) {
    c.hEvent_.value = NULL;
  }
  if (c.hEvent_.value) {
    c.skip = true;
  }
  static bool printed = false;
  if (!printed) {
    // not implemented yet in capture
    Log(ERR) << "ID3D12Device1::SetEventOnMultipleFenceCompletion not handled!";
    printed = true;
  }
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  FenceObjectInfo* info = new FenceObjectInfo();
  info->lastSignaledValue = c.InitialValue_.value;
  c.ppFence_.objectInfo->addObjectInfo(this, info);
}

void ReplayCustomizationLayer::post(ID3D12FenceSignalCommand& c) {

  FenceObjectInfo* info = static_cast<FenceObjectInfo*>(c.object_.objectInfo->getObjectInfo(this));
  if (c.Value_.value < info->lastSignaledValue) {
    info->incremental = false;
  }
  info->lastSignaledValue = c.Value_.value;
  info->signaled = true;
}

void ReplayCustomizationLayer::post(ID3D12CommandQueueSignalCommand& c) {

  FenceObjectInfo* info = static_cast<FenceObjectInfo*>(c.pFence_.objectInfo->getObjectInfo(this));
  if (c.Value_.value < info->lastSignaledValue) {
    info->incremental = false;
  }
  info->lastSignaledValue = c.Value_.value;
  info->signaled = true;
}

void ReplayCustomizationLayer::pre(ID3D12FenceGetCompletedValueCommand& c) {

  FenceObjectInfo* info = static_cast<FenceObjectInfo*>(c.object_.objectInfo->getObjectInfo(this));
  waitForFence(c.key, info, c.object_.value, c.result_.value);
}

void ReplayCustomizationLayer::pre(WaitForFenceSignaledCommand& c) {
  // fence could be removed before wait is signaled
  if (c.fence_.value) {
    FenceObjectInfo* info = static_cast<FenceObjectInfo*>(c.fence_.objectInfo->getObjectInfo(this));
    waitForFence(c.key, info, c.fence_.value, c.value_.value);
  }
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  manager_.getGpuAddressService().createResource(c.ppvResource_.key, resource);
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.pHeap_.value);
  manager_.getGpuAddressService().createPlacedResource(c.ppvResource_.key, resource, c.pHeap_.key,
                                                       heap, c.HeapOffset_.value);
}

void ReplayCustomizationLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.pHeap_.value);
  manager_.getGpuAddressService().createPlacedResource(c.ppvResource_.key, resource, c.pHeap_.key,
                                                       heap, c.HeapOffset_.value);
}

void ReplayCustomizationLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.pHeap_.value);
  manager_.getGpuAddressService().createPlacedResource(c.ppvResource_.key, resource, c.pHeap_.key,
                                                       heap, c.HeapOffset_.value);
}

void ReplayCustomizationLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.ppvHeap_.value);
  manager_.getGpuAddressService().createHeap(c.ppvHeap_.key, heap);
}

void ReplayCustomizationLayer::post(ID3D12Device4CreateHeap1Command& c) {
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.ppvHeap_.value);
  manager_.getGpuAddressService().createHeap(c.ppvHeap_.key, heap);
}

void ReplayCustomizationLayer::post(INTC_D3D12_CreateHeapCommand& c) {
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.ppvHeap_.value);
  manager_.getGpuAddressService().createHeap(c.ppvHeap_.key, heap);
}

void ReplayCustomizationLayer::pre(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  c.pAddress_.value =
      manager_.getHeapAllocationService().getHeapAllocation(const_cast<void*>(c.pAddress_.value));
}

void ReplayCustomizationLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.ppvHeap_.value);
  manager_.getGpuAddressService().createHeap(c.ppvHeap_.key, heap);

  HeapObjectInfo* info = new HeapObjectInfo();
  info->replayHeapAllocationAddress = const_cast<void*>(c.pAddress_.value);
  c.ppvHeap_.objectInfo->addObjectInfo(this, info);
}

void ReplayCustomizationLayer::pre(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  c.pAddress_.value =
      manager_.getHeapAllocationService().getHeapAllocation(const_cast<void*>(c.pAddress_.value));
}

void ReplayCustomizationLayer::post(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.ppvHeap_.value);
  manager_.getGpuAddressService().createHeap(c.ppvHeap_.key, heap);

  HeapObjectInfo* info = new HeapObjectInfo();
  info->replayHeapAllocationAddress = const_cast<void*>(c.pAddress_.value);
  c.ppvHeap_.objectInfo->addObjectInfo(this, info);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  fillGpuAddressArgument(c.BufferLocation_);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.DestDescriptor_);
  if (c.pDesc_.value && c.pDesc_.value->BufferLocation) {
    c.pDesc_.value->BufferLocation = manager_.getGpuAddressService().getGpuAddress(
        c.pDesc_.bufferLocationKey, c.pDesc_.bufferLocationOffset);
  }
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.pView_.value && c.pView_.value->BufferLocation) {
    c.pView_.value->BufferLocation = manager_.getGpuAddressService().getGpuAddress(
        c.pView_.bufferLocationKey, c.pView_.bufferLocationOffset);
  }
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.pViews_.value) {
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {

      if (c.pViews_.value[i].BufferLocation) {
        c.pViews_.value[i].BufferLocation = manager_.getGpuAddressService().getGpuAddress(
            c.pViews_.bufferLocationKeys[i], c.pViews_.bufferLocationOffsets[i]);
      }
    }
  }
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.pViews_.value) {
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {
      if (c.pViews_.value[i].BufferLocation) {

        if (c.pViews_.value[i].BufferLocation) {
          c.pViews_.value[i].BufferLocation = manager_.getGpuAddressService().getGpuAddress(
              c.pViews_.bufferLocationKeys[i], c.pViews_.bufferLocationOffsets[i]);
        }
        if (c.pViews_.value[i].BufferFilledSizeLocation) {
          c.pViews_.value[i].BufferFilledSizeLocation =
              manager_.getGpuAddressService().getGpuAddress(
                  c.pViews_.bufferFilledSizeLocationKeys[i],
                  c.pViews_.bufferFilledSizeLocationOffsets[i]);
        }
      }
    }
  }
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.pParams_.value) {
    for (unsigned i = 0; i < c.Count_.value; ++i) {

      if (c.pParams_.value[i].Dest) {
        c.pParams_.value[i].Dest = manager_.getGpuAddressService().getGpuAddress(
            c.pParams_.destKeys[i], c.pParams_.destOffsets[i]);
      }
    }
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCheckFeatureSupportCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12Device1CreatePipelineLibraryCommand& c) {
  pipelineLibraryService_.createPipelineLibrary(c);
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibrarySerializeCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibraryGetSerializedSizeCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibraryStorePipelineCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  c.pDesc_.value->CachedPSO.pCachedBlob = nullptr;
  pipelineLibraryService_.createPipelineState(c.ppPipelineState_.key);
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  c.pDesc_.value->CachedPSO.pCachedBlob = nullptr;
  if (c.result_.value != S_OK || manager_.multithreadedShaderCompilation()) {
    return;
  }
  c.skip = true;
  pipelineLibraryService_.loadGraphicsPipeline(c);
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  c.pDesc_.value->CachedPSO.pCachedBlob = nullptr;
  pipelineLibraryService_.createPipelineState(c.ppPipelineState_.key);
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  c.pDesc_.value->CachedPSO.pCachedBlob = nullptr;
  if (c.result_.value != S_OK || manager_.multithreadedShaderCompilation()) {
    return;
  }
  c.skip = true;
  pipelineLibraryService_.loadComputePipeline(c);
}

void ReplayCustomizationLayer::pre(ID3D12Device2CreatePipelineStateCommand& c) {
  removeCachedPSO(*c.pDesc_.value);
  pipelineLibraryService_.createPipelineState(c.ppPipelineState_.key);
}

void ReplayCustomizationLayer::pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  removeCachedPSO(*c.pDesc_.value);
  if (c.result_.value != S_OK || manager_.multithreadedShaderCompilation()) {
    return;
  }
  c.skip = true;
  pipelineLibraryService_.loadPipeline(c);
}

void ReplayCustomizationLayer::pre(
    IDXGIAdapter3RegisterVideoMemoryBudgetChangeNotificationEventCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(
    IDXGIAdapter3UnregisterVideoMemoryBudgetChangeNotificationCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12DeviceGetAdapterLuidCommand& c) {
  manager_.getAdapterService().setCaptureAdapterLuid(c.object_.key, c.result_.value);
}

void ReplayCustomizationLayer::post(ID3D12DeviceGetAdapterLuidCommand& c) {
  manager_.getAdapterService().setCurrentAdapterLuid(c.object_.key, c.result_.value);
}

void ReplayCustomizationLayer::pre(IDXGIAdapterEnumOutputsCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIAdapterGetDescCommand& c) {
  manager_.getAdapterService().setCaptureAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::post(IDXGIAdapterGetDescCommand& c) {
  manager_.getAdapterService().setCurrentAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
}

void ReplayCustomizationLayer::pre(IDXGIAdapterCheckInterfaceSupportCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIAdapter1GetDesc1Command& c) {
  manager_.getAdapterService().setCaptureAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::post(IDXGIAdapter1GetDesc1Command& c) {
  manager_.getAdapterService().setCurrentAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
}

void ReplayCustomizationLayer::pre(IDXGIAdapter2GetDesc2Command& c) {
  manager_.getAdapterService().setCaptureAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::post(IDXGIAdapter2GetDesc2Command& c) {
  manager_.getAdapterService().setCurrentAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
}

void ReplayCustomizationLayer::pre(IDXGIAdapter4GetDesc3Command& c) {
  manager_.getAdapterService().setCaptureAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::post(IDXGIAdapter4GetDesc3Command& c) {
  manager_.getAdapterService().setCurrentAdapterLuid(c.object_.key, c.pDesc_.value->AdapterLuid);
}

void ReplayCustomizationLayer::pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(
    IDXGIAdapter3RegisterHardwareContentProtectionTeardownStatusEventCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(IDXGIAdapter3SetVideoMemoryReservationCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(
    IDXGIAdapter3UnregisterHardwareContentProtectionTeardownStatusCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  c.AdapterLuid_.value = manager_.getAdapterService().getCurrentLuid(c.AdapterLuid_.value);
  if ((c.AdapterLuid_.value.HighPart == 0) && (c.AdapterLuid_.value.LowPart == 0)) {
    c.skip = true;
    Log(WARN) << "EnumAdapterByLuid - Cannot find capture-to-current LUID in map. Command "
              << c.key;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputFindClosestMatchingModeCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetDescCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetDisplayModeListCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetDisplaySurfaceDataCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetFrameStatisticsCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetGammaControlCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputGetGammaControlCapabilitiesCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputReleaseOwnershipCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputSetDisplaySurfaceCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputSetGammaControlCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputTakeOwnershipCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutputWaitForVBlankCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput1DuplicateOutputCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput1FindClosestMatchingMode1Command& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput1GetDisplayModeList1Command& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput1GetDisplaySurfaceData1Command& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput2SupportsOverlaysCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput3CheckOverlaySupportCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput4CheckOverlayColorSpaceSupportCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput5DuplicateOutput1Command& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput6CheckHardwareCompositionSupportCommand& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIOutput6GetDesc1Command& c) {
  if (!c.object_.value) {
    c.skip = true;
  }
}

void ReplayCustomizationLayer::pre(IDXGIInfoQueueAddStorageFilterEntriesCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12InfoQueueAddStorageFilterEntriesCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12InfoQueuePushStorageFilterCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12Device12GetResourceAllocationInfo3Command& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (Config::Get().directx.player.skipResolveQueryData) {
    c.skip = true;
    if (c.Type_.value == D3D12_QUERY_TYPE_OCCLUSION ||
        c.Type_.value == D3D12_QUERY_TYPE_BINARY_OCCLUSION) {
      static bool logged = false;
      if (!logged) {
        Log(WARN) << "Skipping ResolveQueryData for occlusion queries may result in corruptions";
        logged = true;
      }
    }
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.ppCommandQueue_.key & Command::stateRestoreKeyMask &&
      c.pDesc_.value->Type == D3D12_COMMAND_LIST_TYPE_COPY &&
      !Config::Get().directx.player.useCopyQueueOnRestore) {
    c.pDesc_.value->Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.ppCommandAllocator_.key & Command::stateRestoreKeyMask &&
      c.type_.value == D3D12_COMMAND_LIST_TYPE_COPY &&
      !Config::Get().directx.player.useCopyQueueOnRestore) {
    c.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.ppCommandList_.key & Command::stateRestoreKeyMask &&
      c.type_.value == D3D12_COMMAND_LIST_TYPE_COPY &&
      !Config::Get().directx.player.useCopyQueueOnRestore) {
    c.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::pre(IDMLDeviceCreateBindingTableCommand& c) {
  c.desc_.value->CPUDescriptorHandle.ptr =
      manager_.getDescriptorHandleService().getDescriptorHandle(
          c.desc_.data.cpuDescHandleKey, ReplayDescriptorHandleService::HandleType::CpuHandle,
          c.desc_.data.cpuDescHandleIndex);

  c.desc_.value->GPUDescriptorHandle.ptr =
      manager_.getDescriptorHandleService().getDescriptorHandle(
          c.desc_.data.gpuDescHandleKey, ReplayDescriptorHandleService::HandleType::GpuHandle,
          c.desc_.data.gpuDescHandleIndex);
}

void ReplayCustomizationLayer::pre(IDMLBindingTableResetCommand& c) {
  c.desc_.value->CPUDescriptorHandle.ptr =
      manager_.getDescriptorHandleService().getDescriptorHandle(
          c.desc_.data.cpuDescHandleKey, ReplayDescriptorHandleService::HandleType::CpuHandle,
          c.desc_.data.cpuDescHandleIndex);

  c.desc_.value->GPUDescriptorHandle.ptr =
      manager_.getDescriptorHandleService().getDescriptorHandle(
          c.desc_.data.gpuDescHandleKey, ReplayDescriptorHandleService::HandleType::GpuHandle,
          c.desc_.data.gpuDescHandleIndex);
}

void ReplayCustomizationLayer::pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  for (unsigned i = 0; i < c.NumRenderTargets_.value; ++i) {
    c.pRenderTargets_.value[i].cpuDescriptor.ptr =
        manager_.getDescriptorHandleService().getDescriptorHandle(
            c.pRenderTargets_.descriptorKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.pRenderTargets_.descriptorIndexes[i]);
  }
  if (c.pDepthStencil_.value) {
    c.pDepthStencil_.value->cpuDescriptor.ptr =
        manager_.getDescriptorHandleService().getDescriptorHandle(
            c.pDepthStencil_.descriptorKey, ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.pDepthStencil_.descriptorIndex);
  }
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  c.pDesc_.value->DestAccelerationStructureData = manager_.getGpuAddressService().getGpuAddress(
      c.pDesc_.destAccelerationStructureKey, c.pDesc_.destAccelerationStructureOffset);
  if (c.pDesc_.value->SourceAccelerationStructureData) {
    c.pDesc_.value->SourceAccelerationStructureData = manager_.getGpuAddressService().getGpuAddress(
        c.pDesc_.sourceAccelerationStructureKey, c.pDesc_.sourceAccelerationStructureOffset);
  }
  c.pDesc_.value->ScratchAccelerationStructureData = manager_.getGpuAddressService().getGpuAddress(
      c.pDesc_.scratchAccelerationStructureKey, c.pDesc_.scratchAccelerationStructureOffset);

  unsigned inputIndex = 0;
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    // c.pDesc_.value->Inputs.InstanceDescs set in GpuPatchLayer
    ++inputIndex;
  } else {
    for (unsigned i = 0; i < c.pDesc_.value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.pDesc_.value->Inputs.pGeometryDescs[i]
              : *c.pDesc_.value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.Triangles.Transform3x4 = manager_.getGpuAddressService().getGpuAddress(
            c.pDesc_.inputKeys[inputIndex], c.pDesc_.inputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.IndexBuffer = manager_.getGpuAddressService().getGpuAddress(
            c.pDesc_.inputKeys[inputIndex], c.pDesc_.inputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.VertexBuffer.StartAddress = manager_.getGpuAddressService().getGpuAddress(
            c.pDesc_.inputKeys[inputIndex], c.pDesc_.inputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.AABBs.AABBs.StartAddress = manager_.getGpuAddressService().getGpuAddress(
            c.pDesc_.inputKeys[inputIndex], c.pDesc_.inputOffsets[inputIndex]);
        ++inputIndex;
      }
    }
  }

  for (unsigned i = 0; i < c.NumPostbuildInfoDescs_.value; ++i) {
    c.pPostbuildInfoDescs_.value[i].DestBuffer = manager_.getGpuAddressService().getGpuAddress(
        c.pPostbuildInfoDescs_.destBufferKeys[i], c.pPostbuildInfoDescs_.destBufferOffsets[i]);
  }
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  c.DestAccelerationStructureData_.value = manager_.getGpuAddressService().getGpuAddress(
      c.DestAccelerationStructureData_.interfaceKey, c.DestAccelerationStructureData_.offset);
  c.SourceAccelerationStructureData_.value = manager_.getGpuAddressService().getGpuAddress(
      c.SourceAccelerationStructureData_.interfaceKey, c.SourceAccelerationStructureData_.offset);
}

void ReplayCustomizationLayer::pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(D3D12CreateRootSignatureDeserializerCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3D12DeviceOpenSharedHandleCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3DBlobGetBufferPointerCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::pre(ID3DBlobGetBufferSizeCommand& c) {
  c.skip = true;
}

void ReplayCustomizationLayer::fillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  if (arg.value) {
    arg.value = manager_.getGpuAddressService().getGpuAddress(arg.interfaceKey, arg.offset);
  }
}

void ReplayCustomizationLayer::fillGpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg) {

  arg.value.ptr = manager_.getDescriptorHandleService().getDescriptorHandle(
      arg.interfaceKey, ReplayDescriptorHandleService::HandleType::GpuHandle, arg.index);
}

void ReplayCustomizationLayer::fillCpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {

  arg.value.ptr = manager_.getDescriptorHandleService().getDescriptorHandle(
      arg.interfaceKey, ReplayDescriptorHandleService::HandleType::CpuHandle, arg.index);
}

void ReplayCustomizationLayer::waitForFence(unsigned commandKey,
                                            FenceObjectInfo* fenceInfo,
                                            ID3D12Fence* fence,
                                            unsigned fenceValue) {
  UINT64 value{};
  static HANDLE event{};
  if (fenceInfo->incremental) {
    while ((value = fence->GetCompletedValue()) < fenceValue) {
    }
  } else {
    if ((value = fence->GetCompletedValue()) != fenceValue) {
      if (!event) {
        event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      }
      fence->SetEventOnCompletion(fenceValue, event);
      DWORD ret = WaitForSingleObject(event, 10000);
      if (ret == WAIT_TIMEOUT) {
        Log(WARN) << "GetCompletedValue - timeout while waiting for fence. Command " << commandKey;
      }
    }
  }
}

void ReplayCustomizationLayer::removeCachedPSO(D3D12_PIPELINE_STATE_STREAM_DESC& desc) {
  if (desc.SizeInBytes == 0 || desc.pPipelineStateSubobjectStream == nullptr) {
    return;
  }

  size_t offset = 0;
  while (offset < desc.SizeInBytes) {
    auto pStream = static_cast<uint8_t*>(desc.pPipelineStateSubobjectStream) + offset;
    auto subobjectType = *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(pStream);
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::pRootSignature);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::VS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::PS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::HS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::GS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::CS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM2::AS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM2::MS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::StreamOutput);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::BlendState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleMask);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::RasterizerState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM4::RasterizerState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM5::RasterizerState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DepthStencilState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM3::DepthStencilState);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::InputLayout);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::IBStripCutValue);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::PrimitiveTopologyType);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::RTVFormats);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DSVFormat);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleDesc);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::NodeMask);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO);
      D3D12_CACHED_PIPELINE_STATE& cachedPSO =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO)*>(pStream);
      cachedPSO.CachedBlobSizeInBytes = 0;
      cachedPSO.pCachedBlob = nullptr;
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::Flags);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM1::ViewInstancingDesc);
      break;
    default:
      GITS_ASSERT(0 && "Unexpected subobject type");
      break;
    }
  }
}

void ReplayCustomizationLayer::overrideAdapter(D3D12CreateDeviceCommand& command) {
  const auto& adapterOverride = Config::Get().directx.player.adapterOverride;
  if (!adapterOverride.enabled) {
    return;
  }

  // Use existing adapter if already overridden
  if (adapterOverride_) {
    command.pAdapter_.value = adapterOverride_.Get();
    return;
  }

  const std::unordered_map<std::string, unsigned> adapterMap = {
      {"", 0}, {"intel", 0x8086}, {"amd", 0x1002}, {"nvidia", 0x10de}};

  std::string adapterVendor = adapterOverride.vendor;
  for (char& c : adapterVendor) {
    c = std::tolower(c);
  }
  if (adapterMap.count(adapterVendor) == 0) {
    Log(WARN) << "AdapterOverride - Unknown vendor: " << adapterOverride.vendor;
    return;
  }

  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  GITS_ASSERT(factory);

  Log(INFO) << "Adapters:";
  unsigned vendorId = adapterMap.at(adapterVendor);
  unsigned adapterIndex = 0;
  unsigned adapterFromVendorIndex = 0;
  unsigned adapterOverrideIndex = 0;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  while (SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter))) {
    DXGI_ADAPTER_DESC1 adapterDesc{};
    HRESULT hr = adapter->GetDesc1(&adapterDesc);
    GITS_ASSERT(hr == S_OK);

    std::wstring descriptionW = adapterDesc.Description;
    std::string description(descriptionW.begin(), descriptionW.end());

    Log(INFO) << "  (" << adapterIndex << ")" << std::hex << std::setfill('0') << " VendorId = 0x"
              << std::setw(4) << adapterDesc.VendorId << " DeviceId = 0x" << std::setw(4)
              << adapterDesc.DeviceId << " Description = " << std::setw(4) << description;

    bool found = vendorId == 0 && adapterIndex == adapterOverride.index;
    if (adapterDesc.VendorId == vendorId) {
      found = adapterFromVendorIndex == adapterOverride.index;
      ++adapterFromVendorIndex;
    }

    if (found) {
      adapterOverride_ = adapter;
      adapterOverrideIndex = adapterIndex;
    }

    ++adapterIndex;
  }

  if (adapterOverride_) {
    command.pAdapter_.value = adapterOverride_.Get();
    Log(INFO) << "AdapterOverride - Set adapter found at index " << adapterOverrideIndex;
  } else {
    Log(WARN) << "AdapterOverride - Adapter not found, will use default adapter (index 0)";
  }
}

} // namespace DirectX
} // namespace gits
