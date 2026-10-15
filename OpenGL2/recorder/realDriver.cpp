// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "realDriver.h"

#include <windows.h>

#include <stdexcept>
#include <string>

namespace gits::opengl2 {

namespace {

HMODULE g_RealDriverLibrary = nullptr;

} // namespace

void InitializeRealDriver() {
  char systemDirectory[MAX_PATH] = {};
  if (GetSystemDirectoryA(systemDirectory, MAX_PATH) == 0) {
    throw std::runtime_error("Could not locate the Windows system directory");
  }

  std::string realPath = std::string(systemDirectory) + "\\opengl32.dll";
  g_RealDriverLibrary = LoadLibraryA(realPath.c_str());
  if (g_RealDriverLibrary == nullptr) {
    throw std::runtime_error("Could not load the real opengl32.dll from \"" + realPath + "\"");
  }
}

void FinalizeRealDriver() {
  if (g_RealDriverLibrary != nullptr) {
    FreeLibrary(g_RealDriverLibrary);
    g_RealDriverLibrary = nullptr;
  }
}

void* GetRealProcAddress(const char* name) {
  return reinterpret_cast<void*>(GetProcAddress(g_RealDriverLibrary, name));
}

} // namespace gits::opengl2
