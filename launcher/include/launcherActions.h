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

bool ValidateGITSConfig(const std::string& config);
void UpdateCLICall();
void SetImGuiStyle(size_t selectedItem);
void LoadConfigFile();
void FileDialogs();
void ShowFileDialog(FileDialogKeys key);
void PlaybackStream();
void SubcaptureStream();

void ResetBasePaths();

void OpenURL(const std::string& url);
bool OpenFolder(const std::filesystem::path& path);
bool OpenFolder(const std::string& path);
} // namespace gits::gui
