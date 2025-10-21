// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "analyzerLayerAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1',
    'ID3D12CommandQueueExecuteCommandLists',
    'ID3D12CommandQueueWait',
    'ID3D12CommandQueueSignal',
    'ID3D12FenceSignal',
    'ID3D12DeviceCreateFence',
    'ID3D12Device3EnqueueMakeResident',
    'ID3D12FenceGetCompletedValue',
    'ID3D12DeviceCopyDescriptorsSimple',
    'ID3D12DeviceCopyDescriptors',
    'ID3D12DeviceCreateRenderTargetView',
    'ID3D12DeviceCreateDepthStencilView',
    'ID3D12DeviceCreateShaderResourceView',
    'ID3D12DeviceCreateUnorderedAccessView',
    'ID3D12DeviceCreateConstantBufferView',
    'ID3D12DeviceCreateSampler',
    'ID3D12DeviceCreateDescriptorHeap',
    'ID3D12DeviceCreateRootSignature',
    'ID3D12Device5CreateStateObject',
    'ID3D12Device7AddToStateObject',
    'ID3D12ResourceGetGPUVirtualAddress',
    'ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStart',
    'ID3D12DeviceCreateCommandSignature',
    'ID3D12DeviceCreateCommittedResource',
    'ID3D12Device4CreateCommittedResource1',
    'ID3D12Device8CreateCommittedResource2',
    'ID3D12Device10CreateCommittedResource3',
    'ID3D12DeviceCreatePlacedResource',
    'ID3D12Device8CreatePlacedResource1',
    'ID3D12Device10CreatePlacedResource2',
    'xessD3D12CreateContext',
    'IDXGIDeviceCreateSurface',
    'ID3D12GraphicsCommandListReset',
    'ID3D12GraphicsCommandListSetComputeRootSignature',
    'ID3D12GraphicsCommandListSetGraphicsRootSignature',
    'ID3D12GraphicsCommandListSetComputeRootDescriptorTable',
    'ID3D12GraphicsCommandListSetGraphicsRootDescriptorTable',
    'ID3D12GraphicsCommandListSetComputeRootConstantBufferView',
    'ID3D12GraphicsCommandListSetGraphicsRootConstantBufferView',
    'ID3D12GraphicsCommandListSetComputeRootShaderResourceView',
    'ID3D12GraphicsCommandListSetGraphicsRootShaderResourceView',
    'ID3D12GraphicsCommandListSetComputeRootUnorderedAccessView',
    'ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessView',
    'ID3D12GraphicsCommandListIASetIndexBuffer',
    'ID3D12GraphicsCommandListIASetVertexBuffers',
    'ID3D12GraphicsCommandListSOSetTargets',
    'ID3D12GraphicsCommandListOMSetRenderTargets',
    'ID3D12GraphicsCommandListClearDepthStencilView',
    'ID3D12GraphicsCommandListClearRenderTargetView',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewUint',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewFloat',
    'ID3D12GraphicsCommandList4SetPipelineState1',
    'ID3D12GraphicsCommandList2WriteBufferImmediate',
    'ID3D12GraphicsCommandListCopyBufferRegion',
    'ID3D12GraphicsCommandListCopyResource',
    'ID3D12GraphicsCommandListCopyTextureRegion',
    'ID3D12GraphicsCommandListCopyTiles',
    'ID3D12GraphicsCommandListDiscardResource',
    'ID3D12GraphicsCommandListResolveSubresource',
    'ID3D12GraphicsCommandListResourceBarrier',
    'ID3D12GraphicsCommandListSetPipelineState',
    'ID3D12GraphicsCommandList1ResolveSubresourceRegion',
    'ID3D12GraphicsCommandList3SetProtectedResourceSession',
    'ID3D12GraphicsCommandList4InitializeMetaCommand',
    'ID3D12GraphicsCommandList4ExecuteMetaCommand',
    'ID3D12GraphicsCommandList7Barrier',
    'ID3D12GraphicsCommandListSetDescriptorHeaps',
    'ID3D12GraphicsCommandListClearState',
    'ID3D12GraphicsCommandListExecuteBundle',
    'ID3D12GraphicsCommandListBeginQuery',
    'ID3D12GraphicsCommandListEndQuery',
    'ID3D12GraphicsCommandListResolveQueryData',
    'ID3D12GraphicsCommandListSetPredication',
    'ID3D12GraphicsCommandList1AtomicCopyBufferUINT',
    'ID3D12GraphicsCommandList1AtomicCopyBufferUINT64',
    'ID3D12GraphicsCommandList4BeginRenderPass',
    'ID3D12GraphicsCommandList5RSSetShadingRateImage',
    'ID3D12GraphicsCommandList6DispatchMesh',
    'ID3D12DeviceCreateGraphicsPipelineState',
    'ID3D12DeviceCreateComputePipelineState',
    'ID3D12PipelineLibraryLoadGraphicsPipeline',
    'ID3D12PipelineLibraryLoadComputePipeline',
    'ID3D12PipelineLibrary1LoadPipeline',
    'ID3D12Device2CreatePipelineState',
    'IDMLDeviceCreateBindingTable',
    'IDMLBindingTableReset',
    'IDMLBindingTableBindInputs',
    'IDMLBindingTableBindOutputs',
    'IDMLDevice1CompileGraph',
    'xessD3D12Init',
    'xessD3D12GetInitParams',
    'xessD3D12Execute',
    'IDStorageFactoryCreateQueue',
    'IDStorageQueueEnqueueRequest',
    'D3D12CreateDevice',
    'DMLCreateDevice',
    'DMLCreateDevice1',
    'ID3D12DeviceCreateCommandQueue',
    'ID3D12DeviceCreateCommandAllocator',
    'ID3D12DeviceCreateCommandList',
    'ID3D12DeviceCreateHeap',
    'ID3D12DeviceCreateReservedResource',
    'ID3D12DeviceCreateSharedHandle',
    'ID3D12DeviceOpenSharedHandle',
    'ID3D12DeviceCreateFence',
    'ID3D12DeviceCreateQueryHeap',
    'ID3D12Device1CreatePipelineLibrary',
    'ID3D12Device4CreateCommandList1',
    'ID3D12Device4CreateProtectedResourceSession',
    'ID3D12Device4CreateHeap1',
    'ID3D12Device4CreateReservedResource1',
    'ID3D12Device5CreateLifetimeTracker',
    'ID3D12Device5CreateMetaCommand',
    'ID3D12Device7CreateProtectedResourceSession1',
    'ID3D12Device9CreateShaderCacheSession',
    'ID3D12Device9CreateCommandQueue1',
    'ID3D12Device10CreateReservedResource2',
    'ID3D12Device14CreateRootSignatureFromSubobjectInLibrary',
    'ID3D12SDKConfiguration1CreateDeviceFactory',
    'ID3D12DeviceConfigurationCreateVersionedRootSignatureDeserializer',
    'ID3D12DeviceConfiguration1CreateVersionedRootSignatureDeserializerFromSubobjectInLibrary',
    'ID3D12DSRDeviceFactoryCreateDSRDevice',
    'IDXGIFactoryCreateSoftwareAdapter',
    'IDXGIResource1CreateSubresourceSurface',
    'IDXGIFactory2CreateSwapChainForHwnd',
    'IDXGIFactory2CreateSwapChainForCoreWindow',
    'IDXGIFactory2CreateSwapChainForComposition',
    'IDXGIFactoryMediaCreateSwapChainForCompositionSurfaceHandle',
    'IDXGIFactoryMediaCreateDecodeSwapChainForCompositionSurfaceHandle',
    'IDMLDeviceCreateOperator',
    'IDMLDeviceCreateOperatorInitializer',
    'IDMLDeviceCreateCommandRecorder',
    'IDStorageFactoryCreateStatusArray',
    'ID3D12PipelineStateGetCachedBlob',
    'ID3D12Device3OpenExistingHeapFromAddress',
    'ID3D12Device3OpenExistingHeapFromFileMapping',
    'ID3D12ProtectedSessionGetStatusFence',
    'ID3D12SwapChainAssistantGetSwapChainObject',
    'ID3D12SwapChainAssistantGetCurrentResourceAndCommandQueue',
    'ID3D12Device13OpenExistingHeapFromAddress1',
    'ID3D12DeviceTools1GetApplicationSpecificDriverState',
    'ID3D12DeviceFactoryGetConfigurationInterface',
    'ID3D12DeviceConfigurationSerializeVersionedRootSignature',
    'IDXGIAdapterEnumOutputs',
    'IDXGISwapChainGetBuffer',
    'IDXGISwapChainGetFullscreenState',
    'IDXGISwapChainGetContainingOutput',
    'IDXGIFactoryEnumAdapters',
    'IDXGIFactory1EnumAdapters1',
    'IDXGIOutputDuplicationAcquireNextFrame',
    'IDXGISurface2GetResource',
    'IDXGISwapChain1GetCoreWindow',
    'IDXGISwapChain1GetRestrictToOutput',
    'IDXGIOutput1DuplicateOutput',
    'IDXGIFactory4EnumAdapterByLuid',
    'IDXGIFactory4EnumWarpAdapter',
    'IDXGIOutput5DuplicateOutput1',
    'IDXGIFactory6EnumAdapterByGpuPreference',
    'IDMLDeviceCompileOperator',
    'IDStorageFactoryOpenFile'
]
%>\
%for function in functions:
%if not function.name in custom:
void AnalyzerLayer::post(${function.name}Command& c) {
  %for param in function.params:
  %if param.is_interface or param.is_interface_creation:
  %if not param.sal_size:
  analyzerService_.notifyObject(c.${param.name}_.key);
  %else:
  analyzerService_.notifyObjects(c.${param.name}_.keys);
  %endif
  %endif
  %endfor
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void AnalyzerLayer::post(${interface.name}${function.name}Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  %for param in function.params:
  %if param.is_interface or param.is_interface_creation:
  %if not param.sal_size:
  analyzerService_.notifyObject(c.${param.name}_.key);
  %else:
  analyzerService_.notifyObjects(c.${param.name}_.keys);
  %endif
  %endif
  %endfor
  %if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('SetName') \
    and not interface.name + function.name in custom:
  analyzerService_.commandListCommand(c.object_.key);
  %endif
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
