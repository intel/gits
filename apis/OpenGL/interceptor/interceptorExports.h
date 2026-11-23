// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Windows exports the entry points through the generated interceptorAuto.def.
#if defined(_WIN32)
#define GL_INTERCEPTOR_EXPORT extern "C"
#else
#define GL_INTERCEPTOR_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace gits::opengl {

void* GetInterceptorProcAddress(const char* name);

} // namespace gits::opengl
