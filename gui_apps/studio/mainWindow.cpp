// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mainWindow.h"

#include "contextHelper.h"
#include "imGuiHelper.h"
#include "tabGroup.h"
#include "studioLabels.h"
#include "resource.h"
#include "studioContext.h"
#include "guiActions.h"
#include "studioActions.h"

#include <algorithm>

namespace gits::gui {
MainWindow::MainWindow() : m_StreamPanel(), m_LiveWorkloadPanel(), m_ContentPanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&MainWindow::PathCallback, this, std::placeholders::_1));
}

MainWindow::~MainWindow() {
  // nothing
}

void MainWindow::Render() {
  auto& context = StudioContext::GetInstance();

  GITSBaseRow();

  if (context.IsLiveMode()) {
    m_LiveWorkloadPanel.Render();
  } else {
    m_StreamPanel.Render();
  }
  ImGui::Separator();

  m_ContentPanel.Render();

  if (m_ShowReleaseNotes) {
    m_ShowReleaseNotes = ImGuiHelper::ShowReleaseNotesModal(RELEASE_NOTES);
  }
}

void MainWindow::GITSBaseRow() {
  auto& context = StudioContext::GetInstance();

  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Separator();
  ImGui::Text(Labels::BASE_PATH);
  ImGui::SameLine();
  WidthLeftColumn = ImGui::GetCursorPosX();

  auto allocatedWidth =
      availableWidth - ImGui::GetCursorPosX() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITS_BASE_PATH);

  context_helper::PathInput("###BasePathInput", Path::GITS_BASE, std::nullopt, 0, allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::BASE_PATH_INPUT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_GITS_BASE_PATH)) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::GITS_BASE, GitsAction::PLAYBACK});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_GITS_BASE_PATH_HINT);

  ImGui::Separator();
};

void MainWindow::RenderMainMenu() {
  namespace MnuLbl = gits::gui::Labels::Menu;

  auto versionLabel =
      ICON_MDI_INFORMATION_OUTLINE + std::string(Labels::VERSION) + ": " + APP_VERSION;

  auto& context = StudioContext::GetInstance();
  if (ImGui::BeginMenuBar()) {
    auto liveModeLabel = context.IsLiveMode() ? Labels::LIVE_MODE : Labels::STREAM_MODE;
    const auto liveModeTextWidth = ImGui::CalcTextSize(Labels::LIVE_MODE).x;
    const auto streamModeTextWidth = ImGui::CalcTextSize(Labels::STREAM_MODE).x;
    const auto modeButtonWidth =
        std::max(liveModeTextWidth, streamModeTextWidth) + 2.0f * ImGui::GetStyle().FramePadding.x;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::BUTTON_STYLE_INFO);
    if (ImGui::Button(liveModeLabel, ImVec2(modeButtonWidth, 0.0f))) {
      context.ToogleLiveStreamMode();
    }
    ImGuiHelper::PopButtonStyle();
    ImGui::PopStyleVar();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x + 3.0f);
    if (ImGui::BeginMenu(MnuLbl::FILE)) {
      if (ImGui::MenuItem(MnuLbl::OPEN, "Ctrl+O")) {
        if (context.IsLiveMode()) {
          gui_actions::ShowFileDialog(FileDialogKey{Path::CAPTURE_TARGET, GitsAction::CAPTURE});
        } else {
          gui_actions::ShowFileDialog(FileDialogKey{Path::INPUT_STREAM, GitsAction::PLAYBACK});
        }
      }
      context_helper::RenderRecentMenu();
      ImGui::Separator();
      if (ImGui::MenuItem(MnuLbl::DETECT_BASE_PATHS)) {
        context.ResetBasePaths();
      }
      ImGui::SetItemTooltip(MnuLbl::DETECT_BASE_PATHS_HINT);
      ImGui::Separator();
      if (ImGui::MenuItem(MnuLbl::QUIT, "Ctrl+Q")) {
        context.ShouldQuit = true;
      }
      ImGui::EndMenu();
    }

    if (!context.IsLiveMode()) {
      if (ImGui::BeginMenu(MnuLbl::ANALYSIS)) {
        auto analysisState = context.Analysis;
        if (ImGui::MenuItem(MnuLbl::ANALYSIS_FULL, "", analysisState.FullyAnalysed())) {
          studio_actions::AnalyzeStream(
              context.ConfigurationForAction(GitsAction::PLAYBACK).ModifiedGitsConfiguration, true,
              true);
        }
        if (ImGui::MenuItem(MnuLbl::ANALYSIS_SCREENSHOTS, "", analysisState.Screenshots)) {
          studio_actions::AnalyzeStream(
              context.ConfigurationForAction(GitsAction::PLAYBACK).ModifiedGitsConfiguration, true,
              false);
        }
        if (ImGui::MenuItem(MnuLbl::ANALYSIS_TRACE, "", analysisState.Trace)) {
          studio_actions::AnalyzeStream(
              context.ConfigurationForAction(GitsAction::PLAYBACK).ModifiedGitsConfiguration, false,
              true);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(MnuLbl::VIEW_METADATA)) {
          NavigateEvent event = NavigateEvent(SideBarItem::METADATA);
          EventBus::GetInstance().publish<NavigateEvent>(event);
        }
        if (ImGui::MenuItem(MnuLbl::VIEW_TRACE)) {
          NavigateEvent event = NavigateEvent(SideBarItem::API_TRACE);
          EventBus::GetInstance().publish<NavigateEvent>(event);
        }
        ImGui::EndMenu();
      }
    }
    if (ImGui::BeginMenu(MnuLbl::HELP)) {
      auto& context = BaseContext::GetBaseInstance();
      auto configuration = context.GetBaseConfiguration();
      if (configuration) {
        ImGui::Separator();
        if (ImGui::MenuItem(MnuLbl::SWITCH_THEME)) {
          configuration->Theme.CycleTheme();
          configuration->Theme.ApplyTheme();
          context.UpdatePalette();
          EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ThemeChanged);
        }
      }

      ImGui::Separator();
      ImGui::MenuItem(versionLabel.c_str(), nullptr, false, false);
      if (ImGui::MenuItem(Labels::RELEASE_NOTES_BUTTON)) {
        m_ShowReleaseNotes = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void MainWindow::PathCallback(const Event& e) {
  const auto& pathEvent = static_cast<const PathEvent&>(e);
  auto& context = StudioContext::GetInstance();

  bool toggleMode = false;
  if ((pathEvent.EventType == Path::INPUT_STREAM) && (pathEvent.Mode == GitsAction::PLAYBACK)) {
    toggleMode = context.IsLiveMode();
  } else if ((pathEvent.EventType == Path::CAPTURE_TARGET) &&
             (pathEvent.Mode == GitsAction::CAPTURE)) {
    toggleMode = !context.IsLiveMode();
  }
  if (toggleMode) {

    context.ToogleLiveStreamMode();
  }
}

} // namespace gits::gui
