// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools_windows.cpp
 *
 * @brief Windows-related tools for GITS project.
 *
 * These tools should depend only on the standard library, the tools_lite.h
 * header, and the Windows header.
 */

#include "tools_windows.h"

#include <array>

namespace gits {

std::string Win32ErrorToString(DWORD error) {
  // TODO: Once C++17 is available, std::string::data() will be able to
  // return a non-const pointer. Use it to avoid conversion from std::array.
  std::array<char, 1024> str_arr;

  DWORD char_count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
                                   error, LOCALE_USER_DEFAULT, str_arr.data(), str_arr.size(), 0);

  std::string str(str_arr.data(), char_count);

  return str;
}

} // namespace gits
