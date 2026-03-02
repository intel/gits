// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contextHelper.h"

#include "imGuiHelper.h"

#include "launcherActions.h"

namespace gits::gui::context_helper {

bool PathInput(const char* labelID,
               Path path,
               std::optional<Mode> appMode,
               ImGuiInputTextFlags flags,
               const float widthInput) {
  auto& context = Context::GetInstance();
  auto optPath = context.GetPath(path, appMode);
  if (!optPath.has_value()) {
    return false;
  }
  std::string str = optPath->string();

  const auto changed = gits::ImGuiHelper::InputString(labelID, str, flags, widthInput);
  if (changed) {
    context.SetPath(std::filesystem::path(str), path, appMode);
  }
  return changed;
}

void PathMenuItem(const std::string& label, Path path, std::optional<Mode> appMode) {
  auto& context = Context::GetInstance();
  auto optPath = context.GetPath(path, appMode);
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
    OpenFolder(pathValue);
  }
  ImGui::EndDisabled();
}
} // namespace gits::gui::context_helper
