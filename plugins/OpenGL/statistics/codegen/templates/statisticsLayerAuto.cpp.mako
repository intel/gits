// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "statisticsLayerAuto.h"

namespace gits::opengl {

% for cmd in commands:
% if cmd.enabled:
void StatisticsLayer::Post(${cmd.name}Command& command) {
  m_StatisticsService.Command("${cmd.name}", command.m_Key);
}

% endif
% endfor
} // namespace gits::opengl
