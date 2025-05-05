// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderUtils.h
 *
 * @brief Utility functions for the gitsRecorder.
 */

#pragma once

#include <filesystem>

namespace gits {
bool ConfigureRecorder(const std::filesystem::path& configPath);
} // namespace gits
