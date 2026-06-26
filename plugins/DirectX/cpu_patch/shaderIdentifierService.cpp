// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "shaderIdentifierService.h"

#include <algorithm>
#include "log.h"

namespace gits {
namespace DirectX {

void ShaderIdentifierService::AddCaptureShaderIdentifier(unsigned commandKey,
                                                         ShaderIdentifier captureIdentifier,
                                                         LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  m_ShaderIdentifiersByCommandKey[commandKey] = captureIdentifier;
  if (m_DumpLookup) {
    m_ExportNamesByCaptureIdentifier[captureIdentifier] = exportName;
  }
}

void ShaderIdentifierService::AddPlayerShaderIdentifier(unsigned commandKey,
                                                        ShaderIdentifier playerIdentifier,
                                                        LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itByCommandKey = m_ShaderIdentifiersByCommandKey.find(commandKey);
  GITS_ASSERT(itByCommandKey != m_ShaderIdentifiersByCommandKey.end());

  auto it = m_ShaderIdentifiers.find(itByCommandKey->second);
  if (it != m_ShaderIdentifiers.end()) {
    return;
  }

  m_ShaderIdentifiers[itByCommandKey->second] =
      ShaderIdentifierMapping{itByCommandKey->second, playerIdentifier};
  m_ShaderIdentifiersByCommandKey.erase(itByCommandKey);
  m_Changed = true;

  if (m_DumpLookup) {
    m_ExportNamesByPlayerIdentifier[playerIdentifier] = exportName;
  }
}

ShaderIdentifierService::ShaderIdentifier* ShaderIdentifierService::
    GetPlayerIdentifierByCaptureIdentifier(const ShaderIdentifier& captureIdentifier) {
  auto it = m_ShaderIdentifiers.find(captureIdentifier);
  if (it == m_ShaderIdentifiers.end()) {
    return nullptr;
  }
  return &it->second.PlayerIdentifier;
}

bool ShaderIdentifierService::GetMappings(std::vector<ShaderIdentifierMapping>& mappings) {
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
    memcpy(captureIdentifierA.data(), &mappingA.CaptureIdentifier, sizeof(ShaderIdentifierGpu));
    ShaderIdentifierGpu captureIdentifierB;
    memcpy(captureIdentifierB.data(), &mappingB.CaptureIdentifier, sizeof(ShaderIdentifierGpu));
    return captureIdentifierA < captureIdentifierB;
  });

  m_Changed = false;
  bool changed = m_Changed;
  m_Changed = false;
  return changed;
}

} // namespace DirectX
} // namespace gits
