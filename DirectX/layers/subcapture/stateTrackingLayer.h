// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "stateTrackingService.h"
#include "subcaptureRecorder.h"
#include "subcaptureRange.h"
#include "fenceTrackingService.h"
#include "mapStateService.h"
#include "resourceStateTrackingService.h"
#include "heapAllocationStateService.h"
#include "reservedResourcesService.h"
#include "descriptorService.h"
#include "commandListService.h"
#include "commandQueueService.h"
#include "xessStateService.h"
#include "accelerationStructuresBuildService.h"
#include "accelerationStructuresSerializeService.h"
#include "gpuExecutionFlusher.h"
#include "residencyService.h"
#include "analyzerResults.h"
#include "resourceForCBVRestoreService.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"

#include <map>
#include <array>

namespace gits {
namespace DirectX {
class StateTrackingLayer : public Layer {
public:
  StateTrackingLayer(SubcaptureRecorder& recorder, SubcaptureRange& subcaptureRange);

  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;
  void Pre(IUnknownReleaseCommand& c) override;
  void Post(IUnknownAddRefCommand& c) override;
  void Post(IUnknownQueryInterfaceCommand& c) override;
  void Post(CreateDXGIFactoryCommand& c) override;
  void Post(CreateDXGIFactory1Command& c) override;
  void Post(CreateDXGIFactory2Command& c) override;
  void Post(IDXGIFactoryEnumAdaptersCommand& c) override;
  void Post(IDXGIFactory1EnumAdapters1Command& c) override;
  void Post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) override;
  void Post(IDXGIFactory4EnumAdapterByLuidCommand& c) override;
  void Post(IDXGIAdapterEnumOutputsCommand& c) override;
  void Post(IDXGIObjectGetParentCommand& c) override;
  void Post(D3D12CreateDeviceCommand& c) override;
  void Post(D3D12EnableExperimentalFeaturesCommand& c) override;
  void Post(D3D12GetInterfaceCommand& Command) override;
  void Post(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void Post(ID3D12Device9CreateCommandQueue1Command& c) override;
  void Pre(IDXGIFactoryCreateSwapChainCommand& c) override;
  void Post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void Pre(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void Post(IDXGISwapChainResizeBuffersCommand& c) override;
  void Post(IDXGISwapChain3ResizeBuffers1Command& c) override;
  void Post(ID3D12ObjectSetNameCommand& c) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void Post(ID3D12DeviceCreateHeapCommand& c) override;
  void Post(ID3D12Device4CreateHeap1Command& c) override;
  void Post(ID3D12DeviceCreateQueryHeapCommand& c) override;
  void Post(CreateHeapAllocationMetaCommand& c) override;
  void Post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) override;
  void Post(IDXGISwapChainGetBufferCommand& c) override;
  void Post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void Post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void Post(ID3D12Device1CreatePipelineLibraryCommand& c) override;
  void Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) override;
  void Post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void Post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void Post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void Post(ID3D12Device5CreateStateObjectCommand& c) override;
  void Post(ID3D12Device7AddToStateObjectCommand& c) override;
  void Post(ID3D12DeviceCreateCommandListCommand& c) override;
  void Post(ID3D12Device4CreateCommandList1Command& c) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void Post(ID3D12Device4CreateReservedResource1Command& c) override;
  void Post(ID3D12Device10CreateReservedResource2Command& c) override;
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void Post(ID3D12DeviceMakeResidentCommand& c) override;
  void Post(ID3D12DeviceEvictCommand& c) override;
  void Post(ID3D12DeviceCreateSamplerCommand& c) override;
  void Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12ResourceMapCommand& c) override;
  void Post(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;
  void Post(ID3D12CommandQueueCopyTileMappingsCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void Post(INTC_D3D12_SetApplicationInfoCommand& c) override;
  void Post(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void Post(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void Post(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void Post(INTC_D3D12_CreateHeapCommand& c) override;
  void Post(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void Post(ID3D12Device1SetResidencyPriorityCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& Command) override;
  void Post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& Command) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void Post(ID3D12GraphicsCommandListResetCommand& c) override;
  void Post(ID3D12GraphicsCommandListCloseCommand& c) override;
  void Post(ID3D12GraphicsCommandListClearStateCommand& Command) override;
  void Post(ID3D12GraphicsCommandListDrawInstancedCommand& Command) override;
  void Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& Command) override;
  void Post(ID3D12GraphicsCommandListDispatchCommand& Command) override;
  void Post(ID3D12GraphicsCommandListCopyBufferRegionCommand& Command) override;
  void Post(ID3D12GraphicsCommandListCopyTextureRegionCommand& Command) override;
  void Post(ID3D12GraphicsCommandListCopyResourceCommand& Command) override;
  void Post(ID3D12GraphicsCommandListCopyTilesCommand& Command) override;
  void Post(ID3D12GraphicsCommandListResolveSubresourceCommand& Command) override;
  void Post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& Command) override;
  void Post(ID3D12GraphicsCommandListRSSetViewportsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& Command) override;
  void Post(ID3D12GraphicsCommandListOMSetStencilRefCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetPipelineStateCommand& Command) override;
  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& Command) override;
  void Post(ID3D12GraphicsCommandListExecuteBundleCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& Command) override;
  void Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSOSetTargetsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& Command) override;
  void Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& Command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& Command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& Command) override;
  void Post(ID3D12GraphicsCommandListDiscardResourceCommand& Command) override;
  void Post(ID3D12GraphicsCommandListBeginQueryCommand& Command) override;
  void Post(ID3D12GraphicsCommandListEndQueryCommand& Command) override;
  void Post(ID3D12GraphicsCommandListResolveQueryDataCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetPredicationCommand& Command) override;
  void Post(ID3D12GraphicsCommandListSetMarkerCommand& Command) override;
  void Post(ID3D12GraphicsCommandListBeginEventCommand& Command) override;
  void Post(ID3D12GraphicsCommandListEndEventCommand& Command) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& Command) override;
  void Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& Command) override;
  void Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& Command) override;
  void Post(ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& Command) override;
  void Post(ID3D12GraphicsCommandList1SetSamplePositionsCommand& Command) override;
  void Post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& Command) override;
  void Post(ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& Command) override;
  void Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& Command) override;
  void Post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4BeginRenderPassCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
                Command) override;
  void Post(ID3D12GraphicsCommandList4EndRenderPassCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& Command) override;
  void Post(ID3D12GraphicsCommandList4SetPipelineState1Command& Command) override;
  void Post(ID3D12GraphicsCommandList5RSSetShadingRateCommand& Command) override;
  void Post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& Command) override;
  void Post(ID3D12GraphicsCommandList6DispatchMeshCommand& Command) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& Command) override;
  void Post(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& Command) override;
  void Post(ID3D12DeviceFactoryCreateDeviceCommand& Command) override;
  void Post(xessD3D12CreateContextCommand& c) override;
  void Post(xessD3D12InitCommand& c) override;
  void Pre(xessDestroyContextCommand& c) override;
  void Post(xessDestroyContextCommand& c) override;
  void Post(xessSetJitterScaleCommand& c) override;
  void Post(xessSetVelocityScaleCommand& c) override;
  void Post(xessSetExposureMultiplierCommand& c) override;
  void Post(xessForceLegacyScaleFactorsCommand& c) override;
  void Post(DStorageGetFactoryCommand& c) override;
  void Post(IDStorageFactoryOpenFileCommand& c) override;
  void Post(IDStorageFactoryCreateQueueCommand& c) override;
  void Post(IDStorageFactoryCreateStatusArrayCommand& c) override;
  void Post(NvAPI_InitializeCommand& c) override;
  void Post(NvAPI_UnloadCommand& c) override;
  void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void Post(DllContainerMetaCommand& c) override;
  void Post(xellD3D12CreateContextCommand& c) override;
  void Pre(xellDestroyContextCommand& c) override;
  void Post(xellDestroyContextCommand& c) override;
  void Post(xellSetSleepModeCommand& c) override;
  void Post(xellAddMarkerDataCommand& c) override;
  void Post(xefgSwapChainD3D12CreateContextCommand& c) override;
  void Pre(xefgSwapChainDestroyCommand& c) override;
  void Post(xefgSwapChainDestroyCommand& c) override;
  void Post(xefgSwapChainSetLatencyReductionCommand& c) override;
  void Post(xefgSwapChainSetEnabledCommand& c) override;
  void Post(xefgSwapChainSetSceneChangeThresholdCommand& c) override;
  void Post(xefgSwapChainD3D12InitFromSwapChainCommand& c) override;
  void Post(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) override;
  void Post(xefgSwapChainD3D12GetSwapChainPtrCommand& c) override;
  void Post(xefgSwapChainD3D12SetDescriptorHeapCommand& c) override;
  void Post(xefgSwapChainEnableDebugFeatureCommand& c) override;

private:
  void SetAsChildInParent(unsigned parentKey, unsigned childKey);
  bool IsResourceHeapMappable(const D3D12_HEAP_PROPERTIES& heapProperties,
                              const D3D12_TEXTURE_LAYOUT& textureLayout) {
    return !(heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT ||
             heapProperties.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE ||
             textureLayout == D3D12_TEXTURE_LAYOUT_UNKNOWN);
  }
  bool IsResourceHeapMappable(unsigned heapKey, const D3D12_TEXTURE_LAYOUT& textureLayout);
  bool IsResourceBarrierRestricted(D3D12_RESOURCE_FLAGS flags);
  void ReleaseSwapChainBuffers(unsigned key, unsigned referenceCount);

private:
  bool m_StateRestored{};
  std::map<unsigned, unsigned> m_DeviceByINTCExtensionContext;
  StateTrackingService m_StateService;
  SubcaptureRecorder& m_Recorder;
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerResults m_AnalyzerResults;
  FenceTrackingService m_FenceTrackingService;
  MapStateService m_MapStateService;
  ResourceStateTrackingService m_ResourceStateTrackingService;
  HeapAllocationStateService m_HeapAllocationStateService;
  ReservedResourcesService m_ReservedResourcesService;
  ResourceForCBVRestoreService m_ResourceForCBVRestoreService;
  DescriptorService m_DescriptorService;
  CommandListService m_CommandListService;
  CommandQueueService m_CommandQueueService;
  XessStateService m_XessStateService;
  XellStateService m_XellStateService;
  XefgStateService m_XefgStateService;
  AccelerationStructuresSerializeService m_AccelerationStructuresSerializeService;
  AccelerationStructuresBuildService m_AccelerationStructuresBuildService;
  ResidencyService m_ResidencyService;
  ResourceUsageTrackingService m_ResourceUsageTrackingService;
  GpuExecutionFlusher m_GpuExecutionFlusher;
  ResourceStateTracker m_ResourceStateTracker;
  CapturePlayerGpuAddressService m_GpuAddressService;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> m_ResourceHeaps;
  std::unordered_map<unsigned, std::vector<unsigned>> m_SwapchainBuffers;

  class CommandQueueSwapChainRefCountTracker {
  public:
    void PreCreateSwapChain(unsigned commandQueueKey,
                            ID3D12CommandQueue* commandQueue,
                            unsigned swapChainKey);
    void PostCreateSwapChain(unsigned commandQueueKey,
                             ID3D12CommandQueue* commandQueue,
                             unsigned swapChainKey);
    unsigned DestroySwapChain(unsigned swapChainKey);

  private:
    unsigned m_RefCountPre{};
    std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> m_RefCountIncrements;
    std::unordered_map<unsigned, unsigned> m_CommandQueueBySwapChain;
    std::unordered_map<unsigned, ID3D12CommandQueue*> m_CommandQueues;
  };
  CommandQueueSwapChainRefCountTracker m_CommandQueueSwapChainRefCountTracker;
};

} // namespace DirectX
} // namespace gits
