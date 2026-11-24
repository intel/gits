// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::opengl::dispatch {

// Implemented by each owner using its own loader. Looks up library exports only.
void* GetDriverExport(const char* name);

} // namespace gits::opengl::dispatch
