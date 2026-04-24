// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "messageBus.h"
#include "statisticsService.h"

#include <string>

namespace gits {
namespace DirectX {

class StatisticsLayer : public Layer {
public:
  StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus);
  ~StatisticsLayer() = default;
  StatisticsLayer(const StatisticsLayer&) = delete;
  StatisticsLayer& operator=(const StatisticsLayer&) = delete;

  void Post(StateRestoreBeginCommand& c) override;
  void Post(StateRestoreEndCommand& c) override;
  void Post(MarkerUInt64Command& c) override;
  void Post(CreateWindowMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;
  void Post(CreateHeapAllocationMetaCommand& command) override;
  void Post(WaitForFenceSignaledCommand& command) override;
  void Post(DllContainerMetaCommand& command) override;
  void Post(IUnknownQueryInterfaceCommand& command) override;
  void Post(IUnknownAddRefCommand& command) override;
  void Post(IUnknownReleaseCommand& command) override;
  %for function in functions:
  void Post(${function.name}Command& command) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void Post(${interface.name}${function.name}Command& command) override;
  %endfor
  %endfor
  void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void Post(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void Post(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void Post(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void Post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(NvAPI_InitializeCommand& command) override;
  void Post(NvAPI_UnloadCommand& command) override;
  void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

private:
  StatisticsService m_StatisticsService;
};

} // namespace DirectX
} // namespace gits
