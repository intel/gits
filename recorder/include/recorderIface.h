// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "configurationLib.h"

namespace gits {

typedef void (*FPrintHandler)(const char* text);

} // namespace gits

typedef gits::FPrintHandler(STDCALL* FPrintHandlerGet)(const char* dir);
typedef gits::Configuration*(STDCALL* FConfigure)(const char* cfgDir);

extern "C" {
gits::FPrintHandler STDCALL PrintHandlerGet(const char* dir) VISIBLE;
gits::Configuration* STDCALL Configure(const char* cfgDir) VISIBLE;
}
