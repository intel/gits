// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mainWindow.h"

#include "imGuiHelper.h"
#include "tabGroup.h"
#include "labels.h"
#include "captureActions.h"
#include "launcherActions.h"
#include "resource.h"
#include "contextHelper.h"
#include "metaDataActions.h"

namespace {
using namespace gits::gui;

void RenderPlaceholder() {
  auto& context = Context::GetInstance();

  auto msg = Labels::PlaceholderText(context.AppMode);

  auto windowSize = ImGui::GetWindowSize();
  auto textSize = ImGui::CalcTextSize(msg.c_str());

  auto spacingX = 16.0f;
  auto spacingY = 16.0f;

  auto cols = static_cast<int>((windowSize.x + spacingX) / (textSize.x + spacingX));
  auto rows = static_cast<int>((windowSize.y + spacingY) / (textSize.y + spacingY));

  auto gridWidth = cols * textSize.x + (cols - 1) * spacingX;
  auto gridHeight = rows * textSize.y + (rows - 1) * spacingY;

  auto offsetX = (windowSize.x - gridWidth) / 2.0f;
  auto offsetY = (windowSize.y - gridHeight) / 2.0f;

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
  for (auto row = 0; row < rows; ++row) {
    for (auto col = 0; col < cols; ++col) {
      auto x = offsetX + col * (textSize.x + spacingX);
      auto y = offsetY + row * (textSize.y + spacingY);
      ImGui::SetCursorPos(ImVec2(x, y));
      ImGui::TextUnformatted(msg.c_str());
    }
  }
  ImGui::PopStyleColor();
}
} // namespace

namespace gits::gui {
MainWindow::MainWindow() {
  contentPanel = std::make_unique<ContentPanel>();
  playbackPanel = std::make_unique<PlaybackPanel>();
  capturePanel = std::make_unique<CapturePanel>();
  subcapturePanel = std::make_unique<SubcapturePanel>();
  tabsToolBar = std::make_unique<ImGuiHelper::TabGroup<Mode>>(Labels::MODE_BUTTONS());

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&MainWindow::CaptureActionCallback, this, std::placeholders::_1),
      {ActionEvent::Type::Capture});
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&MainWindow::PathCallback, this, std::placeholders::_1));
};

MainWindow::~MainWindow() {
  contentPanel.reset();
  playbackPanel.reset();
  capturePanel.reset();
  subcapturePanel.reset();
}

const std::string MainWindow::GetCLIArguments() const {
  auto& context = Context::GetInstance();

  std::string args;
  if (context.AppMode == Mode::SUBCAPTURE) {
    args += subcapturePanel->GetCLIArguments();
  }

  return args;
}

const CapturePanel::CaptureCleanupOptions MainWindow::GetCleanupOptions() const {
  return capturePanel->GetSelectedCleanupOptions();
}

void MainWindow::SetPlaybackFile(const std::filesystem::path& filePath) {
  auto& context = Context::GetInstance();
  context.Paths.Playback.InputStreamPath = filePath;
  context.Paths.Subcapture.InputStreamPath = filePath;
  context.ChangeMode(Mode::PLAYBACK);
  tabsToolBar->SelectEntry(Mode::PLAYBACK);
}

void MainWindow::Render() {
  auto& context = Context::GetInstance();

  if (m_CaptureInProgress || context.SubcaptureInProgress) {
    RenderPlaceholder();

    return;
  }

  WidthLeftColumn = contentPanel ? contentPanel->WidthColumn1(false) : 0.0f;

  GITSButton();

  ModeSelectionButtons();

  MainActionButtons();

  ImGui::Separator();
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::NOTICE)) /
                       2.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE);
  ImGui::PopStyleColor();

  GITSBaseRow();

  ImGui::Separator();

  switch (Context::GetInstance().AppMode) {
  case Mode::PLAYBACK:
    if (playbackPanel) {
      playbackPanel->Render();
    }
    break;
  case Mode::CAPTURE:
    if (capturePanel) {
      capturePanel->Render();
    }
    break;
  case Mode::SUBCAPTURE:
    if (playbackPanel) {
      playbackPanel->Render();
    }
    if (subcapturePanel) {
      ImGui::Separator();
      subcapturePanel->Render();
    }
    break;
  default:
    break;
  }

  if (contentPanel) {
    ImGui::Separator();
    contentPanel->Render();
  }

  ShowReleaseNotesModal();
  ShowCCodeModal();
}

void MainWindow::GITSButton() {
  auto& context = Context::GetInstance();

  bool button_clicked = ImGui::Button(Labels::TITLE);

  if (button_clicked) {
    ImGui::OpenPopup("options_popup");
  }

  if (ImGui::IsPopupOpen("options_popup")) {
    ImVec2 button_pos = ImGui::GetItemRectMin();
    ImVec2 button_size = ImGui::GetItemRectSize();
    ImGui::SetNextWindowPos(ImVec2(button_pos.x, button_pos.y + button_size.y + 8));
  }

  if (ImGui::BeginPopup("options_popup")) {
    context_helper::PathMenuItem(Labels::GITS_BASE_BUTTON, Path::GITS_BASE);

    context_helper::PathMenuItem(Labels::GITS_STREAM_PLAYBACK_BUTTON, Path::INPUT_STREAM,
                                 Mode::PLAYBACK);
    context_helper::PathMenuItem(Labels::GITS_STREAM_SUBCAPTURE_BUTTON, Path::INPUT_STREAM,
                                 Mode::SUBCAPTURE);

    context_helper::PathMenuItem(Labels::GITS_TARGET_BUTTON, Path::CAPTURE_TARGET, Mode::CAPTURE);
    context_helper::PathMenuItem(Labels::GITS_CAPTURE_BUTTON, Path::OUTPUT_STREAM, Mode::CAPTURE);

    context_helper::PathMenuItem(Labels::GITS_SCREENSHOT_BUTTON, Path::SCREENSHOTS, Mode::PLAYBACK);
    context_helper::PathMenuItem(Labels::GITS_TRACE_BUTTON, Path::TRACE, Mode::PLAYBACK);
    context_helper::PathMenuItem(Labels::GITS_SUBCAPTURE_BUTTON, Path::OUTPUT_STREAM,
                                 Mode::SUBCAPTURE);

    ImGui::Separator();
    auto versionLabel = std::string(Labels::VERSION) + ": " + APP_VERSION;
    ImGui::MenuItem(versionLabel.c_str());
    if (ImGui::MenuItem(Labels::RELEASE_NOTES_BUTTON)) {
      m_ShowReleaseNotes = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit")) {
      context.ShouldQuit = true;
    }
    ImGui::EndPopup();
  }
}

void MainWindow::ModeSelectionButtons() {
  auto& context = Context::GetInstance();

  ImGui::SameLine();

  auto offsetX = (ImGui::GetWindowWidth() - tabsToolBar->GetSize().x) / 2.0f;
  ImGui::SetCursorPosX(offsetX);

  if (tabsToolBar->Render(true)) {
    context.ChangeMode(tabsToolBar->Selected());
  }
}

void MainWindow::MainActionButtons() {
  auto& context = Context::GetInstance();
  auto mainActionWidth =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::MainAction(context.AppMode));

  if (context.AppMode == Mode::PLAYBACK) {
    ImGui::SameLine(
        ImGui::GetWindowWidth() - mainActionWidth - 8 -
        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CCODE_GENERATION_BUTTON) - 8);
    ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::Success);
    if (ImGui::Button(Labels::CCODE_GENERATION_BUTTON)) {
      m_CCodeParameters.StreamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
      m_CCodeParameters.CCodePath = context.GetPathSafe(Path::GITS_BASE) / "ccode";
      m_ShowCCodeGeneration = true;
    }
    ImGuiHelper::AddTooltip(Labels::CCODE_GENERATION_BUTTON_HINT);
    ImGuiHelper::PopButtonStyle();
  }

  ImGui::SameLine(ImGui::GetWindowWidth() - mainActionWidth - 8);

  ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::Success);

  const auto label = Labels::MainAction(context.AppMode);
  if (ImGui::Button(label.c_str())) {
    switch (context.AppMode) {
    case Mode::PLAYBACK:
      PlaybackStream();
      break;
    case Mode::CAPTURE:
      gui::capture_actions::CaptureStream();
      break;
    case Mode::SUBCAPTURE:
      SubcaptureStream();
      break;
    default:
      break;
    }
  }
  ImGuiHelper::PopButtonStyle();
}

void MainWindow::ShowReleaseNotesModal() {
  if (!m_ShowReleaseNotes) {
    return;
  }
  if (m_ShowReleaseNotes) {
    ImGui::OpenPopup(Labels::RELEASE_NOTES_WINDOW_TITLE);
  }
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 viewportSize = viewport->Size;

  ImVec2 modalSize = ImVec2(viewportSize.x * 0.8f, viewportSize.y * 0.8f);

  ImVec2 center = viewport->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);

  if (ImGui::BeginPopupModal(Labels::RELEASE_NOTES_WINDOW_TITLE, &m_ShowReleaseNotes,
                             ImGuiWindowFlags_NoResize)) {
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    float buttonWidth = 120.0f;
    float buttonHeight = ImGui::GetFrameHeight();
    float padding = ImGui::GetStyle().WindowPadding.y;

    float contentHeight = contentRegion.y - buttonHeight - padding * 2 - 1; // 1 for separator

    if (ImGui::BeginChild("ReleaseNotesContent", ImVec2(0, contentHeight),
                          true, // border
                          ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
      ImGui::TextWrapped("%s", RELEASE_NOTES);
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::SetCursorPosX(windowSize.x - buttonWidth - ImGui::GetStyle().WindowPadding.x);
    ImGui::SetCursorPosY(windowSize.y - buttonHeight - ImGui::GetStyle().WindowPadding.y);

    if (ImGui::Button(Labels::RELEASE_NOTES_CLOSE_BUTTON, ImVec2(buttonWidth, 0))) {
      m_ShowReleaseNotes = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void MainWindow::ShowCCodeModal() {
  auto& context = Context::GetInstance();
  if (!m_ShowCCodeGeneration || context.AppMode != Mode::PLAYBACK) {
    return;
  }

  if (m_ShowCCodeGeneration) {
    ImGui::OpenPopup(Labels::CCODE_GENERATION_WINDOW_TITLE);
  }
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 viewportSize = viewport->Size;

  ImVec2 modalSize = ImVec2(viewportSize.x * 0.8f, -1.0f);

  ImVec2 center = viewport->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);

  if (ImGui::BeginPopupModal(Labels::CCODE_GENERATION_WINDOW_TITLE, &m_ShowCCodeGeneration,
                             ImGuiWindowFlags_NoResize)) {

    const auto labels = {Labels::CCODE_COMMANDS_PER_BLOCK_INPUT, Labels::CCODE_WRAP_CALLS_CHECKBOX,
                         Labels::CCODE_PATH_INPUT};
    auto labelWidth =
        std::ranges::max(labels | std::views::transform([](const auto& label) {
                           return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                         }));
    auto remainingWidth = ImGui::GetContentRegionAvail().x - labelWidth - 16; // 16 for spacing

    ImGui::Text(Labels::CCODE_COMMANDS_PER_BLOCK_INPUT);
    ImGui::SameLine(labelWidth + 16); // 16 for spacing
    ImGui::SetNextItemWidth(remainingWidth);
    ImGui::InputInt("###1", &m_CCodeParameters.CommandsPerBlock);
    ImGuiHelper::AddTooltip(Labels::CCODE_COMMANDS_PER_BLOCK_INPUT_HINT);

    ImGui::Text(Labels::CCODE_WRAP_CALLS_CHECKBOX);
    ImGui::SameLine(labelWidth + 16); // 16 for spacing
    ImGui::Checkbox("###2", &m_CCodeParameters.WrapAPICalls);
    ImGuiHelper::AddTooltip(Labels::CCODE_WRAP_CALLS_CHECKBOX_HINT);

    ImGui::Text(Labels::CCODE_PATH_INPUT);
    ImGui::SameLine(labelWidth + 16); // 16 for spacing
    ImGuiHelper::InputString("###3", m_CCodeParameters.CCodePath, ImGuiInputTextFlags_ReadOnly,
                             remainingWidth);
    ImGuiHelper::AddTooltip(Labels::CCODE_PATH_INPUT_HINT);

    if (ImGui::Button(Labels::CCODE_GENERATION_CANCEL_BUTTON)) {
      m_ShowCCodeGeneration = false;
      ImGui::CloseCurrentPopup();
    }

    ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::Success);
    ImGui::SameLine();
    if (ImGui::Button(Labels::CCODE_GENERATION_GO_BUTTON)) {
      GenerateCCode(m_CCodeParameters);
      m_ShowCCodeGeneration = false;
      ImGui::CloseCurrentPopup();
    }
    ImGuiHelper::PopButtonStyle();

    ImGui::EndPopup();
  }
}

void MainWindow::GITSBaseRow() {
  auto& context = Context::GetInstance();

  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Separator();
  ImGui::Text(Labels::BASE_PATH);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGui::GetCursorPosX() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITS_BASE_PATH);

  context_helper::PathInput("###BasePathInput", Path::GITS_BASE, std::nullopt, 0, allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITS_BASE_PATH)) {
    ShowFileDialog(FileDialogKey{Path::GITS_BASE, Mode::PLAYBACK});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITS_BASE_PATH_HINT);

  // Buttons to reset paths and things.
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);
  if (ImGui::Button(Labels::DETECT_BASE_PATHS)) {
    ResetBasePaths();
  }
  ImGuiHelper::AddTooltip(Labels::DETECT_BASE_PATHS_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::UPDATE_CONFIG_PATH)) {
    std::filesystem::path configPath = "";
    if (context.AppMode == Mode::CAPTURE) {
      configPath = GetRecorderConfigPathForApi(context.SelectedApiForCapture);
    } else if (context.AppMode == Mode::PLAYBACK || context.AppMode == Mode::SUBCAPTURE) {
      configPath = GetPlayerConfigPath();
    }
    if (std::filesystem::exists(configPath)) {
      context.SetPath(std::move(configPath), Path::CONFIG, context.AppMode);
    }
  }
  ImGuiHelper::AddTooltip(Labels::UPDATE_CONFIG_PATH_HINT);
  ImGui::SameLine();
  if (ImGui::Button(Labels::USE_ALL_CONFIGS_FROM_BASE_PATH)) {
    const auto captureConfigPath = GetRecorderConfigPathForApi(context.SelectedApiForCapture);
    if (std::filesystem::exists(captureConfigPath)) {
      context.SetPath(captureConfigPath, Path::CONFIG, Mode::CAPTURE);
    }
    const auto playbackConfigPath = GetPlayerConfigPath();
    if (std::filesystem::exists(playbackConfigPath)) {
      context.SetPath(playbackConfigPath, Path::CONFIG, Mode::PLAYBACK);
      context.SetPath(playbackConfigPath, Path::CONFIG, Mode::SUBCAPTURE);
    }
  }
  ImGuiHelper::AddTooltip(Labels::USE_ALL_CONFIGS_FROM_BASE_PATH_HINT);
};

void MainWindow::CaptureActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);

  auto& context = Context::GetInstance();

  switch (actionEvent.ActionState) {
  case ActionEvent::State::Started:
    m_CaptureInProgress = true;
    break;
  case ActionEvent::State::Ended:
    m_CaptureInProgress = false;
    break;
  default:
    break;
  }
}

void MainWindow::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  auto& context = Context::GetInstance();
  context.LauncherConfiguration.ToFile();

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != Mode::PLAYBACK) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    const auto& streamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
    if (!streamPath.empty()) {
      context.MetaData = GetStreamMetaData(streamPath);
      EventBus::GetInstance().publish<ContextEvent>(ContextEvent::Type::MetadataLoaded);
    }
  } else if (pathEvent.EventType == PathEvent::Type::GITS_LOG) {
    context.GITSLogEditor->SaveToFile(pathEvent.CustomPath.value());
  }
}
} // namespace gits::gui
