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

} // namespace DirectX
} // namespace gits
