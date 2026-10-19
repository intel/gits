// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contextHelper.h"
#include "baseCommon.h"
#include "imGuiHelper.h"

#include "guiActions.h"

namespace gits::gui::context_helper {

bool PathInput(const char* labelID,
               Path path,
               std::optional<GitsAction> gitsAction,
               ImGuiInputTextFlags flags,
               const float widthInput) {
  auto& context = BaseContext::GetBaseInstance();
  auto optPath = context.GetPath(path, gitsAction);
  std::string str = "";
  if (optPath.has_value()) {
    str = optPath->string();
  }

  const auto changed = gits::ImGuiHelper::InputString(labelID, str, flags, widthInput);
  if (changed) {
    context.SetPath(std::filesystem::path(str), path, gitsAction);
  }
  return changed;
}

void PathMenuItem(const std::string& label, Path path, std::optional<GitsAction> gitsAction) {
  auto& context = BaseContext::GetBaseInstance();

  auto optPath = context.GetPath(path, gitsAction);
  if (!optPath.has_value()) {
    return;
  }
  ImGui::BeginDisabled(optPath->empty());
  if (ImGui::MenuItem(label.c_str())) {
    auto pathValue = optPath.value();
    if (std::filesystem::is_directory(pathValue)) {
      pathValue = std::filesystem::absolute(pathValue);
    } else {
      pathValue = pathValue.parent_path();
    }
    gui_actions::OpenFolder(pathValue);
  }
  ImGui::EndDisabled();
}

void RenderUIViewWidget(float width) {
  auto& context = BaseContext::GetBaseInstance();
  float horizontalSkip = ImGui::GetContentRegionAvail().y - 4.0f * ImGui::GetTextLineHeight();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + horizontalSkip);

  auto configuration = context.GetBaseConfiguration();
  if (configuration == nullptr) {
    return;
  }
  static int selectedItem = configuration->Theme.CurrentThemeIdx;
  ImGui::Text("- View Options");
  auto buttonSize = ImVec2(
      (width - 16.0f - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, "Scale")) / 3.0f, 0.0f);
  ImGui::Text("Scale");
  ImGui::SameLine();

  if (ImGui::Button("-", buttonSize)) {
    context.UIScaleDelta = -0.1f;
  }
  ImGui::SameLine();
  if (ImGui::Button("1", buttonSize)) {
    context.UIScaleDelta = 0.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("+", buttonSize)) {
    context.UIScaleDelta = +0.1f;
  }
}

static size_t imGuiID = 0;

std::filesystem::path RenderRecentMenuList(const std::vector<std::filesystem::path>& entries,
                                           const std::string& name) {
  std::filesystem::path result;
  if (entries.size() == 0) {
    ImGui::BeginDisabled();
    ImGui::MenuItem((std::string("No recent ") + name).c_str(), nullptr, false, false);
    ImGui::EndDisabled();
  } else {
    for (const auto& entry : entries) {
      auto strImGuiID = std::to_string(imGuiID++);
      if (ImGui::MenuItem((entry.filename().string() + "###" + strImGuiID).c_str())) {
        result = entry;
      }
      ImGuiHelper::AddTooltip(entry.string().c_str());
    }
  }
  return result;
}

void RenderRecentMenu(bool showStreams, bool showTargets) {
  auto& context = BaseContext::GetBaseInstance();

  auto config = context.GetBaseConfiguration();
  if (config && (showStreams || showTargets)) {
    if (ImGui::BeginMenu("Recent")) {
      if (showStreams) {
        const auto& recentStreams = config->RecentStreams();
        auto selectedStream = RenderRecentMenuList(recentStreams, "streams");
        if (!selectedStream.empty()) {
          context.SelectedAction = GitsAction::PLAYBACK;
          context.SetPath(selectedStream, Path::INPUT_STREAM, context.SelectedAction);
        }
      }
      if (showStreams && showTargets) {
        ImGui::Separator();
      }
      if (showTargets) {
        const auto& recentTargets = config->RecentTargets();
        auto selectedTarget = RenderRecentMenuList(recentTargets, "targets");
        if (!selectedTarget.empty()) {
          context.SelectedAction = GitsAction::CAPTURE;
          context.SetPath(selectedTarget, Path::CAPTURE_TARGET, context.SelectedAction);
        }
      }
      ImGui::EndMenu();
    }
  }
}
} // namespace gits::gui::context_helper
