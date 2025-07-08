// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPreservationLayer.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    captureGpuAddresses_.push_back(c.pDesc_.value->Inputs.InstanceDescs);
  }
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    c.pDesc_.value->Inputs.InstanceDescs = captureGpuAddresses_[0];
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  captureGpuAddresses_.push_back(c.pDesc_.value->RayGenerationShaderRecord.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->MissShaderTable.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->HitGroupTable.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->CallableShaderTable.StartAddress);
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  c.pDesc_.value->RayGenerationShaderRecord.StartAddress = captureGpuAddresses_[0];
  c.pDesc_.value->MissShaderTable.StartAddress = captureGpuAddresses_[1];
  c.pDesc_.value->HitGroupTable.StartAddress = captureGpuAddresses_[2];
  c.pDesc_.value->CallableShaderTable.StartAddress = captureGpuAddresses_[3];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  captureGpuAddresses_.push_back(c.result_.value);
}

void CommandPreservationLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  c.result_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    captureShaderIdentifier_[i] = static_cast<uint8_t*>(c.result_.value)[i];
  }
}

void CommandPreservationLayer::post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    static_cast<uint8_t*>(c.result_.value)[i] = captureShaderIdentifier_[i];
  }
}

void CommandPreservationLayer::pre(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  captureGpuDescriptorHandle_ = c.result_.value;
}

void CommandPreservationLayer::post(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  c.result_.value = captureGpuDescriptorHandle_;
}

void CommandPreservationLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.pDesc_.cs = c.pDesc_.value->CS.pShaderBytecode;
  c.pDesc_.compileOptions = c.pDesc_.value->CompileOptions;
  c.pDesc_.internalOptions = c.pDesc_.value->InternalOptions;
}

} // namespace DirectX
} // namespace gits
