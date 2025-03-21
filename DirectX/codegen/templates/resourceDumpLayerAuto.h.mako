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
#include "resourceDumpService.h"

namespace gits {
namespace DirectX {

class ResourceDumpLayer : public Layer {
public:
  ResourceDumpLayer() : Layer("ResourceDump"){};

  void post(IUnknownReleaseCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(IDXGISwapChainGetBufferCommand& c) override;
  %for interface in interfaces:
  %for function in interface.functions:
  %if function.name.startswith('CreateCommittedResource') or function.name.startswith('CreatePlacedResource') or function.name.startswith('CreateReservedResource'):
  void post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.name.startswith('ID3D12GraphicsCommandList'):
  void post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

private:
  ResourceDumpService resourceDumpService_;
};

} // namespace DirectX
} // namespace gits
