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
#include "resourceDumpService.h"

namespace gits {
namespace DirectX {

class ResourceDumpLayer : public Layer {
public:
  ResourceDumpLayer() : Layer("ResourceDump"){};

  void Post(IUnknownReleaseCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void Post(IDXGISwapChainGetBufferCommand& c) override;
  %for interface in interfaces:
  %for function in interface.functions:
  %if function.name.startswith('CreateCommittedResource') or function.name.startswith('CreatePlacedResource') or function.name.startswith('CreateReservedResource'):
  void Post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.name.startswith('ID3D12GraphicsCommandList'):
  void Post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

private:
  ResourceDumpService m_ResourceDumpService;
};

} // namespace DirectX
} // namespace gits
