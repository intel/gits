// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "exception.h"
#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include "tools_windows.h"
#include <windows.h>
#include <psapi.h>
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

SharedObject symbol_library(const void* symbol) {
  DWORD size = 0;

  HANDLE process = GetCurrentProcess();
  EnumProcessModules(process, 0, 0, &size);

  std::vector<HMODULE> modules(size / sizeof(HMODULE));
  EnumProcessModules(process, &modules[0], size, &size);

  void* targetModule = 0;
  for (auto module : modules) {
    MODULEINFO info;
    GetModuleInformation(process, module, &info, sizeof(MODULEINFO));
    void* beginOfModule = info.lpBaseOfDll;
    void* endOfModule = (char*)info.lpBaseOfDll + info.SizeOfImage;

    if (symbol >= beginOfModule && symbol < endOfModule) {
      targetModule = beginOfModule;
      break;
    }
  }
  return (SharedObject)targetModule;
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

SharedObject symbol_library(const void* symbol) {
  Dl_info info;
  if ((dladdr(symbol, &info) == 0) || (info.dli_fname == nullptr) || (info.dli_fname[0] == 0)) {
    return nullptr;
  }

  void* lib = dlopen(info.dli_fname, RTLD_LOCAL | RTLD_NOW);
  if (lib == nullptr) {
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  dlclose(lib);
  return (SharedObject)lib;
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
