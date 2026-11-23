// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// GITS versions are MAJOR.MINOR.PATCH, see docs/development/versioning.md. PATCH is not kept
// here: release builds get it from the build number, development builds use
// GITS_VERSION_PATCH_DEVELOPMENT.
// Build scripts parse this file; keep one "NAME = number;" definition per line.
namespace gits {
inline constexpr unsigned short GITS_VERSION_MAJOR = 3;
inline constexpr unsigned short GITS_VERSION_MINOR = 0;

inline constexpr unsigned short GITS_VERSION_PATCH_RELEASE_MAX = 998;
inline constexpr unsigned short GITS_VERSION_PATCH_DEVELOPMENT = 999;
} // namespace gits
