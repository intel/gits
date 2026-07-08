// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "registryManager.h"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>
#include <cstdio>

#include <yaml-cpp/yaml.h>

#include "labels.h"
#include "log.h"

namespace gits::gui {

namespace {

constexpr const char* REGISTRY_KEYS_ROOT = "RegistryKeys";

std::string ToUpper(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return text;
}

bool TryParseValue(const YAML::Node& valueNode,
                   const std::string& type,
                   RegistryManager::RegValue& regValue) {
  const std::string typeUpper = ToUpper(type);

  if (typeUpper == "REG_DWORD") {
    regValue = RegistryManager::RegValueDWORD{valueNode.as<DWORD>()};
    return true;
  }
  if (typeUpper == "REG_QWORD") {
    regValue = RegistryManager::RegValueQWORD{valueNode.as<ULONGLONG>()};
    return true;
  }
  if (typeUpper == "REG_SZ" || typeUpper == "REG_STRING") {
    regValue = RegistryManager::RegValueString{valueNode.as<std::string>()};
    return true;
  }
  if (typeUpper == "REG_MULTI_SZ") {
    RegistryManager::RegValueMultiString multiString;
    for (const auto& item : valueNode) {
      multiString.Values.push_back(item.as<std::string>());
    }
    regValue = std::move(multiString);
    return true;
  }
  if (typeUpper == "REG_BINARY") {
    RegistryManager::RegValueBinary binary;
    for (const auto& item : valueNode) {
      const int byteValue = item.as<int>();
      if (byteValue < 0 || byteValue > 255) {
        return false;
      }
      binary.Data.push_back(static_cast<BYTE>(byteValue));
    }
    regValue = std::move(binary);
    return true;
  }

  return false;
}

std::vector<char> BuildMultiString(const std::vector<std::string>& strings) {
  std::vector<char> buffer;
  for (const auto& s : strings) {
    buffer.insert(buffer.end(), s.begin(), s.end());
    buffer.push_back('\0');
  }
  buffer.push_back('\0');
  return buffer;
}

} // namespace

RegistryManager::RegistryManager() {}

RegistryManager::~RegistryManager() {
  ClearAll();
}

RegistryManager& RegistryManager::Instance() {
  static RegistryManager instance;
  return instance;
}

std::string RegistryManager::RegValueToString(const RegValue& regValue) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, RegValueDWORD>) {
          char buf[16];
          std::snprintf(buf, sizeof(buf), "0x%02X", v.Value);
          return buf;
        } else if constexpr (std::is_same_v<T, RegValueQWORD>) {
          char buf[24];
          std::snprintf(buf, sizeof(buf), "0x%02llX", v.Value);
          return buf;
        } else if constexpr (std::is_same_v<T, RegValueString>) {
          return "\"" + v.Value + "\"";
        } else if constexpr (std::is_same_v<T, RegValueMultiString>) {
          std::string result = "{";
          for (size_t i = 0; i < v.Values.size(); ++i) {
            result += "\"" + v.Values[i] + "\"";
            if (i + 1 < v.Values.size()) {
              result += ", ";
            }
          }
          return result + "}";
        } else if constexpr (std::is_same_v<T, RegValueBinary>) {
          std::string result = "[";
          for (size_t i = 0; i < v.Data.size(); ++i) {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%02X", v.Data[i]);
            result += buf;
            if (i + 1 < v.Data.size()) {
              result += " ";
            }
          }
          return result + "]";
        }

        return "unknown";
      },
      regValue);
}

bool ExtractHiveAndKeyPath(const std::string& fullKeyPath, HKEY& hive, std::string& keyPath) {
  const auto firstSeparator = fullKeyPath.find('\\');
  if (firstSeparator == std::string::npos || firstSeparator == 0 ||
      firstSeparator + 1 >= fullKeyPath.size()) {
    return false;
  }

  const std::string hiveText = ToUpper(fullKeyPath.substr(0, firstSeparator));
  if (hiveText == "HKLM" || hiveText == "HKEY_LOCAL_MACHINE") {
    hive = HKEY_LOCAL_MACHINE;
  } else if (hiveText == "HKCU" || hiveText == "HKEY_CURRENT_USER") {
    hive = HKEY_CURRENT_USER;
  } else if (hiveText == "HKCR" || hiveText == "HKEY_CLASSES_ROOT") {
    hive = HKEY_CLASSES_ROOT;
  } else if (hiveText == "HKU" || hiveText == "HKEY_USERS") {
    hive = HKEY_USERS;
  } else if (hiveText == "HKCC" || hiveText == "HKEY_CURRENT_CONFIG") {
    hive = HKEY_CURRENT_CONFIG;
  } else {
    return false;
  }

  keyPath = fullKeyPath.substr(firstSeparator + 1);
  return !keyPath.empty();
}

bool ExtractKeyPathAndValueName(const std::string& fullPath,
                                std::string& keyPath,
                                std::string& valueName) {
  const auto lastSeparator = fullPath.find_last_of('\\');
  if (lastSeparator == std::string::npos || lastSeparator == 0 ||
      lastSeparator + 1 >= fullPath.size()) {
    return false;
  }

  keyPath = fullPath.substr(0, lastSeparator);
  valueName = fullPath.substr(lastSeparator + 1);
  return !keyPath.empty() && !valueName.empty();
}

bool TryGetRegistryLocation(const RegistryManager::RegKeyValue& kv,
                            HKEY& hive,
                            std::string& keyPath) {
  return ExtractHiveAndKeyPath(kv.KeyPath, hive, keyPath);
}

void RegistryManager::AddEntry(RegistryEntry entry) {
  bool allExist = !entry.KeyValues.empty();
  for (const auto& kv : entry.KeyValues) {
    HKEY hive = nullptr;
    std::string keyPath;
    if (!TryGetRegistryLocation(kv, hive, keyPath) || !ValueExists(hive, keyPath, kv.ValueName)) {
      allExist = false;
      break;
    }
  }

  entry.Enabled = allExist;
  m_Entries.push_back(std::move(entry));
}

bool RegistryManager::LoadEntriesFromYamlFile(const std::filesystem::path& filePath) {
  m_Entries.clear();

  if (!std::filesystem::exists(filePath)) {
    LOG_WARNING << "[RegistryManager] Registry keys YAML file not found: " << filePath.string();
    return false;
  }

  return AddEntriesFromYamlFile(filePath);
}

bool RegistryManager::AddEntriesFromYamlFile(const std::filesystem::path& filePath) {
  try {
    const YAML::Node root = YAML::LoadFile(filePath.string());
    const YAML::Node entriesNode = root[REGISTRY_KEYS_ROOT];
    if (!entriesNode || !entriesNode.IsSequence()) {
      LOG_WARNING << "[RegistryManager] YAML file does not contain a valid '" << REGISTRY_KEYS_ROOT
                  << "' sequence: " << filePath.string();
      return false;
    }

    size_t addedEntries = 0;
    for (const auto& entryNode : entriesNode) {
      if (!entryNode.IsMap() || !entryNode["Name"]) {
        continue;
      }

      RegistryEntry entry;
      entry.Label = entryNode["Name"].as<std::string>();
      if (entryNode["Remark"]) {
        entry.Remark = entryNode["Remark"].as<std::string>();
      }

      const YAML::Node valuesNode = entryNode["Values"];
      if (!valuesNode || !valuesNode.IsSequence()) {
        LOG_WARNING << "[RegistryManager] Entry '" << entry.Label
                    << "' has no valid values list in: " << filePath.string();
        continue;
      }

      bool entryValid = true;
      for (const auto& valueNode : valuesNode) {
        if (!valueNode.IsMap() || !valueNode["Path"] || !valueNode["Type"] || !valueNode["Value"]) {
          entryValid = false;
          break;
        }

        const std::string fullPath = valueNode["Path"].as<std::string>();
        HKEY hive = nullptr;
        std::string keyPathWithHive;
        std::string keyPath;
        std::string valueName;
        if (!ExtractKeyPathAndValueName(fullPath, keyPathWithHive, valueName) ||
            !ExtractHiveAndKeyPath(keyPathWithHive, hive, keyPath)) {
          LOG_WARNING << "[RegistryManager] Invalid registry path '" << fullPath
                      << "' in: " << filePath.string();
          entryValid = false;
          break;
        }

        RegValue regValue;
        const std::string valueType = valueNode["Type"].as<std::string>();
        if (!TryParseValue(valueNode["Value"], valueType, regValue)) {
          LOG_WARNING << "[RegistryManager] Unsupported or invalid value type '" << valueType
                      << "' for entry '" << entry.Label << "' in: " << filePath.string();
          entryValid = false;
          break;
        }

        std::string remark;
        if (valueNode["Remark"]) {
          remark = valueNode["Remark"].as<std::string>();
        }

        entry.KeyValues.push_back({std::move(keyPathWithHive), std::move(valueName),
                                   std::move(regValue), std::move(remark)});
      }

      if (!entryValid || entry.KeyValues.empty()) {
        continue;
      }

      AddEntry(std::move(entry));
      ++addedEntries;
    }

    return addedEntries > 0;
  } catch (const YAML::Exception& e) {
    LOG_WARNING << "[RegistryManager] Failed to parse YAML registry file '" << filePath.string()
                << "': " << e.what();
  } catch (const std::exception& e) {
    LOG_WARNING << "[RegistryManager] Failed to load registry file '" << filePath.string()
                << "': " << e.what();
  }

  return false;
}

void RegistryManager::SetEntryEnabled(size_t index, bool enabled) {
  if (index >= m_Entries.size()) {
    return;
  }

  auto& entry = m_Entries[index];
  if (entry.Enabled == enabled) {
    return;
  }

  entry.Enabled = enabled;

  if (enabled) {
    ApplyEntry(entry);
  } else {
    RemoveEntry(entry);
  }
}

void RegistryManager::SyncEntries() {
  for (auto& entry : m_Entries) {
    bool allExist = !entry.KeyValues.empty();
    for (const auto& kv : entry.KeyValues) {
      HKEY hive = nullptr;
      std::string keyPath;
      if (!TryGetRegistryLocation(kv, hive, keyPath) || !ValueExists(hive, keyPath, kv.ValueName)) {
        allExist = false;
        break;
      }
    }

    entry.Enabled = allExist;
  }
}

void RegistryManager::ClearAll() {
  for (auto& entry : m_Entries) {
    if (entry.Enabled) {
      RemoveEntry(entry);
      entry.Enabled = false;
    }
  }
}

void RegistryManager::ApplyEntry(RegistryEntry& entry) {
  for (const auto& kv : entry.KeyValues) {
    HKEY hive = nullptr;
    std::string keyPath;
    if (!TryGetRegistryLocation(kv, hive, keyPath)) {
      LOG_ERROR << "[RegistryManager] Invalid registry key path '" << kv.KeyPath << "'";
      entry.Enabled = false;
      return;
    }

    HKEY rawKey = nullptr;
    DWORD disposition = 0;

    LONG createResult = RegCreateKeyExA(hive, keyPath.c_str(), 0, nullptr, REG_OPTION_VOLATILE,
                                        KEY_WRITE, nullptr, &rawKey, &disposition);

    if (createResult != ERROR_SUCCESS) {
      LOG_ERROR << "[RegistryManager] Failed to create key '" << kv.KeyPath
                << "': " << FormatWindowsError(static_cast<DWORD>(createResult)) << "\n";
      RemoveEntry(entry);
      entry.Enabled = false;
      return;
    }

    LONG writeResult = WriteValue(rawKey, kv);
    RegCloseKey(rawKey);

    if (writeResult != ERROR_SUCCESS) {
      LOG_ERROR << "[RegistryManager] Failed to write value '" << kv.ValueName
                << "': " << FormatWindowsError(static_cast<DWORD>(writeResult)) << "\n";
      RemoveEntry(entry);
      entry.Enabled = false;
      return;
    }
  }
}

void RegistryManager::RemoveEntry(const RegistryEntry& entry) {
  for (const auto& kv : entry.KeyValues) {
    HKEY hive = nullptr;
    std::string keyPath;
    if (!TryGetRegistryLocation(kv, hive, keyPath)) {
      continue;
    }

    HKEY rawKey = nullptr;

    LONG openResult = RegOpenKeyExA(hive, keyPath.c_str(), 0, KEY_WRITE | KEY_QUERY_VALUE, &rawKey);

    if (openResult != ERROR_SUCCESS) {
      continue;
    }

    RegDeleteValueA(rawKey, kv.ValueName.c_str());
    RegCloseKey(rawKey);

    DeleteKeyIfEmpty(hive, keyPath);
  }
}

LONG RegistryManager::WriteValue(HKEY hKey, const RegKeyValue& kv) {
  return std::visit(
      [&](const auto& v) -> LONG {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, RegValueDWORD>) {
          return RegSetValueExA(hKey, kv.ValueName.c_str(), 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&v.Value), sizeof(DWORD));
        } else if constexpr (std::is_same_v<T, RegValueQWORD>) {
          return RegSetValueExA(hKey, kv.ValueName.c_str(), 0, REG_QWORD,
                                reinterpret_cast<const BYTE*>(&v.Value), sizeof(ULONGLONG));
        } else if constexpr (std::is_same_v<T, RegValueString>) {
          return RegSetValueExA(hKey, kv.ValueName.c_str(), 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(v.Value.c_str()),
                                static_cast<DWORD>(v.Value.size() + 1));
        } else if constexpr (std::is_same_v<T, RegValueMultiString>) {
          auto buffer = BuildMultiString(v.Values);
          return RegSetValueExA(hKey, kv.ValueName.c_str(), 0, REG_MULTI_SZ,
                                reinterpret_cast<const BYTE*>(buffer.data()),
                                static_cast<DWORD>(buffer.size()));
        } else if constexpr (std::is_same_v<T, RegValueBinary>) {
          return RegSetValueExA(hKey, kv.ValueName.c_str(), 0, REG_BINARY, v.Data.data(),
                                static_cast<DWORD>(v.Data.size()));
        }

        return ERROR_INVALID_PARAMETER;
      },
      kv.Value);
}

LONG RegistryManager::DeleteKeyIfEmpty(HKEY hRoot, const std::string& keyPath) {
  HKEY rawKey = nullptr;

  LONG openResult = RegOpenKeyExA(hRoot, keyPath.c_str(), 0, KEY_QUERY_VALUE, &rawKey);

  if (openResult != ERROR_SUCCESS) {
    return openResult;
  }

  DWORD valueCount = 0;
  DWORD subkeyCount = 0;
  RegQueryInfoKeyA(rawKey, nullptr, nullptr, nullptr, &subkeyCount, nullptr, nullptr, nullptr,
                   &valueCount, nullptr, nullptr, nullptr);

  RegCloseKey(rawKey);

  if (valueCount == 0 && subkeyCount == 0) {
    return RegDeleteKeyA(hRoot, keyPath.c_str());
  }

  return ERROR_SUCCESS;
}

bool RegistryManager::ValueExists(HKEY hRoot,
                                  const std::string& keyPath,
                                  const std::string& valueName) {
  HKEY rawKey = nullptr;

  if (RegOpenKeyExA(hRoot, keyPath.c_str(), 0, KEY_QUERY_VALUE, &rawKey) != ERROR_SUCCESS) {
    return false;
  }

  LONG result = RegQueryValueExA(rawKey, valueName.c_str(), nullptr, nullptr, nullptr, nullptr);
  RegCloseKey(rawKey);
  return result == ERROR_SUCCESS;
}

std::string RegistryManager::FormatWindowsError(DWORD errorCode) {
  char* msgBuffer = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 reinterpret_cast<LPSTR>(&msgBuffer), 0, nullptr);

  std::string msg = msgBuffer ? msgBuffer : Labels::UNKNOWN_WINDOWS_ERROR;
  if (msgBuffer) {
    LocalFree(msgBuffer);
  }

  while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
    msg.pop_back();
  }

  return msg;
}

} // namespace gits::gui
