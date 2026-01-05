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

class ContentPanel : public BasePanel {
public:
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  float WidthColumn1(bool resetSize = false);
  void Render() override;

private:
  void ChildWindowConfig();
  void GITSButton();
};

} // namespace gits::gui
