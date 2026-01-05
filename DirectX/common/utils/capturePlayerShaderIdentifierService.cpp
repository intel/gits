// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerShaderIdentifierService.h"
#include "gits.h"

#include <algorithm>

namespace gits {
namespace DirectX {

void CapturePlayerShaderIdentifierService::addCaptureShaderIdentifier(
    unsigned commandKey, ShaderIdentifier captureIdentifier, LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(mutex_);

  m_ShaderIdentifiersByCommandKey[commandKey] = captureIdentifier;
  if (dumpLookup_) {
    m_ExportNamesByCaptureIdentifier[captureIdentifier] = exportName;
  }
}

void CapturePlayerShaderIdentifierService::addPlayerShaderIdentifier(
    unsigned commandKey, ShaderIdentifier playerIdentifier, LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto itByCommandKey = m_ShaderIdentifiersByCommandKey.find(commandKey);
  GITS_ASSERT(itByCommandKey != m_ShaderIdentifiersByCommandKey.end());

  auto it = m_ShaderIdentifiers.find(itByCommandKey->second);
  if (it != m_ShaderIdentifiers.end()) {
    return;
  }

  m_ShaderIdentifiers[itByCommandKey->second] =
      ShaderIdentifierMapping{itByCommandKey->second, playerIdentifier};
  m_ShaderIdentifiersByCommandKey.erase(itByCommandKey);
  changed_ = true;

  if (dumpLookup_) {
    m_ExportNamesByPlayerIdentifier[playerIdentifier] = exportName;
  }
}

bool CapturePlayerShaderIdentifierService::getMappings(
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
    ShaderIdentifierGpu captureIdentifierA;
    memcpy(captureIdentifierA.data(), &mappingA.captureIdentifier, sizeof(ShaderIdentifierGpu));
    ShaderIdentifierGpu captureIdentifierB;
    memcpy(captureIdentifierB.data(), &mappingB.captureIdentifier, sizeof(ShaderIdentifierGpu));
    return captureIdentifierA < captureIdentifierB;
  });

  changed_ = false;
  bool changed = changed_;
  changed_ = false;
  return changed;
}

} // namespace DirectX
} // namespace gits
