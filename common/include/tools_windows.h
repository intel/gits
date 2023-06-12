// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools_windows.h
 *
 * @brief Windows-related tools for GITS project.
 *
 * These tools should depend only on the standard library, the tools_lite.h
 * header, and the Windows header.
 */

#pragma once

#include <string>
#include <windows.h>

namespace gits {
std::string Win32ErrorToString(DWORD error);
} // namespace gits
