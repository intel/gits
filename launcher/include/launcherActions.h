// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "common.h"
#include "context.h"

#include <filesystem>
#include <string>
#include <configurationAuto.h>
#include <optional>
#include <yaml-cpp/node/node.h>

namespace gits::gui {

struct CCodeExport {
  std::filesystem::path StreamPath;
  std::filesystem::path CCodePath;
  bool WrapAPICalls = false;
  int CommandsPerBlock = 1000;
};

std::optional<std::string> ValidateYaml(const std::string& text);
std::optional<std::string> ValidateGITSConfig(const std::string& config);
#ifdef _WIN32
std::string QuoteWindowsPath(const std::string& path);
#endif
void UpdateCLICall();
void SetImGuiStyle(size_t selectedItem);
void LoadConfigFile(Mode mode);
void LoadConfigFromMemory(Mode mode);
std::string GetYamlStringFromConfig(const gits::Configuration& configuration,
                                    std::optional<YAML::Node> overrides = std::nullopt);
std::string GetYamlStringFromConfig(const Context::ModeConfiguration& config);
void FileDialogs();
void ShowFileDialog(FileDialogKey key, const std::filesystem::path& path = "");
TemporaryConfigInfo PrepareTemporaryConfigFile(Mode mode, const std::filesystem::path& directory);
void RestoreOriginalConfigIfNeeded(const TemporaryConfigInfo& tmpConfigInfo);
void PlaybackStream();
void SubcaptureStream();

void GenerateCCode(CCodeExport parameters);

std::string GetRecorderDirectoryNameForApi(Api api);
std::filesystem::path GetRecorderConfigPathForApi(Api api);
std::filesystem::path GetPlayerConfigPath();
bool IsValidGITSBasePath(const std::filesystem::path& path);
bool DetectBasePaths();
void ResetBasePaths();
void SetAllConfigsFromBasePath();
void NewLauncherSession();
void SetTracePathFromInputStream();
void SetTracePathFromTargetExecutable();

void OpenURL(const std::string& url);
bool OpenFolder(const std::filesystem::path& path);
bool OpenFolder(const std::string& path);
std::string CreateEmailBodyWithLog(const std::string& logText);
void SendEmail(const std::string& recipient, const std::string& subject, const std::string& body);
void SendLogByEmail(const std::string& recipient,
                    const std::string& subject,
                    const std::string& body);

} // namespace gits::gui
