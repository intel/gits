// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <filesystem>

namespace gits {
namespace stream {

// Default minimum free-space threshold (100 MiB), mirroring the constant used by
// legacy CheckMinimumAvailableDiskSize() in common/legacy/tools.cpp. The legacy
// helper is intentionally left in place for the existing CBinOStream callers; this
// utility provides the same warning for stream::StreamWriter consumers (notably
// the Vulkan2 subcapture pipeline) which bypass legacy infrastructure.
constexpr std::uint64_t kDefaultMinDiskSpaceBytes = 104857600ULL;

// Emits a LOG_WARNING when the filesystem hosting `dir` has less than `minBytes`
// available. If `dir` does not yet exist, the nearest existing parent is probed.
// Never throws -- std::filesystem::filesystem_error is caught internally.
void CheckAvailableDiskSpace(const std::filesystem::path& dir,
                             const char* purpose,
                             std::uint64_t minBytes = kDefaultMinDiskSpaceBytes);

} // namespace stream
} // namespace gits
