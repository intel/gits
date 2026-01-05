// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>

#include "context.h"

namespace gits::gui::capture_actions {
bool UpdateConfigDumpPath(gui::Context& context);
bool CopyRecorderFiles(std::filesystem::path gitsBasePath,
                       std::filesystem::path targetDirectory,
                       gui::Context::Api api);
std::filesystem::path FindLatestRecorderLog(std::filesystem::path directory);
void CaptureStream(gui::Context& context);
} // namespace gits::gui::capture_actions
