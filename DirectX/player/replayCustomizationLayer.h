// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  void post(IUnknownReleaseCommand& command) override;
  void post(IUnknownAddRefCommand& command) override;
  void pre(D3D12CreateDeviceCommand& command) override;
  void pre(IDXGISwapChainSetFullscreenStateCommand& command) override;
  void pre(IDXGIFactoryCreateSwapChainCommand& command) override;
  void pre(IDXGIFactory2CreateSwapChainForHwndCommand& command) override;
  void pre(IDXGIFactoryMakeWindowAssociationCommand& command) override;
  void post(ID3D12ResourceMapCommand& command) override;
  void post(ID3D12DeviceCreateDescriptorHeapCommand& command) override;
  void pre(ID3D12DeviceCreateRenderTargetViewCommand& command) override;
  void pre(ID3D12DeviceCreateShaderResourceViewCommand& command) override;
  void pre(ID3D12DeviceCreateUnorderedAccessViewCommand& command) override;
  void pre(ID3D12DeviceCreateDepthStencilViewCommand& command) override;
  void pre(ID3D12Device8CreateSamplerFeedbackUnorderedAccessViewCommand& command) override;
  void pre(ID3D12DeviceCreateSamplerCommand& command) override;
  void pre(ID3D12Device11CreateSampler2Command& command) override;
  void pre(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& command) override;
  void pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void pre(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void pre(ID3D12DeviceCopyDescriptorsCommand& command) override;
  void pre(ID3D12DeviceCopyDescriptorsSimpleCommand& command) override;
  void pre(ID3D12FenceSetEventOnCompletionCommand& command) override;
  void pre(ID3D12Device1SetEventOnMultipleFenceCompletionCommand& command) override;
  void pre(ID3D12FenceGetCompletedValueCommand& command) override;
  void pre(WaitForFenceSignaledCommand& command) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void post(ID3D12Device4CreateReservedResource1Command& command) override;
  void post(ID3D12Device10CreateReservedResource2Command& command) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void post(ID3D12DeviceCreateHeapCommand& command) override;
  void post(ID3D12Device4CreateHeap1Command& command) override;
  void post(INTC_D3D12_CreateHeapCommand& command) override;
  void pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) override;
  void pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) override;
  void pre(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void pre(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void pre(ID3D12DeviceCreateConstantBufferViewCommand& command) override;
  void pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& command) override;
  void pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& command) override;
  void pre(ID3D12GraphicsCommandListSOSetTargetsCommand& command) override;
  void pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& command) override;
  void pre(ID3D12DeviceCheckFeatureSupportCommand& command) override;
  void pre(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) override;
  void pre(ID3D12Device1CreatePipelineLibraryCommand& command) override;
  void pre(ID3D12ObjectGetPrivateDataCommand& command) override;
  void pre(ID3D12ObjectSetNameCommand& command) override;
  void pre(ID3D12ObjectSetPrivateDataCommand& command) override;
  void pre(ID3D12ObjectSetPrivateDataInterfaceCommand& command) override;
  void pre(ID3D12PipelineLibrarySerializeCommand& command) override;
  void pre(ID3D12PipelineLibraryGetSerializedSizeCommand& command) override;
  void pre(ID3D12PipelineLibraryStorePipelineCommand& command) override;
  void pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void pre(ID3D12DeviceCreateComputePipelineStateCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void pre(ID3D12Device2CreatePipelineStateCommand& command) override;
  void pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;
  void pre(ID3D12PipelineStateGetCachedBlobCommand& command) override;
  void pre(IDXGIAdapter3RegisterVideoMemoryBudgetChangeNotificationEventCommand& command) override;
  void pre(ID3D12DeviceGetAdapterLuidCommand& command) override;
  void post(ID3D12DeviceGetAdapterLuidCommand& command) override;
  void pre(IDXGIAdapterEnumOutputsCommand& command) override;
  void pre(IDXGIAdapterGetDescCommand& command) override;
  void post(IDXGIAdapterGetDescCommand& command) override;
  void pre(IDXGIAdapterCheckInterfaceSupportCommand& command) override;
  void pre(IDXGIAdapter1GetDesc1Command& command) override;
  void post(IDXGIAdapter1GetDesc1Command& command) override;
  void pre(IDXGIAdapter2GetDesc2Command& command) override;
  void post(IDXGIAdapter2GetDesc2Command& command) override;
  void pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& command) override;
  void pre(
      IDXGIAdapter3RegisterHardwareContentProtectionTeardownStatusEventCommand& command) override;
  void pre(IDXGIAdapter3SetVideoMemoryReservationCommand& command) override;
  void pre(IDXGIAdapter3UnregisterHardwareContentProtectionTeardownStatusCommand& command) override;
  void pre(IDXGIAdapter3UnregisterVideoMemoryBudgetChangeNotificationCommand& command) override;
  void pre(IDXGIAdapter4GetDesc3Command& command) override;
  void post(IDXGIAdapter4GetDesc3Command& command) override;
  void pre(IDXGIFactory4EnumAdapterByLuidCommand& command) override;
  void pre(IDXGIOutputFindClosestMatchingModeCommand& command) override;
  void pre(IDXGIOutputGetDescCommand& command) override;
  void pre(IDXGIOutputGetDisplayModeListCommand& command) override;
  void pre(IDXGIOutputGetDisplaySurfaceDataCommand& command) override;
  void pre(IDXGIOutputGetFrameStatisticsCommand& command) override;
  void pre(IDXGIOutputGetGammaControlCommand& command) override;
  void pre(IDXGIOutputGetGammaControlCapabilitiesCommand& command) override;
  void pre(IDXGIOutputReleaseOwnershipCommand& command) override;
  void pre(IDXGIOutputSetDisplaySurfaceCommand& command) override;
  void pre(IDXGIOutputSetGammaControlCommand& command) override;
  void pre(IDXGIOutputTakeOwnershipCommand& command) override;
  void pre(IDXGIOutputWaitForVBlankCommand& command) override;
  void pre(IDXGIOutput1DuplicateOutputCommand& command) override;
  void pre(IDXGIOutput1FindClosestMatchingMode1Command& command) override;
  void pre(IDXGIOutput1GetDisplayModeList1Command& command) override;
  void pre(IDXGIOutput1GetDisplaySurfaceData1Command& command) override;
  void pre(IDXGIOutput2SupportsOverlaysCommand& command) override;
  void pre(IDXGIOutput3CheckOverlaySupportCommand& command) override;
  void pre(IDXGIOutput4CheckOverlayColorSpaceSupportCommand& command) override;
  void pre(IDXGIOutput5DuplicateOutput1Command& command) override;
  void pre(IDXGIOutput6CheckHardwareCompositionSupportCommand& command) override;
  void pre(IDXGIOutput6GetDesc1Command& command) override;
  void pre(IDXGIInfoQueueAddStorageFilterEntriesCommand& command) override;
  void pre(ID3D12InfoQueueAddStorageFilterEntriesCommand& command) override;
  void pre(ID3D12InfoQueuePushStorageFilterCommand& command) override;
  void pre(ID3D12Device12GetResourceAllocationInfo3Command& command) override;
  void pre(ID3D12GraphicsCommandListResolveQueryDataCommand& command) override;
  void pre(ID3D12DeviceCreateCommandQueueCommand& command) override;
  void pre(ID3D12DeviceCreateCommandAllocatorCommand& command) override;
  void pre(ID3D12DeviceCreateCommandListCommand& command) override;
  void pre(IDMLDeviceCreateBindingTableCommand& command) override;
  void pre(IDMLBindingTableResetCommand& command) override;
  void pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BeginRenderPassCommand& command) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&
               command) override;
  void pre(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void pre(ID3D12DeviceOpenSharedHandleCommand& command) override;
  void pre(ID3DBlobGetBufferPointerCommand& command) override;
  void pre(ID3DBlobGetBufferSizeCommand& command) override;
  void pre(ID3D12SDKConfigurationSetSDKVersionCommand& command) override;
  void pre(ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand& command) override;
  void pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void pre(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void post(CreateDXGIFactory2Command& command) override;

private:
  struct NvAPIShaderExtnSlot {
    unsigned deviceKey;
    unsigned uavSlot;
    unsigned uavSpace;

    bool operator==(const NvAPIShaderExtnSlot& other) const {
      return deviceKey == other.deviceKey && uavSlot == other.uavSlot && uavSpace == other.uavSpace;
    }
  };

private:
  void fillGpuAddressArgument(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
  void fillGpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg);
  void fillCpuDescriptorHandleArgument(DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg);
  void waitForFence(unsigned commandKey, ID3D12Fence* fence, unsigned fenceValue);
  void removeCachedPSO(D3D12_PIPELINE_STATE_STREAM_DESC& desc);

private:
  PlayerManager& manager_;
  PipelineLibraryService& pipelineLibraryService_;
  HANDLE waitForFenceEvent_{};
  std::vector<NvAPIShaderExtnSlot> nvapiShaderExtnSlotsUsed_;
  bool useAddressPinning_{};
};

} // namespace DirectX
} // namespace gits
