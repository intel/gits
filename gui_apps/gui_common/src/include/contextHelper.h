// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <string>
#include <optional>
#include "baseCommon.h"
#include "baseContext.h"

namespace gits::gui::context_helper {

bool PathInput(const char* labelID,
               Path path,
               std::optional<GitsAction> gitsAction = std::nullopt,
               ImGuiInputTextFlags flags = 0,
               const float widthInput = -1.0f);

void PathMenuItem(const std::string& label,
                  Path path,
                  std::optional<GitsAction> gitsAction = std::nullopt);

void RenderUIViewWidget(float width);

void RenderRecentMenu(bool showStreams = true, bool showTargets = true);

} // namespace gits::gui::context_helper
