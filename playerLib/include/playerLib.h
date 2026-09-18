// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"

extern "C" {
int STDCALL Initialize(int argc, char* argv[]) VISIBLE;
const char* STDCALL GetApplicationName() VISIBLE;
int STDCALL Play() VISIBLE;
}
