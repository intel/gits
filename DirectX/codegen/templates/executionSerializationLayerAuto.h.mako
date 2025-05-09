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
#include "executionSerializationRecorder.h"
#include "commandListExecutionService.h"

namespace gits {
namespace DirectX {

class ExecutionSerializationLayer : public Layer {
public:
  ExecutionSerializationLayer(ExecutionSerializationRecorder& recorder) 
      : Layer("ExecutionSerialization"), recorder_(recorder), executionService_(recorder) {}
  void pre(CreateWindowMetaCommand& c) override;
  void pre(MappedDataMetaCommand& c) override;
  void pre(CreateHeapAllocationMetaCommand& c) override;
  void pre(WaitForFenceSignaledCommand& c) override;
  void pre(IUnknownQueryInterfaceCommand& c) override;
  void pre(IUnknownAddRefCommand& c) override;
  void pre(IUnknownReleaseCommand& c) override;
  %for function in functions:
  void pre(${function.name}Command& c) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void pre(${interface.name}${function.name}Command& c) override;
  %endfor
  %endfor
  void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void pre(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void pre(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void pre(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void pre(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void pre(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void pre(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void pre(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void pre(INTC_D3D12_CreateHeapCommand& c) override;

private:
  ExecutionSerializationRecorder& recorder_;
  CommandListExecutionService executionService_;
};

} // namespace DirectX
} // namespace gits
