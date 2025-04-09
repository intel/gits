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

namespace gits {
namespace DirectX {

class RecordingLayer : public Layer {
public:
  RecordingLayer(SubcaptureRecorder& recorder) 
      : Layer("Recording"), recorder_(recorder) {}

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
  void post(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void post(INTC_D3D12_CreateHeapCommand& command) override;

private:
  SubcaptureRecorder& recorder_;
};

} // namespace DirectX
} // namespace gits
