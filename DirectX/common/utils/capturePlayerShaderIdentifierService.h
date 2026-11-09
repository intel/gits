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

  // Every bit of the patch mask corresponds to one 8-byte slot of the shader record's local root
  // arguments (slot k is at byte offset D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * k).
  // A set bit means the slot holds a descriptor table handle or a root descriptor GPU virtual
  // address, so the binding table patch shader may translate it. A cleared bit means the slot
  // holds something else (e.g. root constants) and must be left untouched.
  // PATCH_MASK_ALL is used when the local root signature layout is unknown: every slot is then
  // treated as a candidate.
  static constexpr uint64_t PATCH_MASK_ALL = ~0ull;
  static constexpr unsigned PATCH_MASK_SLOT_COUNT = 64;

  // The layout must match the ShaderIdentifierMapping struct in the binding table patch shader.
  struct ShaderIdentifierMapping {
    ShaderIdentifier CaptureIdentifier;
    ShaderIdentifier PlayerIdentifier;
    uint64_t PatchMask{PATCH_MASK_ALL};
  };

public:
  void AddCaptureShaderIdentifier(CommandKey commandKey,
                                  ShaderIdentifier captureIdentifier,
                                  LPWSTR exportName);
  void AddPlayerShaderIdentifier(CommandKey commandKey,
                                 ShaderIdentifier playerIdentifier,
                                 LPWSTR exportName,
                                 uint64_t patchMask = PATCH_MASK_ALL);
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
