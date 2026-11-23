// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include "tools_windows.h"
#include <windows.h>
#include <vector>
#include <string>
#else
#include <dlfcn.h>
#endif

#include "dynamic_linker.h"

namespace dl {
#ifdef GITS_PLATFORM_WINDOWS
static DWORD last_error_value;
SharedObject open_library(const char* name) {
  SetLastError(NO_ERROR);
  auto ret = LoadLibrary(name);
  if (ret == 0) {
    last_error_value = GetLastError();
  }

  return (SharedObject)ret;
}

void close_library(SharedObject lib) {
  if (lib) {
    FreeLibrary((HMODULE)lib);
  }
}

void* load_symbol(SharedObject lib, const char* name) {
  SetLastError(NO_ERROR);
  void* ret = GetProcAddress((HMODULE)lib, name);
  if (ret == 0) {
    // GL and WGL APIs not part of base OpenGL are exported with "_" prefix.
    // Refer HSD issue 5555894
    std::string modname("_");
    modname += name;
    ret = GetProcAddress((HMODULE)lib, modname.c_str());
  }
  if (ret == 0) {
    last_error_value = GetLastError();
  }
  return ret;
}

const char* last_error() {
  static std::string err_str;
  err_str = gits::Win32ErrorToString(last_error_value);
  return err_str.c_str();
}

std::string this_library_path() {
  HMODULE module = 0;
  BOOL result = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                  (LPCTSTR)&this_library_path, &module);
  if (result != TRUE || module == 0) {
    abort();
  }

  const size_t bufferSize = 1024;
  std::vector<char> dllName(bufferSize, 0);
  GetModuleFileName(module, dllName.data(), bufferSize);
  return dllName.data();
}

#else
SharedObject open_library(const char* name) {
  return (SharedObject)dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

void close_library(SharedObject lib) {
  if (lib) {
    dlclose(lib);
  }
}

void* load_symbol(SharedObject lib, const char* name) {
  return dlsym(lib, name);
}

const char* last_error() {
  return dlerror();
}

std::string this_library_path() {
  std::string dllName;
  Dl_info so_info;
  if (dladdr((void*)&this_library_path, &so_info) != 0) {
    dllName = std::string(so_info.dli_fname);
  }
  return dllName;
}

#endif
} // namespace dl
