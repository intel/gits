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
#include "ccodeStream.h"

namespace gits {
namespace DirectX {
namespace ccode {
class CommandPrinter;
} // namespace ccode

class CCodeLayer : public Layer {
public:
  CCodeLayer();

  // Used for determining code blocks 
  void pre(StateRestoreBeginCommand& command);
  void pre(StateRestoreEndCommand& command);
  void pre(IDXGISwapChainPresentCommand& command);
  void pre(IDXGISwapChain1Present1Command& command);
  
  // Used for extra metadata
  void post(CreateWindowMetaCommand& command);
  void post(WaitForFenceSignaledCommand& command);
  void post(MappedDataMetaCommand& command);
  void post(CreateHeapAllocationMetaCommand& command);

  // Intel Extensions
  void post(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void post(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void post(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void post(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void post(INTC_D3D12_CreateHeapCommand& command) override;

%for function in functions:
%if function.api == Api.D3D12 or function.api == Api.DXGI:
void post(${function.name}Command& command);
%endif
%endfor

%for interface in interfaces:
%if interface.api == Api.D3D12 or interface.api == Api.DXGI:
%for function in interface.functions:
void post(${interface.name}${function.name}Command& command);
%endfor
%endif
%endfor

private:
  void nextCommand();
  bool isStateRestore_{false};
  unsigned currentFrame_{0};

  // Custom command pre process
  // Used before arguments have been processed
  void preProcess(ccode::CommandPrinter& p, auto& c) {}
  void preProcess(ccode::CommandPrinter& p, ID3D12FenceSetEventOnCompletionCommand& c);
  void preProcess(ccode::CommandPrinter& p, ID3D12ObjectGetPrivateDataCommand& c);
  void preProcess(ccode::CommandPrinter& p, ID3D12ObjectSetPrivateDataCommand& c);
  
  // Custom command post process
  // Used after the arguments have been processed and the command is ready to be printed
  void postProcess(ccode::CommandPrinter& p, auto& c) {}
  void postProcess(ccode::CommandPrinter& p, ID3D12FenceGetCompletedValueCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateDescriptorHeapCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12ResourceMapCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateCommittedResourceCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device4CreateCommittedResource1Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device8CreateCommittedResource2Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device10CreateCommittedResource3Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateReservedResourceCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device4CreateReservedResource1Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device10CreateReservedResource2Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreatePlacedResourceCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device8CreatePlacedResource1Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device10CreatePlacedResource2Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateHeapCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device4CreateHeap1Command& c);
  void postProcess(ccode::CommandPrinter& p, INTC_D3D12_CreateHeapCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device3OpenExistingHeapFromAddressCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12Device13OpenExistingHeapFromAddress1Command& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12PipelineLibraryLoadComputePipelineCommand& c);
  void postProcess(ccode::CommandPrinter& p, ID3D12PipelineLibrary1LoadPipelineCommand& c);
};

} // namespace DirectX
} // namespace gits