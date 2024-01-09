// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "pragmas.h"
#include <vector>
#include <map>
#include <string>
#include <filesystem>

namespace gits {
std::map<int, std::string> scopeAnalyze(const std::filesystem::path& path);
std::map<std::string, std::vector<int>> calculateContinuations(
    const std::map<int, std::string>& breakPoints);
std::string getOutputFileName(const std::filesystem::path& path, int num);
void divideFile(const std::filesystem::path& path,
                const std::filesystem::path& outputPath,
                std::map<int, std::string>& breakPoints);
} // namespace gits
