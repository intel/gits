// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "pragmas.h"
#include <vector>
#include <map>
#include <string>
DISABLE_WARNINGS
#include <boost/filesystem.hpp>
ENABLE_WARNINGS

namespace gits {
std::map<int, std::string> scopeAnalyze(const boost::filesystem::path& path);
std::map<std::string, std::vector<int>> calculateContinuations(
    const std::map<int, std::string>& breakPoints);
std::string getOutputFileName(const boost::filesystem::path& path, int num);
void divideFile(const boost::filesystem::path& path,
                const boost::filesystem::path& outputPath,
                std::map<int, std::string>& breakPoints);
} // namespace gits
