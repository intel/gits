// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::opengl {

void InitializeRealDriver();
void FinalizeRealDriver();
void* GetRealProcAddress(const char* name);

} // namespace gits::opengl
