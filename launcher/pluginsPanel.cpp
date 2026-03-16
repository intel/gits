// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginsPanel.h"
#include "configurationLib.h"
#include "fileActions.h"
#include "launcherActions.h"

namespace {
// Helpers and messages / labels
const std::filesystem::path GetPluginsDirectory() {
  const auto& gitsBasePath =
      gits::gui::Context::GetInstance().GetPathSafe(gits::gui::Path::GITS_BASE);

  if (gitsBasePath.empty() || !std::filesystem::exists(gitsBasePath)) {
    return std::string();
  }

  // As of now plugins are DirectX only
  return gitsBasePath / "Plugins" / "DirectX";
}

static constexpr const char* PLUGIN_CONFIG_FILENAME = "config.yml";
static constexpr const char* PLUGIN_CONFIG_GENERIC_ERROR_MESSAGE = "Couldn't load plugin's config.";
static constexpr const char* PLUGIN_CONFIG_DOESNT_EXIST_MESSAGE =
    "Couldn't load plugin's config. Plugin's config file doesn't exist";
static constexpr const char* NO_GITS_BASE_PATH_MESSAGE =
    "Couldn't find plugins directory. No GITS base path was selected.";
static constexpr const char* GITS_BASE_PATH_DOESNT_EXIST_MESSAGE =
    "Couldn't find plugins directory. Selected GITS base path doesn't exist.";
static constexpr const char* INVALID_GITS_CONFIG_PATH_MESSAGE =
    "Enabling plugins requires a GITS config. Please select a valid config path.";
} // namespace

namespace gits::gui {

PluginsPanel::PluginsPanel() : BasePanel(), m_PluginConfigEditor("PluginConfigEditor") {
  m_PluginConfigEditor.SetCheckCallback(
      ValidateYaml); // Since plugin configs don't have a set structure, we use a generic YAML validation

  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&PluginsPanel::BasePathCallback, this, std::placeholders::_1),
      {PathEvent::Type::GITS_BASE});
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&PluginsPanel::ConfigPathCallback, this, std::placeholders::_1),
      {PathEvent::Type::CONFIG});
  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&PluginsPanel::ModeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ModeChanged});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&PluginsPanel::ConfigEditedCallback, this, std::placeholders::_1),
      {ContextEvent::Type::ConfigEdited});
}

void PluginsPanel::Render() {
  const auto& gitsBasePath = Context::GetInstance().GetPathSafe(Path::GITS_BASE);

  if (gitsBasePath.empty()) {
    ImGui::Text(NO_GITS_BASE_PATH_MESSAGE);
    return;
  }

  if (!std::filesystem::exists(gitsBasePath)) {
    ImGui::Text(GITS_BASE_PATH_DOESNT_EXIST_MESSAGE);
    return;
  }

  const auto& pluginsPath = GetPluginsDirectory();

  if (!std::filesystem::exists(pluginsPath)) {
    ImGui::Text(("Couldn't find plugins directory. Expected directory: " + pluginsPath.string() +
                 " doesn't exist.")
                    .c_str());
    return;
  }

  const auto& currentConfig = Context::GetInstance().GetPathSafe(Path::CONFIG);

  if (currentConfig.empty() || !std::filesystem::exists(currentConfig)) {
    ImGui::Text(INVALID_GITS_CONFIG_PATH_MESSAGE);
    return;
  }

  if (ImGui::BeginTable("PluginsPanelTable", 2,
                        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {

    // PluginsList
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild("LeftPane", ImVec2(0, 0), ImGuiChildFlags_Border);
    RenderPluginsList();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // Selected plugin config
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild("RightPane", ImVec2(0, 0), ImGuiChildFlags_Border);
    RenderSelectedPluginConfig();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndTable();
  }
}

void PluginsPanel::InvalidatePluginsList() {
  m_PluginsListNeedsUpdating = true;
}

bool PluginsPanel::LoadPluginConfig(std::filesystem::path pluginDirectoryName) {
  const auto& pluginsPath = GetPluginsDirectory();
  if (pluginsPath.empty() || !std::filesystem::exists(pluginsPath)) {
    m_PluginConfigEditor.SetText(PLUGIN_CONFIG_GENERIC_ERROR_MESSAGE);
    return false;
  }

  const auto& pluginConfigPath = GetPluginConfigPath(pluginDirectoryName);

  if (pluginConfigPath.empty() || !std::filesystem::exists(pluginConfigPath)) {
    m_PluginConfigEditor.SetText(PLUGIN_CONFIG_DOESNT_EXIST_MESSAGE);
    return false;
  }

  auto fhandle = std::ifstream(pluginConfigPath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    m_PluginConfigEditor.SetText(str);
    m_PluginConfigEditor.SetFilePath(pluginConfigPath);
  } else {
    m_PluginConfigEditor.SetText("// Could not open file:\n> " + pluginConfigPath.string());
    return false;
  }

  return true;
}

std::filesystem::path PluginsPanel::GetPluginConfigPath(std::filesystem::path pluginDirectoryName) {
  const auto pluginsPath =
      Context::GetInstance().GetPathSafe(Path::GITS_BASE) / "Plugins" / "DirectX";

  if (pluginsPath.empty()) {
    return std::filesystem::path();
  }

  const auto pluginConfigPath = pluginsPath / pluginDirectoryName / PLUGIN_CONFIG_FILENAME;
  if (!std::filesystem::exists(pluginConfigPath)) {
    return std::filesystem::path();
  }

  return pluginConfigPath;
}

std::optional<PluginsPanel::PluginListEntry> PluginsPanel::GetPluginListEntry(
    std::filesystem::path pluginPath) {
  std::optional<PluginListEntry> entry = PluginListEntry{};

  if (pluginPath.empty() || !std::filesystem::exists(pluginPath)) {
    LOG_WARNING << "Couldn't add plugin " << entry.value().DirectoryName
                << " to the plugins list. Plugin path: " << pluginPath << " doesn't exist";
    return std::nullopt;
  }

  entry.value().DirectoryName = pluginPath.filename();

  const auto& pluginConfigPath = GetPluginConfigPath(entry.value().DirectoryName);
  if (pluginConfigPath.empty() || !std::filesystem::exists(pluginConfigPath)) {
    LOG_WARNING << "Couldn't add plugin " << entry.value().DirectoryName
                << " to the plugins list. Plugin's config path: " << pluginConfigPath
                << " doesn't exist";
    return std::nullopt;
  }

  try {
    const auto& yaml = YAML::LoadFile(pluginConfigPath.string());
    if (yaml["Info"]) {
      if (const auto nameNode = yaml["Info"]["Name"]) {
        const auto& name = nameNode.as<std::string>();
        if (name.empty()) {
          return std::nullopt;
        }
        entry.value().Name = name;
      } else {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  } catch (const std::exception& e) {
    LOG_WARNING << "Couldn't add plugin " << entry.value().DirectoryName
                << " to the plugins list. Couldn't load plugin's config: "
                << pluginConfigPath.string() << ". Error: " << e.what();
    return std::nullopt;
  }

  const auto& pluginsList = Context::GetInstance().AppMode == Mode::CAPTURE
                                ? Configurator::Get().directx.recorder.plugins
                                : Configurator::Get().directx.player.plugins;

  entry.value().Enabled = std::ranges::find(pluginsList, entry.value().Name) != pluginsList.end();

  return entry;
}

void PluginsPanel::LoadPluginsList() {
  m_PluginsList.clear();

  // We need to load the GITS config to know which plugins are already enabled
  const auto& gitsConfigPath = Context::GetInstance().GetPathSafe(Path::CONFIG);
  if (gitsConfigPath.empty() || !std::filesystem::exists(gitsConfigPath)) {
    return;
  }

  if (!Configurator::Instance().Load(gitsConfigPath)) {
    return;
  }

  const auto& pluginsPath = GetPluginsDirectory();

  if (!pluginsPath.empty() && std::filesystem::exists(pluginsPath)) {
    for (const auto& entry : std::filesystem::directory_iterator{pluginsPath}) {

      if (entry.is_directory()) {
        const auto listEntry = GetPluginListEntry(entry.path());

        if (!listEntry.has_value()) {
          continue;
        }

        m_PluginsList.push_back(listEntry.value());
      }
    }
  }
}

void PluginsPanel::UpdateGitsConfig(std::string pluginName, bool enable) {
  const auto& gitsConfigPath = Context::GetInstance().GetPathSafe(Path::CONFIG);
  if (gitsConfigPath.empty() || !std::filesystem::exists(gitsConfigPath)) {
    return;
  }

  if (enable) {
    LOG_INFO << "Enabling " << pluginName << "DirectX plugin";
  } else {
    LOG_INFO << "Disabling " << pluginName << "DirectX plugin";
  }
  std::vector<std::string> pluginsList = Context::GetInstance().AppMode == Mode::CAPTURE
                                             ? Configurator::Get().directx.recorder.plugins
                                             : Configurator::Get().directx.player.plugins;
  if (enable) {
    pluginsList.push_back(pluginName);
  } else {
    std::erase(pluginsList, pluginName);
  }

  const std::string modeString =
      Context::GetInstance().AppMode == Mode::CAPTURE ? "Recorder" : "Player";

  FileActions::UpdateConfigYamlPath(gitsConfigPath, {"DirectX", modeString, "Plugins"},
                                    pluginsList);

  EventBus::GetInstance().publish<ContextEvent>(ContextEvent::Type::PluginsUpdated);
  EventBus::GetInstance().publish<ContextEvent>(ContextEvent::Type::ConfigEdited);
}

void PluginsPanel::RenderPluginsList() {
  const auto& pluginsPath = GetPluginsDirectory();
  ImGui::Text(("Plugins path: " + pluginsPath.string()).c_str());

  if (m_PluginsListNeedsUpdating) {
    LoadPluginsList();
    m_PluginsListNeedsUpdating = false;
  }

  for (int i = 0; i < m_PluginsList.size(); i++) {
    auto& plugin = m_PluginsList[i];
    if (ImGui::Checkbox(("##" + plugin.Name + "checkbox").c_str(), &plugin.Enabled)) {
      UpdateGitsConfig(plugin.Name, plugin.Enabled);
    }
    // We render the label separately for it to not extend the checkbox's hitbox
    ImGui::SameLine();
    bool selected = m_CurrentSelection == i;
    if (ImGui::Selectable(plugin.Name.c_str(), &selected)) {
      m_CurrentSelection = i;

      const auto& selectedPlugin = m_PluginsList[m_CurrentSelection];
      const auto pluginConfigPath = GetPluginConfigPath(selectedPlugin.DirectoryName.string());

      if (pluginConfigPath.empty()) {
        return;
      }

      LoadPluginConfig(selectedPlugin.DirectoryName);
    }
  }
}

void PluginsPanel::RenderSelectedPluginConfig() {
  if (!m_PluginsList.empty() && m_CurrentSelection > -1 &&
      m_CurrentSelection < m_PluginsList.size()) {
    const auto& selectedPlugin = m_PluginsList[m_CurrentSelection];

    ImGui::Text(("Plugin: " + selectedPlugin.Name).c_str());
    m_PluginConfigEditor.Render();
  }
}

void PluginsPanel::BasePathCallback(const Event& e) {
  InvalidatePluginsList();
}

void PluginsPanel::ConfigPathCallback(const Event& e) {
  InvalidatePluginsList();
}

void PluginsPanel::ModeChangedCallback(const Event& e) {
  InvalidatePluginsList();
}

void PluginsPanel::ConfigEditedCallback(const Event& e) {
  InvalidatePluginsList();
}

} // namespace gits::gui
