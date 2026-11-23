// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

typedef gits::Configuration*(STDCALL* FConfigure)(const char* cfgDir);

extern "C" {
gits::Configuration* STDCALL Configure(const char* cfgDir) VISIBLE;
}
