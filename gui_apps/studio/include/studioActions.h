// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
#pragma once

#include "configurator.h"

namespace gits::gui::studio_actions {
void OpenStream();
void MakeConfigInteractive(Configuration& config);
void AnalyzeStream(Configuration config, bool genScrenshots, bool genTrace);
void ProcessAnalysis(bool logFail);
} // namespace gits::gui::studio_actions
