// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "utfConversion.h"
#include "windows.h"

namespace gits {
namespace DirectX {

std::string utf16ToUtf8(const std::wstring& utf16) {
  if (utf16.empty()) {
    return {};
  }

  int utf8Length = WideCharToMultiByte(CP_UTF8,      // Code page
                                       0,            // Flags
                                       utf16.data(), // Input UTF-16 string
                                       static_cast<int>(utf16.size()),
                                       nullptr, // No output buffer yet
                                       0,       // Ask for required size
                                       nullptr, nullptr);

  if (utf8Length <= 0) {
    // Failed to get UTF-8 buffer size.
    return {};
  }

  std::string utf8(utf8Length, 0);
  int result = WideCharToMultiByte(CP_UTF8, 0, utf16.data(), static_cast<int>(utf16.size()),
                                   utf8.data(), utf8Length, nullptr, nullptr);

  if (result == 0) {
    // Failed to convert UTF-16 to UTF-8.
    return {};
  }

  return utf8;
}

std::wstring utf8ToUtf16(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }

  // Get required size (in wchar_t units)
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  if (size_needed <= 0) {
    // MultiByteToWideChar failed.
    return {};
  }

  // Allocate std::wstring of the required size
  std::wstring utf16(size_needed, L'\0');

  // Convert
  int converted = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                                      &utf16[0], size_needed);
  if (converted != size_needed) {
    // Conversion size mismatch.
    return {};
  }

  return utf16;
}

} // namespace DirectX
} // namespace gits
