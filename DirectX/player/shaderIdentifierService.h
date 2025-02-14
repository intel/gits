// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <vector>
#include <array>
#include <d3d12.h>
#include <string>

namespace gits {
namespace DirectX {

class ShaderIdentifierService {
public:
  using ShaderIdentifier = std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>;
  struct ShaderIdentifierMapping {
    ShaderIdentifier captureIdentifier;
    ShaderIdentifier playerIdentifier;
  };

public:
  void addCaptureShaderIdentifier(unsigned commandKey,
                                  ShaderIdentifier captureIdentifier,
                                  LPWSTR exportName);
  void addPlayerShaderIdentifier(unsigned commandKey,
                                 ShaderIdentifier playerIdentifier,
                                 LPWSTR exportName);
  bool getMappings(std::vector<ShaderIdentifierMapping>& mappings);
  std::wstring& getExportNameByCaptureIdentifier(ShaderIdentifier identifier) {
    return m_ExportNamesByCaptureIdentifier[identifier];
  }
  std::wstring& getExportNameByPlayerIdentifier(ShaderIdentifier identifier) {
    return m_ExportNamesByPlayerIdentifier[identifier];
  }
  void enableDumpLookup() {
    dumpLookup_ = true;
  }

private:
  std::map<ShaderIdentifier, ShaderIdentifierMapping> m_ShaderIdentifiers;
  std::map<unsigned, ShaderIdentifier> m_ShaderIdentifiersByCommandKey;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByCaptureIdentifier;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByPlayerIdentifier;
  bool changed_{};
  bool dumpLookup_{};
};

} // namespace DirectX
} // namespace gits
