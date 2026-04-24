// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace DirectX {

class DebugHelperLayer : public Layer {
public:
  DebugHelperLayer();

  void Pre(ID3D12ObjectSetNameCommand& c) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void Post(ID3D12Device4CreateReservedResource1Command& c) override;
  void Post(ID3D12Device10CreateReservedResource2Command& c) override;
  void Post(ID3D12DeviceCreateHeapCommand& c) override;
  void Post(ID3D12Device4CreateHeap1Command& c) override;
  void Post(ID3D12DeviceCreateQueryHeapCommand& c) override;
  void Post(ID3D12DeviceCreateCommandListCommand& c) override;
  void Post(ID3D12Device4CreateCommandList1Command& c) override;
  void Post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void Post(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void Post(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void Post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void Post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void Post(ID3D12Device5CreateStateObjectCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) override;
  void Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) override;
  void Post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) override;
  void Post(IDXGISwapChainGetBufferCommand& c) override;

private:
  void SetD3D12ObjectName(void* object, unsigned key);
  void SetDXGIObjectName(void* object, unsigned key);

  bool m_MultithreadedShaderCompilation;
};

} // namespace DirectX
} // namespace gits
