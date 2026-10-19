// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/server/rtsrSessionStore.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gits::rtsr {

enum class SourceKind {
  Hlsl,
  Glsl,
  Disassembly
};

struct ShaderSource {
  SourceKind Kind{SourceKind::Disassembly};
  std::string Text;
  std::string Diagnostic;
  // The entrypoint emitted into Text. It comes from the intermediate SPIR-V,
  // not from the placeholder name attached during pipeline harvesting.
  std::string EntrypointName;
};

struct ShaderCompileInput {
  RTSR_SHADER_STAGE Stage{RTSR_SHADER_STAGE_UNKNOWN};
  size_t D3DShaderVersion{};
  std::string EntrypointName;
  std::string OriginalEntrypointName;
  std::string Source;
  RTSR_API TargetApi{RTSR_API_UNKNOWN};
};

struct ShaderCompileResult {
  std::vector<uint8_t> Bytecode;
  std::string EntrypointName;
  // Contains warnings on success and the reason on failure.
  std::string Diagnostic;

  explicit operator bool() const {
    return !Bytecode.empty();
  }
};

// CPU-only conversion. Call off the UI thread with an owned snapshot of the blob.
ShaderSource Decompile(const SessionStore::ShaderBlob& blob,
                       const std::filesystem::path& playerPath);

// Compiles reconstructed HLSL into a validated DXIL container. CPU-only and
// potentially slow: call it off the UI thread.
ShaderCompileResult CompileHlslToDxil(const ShaderCompileInput& input,
                                      const std::vector<uint8_t>& originalBytecode,
                                      const std::filesystem::path& playerPath);
ShaderCompileResult CompileGlslToSpirv(const ShaderCompileInput& input,
                                       const std::vector<uint8_t>& originalBytecode);

} // namespace gits::rtsr
