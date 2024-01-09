// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderIface.h
 * 
 * @brief Declaration of GITS recorder interface.
 */

#pragma once

#include "platform.h"

namespace gits {

struct Config;
typedef void (*FPrintHandler)(const char* text);

} // namespace gits

typedef gits::FPrintHandler(STDCALL* FPrintHandlerGet)(const char* dir);
typedef const gits::Config*(STDCALL* FConfigure)(const char* cfgDir);

extern "C" {
gits::FPrintHandler STDCALL PrintHandlerGet(const char* dir) VISIBLE;
const gits::Config* STDCALL Configure(const char* cfgDir) VISIBLE;
}
