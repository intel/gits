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
#include <limits>

#include <imgui.h>

#include "imGuiHelper.h"

namespace gits {
namespace ImGuiHelper {

enum class ButtonStatus {
  BUTTON_STATUS_DEFAULT,
  BUTTON_STATUS_SUCCESS,
  BUTTON_STATUS_FAILURE,
  BUTTON_STATUS_WARNING,
};

enum class ItemType {
  Button,
  Label,
  CheckableButton
};

struct ButtonGroupItem {
  std::string label;
  std::string tooltip = "";
  ButtonStatus status = ButtonStatus::BUTTON_STATUS_DEFAULT;
  std::string short_label = "";
  bool enabled = true;
  bool visible = true;
  ItemType type = ItemType::Button;
  bool checked = false;
};

template <typename T>
class BaseButtonGroup {
public:
  BaseButtonGroup(const std::map<T, ButtonGroupItem>& items,
                  bool horizontal = true,
                  bool regularLabel = true,
                  bool allowNoSelection = false);
  virtual ~BaseButtonGroup() = default;

  void SetStatus(const T& key, ButtonStatus status);

  void SetEnabled(const T& key, bool enabled);
  bool IsEnabled(const T& key) const;

  void SetVisible(const T& key, bool visible);
  bool IsVisible(const T& key) const;

  void SetChecked(const T& key, bool checked);
  bool IsChecked(const T& key) const;

  void SelectEntry(T entry);
  void DeselectEntry();
  bool HasSelection() const;
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

  static constexpr size_t NoSelectionIndex = std::numeric_limits<size_t>::max();

  std::map<T, bool> btnEnabled;
  std::map<T, bool> btnVisible;
  std::map<T, bool> btnChecked;
  size_t selectedIndex = 0;
  bool allowNoSelection = false;
  bool isHorizontal;
  bool useRegularLabel;
  std::map<T, ButtonGroupItem> items;
};

template <typename T>
BaseButtonGroup<T>::BaseButtonGroup(const std::map<T, ButtonGroupItem>& items_,
                                    bool horizontal,
                                    bool regularLabel,
                                    bool allowNoSelection_) {
  if (items_.size() < 1) {
    throw std::runtime_error("ButtonGroup must have at least one item");
  }
  isHorizontal = horizontal;
  useRegularLabel = regularLabel;
  allowNoSelection = allowNoSelection_;
  items = items_;
  selectedIndex = allowNoSelection ? NoSelectionIndex : 0;
  btnEnabled = std::map<T, bool>();
  btnVisible = std::map<T, bool>();
  btnChecked = std::map<T, bool>();
  for (const auto& p : items) {
    btnEnabled[p.first] = p.second.enabled;
    btnVisible[p.first] = p.second.visible;
    btnChecked[p.first] = p.second.checked;
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
void BaseButtonGroup<T>::SetEnabled(const T& key, bool enabled) {
  auto it = btnEnabled.find(key);
  if (it != btnEnabled.end()) {
    it->second = enabled;
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
  auto it = items.find(key);
  if (it != items.end()) {
    return it->second.visible;
  }
  return false;
}

template <typename T>
void BaseButtonGroup<T>::SetChecked(const T& key, bool checked) {
  auto it = btnChecked.find(key);
  if (it != btnChecked.end()) {
    it->second = checked;
  }
}

template <typename T>
bool BaseButtonGroup<T>::IsChecked(const T& key) const {
  auto it = btnChecked.find(key);
  if (it != btnChecked.end()) {
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
void BaseButtonGroup<T>::DeselectEntry() {
  if (!allowNoSelection) {
    throw std::runtime_error("ButtonGroup: no-selection mode is disabled");
  }
  selectedIndex = NoSelectionIndex;
}

template <typename T>
bool BaseButtonGroup<T>::HasSelection() const {
  return selectedIndex != NoSelectionIndex;
}

template <typename T>
T BaseButtonGroup<T>::Selected() const {
  if (!HasSelection()) {
    throw std::runtime_error("ButtonGroup: no entry is selected");
  }
  auto it = items.begin();
  std::advance(it, selectedIndex);
  return it->first;
}

template <typename T>
ButtonGroupItem BaseButtonGroup<T>::SelectedItem() const {
  if (!HasSelection()) {
    throw std::runtime_error("ButtonGroup: no entry is selected");
  }
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
    if (item.type == ItemType::Button) {
      btnSize.x = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, label);
    } else if (item.type == ItemType::Label) {
      btnSize.x = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
    } else if (item.type == ItemType::CheckableButton) {
      btnSize.x = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, label);
    }

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
  case ButtonStatus::BUTTON_STATUS_SUCCESS:
    ImGuiHelper::PushButtonStyle(ButtonStyle::BUTTON_STYLE_SUCCESS);
    break;
  case ButtonStatus::BUTTON_STATUS_FAILURE:
    ImGuiHelper::PushButtonStyle(ButtonStyle::BUTTON_STYLE_FAILURE);
    break;
  case ButtonStatus::BUTTON_STATUS_WARNING:
    ImGuiHelper::PushButtonStyle(ButtonStyle::BUTTON_STYLE_WARNING);
    break;
  default:
    ImGuiHelper::PushButtonStyle(ButtonStyle::BUTTON_STYLE_DEFAULT);
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
