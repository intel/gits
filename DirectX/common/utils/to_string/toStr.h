// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "enumToStrAuto.h"
#include "enumToStrCustom.h"
#include "guidToStrAuto.h"

#include <string>

namespace gits {
namespace DirectX {

std::string toStr(const wchar_t* s);
std::string keyToStr(unsigned key);
std::wstring keyToWStr(unsigned key);

std::string toStr(const LARGE_INTEGER& i);
std::string toStr(const float& f);
std::string toStr(const wchar_t* s);

template <size_t N>
std::string toStr(const wchar_t (&s)[N]) {
  return toStr(s);
}

template <typename T>
std::string toStr(const T& value) {
  if constexpr (std::is_arithmetic_v<T>) {
    return std::to_string(value);
  }
  return "TO_STR_UNKNOWN_TYPE";
}

} // namespace DirectX
} // namespace gits
