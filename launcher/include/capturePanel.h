// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tabGroup.h"

#include "basePanel.h"
#include "eventBus.h"

namespace gits::gui {

class CapturePanel : public BasePanel {
public:
  CapturePanel();

  void Render() override;

  struct CaptureCleanupOptions {
    bool CleanRecorderFiles = true;
    bool CleanRecorderConfig = true;
    bool CleanRecorderLog = false;
  };

  // Capture logic
  void CaptureStream();
  const CaptureCleanupOptions GetSelectedCleanupOptions();

private:
  // UI rendering
  void RowTargetPath();
  void RowCleanup();
  void RowConfigPath();
  void RowArguments();
  void RowOutputPath();

  // Event callbacks
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
  void ContextCallback(const Event& e);
  void ActionCallback(const Event& e);

  TemporaryConfigInfo TmpConfigInfo;
  CaptureCleanupOptions CleanupOptions;
  std::unique_ptr<gits::ImGuiHelper::TabGroup<Api>> apiToolBar;
  std::optional<std::filesystem::path> DroppedFilePath;
};

} // namespace gits::gui
