// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "command.h"

#include <map>
#include <vector>
#include <array>
#include <d3d12.h>
#include <string>
#include <mutex>

namespace gits {
namespace DirectX {

class CapturePlayerShaderIdentifierService {
public:
  using ShaderIdentifier = std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>;

  // Bit k of a patch mask is set if the 8-byte slot k of the shader record's local root arguments
  // holds a descriptor handle or GPU virtual address that may be translated, cleared if it holds
  // something else (e.g. root constants). PATCH_MASK_ALL means the layout is unknown: all slots,
  // including slots past PATCH_MASK_SLOT_COUNT, may be translated.
  static constexpr uint64_t PATCH_MASK_ALL = ~0ull;
  static constexpr unsigned PATCH_MASK_SLOT_COUNT = 64;

  // Must match ShaderIdentifierMapping in the binding table patch shader
  struct ShaderIdentifierMapping {
    ShaderIdentifier CaptureIdentifier;
    ShaderIdentifier PlayerIdentifier;
    uint64_t PatchMask{PATCH_MASK_ALL};
  };
  static_assert(sizeof(ShaderIdentifierMapping) ==
                2 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(uint64_t));

public:
  void AddCaptureShaderIdentifier(CommandKey commandKey,
                                  ShaderIdentifier captureIdentifier,
                                  LPWSTR exportName);
  void AddPlayerShaderIdentifier(CommandKey commandKey,
                                 ShaderIdentifier playerIdentifier,
                                 LPWSTR exportName,
                                 uint64_t patchMask);
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
  std::map<ShaderIdentifier, ShaderIdentifierMapping> m_ShaderIdentifiers;
  std::map<CommandKey, ShaderIdentifier> m_ShaderIdentifiersByCommandKey;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByCaptureIdentifier;
  std::map<ShaderIdentifier, std::wstring> m_ExportNamesByPlayerIdentifier;
  bool m_Changed{};
  bool m_DumpLookup{};
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
