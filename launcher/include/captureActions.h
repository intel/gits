// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "context.h"
#include "capturePanel.h"

namespace gits::gui::capture_actions {
bool UpdateConfigDumpPath(gui::Context& context);
std::string GetRecorderDirectoryNameForApi(gui::Context::Api api);
bool CopyRecorderFiles(std::filesystem::path gitsBasePath,
                       std::filesystem::path targetDirectory,
                       gui::Context::Api api);
std::filesystem::path FindLatestRecorderLog(std::filesystem::path directory);
void CaptureStream(gui::Context& context);
std::vector<std::string> GetRecorderFilesForApi(gui::Context& context, gui::Context::Api api);
bool CleanupRecorderFiles(gui::Context& context,
                          gui::Context::Api api,
                          gui::CapturePanel::CaptureCleanupOptions cleanupSelections);
} // namespace gits::gui::capture_actions
