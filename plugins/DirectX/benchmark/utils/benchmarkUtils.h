// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <Windows.h>
#include <string>

namespace gits {

class CGits;

namespace DirectX {

void initializeLog(CGits* gits);
std::string hrToString(HRESULT hr);
void logAndThrow(const std::string& errorMsg);
void throwIfFailed(HRESULT hr);

} // namespace DirectX
} // namespace gits
