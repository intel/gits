// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "shaderIdentifierService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void ShaderIdentifierService::addCaptureShaderIdentifier(unsigned commandKey,
                                                         ShaderIdentifier captureIdentifier,
                                                         LPWSTR exportName) {
  std::lock_guard<std::mutex> lock(mutex_);

  m_ShaderIdentifiersByCommandKey[commandKey] = captureIdentifier;
  if (dumpLookup_) {
    m_ExportNamesByCaptureIdentifier[captureIdentifier] = exportName;
  }
}

void ShaderIdentifierService::addPlayerShaderIdentifier(unsigned commandKey,
                                                        ShaderIdentifier playerIdentifier,
                                                        LPWSTR exportName) {
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

bool ShaderIdentifierService::getMappings(std::vector<ShaderIdentifierMapping>& mappings) {
  mappings.resize(m_ShaderIdentifiers.size());
  unsigned index = 0;
  for (auto& it : m_ShaderIdentifiers) {
    mappings[index] = it.second;
    ++index;
  }
  changed_ = false;
  bool changed = changed_;
  changed_ = false;
  return changed;
}

} // namespace DirectX
} // namespace gits
