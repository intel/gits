// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interceptorExports.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace gits::opengl {

void* GetInterceptorProcAddress(const char* name) {
#ifdef _WIN32
  HMODULE interceptorModule = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          reinterpret_cast<LPCSTR>(&GetInterceptorProcAddress),
                          &interceptorModule)) {
    return nullptr;
  }

  void* procAddress = reinterpret_cast<void*>(GetProcAddress(interceptorModule, name));
  FreeLibrary(interceptorModule);
  return procAddress;
#else
  Dl_info info{};
  if (dladdr(reinterpret_cast<void*>(&GetInterceptorProcAddress), &info) == 0) {
    return nullptr;
  }
  void* interceptorModule = dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD);
  if (interceptorModule == nullptr) {
    return nullptr;
  }
  void* procAddress = dlsym(interceptorModule, name);
  dlclose(interceptorModule);
  return procAddress;
#endif
}

} // namespace gits::opengl
