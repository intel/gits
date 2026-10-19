// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits::rtsr {

// Wire format constants shared with RTSR and RTSREditor. SERIALIZATION_VERSION
// is written as "Version" into every document header, SERIALIZED_API_DX12 is
// the numeric value of RTSR's RTSR_API_DX12 enumerator written as "Api".
// Changing either breaks compatibility with the editor.
constexpr const char* SERIALIZATION_VERSION = "1.0";
constexpr int SERIALIZED_API_DX12 = 5;

// Written as "Type" into the serialized state, values are RTSR's. Unknown is
// local to this module (empty or moved from state) and is never serialized.
enum class PipelineStateType : int {
  Unknown = 0,
  Graphics = 1,
  Compute = 2,
  MeshShader = 3
};

// Pipeline state as the serializer consumes and the deserializer produces it: a
// graphics or compute description together with the description of the root
// signature it was created with, because the root signature object itself
// cannot be serialized.
struct PipelineStateInfo {
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignatureDesc;
  PipelineStateType Type;
  union {
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pGraphicsPipelineStateDesc;
    const D3D12_COMPUTE_PIPELINE_STATE_DESC* pComputePipelineStateDesc;
    const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC* pMeshShaderPipelineStateDesc;
  };
};

// The document header advertises sizeof(PipelineStateInfo) as
// "DeserializedSize", so the layout has to stay the one RTSR publishes.
static_assert(sizeof(PipelineStateInfo) == 24,
              "PipelineStateInfo layout is part of the wire format and must match RTSR's");

} // namespace gits::rtsr
