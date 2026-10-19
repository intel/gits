// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "shaderInfo.h"

#include "rtsr/shared/rtsrCommon.h"

#include <cstring>
#include <string_view>

namespace gits {
namespace DirectX {

namespace {

// A DXBC container: 4 byte magic, 16 byte checksum, 4 reserved, total size,
// chunk count, then chunkCount 32 bit offsets. DXIL is wrapped in the same
// container, which is why one walk covers both.
constexpr size_t DXBC_CHUNK_INDEX_OFFSET = 32;
constexpr size_t DXBC_HEADER_SIZE = DXBC_CHUNK_INDEX_OFFSET;

struct DxbcContainerHeader {
  char FourCC[4];
  char CheckSum[16];
  char Placeholder[4];
  uint32_t TotalSize;
  uint32_t ChunkCount;
};
static_assert(sizeof(DxbcContainerHeader) == DXBC_HEADER_SIZE, "DXBC container header layout");

rtsr::RTSR_BYTECODE_TYPE BytecodeTypeFromShaderVersion(size_t shaderVersion) {
  if (shaderVersion >= 20 && shaderVersion <= 51) {
    return rtsr::RTSR_BYTECODE_TYPE_DXBC;
  }
  if (shaderVersion > 51) {
    return rtsr::RTSR_BYTECODE_TYPE_DXIL;
  }
  return rtsr::RTSR_BYTECODE_TYPE_UNKNOWN;
}

} // namespace

bool GetShaderVersionInfo(const void* bytecode, size_t size, ShaderVersionInfo& info) {
  // RTSR reads the header and the chunk index without validating the size at
  // all, so a short or truncated blob walks off the end of the allocation.
  if (bytecode == nullptr || size < DXBC_HEADER_SIZE) {
    return false;
  }

  const auto* bytes = static_cast<const uint8_t*>(bytecode);
  DxbcContainerHeader header{};
  std::memcpy(&header, bytes, sizeof(header));

  const size_t indexBytes = static_cast<size_t>(header.ChunkCount) * sizeof(uint32_t);
  if (header.ChunkCount == 0 || indexBytes / sizeof(uint32_t) != header.ChunkCount ||
      DXBC_CHUNK_INDEX_OFFSET + indexBytes > size) {
    return false;
  }

  for (uint32_t i = 0; i < header.ChunkCount; ++i) {
    uint32_t chunkOffset = 0;
    std::memcpy(&chunkOffset, bytes + DXBC_CHUNK_INDEX_OFFSET + i * sizeof(uint32_t),
                sizeof(chunkOffset));

    // The version nibbles and the program type live at chunk+8 and chunk+10.
    constexpr size_t MIN_CHUNK_BYTES = 12;
    if (chunkOffset > size || size - chunkOffset < MIN_CHUNK_BYTES) {
      continue;
    }

    const uint8_t* chunk = bytes + chunkOffset;
    const std::string_view fourCC(reinterpret_cast<const char*>(chunk), 4);
    if (fourCC != "SHDR" && fourCC != "SHEX" && fourCC != "DXIL") {
      continue;
    }

    info.MinorVersion = chunk[8] & 0x0f;
    info.MajorVersion = (chunk[8] & 0xf0) >> 4;
    std::memcpy(&info.ProgramType, chunk + 10, sizeof(info.ProgramType));
    return true;
  }

  return false;
}

void AppendShaderStage(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                       rtsr::RTSR_SHADER_STAGE stage,
                       const D3D12_SHADER_BYTECODE& bytecode) {
  if (bytecode.pShaderBytecode == nullptr || bytecode.BytecodeLength == 0) {
    return;
  }

  ShaderVersionInfo version{};
  size_t shaderVersion = 0;
  if (GetShaderVersionInfo(bytecode.pShaderBytecode, bytecode.BytecodeLength, version)) {
    shaderVersion = 10 * version.MajorVersion + version.MinorVersion;
  }

  const auto* bytes = static_cast<const uint8_t*>(bytecode.pShaderBytecode);

  rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO info;
  info.ShaderStage = stage;
  info.Bytecode.assign(bytes, bytes + bytecode.BytecodeLength);
  info.BytecodeHash = rtsr::ComputeBytecodeHash(bytecode.pShaderBytecode, bytecode.BytecodeLength);
  info.EntrypointName = rtsr::RTSR_DEFAULT_ENTRYPOINT_NAME;
  info.BytecodeType = BytecodeTypeFromShaderVersion(shaderVersion);
  info.D3DShaderVersion = shaderVersion;

  stages.push_back(std::move(info));
}

void AppendGraphicsShaderStages(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                                const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
  // Order matches RTSR so the editor sees stages in the sequence it expects.
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_VERTEX, desc.VS);
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_PIXEL, desc.PS);
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_GEOMETRY, desc.GS);
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_TESSELLATION_HULL, desc.HS);
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_TESSELLATION_DOMAIN, desc.DS);
}

void AppendComputeShaderStages(std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
                               const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) {
  AppendShaderStage(stages, rtsr::RTSR_SHADER_STAGE_COMPUTE, desc.CS);
}

} // namespace DirectX
} // namespace gits
