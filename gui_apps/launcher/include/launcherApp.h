// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiApp.h"

#include "launcherConfig.h"
#include "mainWindow.h"

namespace gits::gui {

class LauncherApp : public ImGuiApp {
public:
  LauncherApp(int argc, char** argv) : ImGuiApp(argc, argv) {}

private:
  LauncherConfig* InitializeApp() override;
  void CreateUI() override;
  void RenderUI() override;
  void DestroyUI() override;

  void OnFileDrop(const std::filesystem::path& filePath) override;

  LauncherConfig* m_LauncherConfig = nullptr;
  std::shared_ptr<MainWindow> mainWindow;
};

} // namespace gits::gui
