// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "driverExports.h"

#ifdef _WIN32
#include <windows.h>
#else
#include "configurator.h"

#include <dlfcn.h>
#endif

#include <mutex>
#include <stdexcept>
#include <string>

namespace gits::opengl::dispatch {

namespace {

#ifdef _WIN32
HMODULE g_RealDriverLibrary = nullptr;
#else
void* g_RealDriverLibrary = nullptr;
#endif

#ifdef _WIN32
void InitializeRealDriver() {
  // The player is a normal, unhooked process - unlike the recorder, which LoadLibraryA's a
  // private handle specifically to bypass its own import-table hook on opengl32.dll (see
  // recorder/realDriver.cpp), the player can simply resolve the one opengl32.dll already (or
  // about to be) loaded into this process the ordinary way.
  g_RealDriverLibrary = GetModuleHandleA("opengl32.dll");
  if (g_RealDriverLibrary == nullptr) {
    g_RealDriverLibrary = LoadLibraryA("opengl32.dll");
  }
  if (g_RealDriverLibrary == nullptr) {
    throw std::runtime_error("Could not load opengl32.dll");
  }
}
#else
void InitializeRealDriver() {
  const std::string path = Configurator::Get().common.shared.libGL.string();
  g_RealDriverLibrary = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (g_RealDriverLibrary == nullptr) {
    throw std::runtime_error("Could not load \"" + path + "\": " + dlerror());
  }
}
#endif

} // namespace

void* GetDriverExport(const char* name) {
  // Self-initializing: unlike the recorder (which has a dedicated bootstrap step in
  // recorder.cpp), this player skeleton has no separate startup sequence yet, so the first
  // resolution request lazily loads the driver instead.
  static std::once_flag initFlag;
  std::call_once(initFlag, &InitializeRealDriver);
#ifdef _WIN32
  return reinterpret_cast<void*>(GetProcAddress(g_RealDriverLibrary, name));
#else
  return dlsym(g_RealDriverLibrary, name);
#endif
}

} // namespace gits::opengl::dispatch
