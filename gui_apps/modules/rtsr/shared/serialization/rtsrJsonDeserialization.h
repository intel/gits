// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsrPipelineStateStorage.h"
#include "rtsrSerdeTypes.h"

#include <nlohmann/json.hpp>

// Internal to the serialization module: the inverse of rtsrJsonSerialization.h.
// DeserializePipelineState() in rtsrStateSerialization.h is the entry point
// callers use.
//
// Everything a descriptor points to is allocated from the storage passed in,
// which is what replaces RTSR's process wide memory pool. The overloads that do
// not take one describe nothing but plain values. Targets are expected to be
// value initialized, fields the wire format does not carry (the root signature
// object of a pipeline state, the shader bytecode) are left untouched.
//
// All overloads throw on malformed input: nlohmann::json exceptions for missing
// or mistyped fields, std::invalid_argument for values that are well formed but
// meaningless, such as an unknown root parameter type.
namespace gits::rtsr::detail {

void FromJson(const nlohmann::json& serialized, D3D12_SHADER_BYTECODE& bytecode);
void FromJson(const nlohmann::json& serialized,
              D3D12_SO_DECLARATION_ENTRY& entry,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_STREAM_OUTPUT_DESC& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, D3D12_RENDER_TARGET_BLEND_DESC& desc);
void FromJson(const nlohmann::json& serialized, D3D12_BLEND_DESC& desc);
void FromJson(const nlohmann::json& serialized, D3D12_RASTERIZER_DESC& desc);
void FromJson(const nlohmann::json& serialized, D3D12_DEPTH_STENCILOP_DESC& desc);
void FromJson(const nlohmann::json& serialized, D3D12_DEPTH_STENCIL_DESC& desc);
void FromJson(const nlohmann::json& serialized,
              D3D12_INPUT_ELEMENT_DESC& element,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_INPUT_LAYOUT_DESC& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, DXGI_SAMPLE_DESC& desc);
void FromJson(const nlohmann::json& serialized, D3D12_DESCRIPTOR_RANGE& range);
void FromJson(const nlohmann::json& serialized, D3D12_DESCRIPTOR_RANGE1& range);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_DESCRIPTOR_TABLE& table,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_DESCRIPTOR_TABLE1& table,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, D3D12_ROOT_CONSTANTS& constants);
void FromJson(const nlohmann::json& serialized, D3D12_ROOT_DESCRIPTOR& descriptor);
void FromJson(const nlohmann::json& serialized, D3D12_ROOT_DESCRIPTOR1& descriptor);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_PARAMETER& parameter,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_PARAMETER1& parameter,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, D3D12_STATIC_SAMPLER_DESC& sampler);
void FromJson(const nlohmann::json& serialized, D3D12_STATIC_SAMPLER_DESC1& sampler);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC1& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC2& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized,
              D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, D3D12_CACHED_PIPELINE_STATE& desc);
void FromJson(const nlohmann::json& serialized,
              D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
              PipelineStateStorage& storage);
void FromJson(const nlohmann::json& serialized, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);
void FromJson(const nlohmann::json& serialized,
              PipelineStateInfo& info,
              PipelineStateStorage& storage);

} // namespace gits::rtsr::detail
