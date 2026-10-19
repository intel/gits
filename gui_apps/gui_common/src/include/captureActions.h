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
#include <optional>
#include <chrono>
#include <cstdint>

#include "baseCommon.h"
#include "baseContext.h"

namespace gits::gui::capture_actions {

constexpr auto QUIET_PERIOD = std::chrono::seconds(3);

std::string GetRecorderDirectoryNameForApi(Api api);
bool UpdateConfigDumpPath();
bool CopyRecorderFiles(const std::filesystem::path& gitsBasePath,
                       const std::filesystem::path& targetDirectory,
                       Api api);
std::filesystem::path FindLatestRecorderLog(std::filesystem::path directory);
std::optional<std::filesystem::path> GetStreamDirectoryFromLog(const std::string& log);
void CaptureStream(bool useInMemoryConfig = false);
void CaptureStream(const Configuration& configuration);
std::vector<std::string> GetRecorderFilesForApi(Api api);
bool CleanupRecorderFiles(Api api, CaptureCleanupOptions cleanupSelections);
void StartPostExitMonitoring(const std::filesystem::path& executablePath);
} // namespace gits::gui::capture_actions
