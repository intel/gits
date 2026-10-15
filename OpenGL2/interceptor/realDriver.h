// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Loads the real system opengl32.dll and resolves function pointers from it.
// This header must never include the real <windows.h> itself: it's included
// alongside glTypes.h elsewhere (e.g. openglDriversAuto.h), and their type
// declarations (BOOL, RECT, PIXELFORMATDESCRIPTOR, ...) can't coexist in the
// same translation unit. Only realDriver.cpp - a leaf translation unit that
// nothing else includes - may include the real <windows.h>.

#pragma once

namespace gits::opengl2 {

// Returns the real function pointer for `name`, resolved from the real
// system opengl32.dll (lazily loaded on first call). Returns nullptr if the
// real DLL couldn't be loaded or doesn't export `name`.
void* GetRealProcAddress(const char* name);

// Returns this DLL's own exported function pointer for `name` (i.e. our own
// generated trampoline, if we generated one for `name` - see
// interceptorAuto.cpp/interceptorAuto.def). Returns nullptr if this DLL
// doesn't export `name` itself. Used to make our own wglGetProcAddress
// override hand out our logging trampolines instead of raw real driver
// pointers, wherever we have one.
void* GetOwnProcAddress(const char* name);

} // namespace gits::opengl2
