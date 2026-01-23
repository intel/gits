// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "context.h"
#include <filesystem>

#include <cstdlib>
#include <string>

namespace gits::gui {

enum class FileDialogKeys {
  PICK_STREAM_PATH = 0,
  PICK_TARGET_PATH = 1,
  PICK_GITSPLAYER_PATH = 2,
  PICK_GITS_BASE_PATH = 3,
  PICK_CONFIG_PATH = 4,
  PICK_CAPTURE_CONFIG_PATH = 5,
  PICK_CAPTURE_OUTPUT_PATH = 6,
  PICK_SUBCAPTURE_PATH = 7,
  PICK_TRACE_PATH = 8,
  PICK_SCREENSHOTS_PATH = 9
};

const std::string str(FileDialogKeys key);

bool ValidateGITSConfig(const std::string& config);
void UpdateCLICall(gui::Context& context);
void SetImGuiStyle(Context* context, size_t selectedItem);
void LoadConfigFile(Context* context);
void FileDialogs(gui::Context& context);
void ShowFileDialog(gui::Context* context, FileDialogKeys key);
void PlaybackStream(gui::Context& context);
void SubcaptureStream(gui::Context& context);

void ResetBasePaths(gui::Context& context);

void OpenURL(const std::string& url);
bool OpenFolder(const std::filesystem::path& path);
bool OpenFolder(const std::string& path);
} // namespace gits::gui
