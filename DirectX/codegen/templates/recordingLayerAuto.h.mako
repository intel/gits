// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "subcaptureRecorder.h"
#include "subcaptureRange.h"

namespace gits {
namespace DirectX {

class RecordingLayer : public Layer {
public:
  RecordingLayer(SubcaptureRecorder& recorder, SubcaptureRange& subcaptureRange) 
      : Layer("Recording"), recorder_(recorder), subcaptureRange_(subcaptureRange) {}
  ~RecordingLayer();

  RecordingLayer(const RecordingLayer&) = delete;
  RecordingLayer& operator=(const RecordingLayer&) = delete;

  void post(CreateWindowMetaCommand& command) override;
  void post(MappedDataMetaCommand& command) override;
  void post(CreateHeapAllocationMetaCommand& command) override;
  void post(WaitForFenceSignaledCommand& command) override;
  void post(IUnknownQueryInterfaceCommand& command) override;
  void post(IUnknownAddRefCommand& command) override;
  void post(IUnknownReleaseCommand& command) override;
  %for function in functions:
  void post(${function.name}Command& command) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void post(${interface.name}${function.name}Command& command) override;
  %endfor
  %endfor
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void post(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void post(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void post(INTC_D3D12_CreateHeapCommand& command) override;
  void post(NvAPI_InitializeCommand& command) override;
  void post(NvAPI_UnloadCommand& command) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

private:
  SubcaptureRecorder& recorder_;
  SubcaptureRange& subcaptureRange_;
};

} // namespace DirectX
} // namespace gits
