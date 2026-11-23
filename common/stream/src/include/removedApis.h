// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace gits {
namespace stream {

// Returns the error to report for a stream recorded with an API that is no longer supported by
// GITS, or std::nullopt for other streams. Reads the stream header into StreamHeader::Get().
std::optional<std::string> FindRemovedApiError(const std::filesystem::path& streamPath);

} // namespace stream
} // namespace gits
