// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "common.h"
#include <filesystem>

#include <cstdlib>
#include <string>

namespace gits::gui {

struct CCodeExport {
  std::filesystem::path StreamPath;
  std::filesystem::path CCodePath;
  bool WrapAPICalls = false;
  int CommandsPerBlock = 1000;
};

bool ValidateYaml(const std::string& text);
bool ValidateGITSConfig(const std::string& config);
void UpdateCLICall();
void SetImGuiStyle(size_t selectedItem);
void LoadConfigFile();
void FileDialogs();
void ShowFileDialog(FileDialogKey key, const std::filesystem::path& path = "");
void PlaybackStream();
void SubcaptureStream();

void GenerateCCode(CCodeExport parameters);

std::string GetRecorderDirectoryNameForApi(Api api);
std::filesystem::path GetRecorderConfigPathForApi(Api api);
std::filesystem::path GetPlayerConfigPath();
bool IsValidGITSBasePath(const std::filesystem::path& path);
void DetectBasePaths();
void ResetBasePaths();

void OpenURL(const std::string& url);
bool OpenFolder(const std::filesystem::path& path);
bool OpenFolder(const std::string& path);
std::string CreateEmailBodyWithLog(const std::string& logText);
void SendLogByEmail(const std::string& recipient,
                    const std::string& subject,
                    const std::string& body);

} // namespace gits::gui
