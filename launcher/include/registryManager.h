// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include "tools_lite.h"

namespace gits::gui {

class RegistryManager : gits::noncopyable {
public:
  // Registry value types
  struct RegValueDWORD {
    DWORD Value;
  };
  struct RegValueQWORD {
    ULONGLONG Value;
  };
  struct RegValueString {
    std::string Value;
  };
  struct RegValueMultiString {
    std::vector<std::string> Values;
  };
  struct RegValueBinary {
    std::vector<BYTE> Data;
  };

  using RegValue = std::
      variant<RegValueDWORD, RegValueQWORD, RegValueString, RegValueMultiString, RegValueBinary>;

  struct RegKeyValue {
    std::string KeyPath;
    std::string ValueName;
    RegValue Value;
    std::string Remark;
  };

  struct RegistryEntry {
    std::string Label;
    std::string Remark;
    std::vector<RegKeyValue> KeyValues;
    bool Enabled = false;
  };

  static std::string RegValueToString(const RegValue& regValue);

  // Singleton
  static RegistryManager& Instance();

  void AddEntry(RegistryEntry entry);
  bool LoadEntriesFromYamlFile(const std::filesystem::path& filePath);
  bool AddEntriesFromYamlFile(const std::filesystem::path& filePath);
  void SetEntryEnabled(size_t index, bool enabled);
  void SyncEntries();
  void ClearAll();

  bool HasWriteAccess() const {
    return m_HasWriteAccess;
  }

  const std::string& WriteAccessStatusMessage() const {
    return m_WriteAccessStatusMessage;
  }

  const std::vector<RegistryEntry>& Entries() const {
    return m_Entries;
  }

private:
  RegistryManager();
  ~RegistryManager();

  void ApplyEntry(RegistryEntry& entry);
  void RemoveEntry(const RegistryEntry& entry);
  void RefreshWriteAccess();
  void UpdateWriteAccessStatus(LONG errorCode);

  static LONG WriteValue(HKEY hKey, const RegKeyValue& kv);
  static LONG DeleteKeyIfEmpty(HKEY hRoot, const std::string& keyPath);
  static LONG ProbeKeyWriteAccess(HKEY hRoot, const std::string& keyPath);
  static bool IsAccessDeniedError(LONG errorCode);
  static bool ValueExists(HKEY hRoot, const std::string& keyPath, const std::string& valueName);
  static std::string FormatWindowsError(DWORD errorCode);

  std::vector<RegistryEntry> m_Entries;
  bool m_HasWriteAccess = true;
  std::string m_WriteAccessStatusMessage;
};

} // namespace gits::gui
