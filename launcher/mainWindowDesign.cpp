// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mainWindow.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <iostream>
#include <istream>
#include <vector>
#include <map>
#include <cmath>
#include "imgui.h"
#include "ImGuiFileDialog.h"

#include "log.h"
#include "imGuiHelper.h"
#include "context.h"
#include "launcherConfig.h"
#include "buttonGroup.h"
#include "resource.h"

/**
 * This file contains the design definition for the main window of the GITS Launcher GUI.
 */

#pragma region label texts, names, enums ...
namespace {
namespace gui = gits::gui;
namespace ImGuiHelper = gits::ImGuiHelper;
typedef gits::gui::Context::SideBarItems SideBarItems;
typedef gits::gui::Context::ConfigSectionItems ConfigSectionItems;

enum class FileDialogKeys {
  PICK_STREAM_PATH = 0,
  PICK_GITSPLAYER_PATH = 1,
  PICK_CONFIG_PATH = 2
};

const std::string str(FileDialogKeys key) {
  switch (key) {
  case FileDialogKeys::PICK_STREAM_PATH:
    return "PickStreamPath";
  case FileDialogKeys::PICK_GITSPLAYER_PATH:
    return "PickGitsPlayerPath";
  case FileDialogKeys::PICK_CONFIG_PATH:
    return "PickConfigPath";
  default:
    return "Unknown";
  }
}

struct Labels {
  // main window
  static constexpr const char* CHOOSE_CONFIG = "Browse";
  static constexpr const char* CHOOSE_CONFIG_HINT =
      "Open File Dialog to choose a configuration yaml";
  static constexpr const char* STREAM = "Stream";
  static constexpr const char* STREAM_HINT = "Select a stream";
  static constexpr const char* TARGET = "Target";
  static constexpr const char* TARGET_HINT = "Select an application to run the stream with";
  static constexpr const char* CAPTURE = "Capture";
  static constexpr const char* CAPTURE_HINT = "Capture a new stream";
  static constexpr const char* PLAYBACK = "Playback";
  static constexpr const char* PLAYBACK_HINT = "Playback an existing stream";
  static constexpr const char* START = "Start";
  static constexpr const char* START_HINT = "Start playback/capture";
  static constexpr const char* BASE_PATH = "Gits Player";
  static constexpr const char* BASE_PATH_HINT = "Path to GITS Player";
  static constexpr const char* CONFIG = "Config";
  static constexpr const char* CONFIG_HINT = "Path to gits_config file";
  static constexpr const char* CHOOSE_STREAM = "Browse";
  static constexpr const char* CHOOSE_STREAM_HINT =
      "Open File Dialog to choose a stream file (inside a folder)";
  static constexpr const char* CHOOSE_GITSPLAYER = "Browse";
  static constexpr const char* CHOOSE_GITSPLAYER_HINT =
      "Open File Dialog to choose the gitsPlayer to be used";
  static constexpr const char* CUSTOM_ARGS = "Custom Args";
  static constexpr const char* CUSTOM_ARGS_INPUT_HINT =
      "Additional command line arguments for the target application";
  static constexpr const char* CLEAR_ARGUMENTS = "Clear";
  static constexpr const char* CLEAR_ARGUMENTS_HINT = "Clear all custom arguments in the textbox";

  // main window - text fields
  static constexpr const char* STREAM_INPUT_HINT = "Path to stream";
  static constexpr const char* BASE_PATH_INPUT_HINT = "Path to GITS base directory";
  static constexpr const char* CONFIG_INPUT_HINT = "Path to gits_config file";

  static const std::string MainAction(gits::gui::Context::MainAction action) {
    switch (action) {
    case gits::gui::Context::MainAction::PLAYBACK:
      return "Start Playback";
    case gits::gui::Context::MainAction::STATISTICS:
      return "Gather Statistics";
    case gits::gui::Context::MainAction::COUNT:
    default:
      return "";
    }
  }

  static const auto& SIDE_BAR() {
    static const std::map<SideBarItems, ImGuiHelper::ButtonGroupItem> items = {
        {SideBarItems::CONFIG, {"Config", "Show current gits_config"}},
        {SideBarItems::CLI, {"CLI", "Show the full command line call"}},
        {SideBarItems::LOG, {"GITS Log", "Show the output of gitsPlayer"}},
        {SideBarItems::STATS, {"Stats", "Show statistics of the stream"}},
        {SideBarItems::APP_LOG, {"Launcher Log", "Show the launcher log"}},
        {SideBarItems::OPTIONS,
         {.label = "EZ-Opt", .tooltip = "Show the easy to setup options", .enabled = false}},
        {SideBarItems::INFO,
         {.label = "Info", .tooltip = "Show the stream information", .enabled = false}},
        {SideBarItems::API_TRACE,
         {.label = "API-Trace", .tooltip = "Show the API-Trace of the stream", .enabled = false}},
    };
    return items;
  }
  static const auto& SIDE_BAR_VEC() {
    static const std::vector<std::string> values([] {
      std::vector<std::string> result;
      for (const auto& pair : SIDE_BAR()) {
        result.push_back(pair.second.label);
      }
      return result;
    }());
    return values;
  }

  static const auto& CONFIG_SECTIONS() {
    static const std::map<ConfigSectionItems, ImGuiHelper::ButtonGroupItem> labels = {
        {ConfigSectionItems::COMMON,
         {"Common", "Common options", ImGuiHelper::ButtonStatus::Default, "Cmn"}},
        {ConfigSectionItems::DIRECTX,
         {"DirectX", "DirectX API options", ImGuiHelper::ButtonStatus::Default, "DX"}},
        {ConfigSectionItems::OPENGL,
         {"OpenGL", "OpenGL API options", ImGuiHelper::ButtonStatus::Default, "GL"}},
        {ConfigSectionItems::VULKAN,
         {"Vulkan", "Vulkan API options", ImGuiHelper::ButtonStatus::Default, "VK"}},
        {ConfigSectionItems::OPENCL,
         {"OpenCL", "OpenCL API options", ImGuiHelper::ButtonStatus::Default, "CL"}},
        {ConfigSectionItems::LEVELZERO,
         {"LevelZero", "LevelZero API options", ImGuiHelper::ButtonStatus::Default, "L0"}},
        {ConfigSectionItems::OVERRIDES,
         {"Overrides", "Per application overrides", ImGuiHelper::ButtonStatus::Default, "Ovr"}},
    };
    return labels;
  }

  float static MaxLength(const std::vector<std::string>& labels) {
    float maximum = std::numeric_limits<float>::min();
    for (const auto& label : labels) {
      maximum = std::max(maximum, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, label));
    }
    return maximum;
  }
};
#pragma endregion

#pragma region functionality related functions
void UpdateConfigSectionPositions(gui::Context* context, const std::vector<std::string>& config) {
  size_t idx = 0;
  for (const auto& item : Labels::CONFIG_SECTIONS()) {
    int pos = -1;
    const std::string sectionHeader = item.second.label + ":";
    for (size_t lineNum = 0; lineNum < config.size(); ++lineNum) {
      const std::string& line = config[lineNum];
      if (line.compare(0, sectionHeader.size(), sectionHeader) == 0) {
        pos = static_cast<int>(lineNum);
        break;
      }
    }
    context->ConfigSectionLines[item.first] = pos;
    idx++;
  }
}

void LoadConfigFile(gui::Context* context) {
  auto filePath = context->GetPath(gui::Context::Paths::CONFIG);
  auto fhandle = std::ifstream(filePath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    context->ConfigEditor->SetText(str);
    UpdateConfigSectionPositions(context, context->ConfigEditor->GetEditor().GetTextLines());
    TextEditor::Breakpoints breakpoints;
    for (const auto& section : context->ConfigSectionLines) {
      int line = section.second;
      context->BtnsAPI->SetEnabled(section.first, line >= 0);
      if (line >= 0) {
        breakpoints.insert(line + 1);
      }
    }
    context->ConfigEditor->SetFilePath(filePath);
    context->ConfigEditor->SetBreakpoints(breakpoints);
  } else {
    context->ConfigEditor->SetText("// Could not open file: " + filePath.string());
  }
}

bool ValidateGITSConfigFile(std::filesystem::path configPath) {
  if (!std::filesystem::exists(configPath)) {
    LOG_ERROR << "Configuration file does not exist: " << configPath;
    return false;
  }

  //// In the future a more detailed validation can be added here
  //const auto result = gits::Configurator::Instance().Load(configPath);
  //if (!result) {
  //  LOG_ERROR << "Error reading in configuration from: " << configPath;
  //  return false;
  //}

  return true;
}

void UpdateCLICall(gui::Context* context) {
  const auto gitsExecutable = context->GetPath(gui::Context::Paths::GITS_PLAYER);

  context->CLIArguments.clear();

  context->CLIArguments.push_back("--config=" +
                                  context->GetPath(gui::Context::Paths::CONFIG).string());
  context->CLIArguments.push_back(context->FixedLauncherArguments);
  context->CLIArguments.push_back(context->CustomArguments);
  context->CLIArguments.push_back(context->GetPath(gui::Context::Paths::STREAM).string());

  context->CLIArguments.erase(std::remove_if(context->CLIArguments.begin(),
                                             context->CLIArguments.end(),
                                             [](const std::string& arg) { return arg.empty(); }),
                              context->CLIArguments.end());

  std::string cliEditorText =
      "# This buffer is write protected, it shows the current command line\n\n";

  cliEditorText += gitsExecutable.string() + "\n";
  if (!std::filesystem::exists(gitsExecutable)) {
    cliEditorText += "!!  [Warning: gitsPlayer path does not exist]\n";
  }

  for (const auto& argument : context->CLIArguments) {
    cliEditorText += "  " + argument + "\n";
  }
  context->CLIEditor->SetText(cliEditorText);
}

void PlaybackStream(gui::Context* context) {
  const auto gitsPlayerPath = context->GetPath(gui::Context::Paths::GITS_PLAYER);

  context->GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(
      gitsPlayerPath, context->CLIArguments, gitsPlayerPath.parent_path(),
      std::bind(&gui::Context::GITSLog, context, std::placeholders::_1));
  context->BtnsSideBar->SelectEntry(SideBarItems::LOG);
}

void GetTraceStats(gui::Context* context) {
  const auto gitsPlayerPath = context->GetPath(gui::Context::Paths::GITS_PLAYER);

  context->GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(
      gitsPlayerPath, {"--stats", context->GetPath(gui::Context::Paths::STREAM).string()},
      gitsPlayerPath.parent_path(),
      std::bind(&gui::Context::TraceStats, context, std::placeholders::_1));
  context->BtnsSideBar->SelectEntry(SideBarItems::STATS);
}

#pragma endregion

#pragma region UI Helpers
std::optional<std::string> ProcessFileDialog(FileDialogKeys key) {
  std::optional<std::string> result = std::nullopt;
  if (ImGuiFileDialog::Instance()->Display(str(key), 32, ImVec2(500, 300))) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      result = ImGuiFileDialog::Instance()->GetFilePathName();
    }
    ImGuiFileDialog::Instance()->Close();
  }
  return result;
}

void ShowFileDialog(gui::Context* context, FileDialogKeys key) {
  std::string title;
  std::string ext;
  IGFD::FileDialogConfig dlgConfig;
  dlgConfig.flags = ImGuiFileDialogFlags_Modal;

  switch (key) {
  case FileDialogKeys::PICK_STREAM_PATH:
    dlgConfig.path = context->StreamPath.parent_path().string();
    title = "Choose gits stream";
    ext = ".gits2";
    break;
  case FileDialogKeys::PICK_GITSPLAYER_PATH:
    dlgConfig.path = context->StreamPath.parent_path().string();
    title = "Choose a gitsPlayer";
    ext = ".exe";
    break;
  case FileDialogKeys::PICK_CONFIG_PATH:
    dlgConfig.path = context->ConfigPath.string();
    title = "Choose gits config";
    ext = ".yml";
    break;
  default:
    LOG_ERROR << "Unknown/unimplemented file dialog key requested:" << int(key) << " - "
              << str(key);
    return;
  }
  ImGuiFileDialog::Instance()->OpenDialog(str(key), title, ext.c_str(), dlgConfig);
}

float WidthColumn1(bool resetSize = false) {
  static float cached_width = -1.0;
  if (cached_width < 0 || resetSize) {
    const std::vector<std::string> row_labels = {Labels::TARGET, Labels::STREAM, Labels::BASE_PATH,
                                                 Labels::CONFIG, Labels::CUSTOM_ARGS};
    float maxWidth = Labels::MaxLength(row_labels);
    maxWidth = std::max(maxWidth, Labels::MaxLength(Labels::SIDE_BAR_VEC()));
    cached_width = maxWidth + 8 + 24;
  }
  return cached_width;
}
#pragma endregion

#pragma region UI rows
int g_imguiIDs = 0;

void RowStreamPath(gui::Context* context) {
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  switch (context->AppMode) {
  case gui::Context::Mode::CAPTURE:
    ImGui::Text(Labels::TARGET);
    break;
  case gui::Context::Mode::PLAYBACK:
    ImGui::Text(Labels::STREAM);
    break;
  default:
    break;
  }

  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthColumn1());

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth =
      remainingWidth -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITSPLAYER) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::PLAYBACK) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::CAPTURE) - 24;

  if (ImGuiHelper::InputString("###InputPath", context->StreamPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::STREAM_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++g_imguiIDs);
  if (ImGui::Button(Labels::CHOOSE_STREAM)) {
    ShowFileDialog(context, FileDialogKeys::PICK_STREAM_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_STREAM_HINT);

  static int rbModeValue = context->IsPlayback() ? 0 : 1;
  ImGui::SameLine();
  ImGui::RadioButton(Labels::PLAYBACK, &rbModeValue, 0);
  ImGuiHelper::AddTooltip(Labels::PLAYBACK_HINT);
  ImGui::SameLine();
  ImGui::BeginDisabled(true);
  ImGui::RadioButton(Labels::CAPTURE, &rbModeValue, 1);
  ImGuiHelper::AddTooltip(Labels::CAPTURE_HINT);
  ImGui::EndDisabled();
  context->AppMode =
      (rbModeValue == 0) ? gui::Context::Mode::PLAYBACK : gui::Context::Mode::CAPTURE;
}

void RowBasePath(gui::Context* context) {
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::BASE_PATH);
  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthColumn1());

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto checkBoxWidth = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::PLAYBACK) +
                       ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::CAPTURE) +
                       24;
  auto allocatedWidth =
      remainingWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::PLAYBACK) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::RadioButton, Labels::CAPTURE) - 24 -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITSPLAYER);

  if (ImGuiHelper::InputString("###BasePathInput", context->GITSPlayerPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);
  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITSPLAYER)) {
    ShowFileDialog(context, FileDialogKeys::PICK_GITSPLAYER_PATH);
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITSPLAYER_HINT);

  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.9f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.7f, 0.25f, 1.0f));

  if (ImGui::Button(Labels::MainAction(context->CurrentMainAction).c_str(),
                    ImVec2(checkBoxWidth - 16, 0))) {
    if (context->IsPlayback()) {
      switch (context->CurrentMainAction) {
      case gui::Context::MainAction::PLAYBACK:
        PlaybackStream(context);
        break;
      case gui::Context::MainAction::STATISTICS:
        GetTraceStats(context);
        break;
      default:
        break;
      }
    }
  }
  ImGui::PopStyleColor(3);
  ImGuiHelper::AddTooltip(Labels::START_HINT);
}

void RowArguments(gui::Context* context) {
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthColumn1());

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CLEAR_ARGUMENTS);
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context->CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++g_imguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS)) {
    context->CustomArguments.clear();
    UpdateCLICall(context);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void RowConfigPath(gui::Context* context) {
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CONFIG);
  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthColumn1());

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth =
      remainingWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_CONFIG);

  if (ImGuiHelper::InputString("###ConfigPathInput", context->ConfigPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++g_imguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CONFIG)) {
    ShowFileDialog(context, FileDialogKeys::PICK_CONFIG_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CONFIG_HINT);
}

#pragma endregion

#pragma region Main Window UI sections
void ChildWindowConfig(gui::Context* context) {
  ImVec2 available = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild("ContentArea", ImVec2(available.x, available.y), true)) {

    if (context->BtnsAPI->Render()) {
      auto lineIdx = context->ConfigSectionLines[context->BtnsAPI->Selected()];
      if (lineIdx > -1) {
        auto pos = TextEditor::Coordinates(lineIdx, 0);
        context->ConfigEditor->GetEditor().SetCursorPosition(pos);
      } else {
        LOG_INFO << "Config section line index is invalid or not found. Selected index: "
                 << context->BtnsAPI->SelectedIndex()
                 << ", item:" << context->BtnsAPI->SelectedItem().label;
      }
    }
    context->ConfigEditor->Render(available);
  }
  ImGui::EndChild();
}

void ContentArea(gui::Context* context) {
  if (ImGui::BeginTable("MainLayoutTable", 2,
                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
    ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, WidthColumn1());
    ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    if (context->BtnsSideBar->Render(true)) {
      context->CurrentMainAction = gui::Context::MainAction::PLAYBACK;
      switch (context->BtnsSideBar->Selected()) {
      case SideBarItems::CONFIG:
        LoadConfigFile(context);
        break;
      case SideBarItems::STATS:
        context->CurrentMainAction = gui::Context::MainAction::STATISTICS;
        break;
      default:
        break;
      }
    }
    float heightX2 = ImGui::GetTextLineHeight() * 2.0f;
    if (ImGui::GetContentRegionAvail().y >= heightX2) {
      float space = ImGui::GetContentRegionAvail().y - heightX2;
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + space);
      ImGui::Text("gitsLauncher");
      ImGui::Text(APP_VERSION);
    }

    ImGui::TableSetColumnIndex(1);
    auto area = ImGui::GetContentRegionAvail();
    switch (context->BtnsSideBar->Selected()) {
    case SideBarItems::CONFIG:
      ChildWindowConfig(context);
      break;
    case SideBarItems::LOG:
      ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
      context->GITSLogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::CLI:
      ImGui::BeginChild("CLIArea", ImVec2(area.x, area.y), true);
      context->CLIEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::APP_LOG:
      ImGui::BeginChild("LauncherLogArea", ImVec2(area.x, area.y), true);
      context->LogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::STATS:
      ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
      context->TraceStatsEditor->Render();
      ImGui::EndChild();
      break;
    default:
      ImGui::BeginChild("ContentArea", ImVec2(area.x, area.y), true);
      ImGui::Text("Not implemented yet.");
      ImGui::EndChild();
      return;
    }
    ImGui::EndTable();
  }
}

void FileDialogs(gui::Context* context) {
  bool update = false;
  if (ProcessFileDialog(FileDialogKeys::PICK_STREAM_PATH)) {
    context->StreamPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_CONFIG_PATH)) {
    context->ConfigPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
    LoadConfigFile(context);
  } else if (ProcessFileDialog(FileDialogKeys::PICK_GITSPLAYER_PATH)) {
    context->GITSPlayerPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
  }

  if (update) {
    UpdateCLICall(context);
  }
}
#pragma endregion
} // namespace

namespace gits::gui {
void GUIController::RenderUI() {
  ::g_imguiIDs = 0;
  try {
    RowStreamPath(&m_Context);
    RowBasePath(&m_Context);
    RowConfigPath(&m_Context);
    RowArguments(&m_Context);
    ContentArea(&m_Context);

    FileDialogs(&m_Context);
  } catch (const std::exception& e) {
    ImGui::Text("UI Rendering Error: %s", e.what());
  }
}

void GUIController::SetupGui() {
  // Setup Context from Config file
  m_LauncherConfig = LauncherConfig::FromFile();

  m_Context.GITSPlayerPath = m_LauncherConfig.GITSPlayerPath;
  m_Context.StreamPath = m_LauncherConfig.StreamPath;
  m_Context.ConfigPath = m_LauncherConfig.ConfigPath;
  m_Context.CustomArguments = m_LauncherConfig.CustomArguments;

  PLOG_INFO << "Attempting to restore window size and position from last session: "
            << ImGuiHelper::ToStr(m_LauncherConfig.WindowPos) << "@"
            << ImGuiHelper::ToStr(m_LauncherConfig.WindowSize);

  ImGuiHelper::UpdateUIScaling(2.0f);
  // Load style
  ImGuiStyle& style = ImGui::GetStyle();
  style.ChildBorderSize = 0.f;
  style.DisabledAlpha = 0.2f;

  // Setup editor
  // - scrollbar for max width of file, not visible part
  m_Context.ConfigEditor = std::make_unique<TextEditorWidget>("ConfigEditor");
  m_Context.ConfigEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.ConfigEditor->GetEditor().SetTabSize(4);
  //m_Context.ConfigEditor->SetCheckCallback(&ValidateGITSConfigFile);

  m_Context.CLIEditor = std::make_unique<TextEditorWidget>("CLIEditor");
  m_Context.CLIEditor->GetEditor().SetReadOnly(true);
  m_Context.CLIEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.CLIEditor->GetEditor().SetTabSize(4);

  m_Context.GITSLogEditor = std::make_unique<TextEditorWidget>("GITSLogEditor");
  m_Context.GITSLogEditor->GetEditor().SetReadOnly(true);
  m_Context.GITSLogEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.GITSLogEditor->GetEditor().SetTabSize(4);

  m_Context.LogEditor = std::make_unique<TextEditorWidget>("LogEditor");
  m_Context.LogEditor->GetEditor().SetReadOnly(true);
  m_Context.LogEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.LogEditor->GetEditor().SetTabSize(4);

  m_Context.TraceStatsEditor = std::make_unique<TextEditorWidget>("TraceStatsEditor");
  m_Context.TraceStatsEditor->GetEditor().SetReadOnly(true);
  m_Context.TraceStatsEditor->GetEditor().SetShowWhitespaces(false);
  m_Context.TraceStatsEditor->GetEditor().SetTabSize(4);
  m_Context.TraceStatsEditor->SetConfig(
      TextEditorWidget::Config{.ShowToolbar = false, .ScrollToBottom = true});

  m_Context.LogAppender = std::make_unique<TextEditorAppender>(m_Context.LogEditor.get());
  plog::get()->addAppender(m_Context.LogAppender.get());

  m_Context.BtnsSideBar = new ImGuiHelper::ButtonGroup(Labels::SIDE_BAR(), false, true,
                                                       ImGuiHelper::ButtonGroupStyle::Tabs);
  m_Context.BtnsAPI = new ImGuiHelper::ButtonGroup(Labels::CONFIG_SECTIONS(), true, false,
                                                   ImGuiHelper::ButtonGroupStyle::Tabs);

  LoadConfigFile(&m_Context);
  UpdateCLICall(&m_Context);
}

void GUIController::TeardownGui() {
  m_LauncherConfig.GITSPlayerPath = m_Context.GITSPlayerPath.string();
  m_LauncherConfig.StreamPath = m_Context.StreamPath.string();
  m_LauncherConfig.ConfigPath = m_Context.ConfigPath.string();
  m_LauncherConfig.CustomArguments = m_Context.CustomArguments;

  if (!m_LauncherConfig.ToFile()) {
    LOG_ERROR << "Failed to save LauncherConfig to file "
              << m_LauncherConfig.GetGITSLauncherConfigPath();
  }

  m_Context.ConfigEditor.reset();
  m_Context.LogEditor.reset();
  m_Context.CLIEditor.reset();

  delete m_Context.BtnsSideBar;
  m_Context.BtnsSideBar = nullptr;
  delete m_Context.BtnsAPI;
  m_Context.BtnsAPI = nullptr;
}

void GUIController::DestroyGui() {
  m_Context.LogAppender.reset();
}
} // namespace gits::gui
