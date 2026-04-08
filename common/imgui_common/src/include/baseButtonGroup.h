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

namespace gits {
namespace ImGuiHelper {

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
  bool visible = true;
};

template <typename T>
class BaseButtonGroup {
public:
  BaseButtonGroup(const std::map<T, ButtonGroupItem>& items,
                  bool horizontal = true,
                  bool regularLabel = true);
  virtual ~BaseButtonGroup() = default;

  void SetEnabled(const T& key, bool enabled);
  void SetStatus(const T& key, ButtonStatus status);
  bool IsEnabled(const T& key) const;

  void SetVisible(const T& key, bool visible);
  bool IsVisible(const T& key) const;

  void SelectEntry(T entry);
  T Selected() const;
  ButtonGroupItem SelectedItem() const;
  size_t SelectedIndex() const;

  bool Render();
  virtual bool Render(bool newLine) = 0;

  ImVec2 GetSize() const;

protected:
  void PushButtonStyle(const ButtonGroupItem& item);
  void PopButtonStyle();

  void AddTooltip(ButtonGroupItem item);

  const std::string& GetLabel(const ButtonGroupItem& item) const;

  std::map<T, bool> btnEnabled;
  std::map<T, bool> btnVisible;
  size_t selectedIndex = 0;
  bool isHorizontal;
  bool useRegularLabel;
  std::map<T, ButtonGroupItem> items;
};

template <typename T>
BaseButtonGroup<T>::BaseButtonGroup(const std::map<T, ButtonGroupItem>& items_,
                                    bool horizontal,
                                    bool regularLabel) {
  if (items_.size() < 1) {
    throw std::runtime_error("ButtonGroup must have at least one item");
  }
  isHorizontal = horizontal;
  useRegularLabel = regularLabel;
  items = items_;
  btnEnabled = std::map<T, bool>();
  btnVisible = std::map<T, bool>();
  for (const auto& p : items) {
    btnEnabled[p.first] = p.second.enabled;
    btnVisible[p.first] = p.second.visible;
  }
}

template <typename T>
void BaseButtonGroup<T>::SetEnabled(const T& key, bool enabled) {
  auto it = btnEnabled.find(key);
  if (it != btnEnabled.end()) {
    it->second = enabled;
  }
}

template <typename T>
void BaseButtonGroup<T>::SetStatus(const T& key, ButtonStatus status) {
  auto it = items.find(key);
  if (it != items.end()) {
    it->second.status = status;
  }
}

template <typename T>
bool BaseButtonGroup<T>::IsEnabled(const T& key) const {
  auto it = btnEnabled.find(key);
  if (it != btnEnabled.end()) {
    return it->second;
  }
  return false;
}

template <typename T>
void BaseButtonGroup<T>::SetVisible(const T& key, bool visible) {
  auto it = items.find(key);
  if (it != items.end()) {
    it->second.visible = visible;
  }
}

template <typename T>
bool BaseButtonGroup<T>::IsVisible(const T& key) const {
  auto it = btnVisible.find(key);
  if (it != btnVisible.end()) {
    return it->second;
  }
  return false;
}

template <typename T>
void BaseButtonGroup<T>::SelectEntry(T entry) {
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
T BaseButtonGroup<T>::Selected() const {
  auto it = items.begin();
  std::advance(it, selectedIndex);
  return it->first;
}

template <typename T>
ButtonGroupItem BaseButtonGroup<T>::SelectedItem() const {
  auto it = items.begin();
  std::advance(it, selectedIndex);
  return it->second;
}

template <typename T>
size_t BaseButtonGroup<T>::SelectedIndex() const {
  return selectedIndex;
}

template <typename T>
bool BaseButtonGroup<T>::Render() {
  return Render(!isHorizontal);
}

template <typename T>
ImVec2 BaseButtonGroup<T>::GetSize() const {
  ImVec2 size(0.0f, 0.0f);
  for (const auto& item : items | std::views::values) {
    if (!item.visible) {
      continue;
    }
    const auto& label =
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
void BaseButtonGroup<T>::PushButtonStyle(const ButtonGroupItem& item) {
  switch (item.status) {
  case ButtonStatus::Success:
    ImGuiHelper::PushButtonStyle(ButtonStyle::Success);
    break;
  case ButtonStatus::Failure:
    ImGuiHelper::PushButtonStyle(ButtonStyle::Failure);
    break;
  case ButtonStatus::Warning:
    ImGuiHelper::PushButtonStyle(ButtonStyle::Warning);
    break;
  default:
    ImGuiHelper::PushButtonStyle(ButtonStyle::Default);
    break;
  }
}
template <typename T>
void BaseButtonGroup<T>::PopButtonStyle() {
  ImGuiHelper::PopButtonStyle();
}

template <typename T>
void BaseButtonGroup<T>::AddTooltip(ButtonGroupItem item) {
  if (!item.tooltip.empty()) {
    ImGuiHelper::AddTooltip(item.tooltip);
  }
}

template <typename T>
const std::string& BaseButtonGroup<T>::GetLabel(const ButtonGroupItem& item) const {
  return useRegularLabel ? item.label : (item.short_label.empty() ? item.label : item.short_label);
}
} // namespace ImGuiHelper

} // namespace gits
