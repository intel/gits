// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <filesystem>

struct Arguments {
  bool EnableScreenshots = false;
  std::filesystem::path OutputDir;
};

bool ParseArguments(int argc, char* argv[], Arguments& args);
