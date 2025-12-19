// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

private:
  void RowTargetPath();
  void RowConfigPath();
  void RowArguments();
  void RowOutputPath();
};

} // namespace gits::gui
