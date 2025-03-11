// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(ID3D12ObjectSetNameCommand& c) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void post(ID3D12Device4CreateReservedResource1Command& c) override;
  void post(ID3D12Device10CreateReservedResource2Command& c) override;
  void post(ID3D12DeviceCreateHeapCommand& c) override;
  void post(ID3D12Device4CreateHeap1Command& c) override;
  void post(ID3D12DeviceCreateQueryHeapCommand& c) override;
  void post(ID3D12DeviceCreateCommandListCommand& c) override;
  void post(ID3D12Device4CreateCommandList1Command& c) override;
  void post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void post(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void post(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void post(ID3D12Device5CreateStateObjectCommand& c) override;
  void post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) override;
  void post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) override;
  void post(ID3D12PipelineLibrary1LoadPipelineCommand& c) override;
  void post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) override;
  void post(IDXGISwapChainGetBufferCommand& c) override;

private:
  void setD3D12ObjectName(void* object, unsigned key);
  void setDXGIObjectName(void* object, unsigned key);

private:
  bool multithreadedShaderCompilation_;
};

} // namespace DirectX
} // namespace gits
