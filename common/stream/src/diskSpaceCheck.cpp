// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "diskSpaceCheck.h"
#include "log.h"

#include <system_error>

namespace gits {
namespace stream {

namespace {

std::filesystem::path NearestExistingParent(const std::filesystem::path& dir) {
  std::error_code ec;
  std::filesystem::path probe = dir;
  while (!probe.empty() && !std::filesystem::exists(probe, ec)) {
    auto parent = probe.parent_path();
    if (parent == probe) {
      break;
    }
    probe = parent;
  }
  return probe;
}

} // namespace

void CheckAvailableDiskSpace(const std::filesystem::path& dir,
                             const char* purpose,
                             std::uint64_t minBytes) {
  try {
    const std::filesystem::path probePath = NearestExistingParent(dir);
    if (probePath.empty()) {
      return;
    }
    const auto info = std::filesystem::space(probePath);
    if (info.available <= minBytes) {
      const auto availableMiB = info.available >> 20;
      const auto requiredMiB = minBytes >> 20;
      LOG_WARNING << "Disk might run out of space at " << dir.string() << ": only " << availableMiB
                  << " MiB available, recommended at least " << requiredMiB << " MiB for "
                  << (purpose ? purpose : "stream output");
    }
  } catch (const std::filesystem::filesystem_error& fe) {
    LOG_WARNING << "Disk space check skipped for " << dir.string() << ": " << fe.what();
  }
}

} // namespace stream
} // namespace gits
