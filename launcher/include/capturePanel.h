// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"

namespace gits::gui {

class CapturePanel : public BasePanel {
public:
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

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
};

} // namespace gits::gui
