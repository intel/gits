// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiApp.h"
#include "studioEventBus.h"
#include "studioConfig.h"
#include "mainWindow.h"

namespace gits::gui {
class StudioApp : public ImGuiApp {
public:
  StudioApp(int argc, char** argv) : ImGuiApp(argc, argv) {}

private:
  void PathCallback(const Event& e);

  BaseConfig* InitializeApp() override;

  void CreateUI() override;
  void RenderUI() override;
  void DestroyUI() override;

  StudioConfig* m_StudioConfig = nullptr;
  std::shared_ptr<MainWindow> mainWindow;
};
} // namespace gits::gui
