// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"

#include <d3d12.h>

namespace gits {
namespace DirectX {

class RaytracingShaderPatchService : public gits::noncopyable {
public:
  RaytracingShaderPatchService();
  ~RaytracingShaderPatchService();
  void patchInstances(ID3D12GraphicsCommandList* commandList,
                      D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
                      unsigned instancesCount,
                      D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                      D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);
  void patchInstancesOffset(ID3D12GraphicsCommandList* commandList,
                            D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
                            D3D12_GPU_VIRTUAL_ADDRESS instancesOffsetsBuffer,
                            unsigned instancesCount,
                            D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                            D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);
  void patchBindingTable(ID3D12GraphicsCommandList* commandList,
                         D3D12_GPU_VIRTUAL_ADDRESS bindingTableBuffer,
                         unsigned recordCount,
                         unsigned recordSize,
                         D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS shaderIdentiferBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS viewDescriptorBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS sampleDescriptorBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);

private:
  void initializeInstances(ID3D12Device* device);
  void initializeInstancesOffset(ID3D12Device* device);
  void initializeBindingTable(ID3D12Device* device);
  void initializePipelineState(const std::string& shaderCode,
                               ID3D12Device* device,
                               ID3D12RootSignature* rootSignature,
                               ID3D12PipelineState** pipelineState);

private:
  ID3D12RootSignature* instancesRootSignature_{};
  ID3D12PipelineState* instancesPipelineState_{};
  ID3D12RootSignature* instancesOffsetRootSignature_{};
  ID3D12PipelineState* instancesOffsetPipelineState_{};
  ID3D12RootSignature* bindingTableRootSignature_{};
  ID3D12PipelineState* bindingTablePipelineState_{};

  HMODULE dxilDll_;
  HMODULE dxcDll_;
};

} // namespace DirectX
} // namespace gits
