// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "baseEventBus.h"

namespace gits::gui {

class CaptureOptionsPanel : public BasePanel {
public:
  CaptureOptionsPanel();

  void Render() override;
  void RenderSimple();

private:
  void PathCallback(const Event& e);
  void ContextCallback(const Event& e);
};

} // namespace gits::gui
