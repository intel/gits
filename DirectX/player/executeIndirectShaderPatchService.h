// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"

#include <d3d12.h>

namespace gits {
namespace DirectX {

class ExecuteIndirectShaderPatchService : public gits::noncopyable {
public:
  ExecuteIndirectShaderPatchService();
  ~ExecuteIndirectShaderPatchService();
  void patchArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                           D3D12_GPU_VIRTUAL_ADDRESS argumentBuffer,
                           D3D12_GPU_VIRTUAL_ADDRESS countBuffer,
                           D3D12_GPU_VIRTUAL_ADDRESS patchOffsetsBuffer,
                           unsigned patchOffsetsCount,
                           D3D12_GPU_VIRTUAL_ADDRESS raytracingPatchBuffer,
                           unsigned raytracingPatchCount,
                           unsigned maxArgumentCount,
                           unsigned commandStride,
                           D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                           D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer);

private:
  void initialize(ID3D12Device* device);

private:
  ID3D12RootSignature* rootSignature_{};
  ID3D12PipelineState* pipelineState_{};

  HMODULE dxilDll_;
  HMODULE dxcDll_;
};

} // namespace DirectX
} // namespace gits
