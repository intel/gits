// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void pre(INTC_D3D12_CreateComputePipelineStateCommand& c) override;

private:
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS> captureGpuAddresses_;
  std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> captureShaderIdentifier_;
  D3D12_GPU_DESCRIPTOR_HANDLE captureGpuDescriptorHandle_;
};

} // namespace DirectX
} // namespace gits
