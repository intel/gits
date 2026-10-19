// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherApp.h"

#ifdef _WIN32
#include "resource.h"
#endif

int main(int argc, char** argv) {
  gits::gui::LauncherApp launcherApp(argc, argv);
#ifdef _WIN32
  launcherApp.Initialize("gitsLauncher", APP_ICON, L"GITS Launcher");
#else
  launcherApp.Initialize("gitsLauncher", L"GITS Launcher");
#endif

  bool done = false;
  while (!done) {
    done = launcherApp.RenderFrame();
  }
  launcherApp.Shutdown();

  return 0;
}
