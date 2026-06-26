// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <mutex>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ShaderIdentifierService {
public:
  using ShaderIdentifier = std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>;
  struct ShaderIdentifierMapping {
    ShaderIdentifier CaptureIdentifier;
    ShaderIdentifier PlayerIdentifier;
  };

public:
  void AddCaptureShaderIdentifier(unsigned commandKey,
                                  ShaderIdentifier captureIdentifier,
                                  LPWSTR exportName);
  void AddPlayerShaderIdentifier(unsigned commandKey,
                                 ShaderIdentifier playerIdentifier,
                                 LPWSTR exportName);
  ShaderIdentifier* GetPlayerIdentifierByCaptureIdentifier(
      const ShaderIdentifier& captureIdentifier);
  bool GetMappings(std::vector<ShaderIdentifierMapping>& mappings);
  std::wstring GetExportNameByCaptureIdentifier(ShaderIdentifier identifier) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ExportNamesByCaptureIdentifier[identifier];
  }
  std::wstring GetExportNameByPlayerIdentifier(ShaderIdentifier identifier) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ExportNamesByPlayerIdentifier[identifier];
  }
  void EnablePlayerIdentifierLookup() {
    m_DumpLookup = true;
  }

private:
  struct ShaderIdHash {
    size_t operator()(std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> shaderId) const {
      const uint64_t* p = reinterpret_cast<const uint64_t*>(shaderId.data());
      return p[0] ^ p[1] ^ p[2] ^ p[3];
    }
  };

  std::unordered_map<ShaderIdentifier, ShaderIdentifierMapping, ShaderIdHash> m_ShaderIdentifiers;
  std::map<unsigned, ShaderIdentifier> m_ShaderIdentifiersByCommandKey;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByCaptureIdentifier;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByPlayerIdentifier;
  bool m_Changed{};
  bool m_DumpLookup{};
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
