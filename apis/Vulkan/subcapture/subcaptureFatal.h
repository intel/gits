// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace gits {
namespace vulkan {

[[noreturn]] inline void FatalSubcaptureError(const std::string& message) {
  LOG_ERROR << "Vulkan subcapture: " << message << std::endl << "Aborting the subcapture.";
  std::fflush(nullptr);
  // Do not finalize stream, just exit
  std::_Exit(EXIT_FAILURE);
}

} // namespace vulkan
} // namespace gits
