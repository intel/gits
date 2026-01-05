// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <ranges>

#include <imgui.h>

#include "imGuiHelper.h"
#include "baseButtonGroup.h"

namespace gits {
namespace ImGuiHelper {

template <typename T>
class ButtonGroup : public BaseButtonGroup<T> {
public:
  using BaseButtonGroup<T>::BaseButtonGroup; // boiler-plate constructors be gone!
  using BaseButtonGroup<T>::Render;          // boiler-plate constructors be gone!

  bool Render(bool newLine) override;
};

template <typename T>
bool ButtonGroup<T>::Render(bool newLine) {
  bool clicked = false;
  auto btnSize = this->isHorizontal ? ImVec2(0.0f, 0.0f) : ImVec2(-1.0f, 0.0f);

  size_t i = 0;
  for (auto it = this->items.begin(); it != this->items.end(); ++it, ++i) {
    auto& item = it->second;
    this->PushButtonStyle(item);
    ImGui::BeginDisabled(!this->btnEnabled[it->first] && !item.enabled);
    if (ImGui::Button(this->GetLabel(item).c_str(), btnSize)) {
      clicked = true;
    }
    this->AddTooltip(item);
    ImGui::EndDisabled();
    this->PopButtonStyle();
    if (this->isHorizontal) {
      ImGui::SameLine();
    }
  }
  if (newLine) {
    ImGui::NewLine();
  }
  return clicked;
}

} // namespace ImGuiHelper

} // namespace gits
