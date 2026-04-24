// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <array>
#include <d3d12.h>

namespace gits {
namespace DirectX {
class CommandPreservationLayer : public Layer {
public:
  CommandPreservationLayer() : Layer("CommandPreservation") {}

  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void Post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void Pre(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) override;
  void Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) override;
  void Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) override;
  void Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) override;
  void Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) override;
  void Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) override;
  void Pre(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c)
      override;
  void Post(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c)
      override;

private:
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS> m_CaptureGpuAddresses;
  std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> m_CaptureShaderIdentifier;
  D3D12_GPU_DESCRIPTOR_HANDLE m_CaptureGpuDescriptorHandle{};
};

} // namespace DirectX
} // namespace gits
