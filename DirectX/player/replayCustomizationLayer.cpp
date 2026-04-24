// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"
#include "configurator.h"
#include "playerManager.h"
#include "interfaceArgumentUpdaters.h"
#include "keyUtils.h"
#include "to_string/toStr.h"
#include "log.h"

#include <d3dx12.h>
#include <setupapi.h>
#include <ntddvdeo.h>
#include <igdext.h>

namespace gits {
namespace DirectX {

static std::string appInfoToStr(INTCExtensionAppInfo1* appInfo) {
  if (!appInfo) {
    return "Application: nullptr, Engine: nullptr";
  }

  std::wstring appNameW = appInfo->pApplicationName ? appInfo->pApplicationName : L"";
  auto appName = std::string(appNameW.begin(), appNameW.end());
  std::wstring engineNameW = appInfo->pEngineName ? appInfo->pEngineName : L"";
  auto engineName = std::string(engineNameW.begin(), engineNameW.end());

  std::ostringstream oss;
  oss << "Application: \"" << appName << "\" (" << appInfo->ApplicationVersion.major << "."
      << appInfo->ApplicationVersion.minor << "." << appInfo->ApplicationVersion.patch << ")"
      << ", Engine: \"" << engineName << "\" (" << appInfo->EngineVersion.major << "."
      << appInfo->EngineVersion.minor << "." << appInfo->EngineVersion.patch << ")";
  return oss.str();
}

ReplayCustomizationLayer::ReplayCustomizationLayer(PlayerManager& manager)
    : Layer("ReplayCustomization"),
      m_Manager(manager),
      m_PipelineLibraryService(manager.GetPipelineLibraryService()) {
  m_UseAddressPinning =
      Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE;
}

void ReplayCustomizationLayer::Post(IUnknownReleaseCommand& c) {
  if (c.Skip) {
    return;
  }
  if (c.m_Result.Value == 0) {
    m_Manager.GetDescriptorHandleService().DestroyDescriptorHeap(c.m_Object.Key);
    m_Manager.GetMapTrackingService().DestroyResource(c.m_Object.Key);
    m_Manager.GetGpuAddressService().DestroyInterface(c.m_Object.Key);
    m_Manager.RemoveObject(c.m_Object.Key);
    m_Manager.GetHeapAllocationService().DestroyHeapAllocation(c.m_Object.Key);
  }
  m_PipelineLibraryService.ReleasePipelineState(c.m_Object.Key, c.m_Result.Value);
}

void ReplayCustomizationLayer::Post(IUnknownAddRefCommand& c) {
  if (c.Skip) {
    return;
  }
  m_PipelineLibraryService.AddRefPipelineState(c.m_Object.Key);
  m_AfterAddRef = true;
}

void ReplayCustomizationLayer::Pre(D3D12CreateDeviceCommand& c) {
  if (c.Skip) {
    return;
  }
  const auto& adapterService = m_Manager.GetAdapterService();
  // Set the adapter override if needed
  if (adapterService.IsAdapterOverride()) {
    c.m_pAdapter.Value = adapterService.GetAdapter();
  }

  // Print adapter description

  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  if (!c.m_pAdapter.Value) {
    // Will get description from the default adapter
    adapter = adapterService.GetAdapter();
  } else {
    c.m_pAdapter.Value->QueryInterface(IID_PPV_ARGS(&adapter));
  }

  DXGI_ADAPTER_DESC1 desc{};
  HRESULT hr = adapter->GetDesc1(&desc);
  GITS_ASSERT(hr == S_OK);

  LOG_INFO << "D3D12CreateDevice - Using adapter: " << toStr(desc.Description);
}

void ReplayCustomizationLayer::Pre(IDXGISwapChainSetFullscreenStateCommand& c) {
  if (c.Skip) {
    return;
  }
  if (c.m_Fullscreen.Value && Configurator::Get().common.player.showWindowBorder) {
    c.m_Fullscreen.Value = false;
    LOG_INFO << "SetFullscreenState: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::Pre(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_pDesc.Value->OutputWindow =
      m_Manager.GetWindowService().GetCurrentHwnd(c.m_pDesc.Value->OutputWindow);
  if (Configurator::Get().common.player.showWindowBorder) {
    c.m_pDesc.Value->Windowed = true;
    LOG_INFO << "CreateSwapChain: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::Pre(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_hWnd.Value = m_Manager.GetWindowService().GetCurrentHwnd(c.m_hWnd.Value);
  if (c.m_pFullscreenDesc.Value && Configurator::Get().common.player.showWindowBorder) {
    c.m_pFullscreenDesc.Value->Windowed = true;
    LOG_INFO << "CreateSwapChainForHwnd: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::Pre(IDXGIFactoryMakeWindowAssociationCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_WindowHandle.Value = m_Manager.GetWindowService().GetCurrentHwnd(c.m_WindowHandle.Value);
}

void ReplayCustomizationLayer::Post(ID3D12ResourceMapCommand& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetMapTrackingService().MapResource(c.m_Object.Key, c.m_Subresource.Value,
                                                c.m_ppData.CaptureValue, c.m_ppData.Value);
}

void ReplayCustomizationLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }

  ID3D12DescriptorHeap* descriptorHeap = static_cast<ID3D12DescriptorHeap*>(*c.m_ppvHeap.Value);

  m_Manager.GetDescriptorHandleService().CreateDescriptorHeap(c.m_ppvHeap.Key, descriptorHeap,
                                                              c.m_pDescriptorHeapDesc.Value);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);

  if (m_UseAddressPinning) {
    return;
  }

  if (c.m_pDesc.Value &&
      c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    c.m_pDesc.Value->RaytracingAccelerationStructure.Location =
        m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pDesc.RaytracingLocationKey,
                                                       c.m_pDesc.RaytracingLocationOffset);
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(
    ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateSamplerCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(ID3D12Device11CreateSampler2Command& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuDescriptorHandleArgument(c.m_BaseDescriptor);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuDescriptorHandleArgument(c.m_BaseDescriptor);
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (c.Skip) {
    return;
  }
  if (c.m_pRenderTargetDescriptors.Value) {
    for (unsigned i = 0; i < c.m_pRenderTargetDescriptors.Size; ++i) {
      c.m_pRenderTargetDescriptors.Value[i].ptr =
          m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
              c.m_pRenderTargetDescriptors.InterfaceKeys[i],
              ReplayDescriptorHandleService::HandleType::CpuHandle,
              c.m_pRenderTargetDescriptors.Indexes[i]);
    }
  }
  if (c.m_pDepthStencilDescriptor.Value) {
    c.m_pDepthStencilDescriptor.Value[0].ptr =
        m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
            c.m_pDepthStencilDescriptor.InterfaceKeys[0],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDepthStencilDescriptor.Indexes[0]);
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DepthStencilView);
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_RenderTargetView);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuDescriptorHandleArgument(c.m_ViewGPUHandleInCurrentHeap);
  FillCpuDescriptorHandleArgument(c.m_ViewCPUHandle);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuDescriptorHandleArgument(c.m_ViewGPUHandleInCurrentHeap);
  FillCpuDescriptorHandleArgument(c.m_ViewCPUHandle);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (c.Skip) {
    return;
  }
  for (unsigned i = 0; i < c.m_NumDestDescriptorRanges.Value; ++i) {
    c.m_pDestDescriptorRangeStarts.Value[i].ptr =
        m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
            c.m_pDestDescriptorRangeStarts.InterfaceKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDestDescriptorRangeStarts.Indexes[i]);
  }
  for (unsigned i = 0; i < c.m_NumSrcDescriptorRanges.Value; ++i) {
    c.m_pSrcDescriptorRangeStarts.Value[i].ptr =
        m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
            c.m_pSrcDescriptorRangeStarts.InterfaceKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.m_pSrcDescriptorRangeStarts.Indexes[i]);
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptorRangeStart);
  FillCpuDescriptorHandleArgument(c.m_SrcDescriptorRangeStart);
}

void ReplayCustomizationLayer::Pre(ID3D12FenceSetEventOnCompletionCommand& c) {
  if (Configurator::Get().directx.player.waitOnEventCompletion) {
    c.m_hEvent.Value = NULL;
  }
  if (c.m_hEvent.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& c) {
  if (c.Skip) {
    return;
  }
  if (Configurator::Get().directx.player.waitOnEventCompletion) {
    c.m_hEvent.Value = NULL;
  }
  if (c.m_hEvent.Value) {
    c.Skip = true;
  }
  static bool printed = false;
  if (!printed) {
    // not implemented yet in capture
    LOG_ERROR << "ID3D12Device1::SetEventOnMultipleFenceCompletion not handled!";
    printed = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12FenceGetCompletedValueCommand& c) {
  m_CapturedFenceValue = c.m_Result.Value;
}

void ReplayCustomizationLayer::Post(ID3D12FenceGetCompletedValueCommand& c) {
  if (c.Skip) {
    return;
  }
  WaitForFence(c.Key, c.m_Object.Value, m_CapturedFenceValue);
  c.m_Result.Value = c.m_Object.Value->GetCompletedValue();
}

void ReplayCustomizationLayer::Post(WaitForFenceSignaledCommand& c) {
  if (c.Skip) {
    return;
  }
  // fence could be removed before wait is signaled
  if (c.m_fence.Value) {
    WaitForFence(c.Key, c.m_fence.Value, c.m_Value.Value);
  }
}

void ReplayCustomizationLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      (m_UseAddressPinning && !IsStateRestoreKey(c.m_ppvResource.Key))) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  m_Manager.GetGpuAddressService().CreateResource(c.m_ppvResource.Key, resource);
}

void ReplayCustomizationLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.m_pHeap.Value);
  m_Manager.GetGpuAddressService().CreatePlacedResource(c.m_ppvResource.Key, resource,
                                                        c.m_pHeap.Key, heap, c.m_HeapOffset.Value);
}

void ReplayCustomizationLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.m_pHeap.Value);
  m_Manager.GetGpuAddressService().CreatePlacedResource(c.m_ppvResource.Key, resource,
                                                        c.m_pHeap.Key, heap, c.m_HeapOffset.Value);
}

void ReplayCustomizationLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_UseAddressPinning) {
    return;
  }

  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(c.m_pHeap.Value);
  m_Manager.GetGpuAddressService().CreatePlacedResource(c.m_ppvResource.Key, resource,
                                                        c.m_pHeap.Key, heap, c.m_HeapOffset.Value);
}

void ReplayCustomizationLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
  m_Manager.GetGpuAddressService().CreateHeap(c.m_ppvHeap.Key, heap);
}

void ReplayCustomizationLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
  m_Manager.GetGpuAddressService().CreateHeap(c.m_ppvHeap.Key, heap);
}

void ReplayCustomizationLayer::Post(INTC_D3D12_CreateHeapCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
  m_Manager.GetGpuAddressService().CreateHeap(c.m_ppvHeap.Key, heap);
}

void ReplayCustomizationLayer::Pre(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  c.m_pAddress.Value =
      m_Manager.GetHeapAllocationService().GetHeapAllocation(const_cast<void*>(c.m_pAddress.Value));
}

void ReplayCustomizationLayer::Post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
  m_Manager.GetGpuAddressService().CreateHeap(c.m_ppvHeap.Key, heap);
}

void ReplayCustomizationLayer::Pre(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  c.m_pAddress.Value =
      m_Manager.GetHeapAllocationService().GetHeapAllocation(const_cast<void*>(c.m_pAddress.Value));
}

void ReplayCustomizationLayer::Post(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
  m_Manager.GetGpuAddressService().CreateHeap(c.m_ppvHeap.Key, heap);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillGpuAddressArgument(c.m_BufferLocation);
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (!c.m_pPipelineState.Key) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (!c.m_pStateObject.Key) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (c.Skip) {
    return;
  }
  FillCpuDescriptorHandleArgument(c.m_DestDescriptor);

  if (m_UseAddressPinning) {
    return;
  }

  if (c.m_pDesc.Value && c.m_pDesc.Value->BufferLocation) {
    c.m_pDesc.Value->BufferLocation = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pDesc.BufferLocationKey, c.m_pDesc.BufferLocationOffset);
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  if (c.m_pView.Value && c.m_pView.Value->BufferLocation) {
    c.m_pView.Value->BufferLocation = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pView.BufferLocationKey, c.m_pView.BufferLocationOffset);
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {

      if (c.m_pViews.Value[i].BufferLocation) {
        c.m_pViews.Value[i].BufferLocation = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pViews.BufferLocationKeys[i], c.m_pViews.BufferLocationOffsets[i]);
      }
    }
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].BufferLocation) {

        if (c.m_pViews.Value[i].BufferLocation) {
          c.m_pViews.Value[i].BufferLocation = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pViews.BufferLocationKeys[i], c.m_pViews.BufferLocationOffsets[i]);
        }
        if (c.m_pViews.Value[i].BufferFilledSizeLocation) {
          c.m_pViews.Value[i].BufferFilledSizeLocation =
              m_Manager.GetGpuAddressService().GetGpuAddress(
                  c.m_pViews.BufferFilledSizeLocationKeys[i],
                  c.m_pViews.BufferFilledSizeLocationOffsets[i]);
        }
      }
    }
  }
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  if (c.m_pParams.Value) {
    for (unsigned i = 0; i < c.m_Count.Value; ++i) {

      if (c.m_pParams.Value[i].Dest) {
        c.m_pParams.Value[i].Dest = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.DestKeys[i], c.m_pParams.DestOffsets[i]);
      }
    }
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCheckFeatureSupportCommand& c) {
  c.Skip = true;
  // The data may contain pointers (from capture) that have not been encoded in the stream
  if (c.m_Feature.Value == D3D12_FEATURE_FEATURE_LEVELS) {
    std::memset(c.m_pFeatureSupportData.Value, 0, c.m_FeatureSupportDataSize.Value);
  }
}

void ReplayCustomizationLayer::Pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  unsigned inputIndex = 0;
  if (c.m_pDesc.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    // requires instances patching
    c.Skip = true;
    ++inputIndex;
  } else if (c.m_pDesc.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.m_pDesc.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->pGeometryDescs[i]
              : *c.m_pDesc.Value->ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.Triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.VertexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.AABBs.AABBs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles =
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles);
          triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          triangles.VertexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          auto& ommLinkage = *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
              desc.OmmTriangles.pOmmLinkage);
          ommLinkage.OpacityMicromapIndexBuffer.StartAddress =
              m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pDesc.InputKeys[inputIndex],
                                                             c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          ommLinkage.OpacityMicromapArray = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
        }
      }
    }
  } else if (c.m_pDesc.Value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->pOpacityMicromapArrayDesc) {
      auto& ommDesc = *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          c.m_pDesc.Value->pOpacityMicromapArrayDesc);
      ommDesc.InputBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
      ++inputIndex;
      ommDesc.PerOmmDescs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
      ++inputIndex;
    }
  }
}

void ReplayCustomizationLayer::Pre(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (c.Skip) {
    return;
  }
  m_PipelineLibraryService.CreatePipelineLibrary(c);
}

void ReplayCustomizationLayer::Pre(ID3D12ObjectGetPrivateDataCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12ObjectSetNameCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12ObjectSetPrivateDataCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12ObjectSetPrivateDataInterfaceCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibrarySerializeCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibraryGetSerializedSizeCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibraryStorePipelineCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_pDesc.Value->CachedPSO.pCachedBlob = nullptr;
  m_PipelineLibraryService.CreatePipelineState(c.m_ppPipelineState.Key);
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_pDesc.Value->CachedPSO.pCachedBlob = nullptr;
  if (c.m_Result.Value != S_OK || m_Manager.MultithreadedShaderCompilation()) {
    return;
  }
  c.Skip = true;
  c.m_Result.Value = m_PipelineLibraryService.LoadGraphicsPipeline(c);
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_pDesc.Value->CachedPSO.pCachedBlob = nullptr;
  m_PipelineLibraryService.CreatePipelineState(c.m_ppPipelineState.Key);
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_pDesc.Value->CachedPSO.pCachedBlob = nullptr;
  if (c.m_Result.Value != S_OK || m_Manager.MultithreadedShaderCompilation()) {
    return;
  }
  c.Skip = true;
  c.m_Result.Value = m_PipelineLibraryService.LoadComputePipeline(c);
}

void ReplayCustomizationLayer::Pre(ID3D12Device2CreatePipelineStateCommand& c) {
  if (c.Skip) {
    return;
  }
  RemoveCachedPso(*c.m_pDesc.Value);
  m_PipelineLibraryService.CreatePipelineState(c.m_ppPipelineState.Key);
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (c.Skip) {
    return;
  }
  RemoveCachedPso(*c.m_pDesc.Value);
  if (c.m_Result.Value != S_OK || m_Manager.MultithreadedShaderCompilation()) {
    return;
  }
  c.Skip = true;
  c.m_Result.Value = m_PipelineLibraryService.LoadPipeline(c);
}

void ReplayCustomizationLayer::Pre(ID3D12PipelineStateGetCachedBlobCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(
    IDXGIAdapter3RegisterVideoMemoryBudgetChangeNotificationEventCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(
    IDXGIAdapter3UnregisterVideoMemoryBudgetChangeNotificationCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceGetAdapterLuidCommand& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCaptureAdapterLuid(c.m_Object.Key, c.m_Result.Value);
}

void ReplayCustomizationLayer::Post(ID3D12DeviceGetAdapterLuidCommand& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCurrentAdapterLuid(c.m_Object.Key, c.m_Result.Value);
}

void ReplayCustomizationLayer::Pre(IDXGIAdapterEnumOutputsCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIAdapterGetDescCommand& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCaptureAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Post(IDXGIAdapterGetDescCommand& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCurrentAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
}

void ReplayCustomizationLayer::Pre(IDXGIAdapterCheckInterfaceSupportCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIAdapter1GetDesc1Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCaptureAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Post(IDXGIAdapter1GetDesc1Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCurrentAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
}

void ReplayCustomizationLayer::Pre(IDXGIAdapter2GetDesc2Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCaptureAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Post(IDXGIAdapter2GetDesc2Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCurrentAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
}

void ReplayCustomizationLayer::Pre(IDXGIAdapter4GetDesc3Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCaptureAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Post(IDXGIAdapter4GetDesc3Command& c) {
  if (c.Skip) {
    return;
  }
  m_Manager.GetAdapterService().SetCurrentAdapterLuid(c.m_Object.Key, c.m_pDesc.Value->AdapterLuid);
}

void ReplayCustomizationLayer::Pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(
    IDXGIAdapter3RegisterHardwareContentProtectionTeardownStatusEventCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(IDXGIAdapter3SetVideoMemoryReservationCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
    return;
  }
  // Clamp reservation to AvailableForReservation to avoid E_INVALIDARG on
  // machines with less reservable VRAM than the recording machine.
  DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
  HRESULT hr = c.m_Object.Value->QueryVideoMemoryInfo(c.m_NodeIndex.Value,
                                                      c.m_MemorySegmentGroup.Value, &memInfo);
  if (SUCCEEDED(hr) && c.m_Reservation.Value > memInfo.AvailableForReservation) {
    LOG_WARNING << "SetVideoMemoryReservation: Requested reservation of " << c.m_Reservation.Value
                << " bytes exceeds available video memory (" << memInfo.AvailableForReservation
                << " bytes). Reserving the maximum available amount instead.";
    c.m_Reservation.Value = memInfo.AvailableForReservation;
  }
}

void ReplayCustomizationLayer::Pre(
    IDXGIAdapter3UnregisterHardwareContentProtectionTeardownStatusCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (c.Skip) {
    return;
  }

  c.m_AdapterLuid.Value = m_Manager.GetAdapterService().GetCurrentLuid(c.m_AdapterLuid.Value);
  if ((c.m_AdapterLuid.Value.HighPart == 0) && (c.m_AdapterLuid.Value.LowPart == 0)) {
    c.Skip = true;
    LOG_WARNING << "EnumAdapterByLuid - Cannot find capture-to-current LUID in map. Command "
                << c.Key;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputFindClosestMatchingModeCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetDescCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetDisplayModeListCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetDisplaySurfaceDataCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetFrameStatisticsCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetGammaControlCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputGetGammaControlCapabilitiesCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputReleaseOwnershipCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputSetDisplaySurfaceCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputSetGammaControlCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputTakeOwnershipCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutputWaitForVBlankCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput1DuplicateOutputCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput1FindClosestMatchingMode1Command& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput1GetDisplayModeList1Command& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput1GetDisplaySurfaceData1Command& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput2SupportsOverlaysCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput3CheckOverlaySupportCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput4CheckOverlayColorSpaceSupportCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput5DuplicateOutput1Command& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput6CheckHardwareCompositionSupportCommand& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIOutput6GetDesc1Command& c) {
  if (!c.m_Object.Value) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(IDXGIInfoQueueAddStorageFilterEntriesCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12InfoQueueAddStorageFilterEntriesCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12InfoQueuePushStorageFilterCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12Device12GetResourceAllocationInfo3Command& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (c.Skip) {
    return;
  }
  if (Configurator::Get().directx.player.skipResolveQueryData) {
    c.Skip = true;
    if (c.m_Type.Value == D3D12_QUERY_TYPE_OCCLUSION ||
        c.m_Type.Value == D3D12_QUERY_TYPE_BINARY_OCCLUSION) {
      static bool logged = false;
      if (!logged) {
        LOG_WARNING << "Skipping ResolveQueryData for occlusion queries may result in corruptions";
        logged = true;
      }
    }
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.Skip) {
    return;
  }
  if (IsStateRestoreKey(c.m_ppCommandQueue.Key) &&
      c.m_pDesc.Value->Type == D3D12_COMMAND_LIST_TYPE_COPY &&
      (!Configurator::Get().directx.player.useCopyQueueOnRestore || m_AfterAddRef)) {
    // AddRefs are at the end of subcapture state restore but before back buffer restore
    c.m_pDesc.Value->Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.Skip) {
    return;
  }
  if (IsStateRestoreKey(c.m_ppCommandAllocator.Key) &&
      c.m_type.Value == D3D12_COMMAND_LIST_TYPE_COPY &&
      (!Configurator::Get().directx.player.useCopyQueueOnRestore || m_AfterAddRef)) {
    // AddRefs are at the end of subcapture state restore but before back buffer restore
    c.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.Skip) {
    return;
  }
  if (IsStateRestoreKey(c.m_ppCommandList.Key) && c.m_type.Value == D3D12_COMMAND_LIST_TYPE_COPY &&
      (!Configurator::Get().directx.player.useCopyQueueOnRestore || m_AfterAddRef)) {
    // AddRefs are at the end of subcapture state restore but before back buffer restore
    c.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void ReplayCustomizationLayer::Pre(IDMLDeviceCreateBindingTableCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_desc.Value->CPUDescriptorHandle.ptr =
      m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
          c.m_desc.TableFields.CpuDescHandleKey,
          ReplayDescriptorHandleService::HandleType::CpuHandle,
          c.m_desc.TableFields.CpuDescHandleIndex);

  c.m_desc.Value->GPUDescriptorHandle.ptr =
      m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
          c.m_desc.TableFields.GpuDescHandleKey,
          ReplayDescriptorHandleService::HandleType::GpuHandle,
          c.m_desc.TableFields.GpuDescHandleIndex);
}

void ReplayCustomizationLayer::Pre(IDMLBindingTableResetCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_desc.Value->CPUDescriptorHandle.ptr =
      m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
          c.m_desc.TableFields.CpuDescHandleKey,
          ReplayDescriptorHandleService::HandleType::CpuHandle,
          c.m_desc.TableFields.CpuDescHandleIndex);

  c.m_desc.Value->GPUDescriptorHandle.ptr =
      m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
          c.m_desc.TableFields.GpuDescHandleKey,
          ReplayDescriptorHandleService::HandleType::GpuHandle,
          c.m_desc.TableFields.GpuDescHandleIndex);
}

void ReplayCustomizationLayer::Pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  if (c.Skip) {
    return;
  }
  for (unsigned i = 0; i < c.m_NumRenderTargets.Value; ++i) {
    c.m_pRenderTargets.Value[i].cpuDescriptor.ptr =
        m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
            c.m_pRenderTargets.DescriptorKeys[i],
            ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.m_pRenderTargets.DescriptorIndexes[i]);
  }
  if (c.m_pDepthStencil.Value) {
    c.m_pDepthStencil.Value->cpuDescriptor.ptr =
        m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
            c.m_pDepthStencil.DescriptorKey, ReplayDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDepthStencil.DescriptorIndex);
  }
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    if (IsStateRestoreKey(c.m_pDesc.ScratchAccelerationStructureKey)) {
      c.m_pDesc.Value->ScratchAccelerationStructureData =
          m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.ScratchAccelerationStructureKey,
              c.m_pDesc.ScratchAccelerationStructureOffset);
    }
    return;
  }

  c.m_pDesc.Value->DestAccelerationStructureData = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_pDesc.DestAccelerationStructureKey, c.m_pDesc.DestAccelerationStructureOffset);
  if (c.m_pDesc.Value->SourceAccelerationStructureData) {
    c.m_pDesc.Value->SourceAccelerationStructureData =
        m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pDesc.SourceAccelerationStructureKey,
                                                       c.m_pDesc.SourceAccelerationStructureOffset);
  }
  c.m_pDesc.Value->ScratchAccelerationStructureData =
      m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pDesc.ScratchAccelerationStructureKey,
                                                     c.m_pDesc.ScratchAccelerationStructureOffset);

  unsigned inputIndex = 0;
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (c.m_pDesc.Value->Inputs.NumDescs == 0) {
      c.m_pDesc.Value->Inputs.InstanceDescs = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pDesc.InputKeys[0], c.m_pDesc.InputOffsets[0]);
    } else {
      // c.m_pDesc.Value->Inputs.InstanceDescs set in GpuPatchLayer
    }
    ++inputIndex;
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
              : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.Triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.Triangles.VertexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.AABBs.AABBs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles =
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles);
          triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          triangles.VertexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          auto& ommLinkage = *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
              desc.OmmTriangles.pOmmLinkage);
          ommLinkage.OpacityMicromapIndexBuffer.StartAddress =
              m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pDesc.InputKeys[inputIndex],
                                                             c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
          ommLinkage.OpacityMicromapArray = m_Manager.GetGpuAddressService().GetGpuAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
          ++inputIndex;
        }
      }
    }
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc) {
      auto& ommDesc = *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc);
      ommDesc.InputBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
      ++inputIndex;
      ommDesc.PerOmmDescs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]);
      ++inputIndex;
    }
  }

  for (unsigned i = 0; i < c.m_NumPostbuildInfoDescs.Value; ++i) {
    c.m_pPostbuildInfoDescs.Value[i].DestBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pPostbuildInfoDescs.DestBufferKeys[i], c.m_pPostbuildInfoDescs.DestBufferOffsets[i]);
  }
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  c.m_DestAccelerationStructureData.Value = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_DestAccelerationStructureData.InterfaceKey, c.m_DestAccelerationStructureData.Offset);
  c.m_SourceAccelerationStructureData.Value = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_SourceAccelerationStructureData.InterfaceKey, c.m_SourceAccelerationStructureData.Offset);
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  c.m_pDesc.Value->DestBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_pDesc.destBufferKey, c.m_pDesc.destBufferOffset);
  for (unsigned i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    c.m_pSourceAccelerationStructureData.Value[i] = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pSourceAccelerationStructureData.InterfaceKeys[i],
        c.m_pSourceAccelerationStructureData.Offsets[i]);
  }
}

void ReplayCustomizationLayer::Pre(D3D12CreateRootSignatureDeserializerCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12DeviceOpenSharedHandleCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3DBlobGetBufferPointerCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3DBlobGetBufferSizeCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(ID3D12SDKConfigurationSetSDKVersionCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(
    ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  if (c.m_pDesc.Value) {
    for (unsigned i = 0; i < c.m_DescCount.Value; ++i) {
      if (c.m_pDesc.Value[i].DataDesc.DestVA) {
        c.m_pDesc.Value[i].DataDesc.DestVA = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.DestKey[i], c.m_pDesc.DestOffset[i]);
      }
      if (c.m_pDesc.Value[i].DataDesc.SrcVA) {
        c.m_pDesc.Value[i].DataDesc.SrcVA = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pDesc.SourceKey[i], c.m_pDesc.SourceOffset[i]);
      }
    }
  }
}

void ReplayCustomizationLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (Configurator::Get().directx.player.applicationInfoOverride.enabled) {
    c.Skip = true;
  } else {
    // Print application info (may affect driver behavior on playback)
    LOG_INFO << "INTC_D3D12_SetApplicationInfo - " << appInfoToStr(c.m_pExtensionAppInfo.Value);
  }
}

void ReplayCustomizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (c.Skip || !c.m_pExtensionAppInfo.Value) {
    return;
  }

  if (Configurator::Get().directx.player.applicationInfoOverride.enabled) {
    // Override with application info from config
    const auto& appInfo = m_Manager.GetIntelExtensionsService().GetAppInfo();
    c.m_pExtensionAppInfo.Value->pApplicationName = appInfo.pApplicationName;
    c.m_pExtensionAppInfo.Value->ApplicationVersion = appInfo.ApplicationVersion.major;
    c.m_pExtensionAppInfo.Value->pEngineName = appInfo.pEngineName;
    c.m_pExtensionAppInfo.Value->EngineVersion = appInfo.EngineVersion.major;
  }
  // Print application info (may affect driver behavior on playback)
  auto* appInfo = c.m_pExtensionAppInfo.Value;
  std::wstring appName = appInfo->pApplicationName ? appInfo->pApplicationName : L"";
  std::wstring engineName = appInfo->pEngineName ? appInfo->pEngineName : L"";
  LOG_INFO << "INTC_D3D12_CreateDeviceExtensionContext - Application: \"" << appName << "\" ("
           << appInfo->ApplicationVersion << "), Engine: \"" << engineName << "\" ("
           << appInfo->EngineVersion << ")";
}

void ReplayCustomizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (c.Skip || !c.m_pExtensionAppInfo.Value) {
    return;
  }

  if (Configurator::Get().directx.player.applicationInfoOverride.enabled) {
    // Override with application info from config
    const auto& appInfo = m_Manager.GetIntelExtensionsService().GetAppInfo();
    c.m_pExtensionAppInfo.Value->pApplicationName = appInfo.pApplicationName;
    c.m_pExtensionAppInfo.Value->ApplicationVersion = appInfo.ApplicationVersion;
    c.m_pExtensionAppInfo.Value->pEngineName = appInfo.pEngineName;
    c.m_pExtensionAppInfo.Value->EngineVersion = appInfo.EngineVersion;
  }
  // Print application info (may affect driver behavior on playback)
  LOG_INFO << "INTC_D3D12_CreateDeviceExtensionContext1 - "
           << appInfoToStr(c.m_pExtensionAppInfo.Value);
}

void ReplayCustomizationLayer::Pre(INTC_D3D12_GetSupportedVersionsCommand& c) {
  c.Skip = true;
}

void ReplayCustomizationLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (c.Skip || c.m_Result.Value != NVAPI_OK || !m_Manager.MultithreadedShaderCompilation()) {
    return;
  }
  m_Manager.FlushMultithreadedShaderCompilation();
}

void ReplayCustomizationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (c.Skip || c.m_Result.Value != NVAPI_OK || !m_Manager.MultithreadedShaderCompilation()) {
    return;
  }

  if (c.m_uavSlot.Value == ~0u) {
    c.Skip = true;
  }
}

void ReplayCustomizationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (c.Skip || c.m_Result.Value != NVAPI_OK || !m_Manager.MultithreadedShaderCompilation()) {
    return;
  }

  c.Skip = true;

  if (c.m_uavSlot.Value == ~0u) {
    return;
  }

  NvAPIShaderExtnSlot slot{c.m_pDev.Key, c.m_uavSlot.Value, c.m_uavSpace.Value};
  if (std::find(m_NvapiShaderExtnSlotsUsed.begin(), m_NvapiShaderExtnSlotsUsed.end(), slot) ==
      m_NvapiShaderExtnSlotsUsed.end()) {
    m_NvapiShaderExtnSlotsUsed.push_back(slot);
    NvAPI_D3D12_SetNvShaderExtnSlotSpace(c.m_pDev.Value, c.m_uavSlot.Value, c.m_uavSpace.Value);
  }
}

void ReplayCustomizationLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  auto* pDescMod = const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(
      c.m_pParams.Value->pDesc);
  pDescMod->destAccelerationStructureData = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_pParams.DestAccelerationStructureKey, c.m_pParams.DestAccelerationStructureOffset);
  if (pDescMod->sourceAccelerationStructureData) {
    pDescMod->sourceAccelerationStructureData = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.SourceAccelerationStructureKey, c.m_pParams.SourceAccelerationStructureOffset);
  }
  pDescMod->scratchAccelerationStructureData = m_Manager.GetGpuAddressService().GetGpuAddress(
      c.m_pParams.ScratchAccelerationStructureKey, c.m_pParams.ScratchAccelerationStructureOffset);

  unsigned inputIndex = 0;
  if (pDescMod->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (pDescMod->inputs.numDescs == 0) {
      pDescMod->inputs.instanceDescs = m_Manager.GetGpuAddressService().GetGpuAddress(
          c.m_pParams.InputKeys[0], c.m_pParams.InputOffsets[0]);
    } else {
      LOG_ERROR << "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx TLAS build not handled!";
    }
    ++inputIndex;
  } else {
    for (unsigned i = 0; i < pDescMod->inputs.numDescs; ++i) {
      NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
          const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX&>(
              pDescMod->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                  ? *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(pDescMod->inputs
                                                                            .pGeometryDescs) +
                                                                pDescMod->inputs
                                                                        .geometryDescStrideInBytes *
                                                                    i)
                  : *pDescMod->inputs.ppGeometryDescs[i]);
      if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.triangles.VertexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.aabbs.AABBs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        desc.ommTriangles.triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.ommTriangles.triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.ommTriangles.triangles.VertexBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.ommTriangles.ommAttachment.opacityMicromapArray =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        desc.dmmTriangles.triangles.Transform3x4 = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.triangles.IndexBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.triangles.VertexBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.dmmTriangles.dmmAttachment.displacementMicromapArray =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_SPHERES_EX) {
        desc.spheres.vertexPositionBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.spheres.vertexRadiusBuffer.StartAddress =
            m_Manager.GetGpuAddressService().GetGpuAddress(c.m_pParams.InputKeys[inputIndex],
                                                           c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.spheres.indexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        desc.lss.vertexPositionBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.lss.vertexRadiusBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
        desc.lss.indexBuffer.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
            c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]);
        ++inputIndex;
      }
    }
  }

  for (unsigned i = 0; i < c.m_pParams.Value->numPostbuildInfoDescs; ++i) {
    const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(
        c.m_pParams.Value->pPostbuildInfoDescs)[i]
        .DestBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.DestPostBuildBufferKeys[i], c.m_pParams.DestPostBuildBufferOffsets[i]);
  }
}

void ReplayCustomizationLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  if (c.m_pParams.Value->pDesc) {
    auto* pDescMod = const_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
        c.m_pParams.Value->pDesc);

    pDescMod->destOpacityMicromapArrayData = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.DestOpacityMicromapArrayDataKey,
        c.m_pParams.DestOpacityMicromapArrayDataOffset);

    pDescMod->inputs.inputBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.InputBufferKey, c.m_pParams.InputBufferOffset);

    pDescMod->inputs.perOMMDescs.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.PerOMMDescsKey, c.m_pParams.PerOMMDescsOffset);

    pDescMod->scratchOpacityMicromapArrayData = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.ScratchOpacityMicromapArrayDataKey,
        c.m_pParams.ScratchOpacityMicromapArrayDataOffset);
  }

  for (unsigned i = 0; i < c.m_pParams.Value->numPostbuildInfoDescs; ++i) {
    const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC*>(
        c.m_pParams.Value->pPostbuildInfoDescs)[i]
        .destBuffer = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.DestPostBuildBufferKeys[i], c.m_pParams.DestPostBuildBufferOffsets[i]);
  }
}

void ReplayCustomizationLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }
  if (c.m_pParams.Value->pDesc) {
    auto* pDescMod = const_cast<NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC*>(
        c.m_pParams.Value->pDesc);

    pDescMod->batchResultData = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.BatchResultDataKey, c.m_pParams.BatchResultDataOffset);

    pDescMod->batchScratchData = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.BatchScratchDataKey, c.m_pParams.BatchScratchDataOffset);

    pDescMod->destinationAddressArray.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.DestinationAddressArrayKey, c.m_pParams.DestinationAddressArrayOffset);

    pDescMod->resultSizeArray.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.ResultSizeArrayKey, c.m_pParams.ResultSizeArrayOffset);

    pDescMod->indirectArgArray.StartAddress = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.IndirectArgArrayKey, c.m_pParams.IndirectArgArrayOffset);

    pDescMod->indirectArgCount = m_Manager.GetGpuAddressService().GetGpuAddress(
        c.m_pParams.IndirectArgCountKey, c.m_pParams.IndirectArgCountOffset);
  }

  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand not handled!";
    logged = true;
  }

  c.Skip = true;
}

void ReplayCustomizationLayer::Post(CreateDXGIFactory2Command& c) {
  if (c.Skip || c.m_Result.Value == S_OK) {
    return;
  }

  LOG_ERROR << "CreateDXGIFactory2 failed. Playback will fail.";
  if (c.m_Flags.Value & DXGI_CREATE_FACTORY_DEBUG) {
    LOG_ERROR
        << "CreateDXGIFactory2 with DXGI_CREATE_FACTORY_DEBUG flag failed."
        << "Make sure that the `Graphics Tools` feature is installed."
        << "On Windows 11 go to Settings->System->Optional features and enable `Graphics Tools`";
  }
}

void ReplayCustomizationLayer::Pre(xefgSwapChainD3D12InitFromSwapChainDescCommand& command) {
  if (command.Skip) {
    return;
  }
  command.m_hWnd.Value = m_Manager.GetWindowService().GetCurrentHwnd(command.m_hWnd.Value);
  if (command.m_pFullscreenDesc.Value && Configurator::Get().common.player.showWindowBorder) {
    command.m_pFullscreenDesc.Value->Windowed = true;
    LOG_INFO << "CreateSwapChainForHwnd: Force windowed mode due to 'showWindowBorder'";
  }
}

void ReplayCustomizationLayer::Pre(xefgSwapChainSetLoggingCallbackCommand& command) {
  command.m_loggingCallback.Value = nullptr;

  static bool logged = false;
  if (!logged) {

    LOG_INFO << "[XeSS-FG] xefgSwapChainSetLoggingCallback: Set the callback function pointer to "
                "nullptr";
    logged = true;
  }
}

void ReplayCustomizationLayer::Pre(xessSetLoggingCallbackCommand& command) {
  command.m_loggingCallback.Value = nullptr;

  static bool logged = false;
  if (!logged) {

    LOG_INFO << "[XeSS] xessSetLoggingCallback: Set the callback function pointer to "
                "nullptr";
    logged = true;
  }
}

void ReplayCustomizationLayer::Pre(xellSetLoggingCallbackCommand& command) {
  command.m_loggingCallback.Value = nullptr;

  static bool logged = false;
  if (!logged) {

    LOG_INFO << "[XeLL] xellSetLoggingCallback: Set the callback function pointer to "
                "nullptr";
    logged = true;
  }
}

void ReplayCustomizationLayer::FillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  if (m_UseAddressPinning) {
    return;
  }

  if (arg.Value) {
    arg.Value = m_Manager.GetGpuAddressService().GetGpuAddress(arg.InterfaceKey, arg.Offset);
  }
}

void ReplayCustomizationLayer::FillGpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg) {

  arg.Value.ptr = m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
      arg.InterfaceKey, ReplayDescriptorHandleService::HandleType::GpuHandle, arg.Index);
}

void ReplayCustomizationLayer::FillCpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {

  arg.Value.ptr = m_Manager.GetDescriptorHandleService().GetDescriptorHandle(
      arg.InterfaceKey, ReplayDescriptorHandleService::HandleType::CpuHandle, arg.Index);
}

void ReplayCustomizationLayer::WaitForFence(unsigned commandKey,
                                            ID3D12Fence* fence,
                                            unsigned fenceValue) {
  UINT64 value = fence->GetCompletedValue();
  if (value >= fenceValue) {
    return;
  }
  if (!m_WaitForFenceEvent) {
    m_WaitForFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    GITS_ASSERT(m_WaitForFenceEvent);
  }
  HRESULT hr = fence->SetEventOnCompletion(fenceValue, m_WaitForFenceEvent);
  GITS_ASSERT(hr == S_OK);
  DWORD timeout = 60000; // 60 sec
  if (Configurator::Get().directx.player.infiniteWaitForFence) {
    timeout = INFINITE;
  }
  DWORD ret = WaitForSingleObject(m_WaitForFenceEvent, timeout);
  if (ret == WAIT_TIMEOUT) {
    value = fence->GetCompletedValue();
    LOG_ERROR << "GetCompletedValue - timeout while waiting for fence value " << fenceValue
              << ". Current value " << value << ". Command " << commandKey;
  }
}

void ReplayCustomizationLayer::RemoveCachedPso(D3D12_PIPELINE_STATE_STREAM_DESC& desc) {
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

} // namespace DirectX
} // namespace gits
