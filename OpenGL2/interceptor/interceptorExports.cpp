// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interceptorExports.h"

#include <windows.h>

namespace gits::opengl2 {

void* GetInterceptorProcAddress(const char* name) {
  HMODULE interceptorModule = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          reinterpret_cast<LPCSTR>(&GetInterceptorProcAddress),
                          &interceptorModule)) {
    return nullptr;
  }

  void* procAddress = reinterpret_cast<void*>(GetProcAddress(interceptorModule, name));
  FreeLibrary(interceptorModule);
  return procAddress;
}

} // namespace gits::opengl2
