// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace gits {
namespace DirectX {

std::string utf16ToUtf8(const std::wstring& utf16);
std::wstring utf8ToUtf16(const std::string& utf8);

} // namespace DirectX
} // namespace gits
