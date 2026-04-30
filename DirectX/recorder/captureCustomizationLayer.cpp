// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayer.h"
#include "captureManager.h"
#include "commandsCustom.h"
#include "commandSerializersCustom.h"
#include "commandSerializersAuto.h"
#include "iunknownWrapper.h"
#include "wrapperUtils.h"
#include "log.h"

#include <windows.h>

namespace gits {
namespace DirectX {

thread_local CaptureCustomizationLayer::HeapInfo CaptureCustomizationLayer::m_HeapInfo;

CaptureCustomizationLayer::CaptureCustomizationLayer(CaptureManager& manager,
                                                     stream::OrderingRecorder& recorder)
    : Layer("CaptureCustomization"), m_Manager(manager), m_Recorder(recorder) {}

void CaptureCustomizationLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    m_Manager.getDescriptorHandleService().destroyDescriptorHeap(c.m_Object.Key);
    m_Manager.getMapTrackingService().destroyResource(c.m_Object.Key);
    m_Manager.getGpuAddressService().destroyInterface(c.m_Object.Key);
    m_Manager.getFenceService().destroyFence(c.m_Object.Key);
  }
}

void CaptureCustomizationLayer::Pre(IDXGIFactoryCreateSwapChainCommand& c) {
  RECT rect;
  BOOL ret = GetClientRect(c.m_pDesc.Value->OutputWindow, &rect);
  GITS_ASSERT(ret);

  CreateWindowMetaCommand createWindowCommand(c.ThreadId);
  createWindowCommand.Key = m_Manager.createCommandKey();
  createWindowCommand.m_hWnd.Value = c.m_pDesc.Value->OutputWindow;
  createWindowCommand.m_width.Value = rect.right - rect.left;
  createWindowCommand.m_height.Value = rect.bottom - rect.top;
  createWindowCommand.m_width.Value = std::max(
      static_cast<unsigned>(createWindowCommand.m_width.Value), c.m_pDesc.Value->BufferDesc.Width);
  createWindowCommand.m_height.Value =
      std::max(static_cast<unsigned>(createWindowCommand.m_height.Value),
               c.m_pDesc.Value->BufferDesc.Height);

  m_Recorder.Record(createWindowCommand.Key, new CreateWindowMetaSerializer(createWindowCommand));
}

void CaptureCustomizationLayer::Pre(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  RECT rect;
  BOOL ret = GetClientRect(c.m_hWnd.Value, &rect);
  GITS_ASSERT(ret);

  CreateWindowMetaCommand createWindowCommand(c.ThreadId);
  createWindowCommand.Key = m_Manager.createCommandKey();
  createWindowCommand.m_hWnd.Value = c.m_hWnd.Value;
  createWindowCommand.m_width.Value = rect.right - rect.left;
  createWindowCommand.m_height.Value = rect.bottom - rect.top;
  createWindowCommand.m_width.Value =
      std::max(static_cast<unsigned>(createWindowCommand.m_width.Value), c.m_pDesc.Value->Width);
  createWindowCommand.m_height.Value =
      std::max(static_cast<unsigned>(createWindowCommand.m_height.Value), c.m_pDesc.Value->Height);

  m_Recorder.Record(createWindowCommand.Key, new CreateWindowMetaSerializer(createWindowCommand));
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ID3D12DescriptorHeap* descriptorHeap = static_cast<ID3D12DescriptorHeap*>(*c.m_ppvHeap.Value);
  m_Manager.getDescriptorHandleService().createDescriptorHeap(c.m_ppvHeap.Key, descriptorHeap,
                                                              c.m_pDescriptorHeapDesc.Value);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateCommittedResourceCommand& c) {
  m_HeapInfo = HeapInfo(c.m_pHeapProperties.Value, c.m_HeapFlags.Value);
  c.m_pHeapProperties.Value = &m_HeapInfo.Properties;
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(*c.m_pHeapProperties.Value,
                                                     c.m_HeapFlags.Value);
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pHeapProperties.Value = m_HeapInfo.PropertiesPtr;
  c.m_HeapFlags.Value = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Pre(ID3D12Device4CreateCommittedResource1Command& c) {
  m_HeapInfo = HeapInfo(c.m_pHeapProperties.Value, c.m_HeapFlags.Value);
  c.m_pHeapProperties.Value = &m_HeapInfo.Properties;
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(*c.m_pHeapProperties.Value,
                                                     c.m_HeapFlags.Value);
}

void CaptureCustomizationLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pHeapProperties.Value = m_HeapInfo.PropertiesPtr;
  c.m_HeapFlags.Value = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Pre(ID3D12Device8CreateCommittedResource2Command& c) {
  m_HeapInfo = HeapInfo(c.m_pHeapProperties.Value, c.m_HeapFlags.Value);
  c.m_pHeapProperties.Value = &m_HeapInfo.Properties;
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(*c.m_pHeapProperties.Value,
                                                     c.m_HeapFlags.Value);
}

void CaptureCustomizationLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pHeapProperties.Value = m_HeapInfo.PropertiesPtr;
  c.m_HeapFlags.Value = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Pre(ID3D12Device10CreateCommittedResource3Command& c) {
  m_HeapInfo = HeapInfo(c.m_pHeapProperties.Value, c.m_HeapFlags.Value);
  c.m_pHeapProperties.Value = &m_HeapInfo.Properties;
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(*c.m_pHeapProperties.Value,
                                                     c.m_HeapFlags.Value);
}

void CaptureCustomizationLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pHeapProperties.Value = m_HeapInfo.PropertiesPtr;
  c.m_HeapFlags.Value = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
}

void CaptureCustomizationLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
}

void CaptureCustomizationLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createResource(c.m_ppvResource.Key, resource);
  }
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createPlacedResource(
        c.m_ppvResource.Key, resource, c.m_pHeap.Key, c.m_pHeap.Value, c.m_HeapOffset.Value,
        c.m_InitialState.Value == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
  }
}

void CaptureCustomizationLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createPlacedResource(
        c.m_ppvResource.Key, resource, c.m_pHeap.Key, c.m_pHeap.Value, c.m_HeapOffset.Value,
        c.m_InitialState.Value == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
  }
}

void CaptureCustomizationLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
    m_Manager.getGpuAddressService().createPlacedResource(
        c.m_ppvResource.Key, resource, c.m_pHeap.Key, c.m_pHeap.Value, c.m_HeapOffset.Value, false);
  }
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateHeapCommand& c) {
  m_HeapInfo = HeapInfo(c.m_pDesc.Value->Properties, c.m_pDesc.Value->Flags);
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(c.m_pDesc.Value->Properties,
                                                     c.m_pDesc.Value->Flags);
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
    m_Manager.getGpuAddressService().createHeap(c.m_ppvHeap.Key, heap);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pDesc.Value->Properties = m_HeapInfo.Properties;
  c.m_pDesc.Value->Flags = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Pre(ID3D12Device4CreateHeap1Command& c) {
  m_HeapInfo = HeapInfo(c.m_pDesc.Value->Properties, c.m_pDesc.Value->Flags);
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(c.m_pDesc.Value->Properties,
                                                     c.m_pDesc.Value->Flags);
}

void CaptureCustomizationLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
    m_Manager.getGpuAddressService().createHeap(c.m_ppvHeap.Key, heap);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pDesc.Value->Properties = m_HeapInfo.Properties;
  c.m_pDesc.Value->Flags = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Pre(INTC_D3D12_CreateHeapCommand& c) {
  auto* desc = c.m_pDesc.Value->pD3D12Desc;
  m_HeapInfo = HeapInfo(desc->Properties, desc->Flags);
  // Modify D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS to enable writewatch
  m_Manager.getMapTrackingService().enableWriteWatch(desc->Properties, desc->Flags);
}

void CaptureCustomizationLayer::Post(INTC_D3D12_CreateHeapCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
    m_Manager.getGpuAddressService().createHeap(c.m_ppvHeap.Key, heap);
  }
  // Restore D3D12_HEAP_PROPERTIES and D3D12_HEAP_FLAGS
  c.m_pDesc.Value->pD3D12Desc->Properties = m_HeapInfo.Properties;
  c.m_pDesc.Value->pD3D12Desc->Flags = m_HeapInfo.Flags;
}

void CaptureCustomizationLayer::Post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
    m_Manager.getGpuAddressService().createHeap(c.m_ppvHeap.Key, heap);

    D3D12_HEAP_DESC desc = heap->GetDesc();

    CreateHeapAllocationMetaCommand command(c.ThreadId);
    command.Key = m_Manager.createCommandKey();
    command.m_heap.Key = c.m_ppvHeap.Key;
    command.m_address.Value = const_cast<void*>(c.m_pAddress.Value);
    command.m_data.Value = const_cast<void*>(c.m_pAddress.Value);
    command.m_data.Size = desc.SizeInBytes;
    m_Recorder.Record(command.Key, new CreateHeapAllocationMetaSerializer(command));

    m_Manager.updateCommandKey(c);
  }
}

void CaptureCustomizationLayer::Post(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  if (c.m_Result.Value == S_OK) {
    ID3D12Heap* heap = static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value);
    m_Manager.getGpuAddressService().createHeap(c.m_ppvHeap.Key, heap);

    D3D12_HEAP_DESC desc = heap->GetDesc();

    CreateHeapAllocationMetaCommand command(c.ThreadId);
    command.Key = m_Manager.createCommandKey();
    command.m_heap.Key = c.m_ppvHeap.Key;
    command.m_address.Value = const_cast<void*>(c.m_pAddress.Value);
    command.m_data.Value = const_cast<void*>(c.m_pAddress.Value);
    command.m_data.Size = desc.SizeInBytes;
    m_Recorder.Record(command.Key, new CreateHeapAllocationMetaSerializer(command));

    m_Manager.updateCommandKey(c);
  }
}

void CaptureCustomizationLayer::Post(ID3D12ResourceMapCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_Manager.getMapTrackingService().mapResource(c.m_Object.Key, c.m_Object.Value,
                                                c.m_Subresource.Value, c.m_ppData.Value);
}

void CaptureCustomizationLayer::Pre(ID3D12ResourceUnmapCommand& c) {
  m_Manager.getMapTrackingService().unmapResource(c.m_Object.Key, c.m_Subresource.Value);
}

void CaptureCustomizationLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  m_Manager.getMapTrackingService().executeCommandLists();
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  if (c.m_pDesc.Value &&
      c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->RaytracingAccelerationStructure.Location, true);
    c.m_pDesc.RaytracingLocationKey = info.ResourceKey;
    c.m_pDesc.RaytracingLocationOffset = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CaptureCustomizationLayer::Pre(
    ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateSamplerCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void CaptureCustomizationLayer::Pre(ID3D12Device11CreateSampler2Command& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  D3D12_DESCRIPTOR_HEAP_TYPE heapType =
      m_Manager.getRootSignatureService().getGraphicsRootSignatureDescriptorHeapType(
          c.m_Object.Key, c.m_RootParameterIndex.Value);
  fillGpuDescriptorHandleArgument(c.m_BaseDescriptor, heapType);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  D3D12_DESCRIPTOR_HEAP_TYPE heapType =
      m_Manager.getRootSignatureService().getComputeRootSignatureDescriptorHeapType(
          c.m_Object.Key, c.m_RootParameterIndex.Value);
  fillGpuDescriptorHandleArgument(c.m_BaseDescriptor, heapType);
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  if (c.m_pRenderTargetDescriptors.Value) {
    for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
      CaptureDescriptorHandleService::HandleInfo info =
          m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
              D3D12_DESCRIPTOR_HEAP_TYPE_RTV, CaptureDescriptorHandleService::HandleType::CpuHandle,
              c.m_pRenderTargetDescriptors.Value[i].ptr);
      c.m_pRenderTargetDescriptors.InterfaceKeys[i] = info.InterfaceKey;
      c.m_pRenderTargetDescriptors.Indexes[i] = info.Index;
      if (c.m_RTsSingleHandleToDescriptorRange.Value) {
        break;
      }
    }
  }
  if (c.m_pDepthStencilDescriptor.Value) {
    CaptureDescriptorHandleService::HandleInfo info =
        m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV, CaptureDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDepthStencilDescriptor.Value[0].ptr);
    c.m_pDepthStencilDescriptor.InterfaceKeys[0] = info.InterfaceKey;
    c.m_pDepthStencilDescriptor.Indexes[0] = info.Index;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DepthStencilView, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_RenderTargetView, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  fillGpuDescriptorHandleArgument(c.m_ViewGPUHandleInCurrentHeap,
                                  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  fillCpuDescriptorHandleArgument(c.m_ViewCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  fillGpuDescriptorHandleArgument(c.m_ViewGPUHandleInCurrentHeap,
                                  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  fillCpuDescriptorHandleArgument(c.m_ViewCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCopyDescriptorsCommand& c) {

  for (unsigned i = 0; i < c.m_NumDestDescriptorRanges.Value; ++i) {
    CaptureDescriptorHandleService::HandleInfo info =
        m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
            c.m_DescriptorHeapsType.Value, CaptureDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDestDescriptorRangeStarts.Value[i].ptr);

    c.m_pDestDescriptorRangeStarts.InterfaceKeys[i] = info.InterfaceKey;
    c.m_pDestDescriptorRangeStarts.Indexes[i] = info.Index;
  }
  for (unsigned i = 0; i < c.m_NumSrcDescriptorRanges.Value; ++i) {
    CaptureDescriptorHandleService::HandleInfo info =
        m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
            c.m_DescriptorHeapsType.Value, CaptureDescriptorHandleService::HandleType::CpuHandle,
            c.m_pSrcDescriptorRangeStarts.Value[i].ptr);

    c.m_pSrcDescriptorRangeStarts.InterfaceKeys[i] = info.InterfaceKey;
    c.m_pSrcDescriptorRangeStarts.Indexes[i] = info.Index;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptorRangeStart, c.m_DescriptorHeapsType.Value);
  fillCpuDescriptorHandleArgument(c.m_SrcDescriptorRangeStart, c.m_DescriptorHeapsType.Value);
}

void CaptureCustomizationLayer::Post(D3D12SerializeRootSignatureCommand& c) {
  m_Manager.getRootSignatureService().serializeRootSignature(c.m_pRootSignature.Value,
                                                             c.m_ppBlob.Key);
}

void CaptureCustomizationLayer::Post(D3D12SerializeVersionedRootSignatureCommand& c) {
  m_Manager.getRootSignatureService().serializeVersionedRootSignature(c.m_pRootSignature.Value,
                                                                      c.m_ppBlob.Key);
}

void CaptureCustomizationLayer::Post(ID3DBlobGetBufferPointerCommand& c) {
  m_Manager.getRootSignatureService().setBlobBufferPointer(c.m_Object.Key, c.m_Result.Value);
}

void CaptureCustomizationLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.m_Result.Value == S_OK) {
    m_Manager.getRootSignatureService().createRootSignature(
        c.m_pBlobWithRootSignature.Value, c.m_blobLengthInBytes.Value, c.m_ppvRootSignature.Key);
  }
}

void CaptureCustomizationLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  // pRootSignature is optional, so it can be nullptr
  if (!c.m_pRootSignature.Value) {
    return;
  }
  m_Manager.getRootSignatureService().setGraphicsRootSignature(c.m_Object.Key,
                                                               c.m_pRootSignature.Key);
}

void CaptureCustomizationLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  // pRootSignature is optional, so it can be nullptr
  if (!c.m_pRootSignature.Value) {
    return;
  }
  m_Manager.getRootSignatureService().setComputeRootSignature(c.m_Object.Key,
                                                              c.m_pRootSignature.Key);
}

void CaptureCustomizationLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  m_Manager.getRootSignatureService().resetRootSignatures(c.m_Object.Key);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  fillGpuAddressArgument(c.m_BufferLocation);
}

void CaptureCustomizationLayer::Pre(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  fillCpuDescriptorHandleArgument(c.m_DestDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  if (c.m_pDesc.Value && c.m_pDesc.Value->BufferLocation) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pDesc.Value->BufferLocation);
    c.m_pDesc.BufferLocationKey = info.ResourceKey;
    c.m_pDesc.BufferLocationOffset = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.m_pView.Value && c.m_pView.Value->BufferLocation) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pView.Value->BufferLocation);
    c.m_pView.BufferLocationKey = info.ResourceKey;
    c.m_pView.BufferLocationOffset = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].BufferLocation) {
        GpuAddressService::GpuAddressInfo info =
            m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pViews.Value[i].BufferLocation);
        c.m_pViews.BufferLocationKeys[i] = info.ResourceKey;
        c.m_pViews.BufferLocationOffsets[i] = info.Offset;
      }
    }
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].BufferLocation) {
        GpuAddressService::GpuAddressInfo info =
            m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pViews.Value[i].BufferLocation);
        c.m_pViews.BufferLocationKeys[i] = info.ResourceKey;
        c.m_pViews.BufferLocationOffsets[i] = info.Offset;
      }
      if (c.m_pViews.Value[i].BufferFilledSizeLocation) {
        GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
            c.m_pViews.Value[i].BufferFilledSizeLocation);
        c.m_pViews.BufferFilledSizeLocationKeys[i] = info.ResourceKey;
        c.m_pViews.BufferFilledSizeLocationOffsets[i] = info.Offset;
      }
    }
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.m_pParams.Value) {
    for (unsigned i = 0; i < c.m_Count.Value; ++i) {
      if (c.m_pParams.Value[i].Dest) {
        GpuAddressService::GpuAddressInfo info =
            m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pParams.Value[i].Dest);
        c.m_pParams.DestKeys[i] = info.ResourceKey;
        c.m_pParams.DestOffsets[i] = info.Offset;
      }
    }
  }
}

void CaptureCustomizationLayer::Post(ID3D12FenceGetCompletedValueCommand& c) {
  static constexpr unsigned maxCommandKeyDifference = 10;

  static thread_local unsigned prevCommandKey = 0;
  static thread_local unsigned prevFenceKey = 0;
  static thread_local unsigned prevValue = 0;
  unsigned commandKey = c.Key;
  unsigned fenceKey = c.m_Object.Key;
  unsigned value = c.m_Result.Value;
  if (commandKey <= prevCommandKey + maxCommandKeyDifference) {
    if (fenceKey == prevFenceKey && value == prevValue) {
      m_Recorder.Skip(c.Key);
    }
  }
  prevCommandKey = commandKey;
  prevFenceKey = fenceKey;
  prevValue = value;
}

void CaptureCustomizationLayer::Pre(ID3D12FenceSetEventOnCompletionCommand& c) {
  m_Manager.getFenceService().setEventOnCompletion(c.m_Object.Value, c.m_Object.Key,
                                                   c.m_Value.Value, c.m_hEvent.Value);
}

void CaptureCustomizationLayer::Pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& c) {
  static bool printed = false;
  if (!printed) {
    LOG_ERROR << "ID3D12Device1::SetEventOnMultipleFenceCompletion not handled!";
    printed = true;
  }
}

void CaptureCustomizationLayer::Post(ID3D12DeviceOpenSharedHandleCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (c.m_riid.Value == IID_ID3D12Resource || c.m_riid.Value == IID_ID3D12Resource1 ||
      c.m_riid.Value == IID_ID3D12Resource2) {
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(*c.m_ppvObj.Value);
    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    D3D12_HEAP_PROPERTIES heapProperties{};
    D3D12_HEAP_FLAGS heapFlags{};
    HRESULT hr = resource->GetHeapProperties(&heapProperties, &heapFlags);
    GITS_ASSERT(hr == S_OK);

    ID3D12DeviceCreateCommittedResourceCommand createResource;
    createResource.Key = m_Manager.createCommandKey();
    createResource.ThreadId = c.ThreadId;
    createResource.m_Object.Key = c.m_Object.Key;
    createResource.m_pHeapProperties.Value = &heapProperties;
    createResource.m_HeapFlags.Value = heapFlags;
    createResource.m_pDesc.Value = &desc;
    createResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_COMMON;
    createResource.m_pOptimizedClearValue.Value = nullptr;
    createResource.m_riidResource.Value = c.m_riid.Value;
    createResource.m_ppvResource.Key = c.m_ppvObj.Key;
    m_Recorder.Record(createResource.Key,
                      new ID3D12DeviceCreateCommittedResourceSerializer(createResource));

    m_Manager.updateCommandKey(c);

  } else if (c.m_riid.Value == IID_ID3D12Fence || c.m_riid.Value == IID_ID3D12Fence1) {
    ID3D12Fence* fence = static_cast<ID3D12Fence*>(*c.m_ppvObj.Value);
    UINT64 fenceValue = fence->GetCompletedValue();

    ID3D12DeviceCreateFenceCommand createFence;
    createFence.Key = m_Manager.createCommandKey();
    createFence.ThreadId = c.ThreadId;
    createFence.m_Object.Key = c.m_Object.Key;
    createFence.m_InitialValue.Value = fenceValue;
    createFence.m_Flags.Value = D3D12_FENCE_FLAG_SHARED;
    createFence.m_riid.Value = c.m_riid.Value;
    createFence.m_ppFence.Key = c.m_ppvObj.Key;
    m_Recorder.Record(createFence.Key, new ID3D12DeviceCreateFenceSerializer(createFence));

    m_Manager.updateCommandKey(c);
  } else {
    LOG_ERROR
        << "ID3D12Device::OpenSharedHandle handled only for ID3D12Resource/1/2 and ID3D12Fence/1.";
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  c.m_pRenderTargets.DescriptorKeys.resize(c.m_NumRenderTargets.Value);
  c.m_pRenderTargets.DescriptorIndexes.resize(c.m_NumRenderTargets.Value);
  for (unsigned i = 0; i < c.m_NumRenderTargets.Value; ++i) {
    if (c.m_pRenderTargets.Value && c.m_pRenderTargets.Value[i].cpuDescriptor.ptr) {
      CaptureDescriptorHandleService::HandleInfo info =
          m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
              D3D12_DESCRIPTOR_HEAP_TYPE_RTV, CaptureDescriptorHandleService::HandleType::CpuHandle,
              c.m_pRenderTargets.Value[i].cpuDescriptor.ptr);

      c.m_pRenderTargets.DescriptorKeys[i] = info.InterfaceKey;
      c.m_pRenderTargets.DescriptorIndexes[i] = info.Index;
    }
  }

  if (c.m_pDepthStencil.Value && c.m_pDepthStencil.Value->cpuDescriptor.ptr) {
    CaptureDescriptorHandleService::HandleInfo info =
        m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV, CaptureDescriptorHandleService::HandleType::CpuHandle,
            c.m_pDepthStencil.Value->cpuDescriptor.ptr);

    c.m_pDepthStencil.DescriptorKey = info.InterfaceKey;
    c.m_pDepthStencil.DescriptorIndex = info.Index;
  }
}

void CaptureCustomizationLayer::Pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  auto resolveInputsGpuAddress = [&](D3D12_GPU_VIRTUAL_ADDRESS address) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(address);
    c.m_pDesc.InputKeys.push_back(info.ResourceKey);
    c.m_pDesc.InputOffsets.push_back(info.Offset);
  };

  if (c.m_pDesc.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    resolveInputsGpuAddress(c.m_pDesc.Value->InstanceDescs);
  } else if (c.m_pDesc.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->NumDescs; ++i) {
      const D3D12_RAYTRACING_GEOMETRY_DESC& desc =
          c.m_pDesc.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->pGeometryDescs[i]
              : *c.m_pDesc.Value->ppGeometryDescs[i];
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        resolveInputsGpuAddress(desc.Triangles.Transform3x4);
        resolveInputsGpuAddress(desc.Triangles.IndexBuffer);
        resolveInputsGpuAddress(desc.Triangles.VertexBuffer.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        resolveInputsGpuAddress(desc.AABBs.AABBs.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->Transform3x4);
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->IndexBuffer);
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->VertexBuffer.StartAddress);
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          resolveInputsGpuAddress(
              desc.OmmTriangles.pOmmLinkage->OpacityMicromapIndexBuffer.StartAddress);
          resolveInputsGpuAddress(desc.OmmTriangles.pOmmLinkage->OpacityMicromapArray);
        }
      }
    }
  } else if (c.m_pDesc.Value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->pOpacityMicromapArrayDesc) {
      resolveInputsGpuAddress(c.m_pDesc.Value->pOpacityMicromapArrayDesc->InputBuffer);
      resolveInputsGpuAddress(c.m_pDesc.Value->pOpacityMicromapArrayDesc->PerOmmDescs.StartAddress);
    }
  }
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->DestAccelerationStructureData, true);
    c.m_pDesc.DestAccelerationStructureKey = info.ResourceKey;
    c.m_pDesc.DestAccelerationStructureOffset = info.Offset;
  }
  if (c.m_pDesc.Value->SourceAccelerationStructureData) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->SourceAccelerationStructureData, true);
    c.m_pDesc.SourceAccelerationStructureKey = info.ResourceKey;
    c.m_pDesc.SourceAccelerationStructureOffset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->ScratchAccelerationStructureData);
    c.m_pDesc.ScratchAccelerationStructureKey = info.ResourceKey;
    c.m_pDesc.ScratchAccelerationStructureOffset = info.Offset;
  }

  auto resolveInputsGpuAddress = [&](D3D12_GPU_VIRTUAL_ADDRESS address) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(address);
    c.m_pDesc.InputKeys.push_back(info.ResourceKey);
    c.m_pDesc.InputOffsets.push_back(info.Offset);
  };

  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    resolveInputsGpuAddress(c.m_pDesc.Value->Inputs.InstanceDescs);
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->Inputs.NumDescs; ++i) {
      const D3D12_RAYTRACING_GEOMETRY_DESC& desc =
          c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
              : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i];
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        resolveInputsGpuAddress(desc.Triangles.Transform3x4);
        resolveInputsGpuAddress(desc.Triangles.IndexBuffer);
        resolveInputsGpuAddress(desc.Triangles.VertexBuffer.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        resolveInputsGpuAddress(desc.AABBs.AABBs.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->Transform3x4);
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->IndexBuffer);
          resolveInputsGpuAddress(desc.OmmTriangles.pTriangles->VertexBuffer.StartAddress);
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          resolveInputsGpuAddress(
              desc.OmmTriangles.pOmmLinkage->OpacityMicromapIndexBuffer.StartAddress);
          resolveInputsGpuAddress(desc.OmmTriangles.pOmmLinkage->OpacityMicromapArray);
        }
      }
    }
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc) {
      resolveInputsGpuAddress(c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc->InputBuffer);
      resolveInputsGpuAddress(
          c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc->PerOmmDescs.StartAddress);
    }
  }

  for (unsigned i = 0; i < c.m_NumPostbuildInfoDescs.Value; ++i) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pPostbuildInfoDescs.Value[i].DestBuffer);
    c.m_pPostbuildInfoDescs.DestBufferKeys.push_back(info.ResourceKey);
    c.m_pPostbuildInfoDescs.DestBufferOffsets.push_back(info.Offset);
  }
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pDesc.Value->DestBuffer);
    c.m_pDesc.destBufferKey = info.ResourceKey;
    c.m_pDesc.destBufferOffset = info.Offset;
  }
  for (unsigned i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pSourceAccelerationStructureData.Value[i]);
    c.m_pSourceAccelerationStructureData.InterfaceKeys[i] = info.ResourceKey;
    c.m_pSourceAccelerationStructureData.Offsets[i] = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_DestAccelerationStructureData.Value, true);
    c.m_DestAccelerationStructureData.InterfaceKey = info.ResourceKey;
    c.m_DestAccelerationStructureData.Offset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_SourceAccelerationStructureData.Value, true);
    c.m_SourceAccelerationStructureData.InterfaceKey = info.ResourceKey;
    c.m_SourceAccelerationStructureData.Offset = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->RayGenerationShaderRecord.StartAddress);
    c.m_pDesc.RayGenerationShaderRecordKey = info.ResourceKey;
    c.m_pDesc.RayGenerationShaderRecordOffset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->MissShaderTable.StartAddress);
    c.m_pDesc.MissShaderTableKey = info.ResourceKey;
    c.m_pDesc.MissShaderTableOffset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->HitGroupTable.StartAddress);
    c.m_pDesc.HitGroupTableKey = info.ResourceKey;
    c.m_pDesc.HitGroupTableOffset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pDesc.Value->CallableShaderTable.StartAddress);
    c.m_pDesc.CallableShaderTableKey = info.ResourceKey;
    c.m_pDesc.CallableShaderTableOffset = info.Offset;
  }
}

void CaptureCustomizationLayer::Pre(
    ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& c) {
  if (c.m_pDesc.Value) {
    c.m_pDesc.DestKey.resize(c.m_DescCount.Value);
    c.m_pDesc.DestOffset.resize(c.m_DescCount.Value);
    c.m_pDesc.SourceKey.resize(c.m_DescCount.Value);
    c.m_pDesc.SourceOffset.resize(c.m_DescCount.Value);
    for (unsigned i = 0; i < c.m_DescCount.Value; ++i) {
      if (c.m_pDesc.Value[i].DataDesc.DestVA) {
        GpuAddressService::GpuAddressInfo info =
            m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pDesc.Value[i].DataDesc.DestVA);
        c.m_pDesc.DestKey[i] = info.ResourceKey;
        c.m_pDesc.DestOffset[i] = info.Offset;
      }
      if (c.m_pDesc.Value[i].DataDesc.SrcVA) {
        GpuAddressService::GpuAddressInfo info =
            m_Manager.getGpuAddressService().getGpuAddressInfo(c.m_pDesc.Value[i].DataDesc.SrcVA);
        c.m_pDesc.SourceKey[i] = info.ResourceKey;
        c.m_pDesc.SourceOffset[i] = info.Offset;
      }
    }
  }
}

void CaptureCustomizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (c.m_pExtensionAppInfo.Value) {
    c.m_pExtensionAppInfo.ApplicationName = c.m_pExtensionAppInfo.Value->pApplicationName;
    c.m_pExtensionAppInfo.EngineName = c.m_pExtensionAppInfo.Value->pEngineName;
  }
}

void CaptureCustomizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (c.m_pExtensionAppInfo.Value) {
    c.m_pExtensionAppInfo.ApplicationName = c.m_pExtensionAppInfo.Value->pApplicationName;
    c.m_pExtensionAppInfo.EngineName = c.m_pExtensionAppInfo.Value->pEngineName;
  }
}

void CaptureCustomizationLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (c.m_pExtensionAppInfo.Value) {
    c.m_pExtensionAppInfo.ApplicationName = c.m_pExtensionAppInfo.Value->pApplicationName;
    c.m_pExtensionAppInfo.EngineName = c.m_pExtensionAppInfo.Value->pEngineName;
  }
}

void CaptureCustomizationLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.m_pDesc.Cs = c.m_pDesc.Value->CS.pShaderBytecode;
  c.m_pDesc.CompileOptions = c.m_pDesc.Value->CompileOptions;
  c.m_pDesc.InternalOptions = c.m_pDesc.Value->InternalOptions;
}

void CaptureCustomizationLayer::Pre(IDMLDeviceCreateBindingTableCommand& c) {
  DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescHandle = {
      c.m_desc.Value->CPUDescriptorHandle};
  fillCpuDescriptorHandleArgument(cpuDescHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  c.m_desc.TableFields.CpuDescHandleKey = cpuDescHandle.InterfaceKey;
  c.m_desc.TableFields.CpuDescHandleIndex = cpuDescHandle.Index;

  DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescHandle = {
      c.m_desc.Value->GPUDescriptorHandle};
  fillGpuDescriptorHandleArgument(gpuDescHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  c.m_desc.TableFields.GpuDescHandleKey = gpuDescHandle.InterfaceKey;
  c.m_desc.TableFields.GpuDescHandleIndex = gpuDescHandle.Index;
}

void CaptureCustomizationLayer::Pre(IDMLBindingTableResetCommand& c) {
  DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescHandle = {
      c.m_desc.Value->CPUDescriptorHandle};
  fillCpuDescriptorHandleArgument(cpuDescHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  c.m_desc.TableFields.CpuDescHandleKey = cpuDescHandle.InterfaceKey;
  c.m_desc.TableFields.CpuDescHandleIndex = cpuDescHandle.Index;

  DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescHandle = {
      c.m_desc.Value->GPUDescriptorHandle};
  fillGpuDescriptorHandleArgument(gpuDescHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  c.m_desc.TableFields.GpuDescHandleKey = gpuDescHandle.InterfaceKey;
  c.m_desc.TableFields.GpuDescHandleIndex = gpuDescHandle.Index;
}

void CaptureCustomizationLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pParams.Value->pDesc->destAccelerationStructureData, true);
    c.m_pParams.DestAccelerationStructureKey = info.ResourceKey;
    c.m_pParams.DestAccelerationStructureOffset = info.Offset;
  }
  if (c.m_pParams.Value->pDesc->sourceAccelerationStructureData) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pParams.Value->pDesc->sourceAccelerationStructureData, true);
    c.m_pParams.SourceAccelerationStructureKey = info.ResourceKey;
    c.m_pParams.SourceAccelerationStructureOffset = info.Offset;
  }
  {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pParams.Value->pDesc->scratchAccelerationStructureData);
    c.m_pParams.ScratchAccelerationStructureKey = info.ResourceKey;
    c.m_pParams.ScratchAccelerationStructureOffset = info.Offset;
  }

  auto resolveInputsGpuAddress = [&](D3D12_GPU_VIRTUAL_ADDRESS address) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(address);
    c.m_pParams.InputKeys.push_back(info.ResourceKey);
    c.m_pParams.InputOffsets.push_back(info.Offset);
  };

  if (c.m_pParams.Value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    resolveInputsGpuAddress(c.m_pParams.Value->pDesc->inputs.instanceDescs);
  } else {
    for (unsigned i = 0; i < c.m_pParams.Value->pDesc->inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
          c.m_pParams.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(c.m_pParams.Value->pDesc->inputs
                                                                        .pGeometryDescs) +
                                                            c.m_pParams.Value->pDesc->inputs
                                                                    .geometryDescStrideInBytes *
                                                                i)
              : *c.m_pParams.Value->pDesc->inputs.ppGeometryDescs[i];
      if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES_EX) {
        resolveInputsGpuAddress(desc.triangles.Transform3x4);
        resolveInputsGpuAddress(desc.triangles.IndexBuffer);
        resolveInputsGpuAddress(desc.triangles.VertexBuffer.StartAddress);
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS_EX) {
        resolveInputsGpuAddress(desc.aabbs.AABBs.StartAddress);
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        resolveInputsGpuAddress(desc.ommTriangles.triangles.Transform3x4);
        resolveInputsGpuAddress(desc.ommTriangles.triangles.IndexBuffer);
        resolveInputsGpuAddress(desc.ommTriangles.triangles.VertexBuffer.StartAddress);
        resolveInputsGpuAddress(
            desc.ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StartAddress);
        resolveInputsGpuAddress(desc.ommTriangles.ommAttachment.opacityMicromapArray);
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        resolveInputsGpuAddress(desc.dmmTriangles.triangles.Transform3x4);
        resolveInputsGpuAddress(desc.dmmTriangles.triangles.IndexBuffer);
        resolveInputsGpuAddress(desc.dmmTriangles.triangles.VertexBuffer.StartAddress);
        resolveInputsGpuAddress(
            desc.dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StartAddress);
        resolveInputsGpuAddress(
            desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress);
        resolveInputsGpuAddress(
            desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StartAddress);
        resolveInputsGpuAddress(
            desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StartAddress);
        resolveInputsGpuAddress(desc.dmmTriangles.dmmAttachment.displacementMicromapArray);
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_SPHERES_EX) {
        resolveInputsGpuAddress(desc.spheres.vertexPositionBuffer.StartAddress);
        resolveInputsGpuAddress(desc.spheres.vertexRadiusBuffer.StartAddress);
        resolveInputsGpuAddress(desc.spheres.indexBuffer.StartAddress);
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        resolveInputsGpuAddress(desc.lss.vertexPositionBuffer.StartAddress);
        resolveInputsGpuAddress(desc.lss.vertexRadiusBuffer.StartAddress);
        resolveInputsGpuAddress(desc.lss.indexBuffer.StartAddress);
      }
    }
  }

  for (unsigned i = 0; i < c.m_pParams.Value->numPostbuildInfoDescs; ++i) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pParams.Value->pPostbuildInfoDescs[i].DestBuffer);
    c.m_pParams.DestPostBuildBufferKeys.push_back(info.ResourceKey);
    c.m_pParams.DestPostBuildBufferOffsets.push_back(info.Offset);
  }
}

void CaptureCustomizationLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (c.m_pParams.Value->pDesc) {
    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->destOpacityMicromapArrayData);
      c.m_pParams.DestOpacityMicromapArrayDataKey = info.ResourceKey;
      c.m_pParams.DestOpacityMicromapArrayDataOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->inputs.inputBuffer);
      c.m_pParams.InputBufferKey = info.ResourceKey;
      c.m_pParams.InputBufferOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->inputs.perOMMDescs.StartAddress);
      c.m_pParams.PerOMMDescsKey = info.ResourceKey;
      c.m_pParams.PerOMMDescsOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->scratchOpacityMicromapArrayData);
      c.m_pParams.ScratchOpacityMicromapArrayDataKey = info.ResourceKey;
      c.m_pParams.ScratchOpacityMicromapArrayDataOffset = info.Offset;
    }
  }

  for (unsigned i = 0; i < c.m_pParams.Value->numPostbuildInfoDescs; ++i) {
    GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
        c.m_pParams.Value->pPostbuildInfoDescs[i].destBuffer);
    c.m_pParams.DestPostBuildBufferKeys.push_back(info.ResourceKey);
    c.m_pParams.DestPostBuildBufferOffsets.push_back(info.Offset);
  }
}

void CaptureCustomizationLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (c.m_pParams.Value->pDesc) {
    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->batchResultData);
      c.m_pParams.BatchResultDataKey = info.ResourceKey;
      c.m_pParams.BatchResultDataOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->batchScratchData);
      c.m_pParams.BatchScratchDataKey = info.ResourceKey;
      c.m_pParams.BatchScratchDataOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->destinationAddressArray.StartAddress);
      c.m_pParams.DestinationAddressArrayKey = info.ResourceKey;
      c.m_pParams.DestinationAddressArrayOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->resultSizeArray.StartAddress);
      c.m_pParams.ResultSizeArrayKey = info.ResourceKey;
      c.m_pParams.ResultSizeArrayOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->indirectArgArray.StartAddress);
      c.m_pParams.IndirectArgArrayKey = info.ResourceKey;
      c.m_pParams.IndirectArgArrayOffset = info.Offset;
    }

    {
      GpuAddressService::GpuAddressInfo info = m_Manager.getGpuAddressService().getGpuAddressInfo(
          c.m_pParams.Value->pDesc->indirectArgCount);
      c.m_pParams.IndirectArgCountKey = info.ResourceKey;
      c.m_pParams.IndirectArgCountOffset = info.Offset;
    }
  }
}

void CaptureCustomizationLayer::Post(IDStorageFactoryOpenFileCommand& c) {
  m_DirectStorageService.openFile(c);
}

void CaptureCustomizationLayer::Pre(IDStorageQueueEnqueueRequestCommand& c) {
  m_DirectStorageService.enqueueRequest(c);
}

void CaptureCustomizationLayer::Pre(xefgSwapChainD3D12InitFromSwapChainCommand& c) {
  LOG_INFO << "xefgSwapChainD3D12InitFromSwapChainCommand: pApplicaitonSwapChain: "
           << c.m_pInitParams.Value->pApplicationSwapChain;
  IDXGISwapChain* swapChain = c.m_pInitParams.Value->pApplicationSwapChain;
  IUnknownWrapper* wrapper = m_Manager.findWrapper(swapChain);
  if (wrapper) {
    m_Manager.removeWrapper(wrapper);
    delete wrapper;
  }
}

void CaptureCustomizationLayer::Pre(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) {
  RECT rect;
  BOOL ret = GetClientRect(c.m_hWnd.Value, &rect);
  GITS_ASSERT(ret);

  CreateWindowMetaCommand createWindowCommand(c.ThreadId);
  createWindowCommand.Key = m_Manager.createCommandKey();
  createWindowCommand.m_hWnd.Value = c.m_hWnd.Value;
  createWindowCommand.m_width.Value = rect.right - rect.left;
  createWindowCommand.m_height.Value = rect.bottom - rect.top;
  createWindowCommand.m_width.Value = std::max(
      static_cast<unsigned>(createWindowCommand.m_width.Value), c.m_pSwapChainDesc.Value->Width);
  createWindowCommand.m_height.Value = std::max(
      static_cast<unsigned>(createWindowCommand.m_height.Value), c.m_pSwapChainDesc.Value->Height);

  m_Recorder.Record(createWindowCommand.Key, new CreateWindowMetaSerializer(createWindowCommand));
}

void CaptureCustomizationLayer::fillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  if (arg.Value) {
    GpuAddressService::GpuAddressInfo info =
        m_Manager.getGpuAddressService().getGpuAddressInfo(arg.Value);
    arg.InterfaceKey = info.ResourceKey;
    arg.Offset = info.Offset;
  }
}

void CaptureCustomizationLayer::fillGpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType) {

  CaptureDescriptorHandleService::HandleInfo info =
      m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
          heapType, CaptureDescriptorHandleService::HandleType::GpuHandle, arg.Value.ptr);

  arg.InterfaceKey = info.InterfaceKey;
  arg.Index = info.Index;
}

void CaptureCustomizationLayer::fillCpuDescriptorHandleArgument(
    DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType) {

  CaptureDescriptorHandleService::HandleInfo info =
      m_Manager.getDescriptorHandleService().getDescriptorHandleInfo(
          heapType, CaptureDescriptorHandleService::HandleType::CpuHandle, arg.Value.ptr);

  arg.InterfaceKey = info.InterfaceKey;
  arg.Index = info.Index;
}

} // namespace DirectX
} // namespace gits
