// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "yamlPanel.h"

#include <optional>
#include <functional>
#include <string>
#include <imgui.h>
#include <algorithm>
#include "imGuiHelper.h"

#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "eventBus.h"
#include "common.h"
#include <format>

namespace gits::gui {

YAMLPanel::YAMLPanel() : m_ConfigEditor("YAMLConfigEditor"), m_YamlTreeViewer() {
  m_YamlTreeViewer.SetTextEditor(&m_ConfigEditor.GetEditor());
  m_ConfigEditor.GetEditor().SetShowWhitespaces(false);
  m_ConfigEditor.GetEditor().SetTabSize(4);
  m_ConfigEditor.SetCheckCallback(&ValidateGITSConfig);
  m_ConfigEditor.GetEditor().SetLanguageDefinition(TextEditorWidget::GetYamlLanguageDefinition());

  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&YAMLPanel::ModeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ModeChanged});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&YAMLPanel::InMemoryConfigChangedCallback, this, std::placeholders::_1),
      {ContextEvent::Type::InMemoryConfigurationChanged});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&YAMLPanel::ConfigFileChangedCallback, this, std::placeholders::_1),
      {ContextEvent::Type::ConfigFileLoaded});
  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&YAMLPanel::ThemeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ThemeChanged});
}

void YAMLPanel::Render() {
  auto currentConfig = m_ActiveConfig;

  auto& context = Context::GetInstance();
  auto configPath = context.GetPathSafe(Path::CONFIG);
  bool hasConfigFile = !configPath.empty();
  std::string configFilename = "default config";
  if (hasConfigFile) {
    configFilename = configPath.filename().string();
  }

  if (ImGui::BeginTabBar("YAMLEditorTabs")) {
    if (ImGui::BeginTabItem("In-Memory Config")) {
      currentConfig = YAMLEditorConfig::FULL_CONFIG;
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(configFilename.c_str())) {
      currentConfig = YAMLEditorConfig::BASE_CONFIG;
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  if (currentConfig != m_ActiveConfig) {
    m_ActiveConfig = currentConfig;

    if (m_ActiveConfig == YAMLEditorConfig::NO_CONFIG) {
      ImGui::Text("Not implemented yet.");
      return;
    }

    UpdateContent();
  }

  ImVec2 available = ImGui::GetContentRegionAvail();
  const auto col1Ratio = std::min(256.0f / available.x, 0.25f);
  if (ImGui::BeginTable("YAMLEditorTable", 2,
                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
    ImGui::TableSetupColumn("TreeViewColumn", ImGuiTableColumnFlags_WidthStretch, col1Ratio);
    ImGui::TableSetupColumn("EditorColumn", ImGuiTableColumnFlags_WidthStretch, 1.0f - col1Ratio);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    m_YamlTreeViewer.Render();
    ImGui::TableSetColumnIndex(1);

    switch (m_ActiveConfig) {
    case YAMLEditorConfig::FULL_CONFIG: {
      auto label = std::format("{} '{}'", Labels::DELTA_CONFIG_NOTICE, configFilename);
      if (ImGui::Checkbox(label.c_str(), &m_ShowOnlyDelta)) {
        UpdateContent();
      }
      break;
    }
    case YAMLEditorConfig::BASE_CONFIG:
      ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
      ImGui::Text(Labels::BASE_CONFIG_NOTICE);
      ImGui::PopStyleColor();
      break;
    default:
      ImGui::Text("");
      break;
    }
    if (m_ActiveConfig != YAMLEditorConfig::BASE_CONFIG) {
      ImGui::SameLine();
    }

    m_ConfigEditor.Render(available);

    ImGui::EndTable();
  }
}

void YAMLPanel::UpdateContent() {
  static auto editableEditorConfig = TextEditorWidget::Config{
      .ShowToolbar = true,
      .ToolBarItems = {
          TextEditorWidget::TOOL_BAR_ITEMS::SAVE, TextEditorWidget::TOOL_BAR_ITEMS::REVERT,
          TextEditorWidget::TOOL_BAR_ITEMS::UNDO, TextEditorWidget::TOOL_BAR_ITEMS::REDO,
          TextEditorWidget::TOOL_BAR_ITEMS::CHECK, TextEditorWidget::TOOL_BAR_ITEMS::EXPORT}};

  auto& context = Context::GetInstance();
  auto& configurationForMode = context.ConfigurationForMode();
  std::optional<std::string> ymlText = std::nullopt;
  switch (m_ActiveConfig) {
  case YAMLEditorConfig::FULL_CONFIG: {
    if (m_ShowOnlyDelta) {
      ymlText = configurationForMode.ConfigDeltaStr;
    } else {
      ymlText = GetYamlStringFromConfig(configurationForMode);
    }
    m_ConfigEditor.SetConfig(editableEditorConfig);
    m_ConfigEditor.SetSaveCallback(
        std::bind(&YAMLPanel::UpdateInMemoryConfig, this, std::placeholders::_1));
    m_ConfigEditor.SetExportCallback(
        std::bind(&YAMLPanel::SaveInMemoryConfigToFile, this, std::placeholders::_1));
    m_ConfigEditor.SetLiveCheck(true);
    m_ConfigEditor.SetShowCheckStatus(true);
  } break;
  case YAMLEditorConfig::BASE_CONFIG: {
    m_ConfigEditor.SetConfig(TextEditorWidget::CONFIG_NO_TOOLBAR);
    m_ConfigEditor.SetLiveCheck(false);
    m_ConfigEditor.SetShowCheckStatus(false);
    ymlText = configurationForMode.BaseGitsConfigurationStr;
  } break;
  default:
    break;
  }

  if (ymlText) {
    m_ConfigEditor.GetEditor().SetReadOnly(m_ActiveConfig == YAMLEditorConfig::BASE_CONFIG);
    m_ConfigEditor.SetText(ymlText.value());
    m_YamlTreeViewer.SetYAMLText(ymlText.value());
  }
}

bool YAMLPanel::UpdateInMemoryConfig(std::filesystem::path filePath) {
  auto& context = Context::GetInstance();
  auto& currentModeConfiguration = context.ConfigurationForMode();

  bool success = false;
  std::string text = m_ConfigEditor.GetText();
  YAML::Node yaml = YAML::Load(text);
  bool result = true;
  if (m_ShowOnlyDelta) {
    result &= gits::Configurator::LoadInto(currentModeConfiguration.BaseGitsConfigurationStr,
                                           &currentModeConfiguration.ModifiedGitsConfiguration);
  }
  result &= gits::Configurator::LoadInto(text, &currentModeConfiguration.ModifiedGitsConfiguration);
  if (result) {
    if (yaml["Overrides"]) {
      currentModeConfiguration.ModifiedOverrides = yaml["Overrides"];
    }
    success = true;
  } else {
    LOG_ERROR << "Couldn't serialize GITS configuration from the YAML editor";
  }
  currentModeConfiguration.ModifiedGitsConfigurationStr = text;

  context.UpdateInMemoryConfig();
  return success;
}

void YAMLPanel::SaveInMemoryConfigToFile(std::filesystem::path filePath) {
  UpdateInMemoryConfig(filePath);
  auto& context = Context::GetInstance();
  auto exportPath = std::filesystem::path();
  switch (context.AppMode) {
  case Mode::CAPTURE:
    exportPath = context.GetPathSafe(Path::CAPTURE_TARGET, Mode::CAPTURE);
    break;
  case Mode::PLAYBACK:
    exportPath = context.GetGITSPlayerPath();
    break;
  case Mode::SUBCAPTURE:
    exportPath = context.GetGITSPlayerPath();
    break;
  default:
    break;
  }
  if (!exportPath.empty()) {
    exportPath = exportPath.parent_path();
    auto timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << "gits_config_" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d_%H-%M-%S")
       << ".yml";
    exportPath /= ss.str();
  }
  ShowFileDialog(FileDialogKey{.Path = Path::CONFIG_EXPORT, .Mode = context.AppMode}, exportPath);
}

void YAMLPanel::ModeChangedCallback(const Event& e) {
  m_ActiveConfig = YAMLEditorConfig::NO_CONFIG;
}

void YAMLPanel::InMemoryConfigChangedCallback(const Event& e) {
  m_ActiveConfig = YAMLEditorConfig::NO_CONFIG;
}

void YAMLPanel::ConfigFileChangedCallback(const Event& e) {
  m_ActiveConfig = YAMLEditorConfig::NO_CONFIG;
}

void YAMLPanel::ThemeChangedCallback(const Event& e) {
  m_ConfigEditor.UpdatePalette();
}
} // namespace gits::gui
