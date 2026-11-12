// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerShaderIdentifierService.h"
#include "arguments.h"
#include "log.h"

#include <algorithm>

namespace gits {
namespace DirectX {

void CapturePlayerShaderIdentifierService::AddCaptureShaderIdentifier(
    CommandKey commandKey, ShaderIdentifier captureIdentifier, LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  m_ShaderIdentifiersByCommandKey[commandKey] = captureIdentifier;
  if (m_DumpLookup) {
    m_ExportNamesByCaptureIdentifier[captureIdentifier] = exportName;
  }
}

void CapturePlayerShaderIdentifierService::AddPlayerShaderIdentifier(
    CommandKey commandKey,
    ShaderIdentifier playerIdentifier,
    LPWSTR exportName,
    uint64_t patchMask) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itByCommandKey = m_ShaderIdentifiersByCommandKey.find(commandKey);
  GITS_ASSERT(itByCommandKey != m_ShaderIdentifiersByCommandKey.end());

  auto it = m_ShaderIdentifiers.find(itByCommandKey->second);
  if (it != m_ShaderIdentifiers.end()) {
    // Exports sharing a capture identifier may have different local root signatures
    uint64_t mergedPatchMask = it->second.PatchMask | patchMask;
    if (mergedPatchMask != it->second.PatchMask) {
      it->second.PatchMask = mergedPatchMask;
      m_Changed = true;
    }
    m_ShaderIdentifiersByCommandKey.erase(itByCommandKey);
    return;
  }

  m_ShaderIdentifiers[itByCommandKey->second] =
      ShaderIdentifierMapping{itByCommandKey->second, playerIdentifier, patchMask};
  m_ShaderIdentifiersByCommandKey.erase(itByCommandKey);
  m_Changed = true;

  if (m_DumpLookup) {
    m_ExportNamesByPlayerIdentifier[playerIdentifier] = exportName;
  }
}

bool CapturePlayerShaderIdentifierService::GetMappings(
    std::vector<ShaderIdentifierMapping>& mappings) {
  mappings.resize(m_ShaderIdentifiers.size());
  unsigned index = 0;
  for (auto& it : m_ShaderIdentifiers) {
    mappings[index] = it.second;
    ++index;
  }

  std::sort(mappings.begin(), mappings.end(), [](const auto& mappingA, const auto& mappingB) {
    using ShaderIdentifierGpu =
        std::array<uint64_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES / sizeof(uint64_t)>;
    static_assert(sizeof(ShaderIdentifierGpu) == D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    ShaderIdentifierGpu captureIdentifierA;
    memcpy(captureIdentifierA.data(), &mappingA.CaptureIdentifier,
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    ShaderIdentifierGpu captureIdentifierB;
    memcpy(captureIdentifierB.data(), &mappingB.CaptureIdentifier,
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    return captureIdentifierA < captureIdentifierB;
  });

  bool changed = m_Changed;
  m_Changed = false;
  return changed;
}

} // namespace DirectX
} // namespace gits
