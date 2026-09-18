// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>

namespace gits {

bool IsLegacyStream(const std::filesystem::path& streamPath);
void PlayStream(const std::filesystem::path& streamPath);

} // namespace gits
