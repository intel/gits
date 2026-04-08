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

  const CaptureCleanupOptions GetSelectedCleanupOptions();

private:
  void RowTargetPath();
  void RowCleanup();
  void RowConfigPath();
  void RowArguments();
  void RowOutputPath();

  CaptureCleanupOptions CleanupOptions;
  std::unique_ptr<gits::ImGuiHelper::TabGroup<Api>> apiToolBar;
  std::optional<std::filesystem::path> DroppedFilePath;

  // Event callbacks
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
};

} // namespace gits::gui
