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

class PlaybackPanel : public BasePanel {
public:
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  void Render() override;

private:
  void RowStreamPath();
  void RowConfigPath();
  void RowArguments();
};

} // namespace gits::gui
