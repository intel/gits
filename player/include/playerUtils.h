// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   playerUtils.h
 *
 * @brief Utility functions for the gitsPlayer.
 *
 */

#pragma once

#include <string>
#include <filesystem>

struct ArgumentParser;

namespace gits {
bool ends_with(const std::string& str, const std::string& ending);

bool ConfigurePlayer(const std::filesystem::path& playerPath, ArgumentParser& args);
} // namespace gits
