// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "analyzerLayerAuto.h"

namespace gits::opengl {

% for name, params in analyzed.items():
void AnalyzerLayer::Pre(${name}Command& command) {
  if (Collecting(command.m_ThreadId)) {
% for param in params:
    Use(command.m_${param});
% endfor
  }
}

% endfor
} // namespace gits::opengl
