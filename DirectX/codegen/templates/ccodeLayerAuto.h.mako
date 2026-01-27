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

  // Custom command post postprocess
  void postProcess(ccode::CommandPrinter& cmdPrinter, auto& command) {}
  void postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12FenceGetCompletedValueCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12DeviceCreateDescriptorHeapCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12ResourceMapCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreateCommittedResourceCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device4CreateCommittedResource1Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device8CreateCommittedResource2Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreateCommittedResource3Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreateReservedResourceCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                              ID3D12Device4CreateReservedResource1Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreateReservedResource2Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreatePlacedResourceCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device8CreatePlacedResource1Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreatePlacedResource2Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12DeviceCreateHeapCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device4CreateHeap1Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter, INTC_D3D12_CreateHeapCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                   ID3D12Device3OpenExistingHeapFromAddressCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                   ID3D12Device13OpenExistingHeapFromAddress1Command& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                   ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                   ID3D12PipelineLibraryLoadComputePipelineCommand& command);
  void postProcess(ccode::CommandPrinter& cmdPrinter,
                   ID3D12PipelineLibrary1LoadPipelineCommand& command);
};

} // namespace DirectX
} // namespace gits