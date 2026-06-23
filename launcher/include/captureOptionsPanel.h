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

class CaptureOptionsPanel : public BasePanel {
public:
  CaptureOptionsPanel();

  void Render() override;

private:
  void PathCallback(const Event& e);
};

} // namespace gits::gui
