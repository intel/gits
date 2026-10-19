// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
#pragma once

#include "basePanel.h"
#include <cstdint>

#include "studioEventBus.h"

namespace gits::gui {
class NavBar : public BasePanel {
public:
  NavBar(bool enableSection = true);
  void Render() override;

private:
  bool m_SectionEnabled = true;
  int64_t m_PlayUntilFrame = 1;
  int64_t m_StepSize = 1;
  bool m_LoopPlayback = false;
  bool m_SectionActive = false;
  float m_HoveredTimelinePercentage = -1.0f;
  int m_HoveredTimelineFrame = -1;
  float m_DragAnchor = 0.0f;

  void ActionCallback(const Event& e);
};
} // namespace gits::gui
