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

namespace {
static const auto& MODE_BUTTONS() {
  static const std::map<gits::gui::MainWindow::MODE_BUTTON_ITEMS,
                        gits::ImGuiHelper::ButtonGroupItem>
      items = {
          {gits::gui::MainWindow::MODE_BUTTON_ITEMS::CAPTURE, {"Capture", "Capture a GITS stream"}},
          {gits::gui::MainWindow::MODE_BUTTON_ITEMS::PLAYBACK,
           {"Playback", "Playback a GITS stream"}},
          {gits::gui::MainWindow::MODE_BUTTON_ITEMS::SUBCAPTURE,
           {"Subcapture", "Subcapture a GITS stream"}},
      };
  return items;
}

static const auto& MODE_BUTTONS_VEC() {
  static const std::vector<std::string> values([] {
    std::vector<std::string> result;
    for (const auto& pair : MODE_BUTTONS()) {
      result.push_back(pair.second.label);
    }
    return result;
  }());
  return values;
}

void RenderPlaceholder(const gits::gui::Context& context) {
  // Renders a placeholder text all over the screen
  auto msg = std::string();
  switch (context.AppMode) {
  case gits::gui::Context::Mode::PLAYBACK:
    msg = "PLAYBACK IN PROGRESS";
    break;
  case gits::gui::Context::Mode::CAPTURE:
    msg = "CAPTURE IN PROGRESS";
    break;
  case gits::gui::Context::Mode::SUBCAPTURE:
    msg = "SUBCAPTURE IN PROGRESS";
    break;
  default:
    break;
  }

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
MainWindow::MainWindow(gits::gui::ISharedContext& sharedContext)
    : gits::gui::BasePanel(sharedContext) {
  contentPanel = std::make_unique<gits::gui::ContentPanel>(sharedContext);
  playbackPanel = std::make_unique<gits::gui::PlaybackPanel>(sharedContext);
  capturePanel = std::make_unique<gits::gui::CapturePanel>(sharedContext);
  subcapturePanel = std::make_unique<gits::gui::SubcapturePanel>(sharedContext);
  tabsToolBar = std::make_unique<ImGuiHelper::TabGroup<MODE_BUTTON_ITEMS>>(MODE_BUTTONS());
};

MainWindow::~MainWindow() {
  contentPanel.reset();
  playbackPanel.reset();
  capturePanel.reset();
  subcapturePanel.reset();
}

const std::string MainWindow::GetCLIArguments() const {
  auto& context = getSharedContext<gui::Context>();

  std::string args;
  if (context.AppMode == Context::Mode::SUBCAPTURE) {
    args += subcapturePanel->GetCLIArguments();
  }

  return args;
}

const CapturePanel::CaptureCleanupOptions MainWindow::GetCleanupOptions() const {
  return capturePanel->GetSelectedCleanupOptions();
}

void MainWindow::Render() {
  const auto& context = getSharedContext<Context>();

  if (context.CaptureInProgress || context.SubcaptureInProgress) {
    RenderPlaceholder(context);

    return;
  }

  WidthLeftColumn = contentPanel->WidthColumn1(false);

  GITSButton();

  ImGui::SameLine();

  float offsetX = (ImGui::GetWindowWidth() - tabsToolBar->GetSize().x) / 2.0f;
  ImGui::SetCursorPosX(offsetX);
  ModeSelectionButtons();
  ImGui::SameLine(ImGui::GetWindowWidth() -
                  ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button,
                                       Labels::MainAction(context.CurrentMainAction)) -
                  8);
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

  switch (getSharedContext<Context>().AppMode) {
  case Context::Mode::PLAYBACK:
    if (playbackPanel) {
      playbackPanel->Render();
    }
    break;
  case Context::Mode::CAPTURE:
    if (capturePanel) {
      capturePanel->Render();
    }
    break;
  case Context::Mode::SUBCAPTURE:
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
  auto& context = getSharedContext<gui::Context>();

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
    ImGui::BeginDisabled(context.GITSBasePath.empty());
    if (ImGui::MenuItem(Labels::GITS_BASE_BUTTON)) {
      OpenFolder(context.GITSBasePath);
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.StreamPath.empty());
    if (ImGui::MenuItem(Labels::GITS_STREAM_BUTTON)) {
      OpenFolder(context.StreamPath.parent_path());
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.TargetPath.empty());
    if (ImGui::MenuItem(Labels::GITS_TARGET_BUTTON)) {
      OpenFolder(context.TargetPath.parent_path());
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.CaptureOutputPath.empty());
    if (ImGui::MenuItem(Labels::GITS_CAPTURE_BUTTON)) {
      OpenFolder(context.CaptureOutputPath);
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.ScreenshotPath.empty());
    if (ImGui::MenuItem(Labels::GITS_SCREENSHOT_BUTTON)) {
      OpenFolder(context.ScreenshotPath);
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.TracePath.empty());
    if (ImGui::MenuItem(Labels::GITS_TRACE_BUTTON)) {
      OpenFolder(context.TracePath);
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(context.SubcapturePath.empty());
    if (ImGui::MenuItem(Labels::GITS_SUBCAPTURE_BUTTON)) {
      OpenFolder(context.SubcapturePath);
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    if (ImGui::MenuItem(Labels::RESET_BASE_PATHS)) {
      ResetBasePaths(context);
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
  auto& context = getSharedContext<gui::Context>();

  if (tabsToolBar->Render(true)) {
    switch (tabsToolBar->Selected()) {
    case MODE_BUTTON_ITEMS::PLAYBACK: {
      context.ChangeMode(gui::Context::Mode::PLAYBACK);
      break;
    }
    case MODE_BUTTON_ITEMS::CAPTURE: {
      context.ChangeMode(gui::Context::Mode::CAPTURE);
      break;
    }
    case MODE_BUTTON_ITEMS::SUBCAPTURE: {
      context.ChangeMode(gui::Context::Mode::SUBCAPTURE);
      break;
    }
    }
  }
}

void MainWindow::MainActionButton() {
  auto& context = getSharedContext<gui::Context>();
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.9f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.7f, 0.25f, 1.0f));
  const auto label = Labels::MainAction(context.CurrentMainAction);
  auto width = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, label);
  if (ImGui::Button(label.c_str())) {
    switch (context.CurrentMainAction) {
    case Context::MainAction::PLAYBACK:
      PlaybackStream(context);
      break;
    case Context::MainAction::CAPTURE:
      gui::capture_actions::CaptureStream(context);
      break;
    case Context::MainAction::SUBCAPTURE:
      SubcaptureStream(context);
      break;
    default:
      break;
    }
  }
  ImGui::PopStyleColor(3);
}

void MainWindow::GITSBaseRow() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Separator();
  ImGui::Text(Labels::BASE_PATH);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGui::GetCursorPosX() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITS_BASE_PATH);

  if (ImGuiHelper::InputString("###BasePathInput", context.GITSBasePath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITS_BASE_PATH)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_GITS_BASE_PATH);
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITS_BASE_PATH_HINT);
};

void MainWindow::GITSPlayerRow() {
  auto& context = getSharedContext<gui::Context>();

  if (ImGui::Checkbox("Custom GITS Player", &context.UseCustomGITSPlayer)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip("Enable to specify a custom GITS Player executable path");

  ImGui::BeginDisabled(!context.UseCustomGITSPlayer);
  ImGui::SameLine();

  auto allocatedWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITSPLAYER) - 8.0f;

  if (ImGuiHelper::InputString("###PlayerPathInput", context.GITSPlayerPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);
  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITSPLAYER)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_GITSPLAYER_PATH);
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITSPLAYER_HINT);
  ImGui::EndDisabled();
}

} // namespace gits::gui
