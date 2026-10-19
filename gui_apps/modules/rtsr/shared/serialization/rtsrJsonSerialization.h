// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsrSerdeTypes.h"

#include <nlohmann/json.hpp>

// Internal to the serialization module: the D3D12 descriptor to JSON mapping
// RTSR and RTSREditor agreed on. Key names and nesting are part of the wire
// format. SerializePipelineState() in rtsrStateSerialization.h is the entry
// point callers use.
//
// The conversions are named functions instead of nlohmann to_json overloads
// because the descriptors live in the global namespace, where argument
// dependent lookup would not find anything declared in gits::rtsr.
namespace gits::rtsr::detail {

nlohmann::json ToJson(const D3D12_SHADER_BYTECODE& bytecode);
nlohmann::json ToJson(const D3D12_SO_DECLARATION_ENTRY& entry);
nlohmann::json ToJson(const D3D12_STREAM_OUTPUT_DESC& desc);
nlohmann::json ToJson(const D3D12_RENDER_TARGET_BLEND_DESC& desc);
nlohmann::json ToJson(const D3D12_BLEND_DESC& desc);
nlohmann::json ToJson(const D3D12_RASTERIZER_DESC& desc);
nlohmann::json ToJson(const D3D12_DEPTH_STENCILOP_DESC& desc);
nlohmann::json ToJson(const D3D12_DEPTH_STENCIL_DESC& desc);
nlohmann::json ToJson(const D3D12_INPUT_ELEMENT_DESC& element);
nlohmann::json ToJson(const D3D12_INPUT_LAYOUT_DESC& desc);
nlohmann::json ToJson(const DXGI_SAMPLE_DESC& desc);
nlohmann::json ToJson(const D3D12_DESCRIPTOR_RANGE& range);
nlohmann::json ToJson(const D3D12_DESCRIPTOR_RANGE1& range);
nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR_TABLE& table);
nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR_TABLE1& table);
nlohmann::json ToJson(const D3D12_ROOT_CONSTANTS& constants);
nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR& descriptor);
nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR1& descriptor);
nlohmann::json ToJson(const D3D12_ROOT_PARAMETER& parameter);
nlohmann::json ToJson(const D3D12_ROOT_PARAMETER1& parameter);
nlohmann::json ToJson(const D3D12_STATIC_SAMPLER_DESC& sampler);
nlohmann::json ToJson(const D3D12_STATIC_SAMPLER_DESC1& sampler);
nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC& desc);
nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC1& desc);
nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC2& desc);
nlohmann::json ToJson(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);
nlohmann::json ToJson(const D3D12_CACHED_PIPELINE_STATE& desc);
nlohmann::json ToJson(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
nlohmann::json ToJson(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

// Throws std::invalid_argument when the descriptors are inconsistent, for
// example an array pointer that is null while its element count is not.
nlohmann::json ToJson(const PipelineStateInfo& info);

} // namespace gits::rtsr::detail
