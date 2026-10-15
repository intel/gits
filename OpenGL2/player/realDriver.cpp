// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "realDriver.h"

#include <windows.h>

#include <mutex>
#include <stdexcept>

namespace gits::opengl2::player {

namespace {

HMODULE g_RealDriverLibrary = nullptr;

} // namespace

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

void FinalizeRealDriver() {
  // Deliberately a no-op: GetModuleHandleA above does not take out a reference, so
  // unconditionally FreeLibrary-ing here could unload the driver out from under other code in
  // this process still using it. Simpler and safer to let the OS unload it at process exit,
  // matching every other player-owned process-global resource in this skeleton.
}

void* GetRealProcAddress(const char* name) {
  // Self-initializing: unlike the recorder (which has a dedicated bootstrap step in
  // recorder.cpp), this player skeleton has no separate startup sequence yet, so the first
  // resolution request lazily loads the driver instead.
  static std::once_flag initFlag;
  std::call_once(initFlag, &InitializeRealDriver);
  return reinterpret_cast<void*>(GetProcAddress(g_RealDriverLibrary, name));
}

} // namespace gits::opengl2::player
