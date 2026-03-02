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
bool UpdateConfigDumpPath();
std::string GetRecorderDirectoryNameForApi(Api api);
bool CopyRecorderFiles(std::filesystem::path gitsBasePath,
                       std::filesystem::path targetDirectory,
                       Api api);
std::filesystem::path FindLatestRecorderLog(std::filesystem::path directory);
void CaptureStream();
std::vector<std::string> GetRecorderFilesForApi(Api api);
bool CleanupRecorderFiles(Api api, CapturePanel::CaptureCleanupOptions cleanupSelections);
} // namespace gits::gui::capture_actions
