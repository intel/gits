// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"

#include <string>

namespace gits {
namespace DirectX {

class Layer {
public:
  Layer(const std::string& name) : name_(name) {}
  virtual ~Layer() {}

  const std::string& GetName() const {
    return name_;
  }

  virtual void pre(CreateWindowMetaCommand& command) {}
  virtual void post(CreateWindowMetaCommand& command) {}

  virtual void pre(MappedDataMetaCommand& command) {}
  virtual void post(MappedDataMetaCommand& command) {}

  virtual void pre(CreateHeapAllocationMetaCommand& command) {}
  virtual void post(CreateHeapAllocationMetaCommand& command) {}

  virtual void pre(WaitForFenceSignaledCommand& command) {}
  virtual void post(WaitForFenceSignaledCommand& command) {}

  virtual void pre(IUnknownQueryInterfaceCommand& command) {}
  virtual void post(IUnknownQueryInterfaceCommand& command) {}

  virtual void pre(IUnknownAddRefCommand& command) {}
  virtual void post(IUnknownAddRefCommand& command) {}

  virtual void pre(IUnknownReleaseCommand& command) {}
  virtual void post(IUnknownReleaseCommand& command) {}

  %for function in functions:
  virtual void pre(${function.name}Command& command) {}
  virtual void post(${function.name}Command& command) {}

  %endfor

  %for interface in interfaces:
  %for function in interface.functions:
  virtual void pre(${interface.name}${function.name}Command& command) {}
  virtual void post(${interface.name}${function.name}Command& command) {}

  %endfor
  %endfor
  virtual void pre(INTC_D3D12_GetSupportedVersionsCommand& command) {}
  virtual void post(INTC_D3D12_GetSupportedVersionsCommand& command) {}

  virtual void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {}
  virtual void post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {}

  virtual void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {}
  virtual void post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {}

  virtual void pre(INTC_D3D12_SetApplicationInfoCommand& command) {}
  virtual void post(INTC_D3D12_SetApplicationInfoCommand& command) {}

  virtual void pre(INTC_DestroyDeviceExtensionContextCommand& command) {}
  virtual void post(INTC_DestroyDeviceExtensionContextCommand& command) {}

  virtual void pre(INTC_D3D12_CheckFeatureSupportCommand& command) {}
  virtual void post(INTC_D3D12_CheckFeatureSupportCommand& command) {}

  virtual void pre(INTC_D3D12_CreateCommandQueueCommand& command) {}
  virtual void post(INTC_D3D12_CreateCommandQueueCommand& command) {}

  virtual void pre(INTC_D3D12_CreateReservedResourceCommand& command) {}
  virtual void post(INTC_D3D12_CreateReservedResourceCommand& command) {}

  virtual void pre(INTC_D3D12_SetFeatureSupportCommand& command) {}
  virtual void post(INTC_D3D12_SetFeatureSupportCommand& command) {}

  virtual void pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {}
  virtual void post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {}

  virtual void pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {}
  virtual void post(INTC_D3D12_CreateComputePipelineStateCommand& command) {}

  virtual void pre(INTC_D3D12_CreatePlacedResourceCommand& command) {}
  virtual void post(INTC_D3D12_CreatePlacedResourceCommand& command) {}

  virtual void pre(INTC_D3D12_CreateCommittedResourceCommand& command) {}
  virtual void post(INTC_D3D12_CreateCommittedResourceCommand& command) {}
  
  virtual void pre(INTC_D3D12_CreateHeapCommand& command) {}
  virtual void post(INTC_D3D12_CreateHeapCommand& command) {}

private:
  std::string name_;
};

} // namespace DirectX
} // namespace gits
