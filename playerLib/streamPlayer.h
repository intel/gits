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
// Logs the GITS version from the stream header read by IsLegacyStream().
void LogStreamRecordingVersion();
void PlayStream(const std::filesystem::path& streamPath);

} // namespace gits
