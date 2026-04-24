// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "pipelineLibraryService.h"

#include <wrl/client.h>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class PlayerManager;

class ReplayCustomizationLayer : public Layer {
public:
  ReplayCustomizationLayer(PlayerManager& manager);
  void Post(IUnknownReleaseCommand& command) override;
  void Post(IUnknownAddRefCommand& command) override;
  void Pre(D3D12CreateDeviceCommand& command) override;
  void Pre(IDXGISwapChainSetFullscreenStateCommand& command) override;
  void Pre(IDXGIFactoryCreateSwapChainCommand& command) override;
  void Pre(IDXGIFactory2CreateSwapChainForHwndCommand& command) override;
  void Pre(IDXGIFactoryMakeWindowAssociationCommand& command) override;
  void Post(ID3D12ResourceMapCommand& command) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& command) override;
  void Pre(ID3D12DeviceCreateRenderTargetViewCommand& command) override;
  void Pre(ID3D12DeviceCreateShaderResourceViewCommand& command) override;
  void Pre(ID3D12DeviceCreateUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateDepthStencilViewCommand& command) override;
  void Pre(ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12DeviceCreateSamplerCommand& command) override;
  void Pre(ID3D12Device11CreateSampler2Command& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsCommand& command) override;
  void Pre(ID3D12DeviceCopyDescriptorsSimpleCommand& command) override;
  void Pre(ID3D12FenceSetEventOnCompletionCommand& command) override;
  void Pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& command) override;
  void Pre(ID3D12FenceGetCompletedValueCommand& command) override;
  void Post(ID3D12FenceGetCompletedValueCommand& command) override;
  void Post(WaitForFenceSignaledCommand& command) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(ID3D12DeviceCreateHeapCommand& command) override;
  void Post(ID3D12Device4CreateHeap1Command& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void Pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void Pre(ID3D12DeviceCreateConstantBufferViewCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void Pre(ID3D12DeviceCheckFeatureSupportCommand& command) override;
  void Pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  void Pre(ID3D12Device1CreatePipelineLibraryCommand& command) override;
  void Pre(ID3D12ObjectGetPrivateDataCommand& command) override;
  void Pre(ID3D12ObjectSetNameCommand& command) override;
  void Pre(ID3D12ObjectSetPrivateDataCommand& command) override;
  void Pre(ID3D12ObjectSetPrivateDataInterfaceCommand& command) override;
  void Pre(ID3D12PipelineLibrarySerializeCommand& command) override;
  void Pre(ID3D12PipelineLibraryGetSerializedSizeCommand& command) override;
  void Pre(ID3D12PipelineLibraryStorePipelineCommand& command) override;
  void Pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void Pre(ID3D12DeviceCreateComputePipelineStateCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void Pre(ID3D12Device2CreatePipelineStateCommand& command) override;
  void Pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;
  void Pre(ID3D12PipelineStateGetCachedBlobCommand& command) override;
  void Pre(IDXGIAdapter3RegisterVideoMemoryBudgetChangeNotificationEventCommand& command) override;
  void Pre(ID3D12DeviceGetAdapterLuidCommand& command) override;
  void Post(ID3D12DeviceGetAdapterLuidCommand& command) override;
  void Pre(IDXGIAdapterEnumOutputsCommand& command) override;
  void Pre(IDXGIAdapterGetDescCommand& command) override;
  void Post(IDXGIAdapterGetDescCommand& command) override;
  void Pre(IDXGIAdapterCheckInterfaceSupportCommand& command) override;
  void Pre(IDXGIAdapter1GetDesc1Command& command) override;
  void Post(IDXGIAdapter1GetDesc1Command& command) override;
  void Pre(IDXGIAdapter2GetDesc2Command& command) override;
  void Post(IDXGIAdapter2GetDesc2Command& command) override;
  void Pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& command) override;
  void Pre(
      IDXGIAdapter3RegisterHardwareContentProtectionTeardownStatusEventCommand& command) override;
  void Pre(IDXGIAdapter3SetVideoMemoryReservationCommand& command) override;
  void Pre(IDXGIAdapter3UnregisterHardwareContentProtectionTeardownStatusCommand& command) override;
  void Pre(IDXGIAdapter3UnregisterVideoMemoryBudgetChangeNotificationCommand& command) override;
  void Pre(IDXGIAdapter4GetDesc3Command& command) override;
  void Post(IDXGIAdapter4GetDesc3Command& command) override;
  void Pre(IDXGIFactory4EnumAdapterByLuidCommand& command) override;
  void Pre(IDXGIOutputFindClosestMatchingModeCommand& command) override;
  void Pre(IDXGIOutputGetDescCommand& command) override;
  void Pre(IDXGIOutputGetDisplayModeListCommand& command) override;
  void Pre(IDXGIOutputGetDisplaySurfaceDataCommand& command) override;
  void Pre(IDXGIOutputGetFrameStatisticsCommand& command) override;
  void Pre(IDXGIOutputGetGammaControlCommand& command) override;
  void Pre(IDXGIOutputGetGammaControlCapabilitiesCommand& command) override;
  void Pre(IDXGIOutputReleaseOwnershipCommand& command) override;
  void Pre(IDXGIOutputSetDisplaySurfaceCommand& command) override;
  void Pre(IDXGIOutputSetGammaControlCommand& command) override;
  void Pre(IDXGIOutputTakeOwnershipCommand& command) override;
  void Pre(IDXGIOutputWaitForVBlankCommand& command) override;
  void Pre(IDXGIOutput1DuplicateOutputCommand& command) override;
  void Pre(IDXGIOutput1FindClosestMatchingMode1Command& command) override;
  void Pre(IDXGIOutput1GetDisplayModeList1Command& command) override;
  void Pre(IDXGIOutput1GetDisplaySurfaceData1Command& command) override;
  void Pre(IDXGIOutput2SupportsOverlaysCommand& command) override;
  void Pre(IDXGIOutput3CheckOverlaySupportCommand& command) override;
  void Pre(IDXGIOutput4CheckOverlayColorSpaceSupportCommand& command) override;
  void Pre(IDXGIOutput5DuplicateOutput1Command& command) override;
  void Pre(IDXGIOutput6CheckHardwareCompositionSupportCommand& command) override;
  void Pre(IDXGIOutput6GetDesc1Command& command) override;
  void Pre(IDXGIInfoQueueAddStorageFilterEntriesCommand& command) override;
  void Pre(ID3D12InfoQueueAddStorageFilterEntriesCommand& command) override;
  void Pre(ID3D12InfoQueuePushStorageFilterCommand& command) override;
  void Pre(ID3D12Device12GetResourceAllocationInfo3Command& command) override;
  void Pre(ID3D12GraphicsCommandListResolveQueryDataCommand& command) override;
  void Pre(ID3D12DeviceCreateCommandQueueCommand& command) override;
  void Pre(ID3D12DeviceCreateCommandAllocatorCommand& command) override;
  void Pre(ID3D12DeviceCreateCommandListCommand& command) override;
  void Pre(IDMLDeviceCreateBindingTableCommand& command) override;
  void Pre(IDMLBindingTableResetCommand& command) override;
  void Pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
               command) override;
  void Pre(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void Pre(ID3D12DeviceOpenSharedHandleCommand& command) override;
  void Pre(ID3DBlobGetBufferPointerCommand& command) override;
  void Pre(ID3DBlobGetBufferSizeCommand& command) override;
  void Pre(ID3D12SDKConfigurationSetSDKVersionCommand& command) override;
  void Pre(ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) override;
  void Pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void Pre(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void Post(CreateDXGIFactory2Command& command) override;
  void Pre(xefgSwapChainD3D12InitFromSwapChainDescCommand& command) override;
  void Pre(xefgSwapChainSetLoggingCallbackCommand& command) override;
  void Pre(xessSetLoggingCallbackCommand& command) override;
  void Pre(xellSetLoggingCallbackCommand& command) override;

private:
  struct NvAPIShaderExtnSlot {
    unsigned DeviceKey{};
    unsigned UavSlot{};
    unsigned UavSpace{};

    bool operator==(const NvAPIShaderExtnSlot& other) const {
      return DeviceKey == other.DeviceKey && UavSlot == other.UavSlot && UavSpace == other.UavSpace;
    }
  };

private:
  void FillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
  void FillGpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg);
  void FillCpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg);
  void WaitForFence(unsigned commandKey, ID3D12Fence* fence, unsigned fenceValue);
  void RemoveCachedPso(D3D12_PIPELINE_STATE_STREAM_DESC& desc);

private:
  PlayerManager& m_Manager;
  PipelineLibraryService& m_PipelineLibraryService;
  HANDLE m_WaitForFenceEvent{};
  UINT64 m_CapturedFenceValue{};
  std::vector<NvAPIShaderExtnSlot> m_NvapiShaderExtnSlotsUsed;
  bool m_UseAddressPinning{};
  bool m_AfterAddRef{};
};

} // namespace DirectX
} // namespace gits
