// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "statisticsLayerAuto.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
% if cmd.name in frame_end_commands:
// A swap ends a frame when its captured return is TRUE. The runner overwrites m_Return with the
// replay result, so the captured one is read in Pre.
void StatisticsLayer::Pre(${cmd.name}Command& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

% endif
void StatisticsLayer::Post(${cmd.name}Command& command) {
  m_StatisticsService.Command("${cmd.name}", command.m_Key);
% if cmd.name in frame_end_commands:
  if (m_SwapEndsFrame) {
    m_StatisticsService.FrameEnd();
  }
% endif
}

% endif
% endfor
} // namespace gits::opengl2
