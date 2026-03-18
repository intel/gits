// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "eventBus.h"
#include "textEditorWidget.h"

namespace gits::gui {

class PluginsPanel : public BasePanel {
public:
  PluginsPanel();
  void Render() override;

private:
  struct PluginListEntry {
    std::filesystem::path DirectoryName;
    std::string Name; // This is the actual plugin name that GITS expects
    bool Enabled;
  };

  std::vector<PluginListEntry> m_PluginsList;
  bool m_PluginsListNeedsUpdating = true;
  int m_CurrentSelection = -1; // Currently selected plugin
  gits::ImGuiHelper::TextEditorWidget m_PluginConfigEditor;

  void InvalidatePluginsList();
  bool LoadPluginConfig(const std::filesystem::path& pluginDirectoryName);
  std::filesystem::path GetPluginConfigPath(const std::filesystem::path& pluginDirectoryName);
  std::optional<PluginListEntry> GetPluginListEntry(const std::filesystem::path& pluginPath);
  void LoadPluginsList();
  void UpdateGitsConfig(const std::string& pluginName, const bool enable);
  void RenderPluginsList();
  void RenderSelectedPluginConfig();

  // Event callbacks
  void BasePathCallback(const Event& e);
  void ConfigPathCallback(const Event& e);
  void ModeChangedCallback(const Event& e);
  void ConfigEditedCallback(const Event& e);
};

} // namespace gits::gui
