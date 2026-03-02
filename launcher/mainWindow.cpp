// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mainWindow.h"

#include "tabGroup.h"
#include "labels.h"
#include "captureActions.h"
#include "launcherActions.h"
#include "resource.h"
#include "contextHelper.h"

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
      std::bind(&MainWindow::CaptureActionCallback, this, std::placeholders::_1));
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

  WidthLeftColumn = contentPanel->WidthColumn1(false);

  GITSButton();

  ImGui::SameLine();

  float offsetX = (ImGui::GetWindowWidth() - tabsToolBar->GetSize().x) / 2.0f;
  ImGui::SetCursorPosX(offsetX);
  ModeSelectionButtons();
  ImGui::SameLine(
      ImGui::GetWindowWidth() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::MainAction(context.AppMode)) - 8);
  MainActionButton();

  ImGui::Separator();
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::NOTICE)) /
                       2.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE);
  ImGui::PopStyleColor();

  GITSBaseRow();
  GITSPlayerRow();

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
    if (ImGui::MenuItem(Labels::RESET_BASE_PATHS)) {
      ResetBasePaths();
    }
    auto versionLabel = std::string(Labels::VERSION) + ": " + APP_VERSION;
    ImGui::MenuItem(versionLabel.c_str());
    ImGui::Separator();
    if (ImGui::MenuItem("Exit")) {
      context.ShouldQuit = true;
    }
    ImGui::EndPopup();
  }
}

void MainWindow::ModeSelectionButtons() {
  auto& context = Context::GetInstance();

  if (tabsToolBar->Render(true)) {
    context.ChangeMode(tabsToolBar->Selected());
  }
}

void MainWindow::MainActionButton() {
  auto& context = Context::GetInstance();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.9f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.7f, 0.25f, 1.0f));
  const auto label = Labels::MainAction(context.AppMode);
  auto width = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, label);
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
  ImGui::PopStyleColor(3);
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
    ShowFileDialog(FileDialogKeys{Path::GITS_BASE, Mode::PLAYBACK});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITS_BASE_PATH_HINT);
};

void MainWindow::GITSPlayerRow() {
  auto& context = Context::GetInstance();

  if (ImGui::Checkbox(Labels::USE_CUSTOM_GITSPLAYER, &context.UseCustomGITSPlayer)) {
    UpdateCLICall();
  }
  ImGuiHelper::AddTooltip(Labels::GITSPLAYER_TOOLTIP);

  ImGui::BeginDisabled(!context.UseCustomGITSPlayer);
  ImGui::SameLine();

  auto allocatedWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITSPLAYER) - 8.0f;

  context_helper::PathInput("###PlayerPathInput", Path::CUSTOM_PLAYER, std::nullopt, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);
  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITSPLAYER)) {
    ShowFileDialog(FileDialogKeys{Path::CUSTOM_PLAYER, Mode::PLAYBACK});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITSPLAYER_HINT);
  ImGui::EndDisabled();
}

void MainWindow::CaptureActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);

  if (actionEvent.EventType != ActionEvent::Type::Capture) {
    return;
  }

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

} // namespace gits::gui
