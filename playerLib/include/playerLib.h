// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"

#include <cstdint>

extern "C" {
int STDCALL Initialize(int argc, char* argv[]) VISIBLE;
const char* STDCALL GetApplicationName() VISIBLE;
// The value the recorded executable exported under the given GPU preference export name, or 0.
uint32_t STDCALL GetGpuPreferenceExport(const char* name) VISIBLE;
int STDCALL Play() VISIBLE;
}
