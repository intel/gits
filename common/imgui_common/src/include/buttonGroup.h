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

namespace gits {
namespace ImGuiHelper {

enum class ButtonGroupStyle {
  Buttons,
  Tabs,
};

enum class ButtonStatus {
  Default,
  Success,
  Failure,
  Warning,
};

struct ButtonGroupItem {
  std::string label;
  std::string tooltip = "";
  ButtonStatus status = ButtonStatus::Default;
  std::string short_label = "";
  bool enabled = true;
};

template <typename T>
class ButtonGroup {
public:
  ButtonGroup(const std::map<T, ButtonGroupItem>& items,
              bool horizontal = true,
              bool regularLabel = true,
              ButtonGroupStyle style = ButtonGroupStyle::Buttons);
  void SetEnabled(const T& key, bool enabled);

  bool Render();
  bool Render(bool newLine);

  ImVec2 GetSize() const;

  void SelectEntry(T entry);
  T Selected() const;
  ButtonGroupItem SelectedItem() const;
  size_t SelectedIndex() const;

private:
  std::map<T, bool> btnEnabled;
  size_t selectedIndex = 0;
  bool isHorizontal;
  bool useRegularLabel;
  std::map<T, ButtonGroupItem> items;
  ButtonGroupStyle m_Style;
};

template <typename T>
ButtonGroup<T>::ButtonGroup(const std::map<T, ButtonGroupItem>& items_,
                            bool horizontal,
                            bool regularLabel,
                            ButtonGroupStyle style) {
  if (items_.size() < 1) {
    throw std::runtime_error("ButtonGroup must have at least one item");
  }
  isHorizontal = horizontal;
  useRegularLabel = regularLabel;
  items = items_;
  btnEnabled = std::map<T, bool>();
  m_Style = style;
  for (const auto& p : items) {
    btnEnabled[p.first] = p.second.enabled;
  }
}

template <typename T>
ImVec2 ButtonGroup<T>::GetSize() const {
  ImVec2 size(0.0f, 0.0f);
  for (const auto& item : items | std::views::values) {
    auto label =
        useRegularLabel ? item.label : (item.short_label.empty() ? item.label : item.short_label);
    ImVec2 btnSize = ImGui::CalcTextSize(label.c_str());
    btnSize.x = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, label);

    if (isHorizontal) {
      size.x += btnSize.x + ImGui::GetStyle().ItemSpacing.x;
      size.y = std::max(size.y, btnSize.y);
    } else {
      size.x = std::max(size.x, btnSize.x);
      size.y += btnSize.y + ImGui::GetStyle().ItemSpacing.y;
    }
  }
  if (isHorizontal) {
    size.x -= ImGui::GetStyle().ItemSpacing.x; // remove last spacing
  } else {
    size.y -= ImGui::GetStyle().ItemSpacing.y; // remove last spacing
  }
  return size;
}

template <typename T>
void ButtonGroup<T>::SetEnabled(const T& key, bool enabled) {
  auto it = btnEnabled.find(key);
  if (it != btnEnabled.end()) {
    it->second = enabled; //&&items[key];
  }
}

template <typename T>
bool ButtonGroup<T>::Render() {
  return Render(!isHorizontal);
}

template <typename T>
void ButtonGroup<T>::SelectEntry(T entry) {
  size_t index = 0;
  for (const auto& item : items | std::views::keys) {
    if (item == entry) {
      selectedIndex = index;
      return;
    }
    ++index;
  }
  throw std::runtime_error("ButtonGroup: entry not found");
}

template <typename T>
T ButtonGroup<T>::Selected() const {
  auto it = items.begin();
  std::advance(it, selectedIndex);
  return it->first;
}

template <typename T>
ButtonGroupItem ButtonGroup<T>::SelectedItem() const {
  auto it = items.begin();
  std::advance(it, selectedIndex);
  return it->second;
}

template <typename T>
size_t ButtonGroup<T>::SelectedIndex() const {
  return selectedIndex;
}

template <typename T>
bool ButtonGroup<T>::Render(bool newLine) {
  size_t new_selected_tab = selectedIndex;
  auto btnSize = isHorizontal ? ImVec2(0.0f, 0.0f) : ImVec2(-1.0f, 0.0f);

  size_t i = 0;
  for (auto it = items.begin(); it != items.end(); ++it, ++i) {
    auto styleButton = m_Style == ButtonGroupStyle::Buttons || i == selectedIndex;
    if (styleButton) {
      switch (it->second.status) {
      case ButtonStatus::Success:
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.9f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.7f, 0.25f, 1.0f));
        break;
      case ButtonStatus::Failure:
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.15f, 0.25f, 1.0f));
        break;
      case ButtonStatus::Warning:
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.9f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.6f, 0.15f, 1.0f));
        break;
      default:
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        break;
      }
    }
    ImGui::BeginDisabled(!btnEnabled[it->first] && !it->second.enabled);
    auto label = useRegularLabel
                     ? it->second.label
                     : (it->second.short_label.empty() ? it->second.label : it->second.short_label);
    if (ImGui::Button(label.c_str(), btnSize)) {
      new_selected_tab = i;
    }
    if (!it->second.tooltip.empty()) {
      ImGuiHelper::AddTooltip(it->second.tooltip);
    }
    ImGui::EndDisabled();
    if (styleButton) {
      ImGui::PopStyleColor(3);
    }
    if (isHorizontal) {
      ImGui::SameLine();
    }
  }
  if (newLine) {
    ImGui::NewLine();
  }
  if (new_selected_tab != selectedIndex) {
    selectedIndex = new_selected_tab;
    return true;
  }
  return false;
}

} // namespace ImGuiHelper

} // namespace gits
