// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
class TabGroup : public BaseButtonGroup<T> {
public:
  using BaseButtonGroup<T>::BaseButtonGroup;
  using BaseButtonGroup<T>::Render;

  bool Render(bool newLine) override;
};

template <typename T>
bool TabGroup<T>::Render(bool newLine) {
  size_t new_selected_tab = this->selectedIndex;
  auto btnSize = this->isHorizontal ? ImVec2(0.0f, 0.0f) : ImVec2(-1.0f, 0.0f);

  size_t i = 0;
  for (auto it = this->items.begin(); it != this->items.end(); ++it, ++i) {
    auto const& item = it->second;
    auto styleButton = i == this->selectedIndex;

    if (!this->btnEnabled[it->first] || !item.enabled) {
      continue;
    }

    if (styleButton) {
      this->PushButtonStyle(item);
    }
    if (ImGui::Button(this->GetLabel(item).c_str(), btnSize)) {
      new_selected_tab = i;
    }
    this->AddTooltip(item);
    if (styleButton) {
      this->PopButtonStyle();
    }
    if (this->isHorizontal) {
      ImGui::SameLine();
    }
  }
  if (newLine) {
    ImGui::NewLine();
  }
  if (new_selected_tab != this->selectedIndex) {
    this->selectedIndex = new_selected_tab;
    return true;
  }
  return false;
}

} // namespace ImGuiHelper

} // namespace gits
