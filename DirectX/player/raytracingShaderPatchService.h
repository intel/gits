// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class RaytracingShaderPatchService {
public:
  RaytracingShaderPatchService();
  ~RaytracingShaderPatchService();
  RaytracingShaderPatchService(const RaytracingShaderPatchService&) = delete;
  RaytracingShaderPatchService& operator=(const RaytracingShaderPatchService&) = delete;

  void PatchInstances(ID3D12GraphicsCommandList* commandList,
                      D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
                      unsigned instancesCount,
                      D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                      D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);
  void PatchInstancesOffset(ID3D12GraphicsCommandList* commandList,
                            D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
                            D3D12_GPU_VIRTUAL_ADDRESS instancesOffsetsBuffer,
                            unsigned instancesCount,
                            D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                            D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);
  void PatchBindingTable(ID3D12GraphicsCommandList* commandList,
                         D3D12_GPU_VIRTUAL_ADDRESS bindingTableBuffer,
                         unsigned recordCount,
                         unsigned recordSize,
                         D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS shaderIdentiferBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS viewDescriptorBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS sampleDescriptorBuffer,
                         D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer,
                         bool patchGpuAdresses);

private:
  void InitializeInstances(ID3D12Device* device);
  void InitializeInstancesOffset(ID3D12Device* device);
  void InitializeBindingTable(ID3D12Device* device);
  void InitializePipelineState(const std::string& shaderCode,
                               ID3D12Device* device,
                               ID3D12RootSignature* rootSignature,
                               ID3D12PipelineState** pipelineState);

private:
  ID3D12RootSignature* m_InstancesRootSignature{};
  ID3D12PipelineState* m_InstancesPipelineState{};
  ID3D12RootSignature* m_InstancesOffsetRootSignature{};
  ID3D12PipelineState* m_InstancesOffsetPipelineState{};
  ID3D12RootSignature* m_BindingTableRootSignature{};
  ID3D12PipelineState* m_BindingTablePipelineState{};

  HMODULE m_DxilDll;
  HMODULE m_DxcDll;
};

} // namespace DirectX
} // namespace gits
