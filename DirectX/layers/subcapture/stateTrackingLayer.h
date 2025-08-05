// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

#include <map>
#include <array>

namespace gits {
namespace DirectX {
class StateTrackingLayer : public Layer {
public:
  StateTrackingLayer(SubcaptureRecorder& recorder, SubcaptureRange& subcaptureRange);

  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;
  void pre(IUnknownReleaseCommand& c) override;
  void post(IUnknownAddRefCommand& c) override;
  void post(IUnknownQueryInterfaceCommand& c) override;
  void post(CreateDXGIFactoryCommand& c) override;
  void post(CreateDXGIFactory1Command& c) override;
  void post(CreateDXGIFactory2Command& c) override;
  void post(IDXGIFactoryEnumAdaptersCommand& c) override;
  void post(IDXGIFactory1EnumAdapters1Command& c) override;
  void post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) override;
  void post(IDXGIFactory4EnumAdapterByLuidCommand& c) override;
  void post(IDXGIAdapterEnumOutputsCommand& c) override;
  void post(IDXGIObjectGetParentCommand& c) override;
  void post(D3D12CreateDeviceCommand& c) override;
  void post(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void post(ID3D12Device9CreateCommandQueue1Command& c) override;
  void post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void post(IDXGISwapChainResizeBuffersCommand& c) override;
  void post(ID3D12ObjectSetNameCommand& c) override;
  void post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void post(ID3D12DeviceCreateHeapCommand& c) override;
  void post(ID3D12Device4CreateHeap1Command& c) override;
  void post(ID3D12DeviceCreateQueryHeapCommand& c) override;
  void post(CreateHeapAllocationMetaCommand& c) override;
  void post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) override;
  void post(IDXGISwapChainGetBufferCommand& c) override;
  void post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void post(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void post(ID3D12Device1CreatePipelineLibraryCommand& c) override;
  void post(ID3D12PipelineLibrary1LoadPipelineCommand& c) override;
  void post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) override;
  void post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) override;
  void post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void post(ID3D12Device5CreateStateObjectCommand& c) override;
  void post(ID3D12Device7AddToStateObjectCommand& c) override;
  void post(ID3D12DeviceCreateCommandListCommand& c) override;
  void post(ID3D12Device4CreateCommandList1Command& c) override;
  void pre(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void pre(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void pre(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void pre(ID3D12Device10CreateCommittedResource3Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void pre(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void pre(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void pre(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void pre(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void pre(ID3D12Device4CreateReservedResource1Command& c) override;
  void post(ID3D12Device4CreateReservedResource1Command& c) override;
  void pre(ID3D12Device10CreateReservedResource2Command& c) override;
  void post(ID3D12Device10CreateReservedResource2Command& c) override;
  void post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(ID3D12DeviceMakeResidentCommand& c) override;
  void post(ID3D12DeviceEvictCommand& c) override;
  void post(ID3D12DeviceCreateSamplerCommand& c) override;
  void post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void post(ID3D12ResourceMapCommand& c) override;
  void post(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;
  void post(ID3D12CommandQueueCopyTileMappingsCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void post(INTC_D3D12_SetApplicationInfoCommand& c) override;
  void post(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void post(INTC_D3D12_CreateHeapCommand& c) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void post(ID3D12Device1SetResidencyPriorityCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void post(ID3D12GraphicsCommandListResetCommand& c) override;
  void post(ID3D12GraphicsCommandListCloseCommand& c) override;
  void post(ID3D12GraphicsCommandListClearStateCommand& command) override;
  void post(ID3D12GraphicsCommandListDrawInstancedCommand& command) override;
  void post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& command) override;
  void post(ID3D12GraphicsCommandListDispatchCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyBufferRegionCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyTextureRegionCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyResourceCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyTilesCommand& command) override;
  void post(ID3D12GraphicsCommandListResolveSubresourceCommand& command) override;
  void post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& command) override;
  void post(ID3D12GraphicsCommandListRSSetViewportsCommand& command) override;
  void post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& command) override;
  void post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& command) override;
  void post(ID3D12GraphicsCommandListOMSetStencilRefCommand& command) override;
  void post(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void post(ID3D12GraphicsCommandListResourceBarrierCommand& command) override;
  void post(ID3D12GraphicsCommandListExecuteBundleCommand& command) override;
  void post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void post(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void post(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void post(ID3D12GraphicsCommandListDiscardResourceCommand& command) override;
  void post(ID3D12GraphicsCommandListBeginQueryCommand& command) override;
  void post(ID3D12GraphicsCommandListEndQueryCommand& command) override;
  void post(ID3D12GraphicsCommandListResolveQueryDataCommand& command) override;
  void post(ID3D12GraphicsCommandListSetPredicationCommand& command) override;
  void post(ID3D12GraphicsCommandListSetMarkerCommand& command) override;
  void post(ID3D12GraphicsCommandListBeginEventCommand& command) override;
  void post(ID3D12GraphicsCommandListEndEventCommand& command) override;
  void post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& command) override;
  void post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& command) override;
  void post(ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& command) override;
  void post(ID3D12GraphicsCommandList1SetSamplePositionsCommand& command) override;
  void post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& command) override;
  void post(ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& command) override;
  void post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& command) override;
  void post(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
                command) override;
  void post(ID3D12GraphicsCommandList4EndRenderPassCommand& command) override;
  void post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& command) override;
  void post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) override;
  void post(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void post(ID3D12GraphicsCommandList5RSSetShadingRateCommand& command) override;
  void post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& command) override;
  void post(ID3D12GraphicsCommandList6DispatchMeshCommand& command) override;
  void post(ID3D12GraphicsCommandList7BarrierCommand& command) override;
  void post(xessD3D12CreateContextCommand& c) override;
  void post(xessD3D12InitCommand& c) override;
  void pre(xessDestroyContextCommand& c) override;
  void post(xessDestroyContextCommand& c) override;
  void post(xessSetJitterScaleCommand& c) override;
  void post(xessSetVelocityScaleCommand& c) override;
  void post(xessSetExposureMultiplierCommand& c) override;
  void post(xessForceLegacyScaleFactorsCommand& c) override;
  void post(DStorageGetFactoryCommand& c) override;
  void post(IDStorageFactoryOpenFileCommand& c) override;
  void post(IDStorageFactoryCreateQueueCommand& c) override;
  void post(IDStorageFactoryCreateStatusArrayCommand& c) override;
  void post(NvAPI_InitializeCommand& c) override;
  void post(NvAPI_UnloadCommand& c) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;

private:
  void setAsChildInParent(unsigned parentKey, unsigned childKey);
  bool isResourceHeapMappable(const D3D12_HEAP_PROPERTIES& heapProperties,
                              const D3D12_TEXTURE_LAYOUT& textureLayout) {
    return !(heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT ||
             heapProperties.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE ||
             textureLayout == D3D12_TEXTURE_LAYOUT_UNKNOWN);
  }
  bool isResourceHeapMappable(unsigned heapKey, const D3D12_TEXTURE_LAYOUT& textureLayout);
  bool isResourceBarrierRestricted(D3D12_RESOURCE_FLAGS flags);
  void releaseSwapChainBuffers(unsigned key, unsigned referenceCount);

private:
  bool stateRestored_{};
  std::map<unsigned, unsigned> deviceByINTCExtensionContext_;
  StateTrackingService stateService_;
  SubcaptureRecorder& recorder_;
  SubcaptureRange& subcaptureRange_;
  AnalyzerResults analyzerResults_;
  FenceTrackingService fenceTrackingService_;
  MapStateService mapStateService_;
  ResourceStateTrackingService resourceStateTrackingService_;
  HeapAllocationStateService heapAllocationStateService_;
  ReservedResourcesService reservedResourcesService_;
  DescriptorService descriptorService_;
  CommandListService commandListService_;
  CommandQueueService commandQueueService_;
  XessStateService xessStateService_;
  AccelerationStructuresSerializeService accelerationStructuresSerializeService_;
  AccelerationStructuresBuildService accelerationStructuresBuildService_;
  ResidencyService residencyService_;
  ResourceUsageTrackingService resourceUsageTrackingService_;
  GpuExecutionFlusher gpuExecutionFlusher_;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> resourceHeaps_;
  std::unordered_map<unsigned, std::vector<unsigned>> swapchainBuffers_;
};

} // namespace DirectX
} // namespace gits
