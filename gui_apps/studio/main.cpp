// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "studioApp.h"

#ifdef _WIN32
#include "resource.h"
#endif

int main(int argc, char** argv) {
  gits::gui::StudioApp studioApp(argc, argv);

#ifdef _WIN32
  studioApp.Initialize("gitsStudio", APP_ICON, L"GITS Studio");
#else
  studioApp.Initialize("gitsStudio", L"GITS Studio");
#endif

  bool done = false;
  while (!done) {
    done = studioApp.RenderFrame();
  }
  studioApp.Shutdown();

  return 0;
}
