// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <array>
#include <string>
#include <vector>

namespace gits {

enum class HUDAnchor;

namespace ImGuiHelper {
std::string ToStr(const ImVec2& v);

ImFont* GetIntelFont(float uiScale);

bool UpdateUIScaling(float scale);

void PositionWindow(gits::HUDAnchor anchor, const ImVec2& windowSize, const ImVec2& padding);

void RenderTableContent(const std::vector<std::array<std::string, 2>>& dataTable);

void AddTableRow(const std::string& left, const std::string& right, const int& col2Width);

}; // namespace ImGuiHelper
}; // namespace gits
