// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsLayerAuto.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

StatisticsLayer::StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : Layer("StatisticsPlugin"), m_StatisticsService(cfg, msgBus) {}

void StatisticsLayer::post(IDXGISwapChainPresentCommand& command) {
  m_StatisticsService.PresentCommand("IDXGISwapChain::Present",
                                     command.Flags_.value & DXGI_PRESENT_TEST,
                                     isStateRestoreKey(command.key));
}

void StatisticsLayer::post(IDXGISwapChain1Present1Command& command) {
  m_StatisticsService.PresentCommand("IDXGISwapChain1::Present1",
                                     command.PresentFlags_.value & DXGI_PRESENT_TEST,
                                     isStateRestoreKey(command.key));
}

void StatisticsLayer::post(StateRestoreBeginCommand& c) {}

void StatisticsLayer::post(StateRestoreEndCommand& c) {
  m_StatisticsService.StateRestoreEnd();
}

void StatisticsLayer::post(MarkerUInt64Command& c) {}

void StatisticsLayer::post(CreateWindowMetaCommand& command) {
  m_StatisticsService.Command("CreateWindowMeta");
}

void StatisticsLayer::post(MappedDataMetaCommand& command) {
  m_StatisticsService.Command("MappedDataMeta");
}

void StatisticsLayer::post(CreateHeapAllocationMetaCommand& command) {
  m_StatisticsService.Command("CreateHeapAllocationMeta");
}

void StatisticsLayer::post(WaitForFenceSignaledCommand& command) {
  m_StatisticsService.Command("WaitForFenceSignaled");
}

void StatisticsLayer::post(DllContainerMetaCommand& command) {
  m_StatisticsService.Command("DllContainerMeta");
}

void StatisticsLayer::post(IUnknownQueryInterfaceCommand& command) {
  m_StatisticsService.Command("IUnknown::QueryInterface");
}

void StatisticsLayer::post(IUnknownAddRefCommand& command) {
  m_StatisticsService.Command("IUnknown::AddRef");
}

void StatisticsLayer::post(IUnknownReleaseCommand& command) {
  m_StatisticsService.Command("IUnknown::Release");
}

void StatisticsLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateDeviceExtensionContext");
}

void StatisticsLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateDeviceExtensionContext1");
}

void StatisticsLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_SetApplicationInfo");
}

void StatisticsLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  m_StatisticsService.Command("INTC_DestroyDeviceExtensionContext");
}

void StatisticsLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_SetFeatureSupport");
}

void StatisticsLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateComputePipelineState");
}

void StatisticsLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreatePlacedResource");
}

void StatisticsLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateCommittedResource");
}

void StatisticsLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateReservedResource");
}

void StatisticsLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateCommandQueue");
}

void StatisticsLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  m_StatisticsService.Command("INTC_D3D12_CreateHeap");
}

void StatisticsLayer::post(NvAPI_InitializeCommand& command) {
  m_StatisticsService.Command("NvAPI_Initialize");
}

void StatisticsLayer::post(NvAPI_UnloadCommand& command) {
  m_StatisticsService.Command("NvAPI_Unload");
}

void StatisticsLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_SetCreatePipelineStateOptions");
}

void StatisticsLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_SetNvShaderExtnSlotSpace");
}

void StatisticsLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
}

void StatisticsLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
}

void StatisticsLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
}

void StatisticsLayer::post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_StatisticsService.Command("NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
}

} // namespace DirectX
} // namespace gits
