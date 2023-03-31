// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

DISABLE_WARNINGS
#include <boost/filesystem/path.hpp>
ENABLE_WARNINGS

#ifndef BUILD_FOR_CCODE
namespace gits {
std::string file_xxhash(const bfs::path& filename);
void sign_directory(const boost::filesystem::path& dir);
void verify_directory(const boost::filesystem::path& dir);
} // namespace gits
#endif
