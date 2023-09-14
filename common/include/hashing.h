// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>

#ifndef BUILD_FOR_CCODE
namespace gits {
std::string file_xxhash(const std::filesystem::path& filename);
void sign_directory(const std::filesystem::path& dir);
void verify_directory(const std::filesystem::path& dir);
} // namespace gits
#endif
