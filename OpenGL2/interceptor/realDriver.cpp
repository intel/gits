// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "realDriver.h"

#include "logging.h"
#include "log.h"

#include <windows.h>

#include <mutex>
#include <stdexcept>
#include <string>

namespace gits::opengl2 {

namespace {

HMODULE g_RealOpenGL32 = nullptr;
std::once_flag g_LoadFlag;

// Loads the real opengl32.dll by its full path in the system directory - not
// by bare name, since LoadLibraryA("opengl32.dll") could otherwise resolve
// back to this very DLL if it's sitting next to the application under the
// same name.
void EnsureRealLibraryLoaded() {
  std::call_once(g_LoadFlag, [] {
    char systemDir[MAX_PATH] = {};
    GetSystemDirectoryA(systemDir, MAX_PATH);
    std::string realPath = std::string(systemDir) + "\\opengl32.dll";

    g_RealOpenGL32 = LoadLibraryA(realPath.c_str());
    if (g_RealOpenGL32 == nullptr) {
      EnsureLoggingInitialized();
      std::string errorMessage = "Could not load the real opengl32.dll from \"" + realPath + "\"";
      LOG_ERROR << errorMessage;
      throw std::runtime_error(errorMessage);
    }
  });
}

} // namespace

void* GetRealProcAddress(const char* name) {
  EnsureRealLibraryLoaded();
  if (g_RealOpenGL32 == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<void*>(GetProcAddress(g_RealOpenGL32, name));
}

void* GetOwnProcAddress(const char* name) {
  HMODULE thisModule = nullptr;
  // GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS: resolves to whichever module
  // this very function is loaded in (this DLL), regardless of what it's
  // named or where the app loaded it from.
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          reinterpret_cast<LPCSTR>(&GetOwnProcAddress), &thisModule)) {
    return nullptr;
  }
  return reinterpret_cast<void*>(GetProcAddress(thisModule, name));
}

} // namespace gits::opengl2
