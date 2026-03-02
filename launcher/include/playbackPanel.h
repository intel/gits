// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "eventBus.h"

namespace gits::gui {

class PlaybackPanel : public BasePanel {
public:
  PlaybackPanel();

  void Render() override;

private:
  void RowStreamPath();
  void RowConfigPath();
  void RowArguments();

  // Event callbacks
  void PathCallback(const Event& e);
};

} // namespace gits::gui
