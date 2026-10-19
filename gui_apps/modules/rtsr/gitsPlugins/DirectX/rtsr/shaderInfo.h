// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <d3d12.h>

#include <vector>

namespace gits {
namespace DirectX {

// Shader model encoded in a DXBC container, read by walking the chunk index. No
// compiler is involved: the client never compiles or reflects shaders, that is
// entirely the editor's job.
struct ShaderVersionInfo {
  uint32_t MajorVersion{};
  uint32_t MinorVersion{};
  uint16_t ProgramType{};
};

bool GetShaderVersionInfo(const void* bytecode, size_t size, ShaderVersionInfo& info);

// Appends one entry describing this bytecode, or nothing when the stage is
// absent. Copies the bytecode, because the descriptor is only valid for the
// duration of the command.
void AppendShaderStage(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                       rtsr::RTSR_SHADER_STAGE stage,
                       const D3D12_SHADER_BYTECODE& bytecode);

void AppendGraphicsShaderStages(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                                const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
void AppendComputeShaderStages(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                               const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

} // namespace DirectX
} // namespace gits
