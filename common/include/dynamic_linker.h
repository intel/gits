// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace dl {

struct SharedObjectTag;
typedef SharedObjectTag* SharedObject;

SharedObject open_library(const char* name);
void close_library(SharedObject lib);
void* load_symbol(SharedObject lib, const char* name);
const char* last_error();
SharedObject symbol_library(const void* symbol);
std::string this_library_path();
} // namespace dl
