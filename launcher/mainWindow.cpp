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
#include "launcherActions.h"
#ifdef _WIN32
#include "registryManager.h"
#endif
#include "resource.h"
#include "contextHelper.h"
#include "metaDataActions.h"
#include "captureActions.h"

#include <algorithm>
#include <chrono>
#include <cstdio>

namespace {

std::string BuildCaptureStatusBadgeText(const gits::gui::Context& context,
                                        bool waitingForQuietPeriod) {
  if (!waitingForQuietPeriod) {
    return gits::gui::Labels::CAPTURE_MONITORING_STATE_ACTIVE;
  }

  std::string statusBadgeText = gits::gui::Labels::CAPTURE_MONITORING_STATE_WAITING;
  const auto quietPeriodStartTick = context.CaptureQuietPeriodStartTick.load();
  if (quietPeriodStartTick <= 0) {
    return statusBadgeText;
  }

  const auto nowTick = std::chrono::steady_clock::now().time_since_epoch().count();
  if (nowTick <= quietPeriodStartTick) {
    return statusBadgeText;
  }

  const auto elapsedTicks = nowTick - quietPeriodStartTick;
  const auto quietPeriodSeconds =
      std::chrono::duration<double>(gits::gui::capture_actions::QUIET_PERIOD).count();
  const auto elapsedSeconds = std::min(
      quietPeriodSeconds,
      std::chrono::duration<double>(std::chrono::steady_clock::duration(elapsedTicks)).count());

  char timerSuffix[48] = {};
  std::snprintf(timerSuffix, sizeof(timerSuffix),
                gits::gui::Labels::CAPTURE_MONITORING_TIMER_FORMAT, elapsedSeconds,
                quietPeriodSeconds);
  statusBadgeText += timerSuffix;

  return statusBadgeText;
}

std::string BuildCapturePidText(uint32_t monitoredPid) {
  return monitoredPid == 0 ? gits::gui::Labels::NOT_AVAILABLE
                           : std::to_string(static_cast<unsigned long long>(monitoredPid));
}

void RenderCenteredStatusPanel(const char* panelId,
                               const std::string& text,
                               const ImVec4& backgroundColor,
                               const ImVec4& borderColor,
                               float panelHeight = 0.0f) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, backgroundColor);
  ImGui::PushStyleColor(ImGuiCol_Border, borderColor);

  const auto effectiveHeight = panelHeight > 0.0f ? panelHeight : ImGui::GetFrameHeight() + 8.0f;
  if (ImGui::BeginChild(panelId, ImVec2(-1.0f, effectiveHeight), true,
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.98f, 0.98f, 1.0f));
    const auto textSize = ImGui::CalcTextSize(text.c_str());
    const auto available = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosX(std::max(0.0f, (available.x - textSize.x) * 0.5f));
    ImGui::SetCursorPosY(
        std::max(0.0f, (effectiveHeight - ImGui::GetTextLineHeight()) * 0.5f - 1.0f));
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void RenderCapturePlaceholderContent(const gits::gui::Context& context, float cardWidth) {
  const auto executablePath = context.GetPathSafe(gits::gui::Path::CAPTURE_TARGET);
  const auto monitoredPid = context.CaptureMonitoredPid.load();
  const auto waitingForQuietPeriod = context.CaptureInQuietPeriod.load();
  const std::string statusBadgeText = BuildCaptureStatusBadgeText(context, waitingForQuietPeriod);
  const std::string pidText = BuildCapturePidText(monitoredPid);

  const auto statusBaseColor = waitingForQuietPeriod ? gits::ImGuiHelper::Colors::WARNING
                                                     : gits::ImGuiHelper::Colors::FAILURE;

  const auto valueColumnX = std::min(130.0f, std::max(95.0f, cardWidth * 0.18f));

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.88f, 0.88f, 1.0f));
  ImGui::TextUnformatted(gits::gui::Labels::CAPTURE_MONITORING_TITLE);
  ImGui::Separator();

  ImGui::Spacing();
  RenderCenteredStatusPanel("CaptureStatusPanel", statusBadgeText, statusBaseColor,
                            statusBaseColor);

  ImGui::Spacing();
  ImGui::TextUnformatted(gits::gui::Labels::CAPTURE_MONITORING_PID_LABEL);
  ImGui::SameLine(valueColumnX);
  ImGui::TextUnformatted(pidText.c_str());

  ImGui::TextUnformatted(gits::gui::Labels::CAPTURE_MONITORING_EXECUTABLE_LABEL);
  ImGui::SameLine(valueColumnX);
  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 0.38f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.42f, 0.42f, 0.40f));
  const auto pathBoxHeight = 78.0f;
  if (ImGui::BeginChild("CapturePathBox", ImVec2(-1.0f, pathBoxHeight), true)) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.90f, 0.92f, 1.0f));
    ImGui::TextWrapped("%s", executablePath.empty() ? gits::gui::Labels::NOT_AVAILABLE
                                                    : executablePath.string().c_str());
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
  ImGui::EndGroup();

  ImGui::PopStyleColor();
}

void RenderSubcapturePlaceholderContent(const gits::gui::Context& context) {
  const auto streamPath =
      context.GetPathSafe(gits::gui::Path::INPUT_STREAM, gits::gui::Mode::SUBCAPTURE);
  const std::string streamPathText =
      streamPath.empty() ? gits::gui::Labels::NOT_AVAILABLE : streamPath.string();

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.88f, 0.88f, 1.0f));
  ImGui::TextUnformatted(gits::gui::Labels::SUBCAPTURE_MONITORING_TITLE);
  ImGui::Separator();

  ImGui::Spacing();
  RenderCenteredStatusPanel(
      "SubcaptureStatusPanel", gits::gui::Labels::PlaceholderText(gits::gui::Mode::SUBCAPTURE),
      gits::ImGuiHelper::Colors::FAILURE, gits::ImGuiHelper::Colors::FAILURE, 48.0f);

  ImGui::Spacing();
  ImGui::TextWrapped("%s", streamPathText.c_str());
  ImGui::PopStyleColor();
}

void RenderPlaceholder() {
  auto& context = gits::gui::Context::GetInstance();

  const auto windowPos = ImGui::GetWindowPos();
  const auto windowSize = ImGui::GetWindowSize();
  auto* drawList = ImGui::GetWindowDrawList();
  const ImVec2 bgMin(windowPos.x, windowPos.y);
  const ImVec2 bgMax(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

  // Fill the whole window area so fullscreen mode looks intentional, not empty.
  drawList->AddRectFilledMultiColor(bgMin, bgMax,
                                    ImGui::GetColorU32(ImVec4(0.10f, 0.07f, 0.07f, 0.85f)),
                                    ImGui::GetColorU32(ImVec4(0.10f, 0.07f, 0.07f, 0.85f)),
                                    ImGui::GetColorU32(ImVec4(0.07f, 0.06f, 0.06f, 0.92f)),
                                    ImGui::GetColorU32(ImVec4(0.07f, 0.06f, 0.06f, 0.92f)));

  for (float y = bgMin.y + 24.0f; y < bgMax.y; y += 36.0f) {
    drawList->AddLine(ImVec2(bgMin.x + 20.0f, y), ImVec2(bgMax.x - 20.0f, y),
                      ImGui::GetColorU32(ImVec4(0.75f, 0.45f, 0.45f, 0.05f)));
  }

  const auto cardWidth = std::max(420.0f, std::min(windowSize.x - 48.0f, 900.0f));
  const auto cardHeight = 250.0f;
  const auto cardX = std::max(24.0f, (windowSize.x - cardWidth) * 0.5f);
  const auto cardY = std::max(40.0f, (windowSize.y - cardHeight) * 0.5f);

  ImGui::SetCursorPos(ImVec2(cardX, cardY));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.62f, 0.31f, 0.31f, 0.48f));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.08f, 0.08f, 0.45f));

  if (!ImGui::BeginChild("CaptureStatusCard", ImVec2(cardWidth, cardHeight), true)) {
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    return;
  }

  if (context.AppMode == gits::gui::Mode::CAPTURE) {
    RenderCapturePlaceholderContent(context, cardWidth);
  } else if (context.AppMode == gits::gui::Mode::SUBCAPTURE) {
    RenderSubcapturePlaceholderContent(context);
  } else {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.80f, 0.80f, 1.0f));
    ImGui::SetCursorPosY(std::max(16.0f, (cardHeight - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::TextUnformatted(gits::gui::Labels::PlaceholderText(context.AppMode).c_str());
    ImGui::PopStyleColor();
  }

  ImGui::EndChild();
  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar(2);
}
} // namespace

namespace gits::gui {
MainWindow::MainWindow() {
  contentPanel = std::make_unique<ContentPanel>();
  playbackPanel = std::make_unique<PlaybackPanel>();
  capturePanel = std::make_unique<CapturePanel>();
  tabsToolBar = std::make_unique<ImGuiHelper::TabGroup<Mode>>(Labels::MODE_BUTTONS());

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&MainWindow::CaptureActionCallback, this, std::placeholders::_1),
      {ActionEvent::Type::Capture});
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&MainWindow::PathCallback, this, std::placeholders::_1));

#ifdef _WIN32
  RegistryManager::Instance().LoadEntriesFromYamlFile(
      Context::GetInstance().GetRegistryKeysYamlPath());
#endif
};

MainWindow::~MainWindow() {
  contentPanel.reset();
  playbackPanel.reset();
  capturePanel.reset();
}

const std::string MainWindow::GetCLIArguments() const {
  std::string args;
  return args;
}

const CapturePanel::CaptureCleanupOptions MainWindow::GetCleanupOptions() const {
  return capturePanel->GetSelectedCleanupOptions();
}

void MainWindow::SetPlaybackFile(const std::filesystem::path& filePath) {
  auto& context = Context::GetInstance();
  context.SetPath(filePath, Path::INPUT_STREAM, Mode::PLAYBACK);
  context.SetPath(filePath, Path::INPUT_STREAM, Mode::SUBCAPTURE);
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

  ImGui::SameLine();
  if (ImGui::Button(Labels::NEW_SESSION_BUTTON)) {
    NewLauncherSession();
  }
  ImGuiHelper::AddTooltip(Labels::NEW_SESSION_BUTTON_TOOLTIP);

  ModeSelectionButtons();

  MainActionButtons();

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

    context_helper::PathMenuItem(Labels::GITS_ARTIFACTS_BUTTON, Path::ARTIFACTS, Mode::PLAYBACK);
    context_helper::PathMenuItem(Labels::GITS_TRACE_BUTTON, Path::TRACE, Mode::PLAYBACK);
    context_helper::PathMenuItem(Labels::GITS_SUBCAPTURE_BUTTON, Path::OUTPUT_STREAM,
                                 Mode::SUBCAPTURE);

    ImGui::Separator();
    if (ImGui::BeginMenu(Labels::GITS_BASE_PATHS_MENU)) {
      if (ImGui::MenuItem(Labels::DETECT_BASE_PATHS)) {
        ResetBasePaths();
      }
      ImGui::SetItemTooltip(Labels::DETECT_BASE_PATHS_HINT);

      if (ImGui::MenuItem(Labels::UPDATE_CONFIG_PATH)) {
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
      ImGui::SetItemTooltip(Labels::UPDATE_CONFIG_PATH_HINT);

      if (ImGui::MenuItem(Labels::USE_ALL_CONFIGS_FROM_BASE_PATH)) {
        SetAllConfigsFromBasePath();
      }
      ImGui::SetItemTooltip(Labels::USE_ALL_CONFIGS_FROM_BASE_PATH_HINT);
      ImGui::EndMenu();
    }
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
    UpdateCLICall();
    switch (context.AppMode) {
    case Mode::PLAYBACK:
      PlaybackStream();
      break;
    case Mode::CAPTURE:
      if (capturePanel) {
        capturePanel->CaptureStream();
      };
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

  ImVec2 center = viewport->GetCenter();
  const auto labels = {Labels::CCODE_COMMANDS_PER_BLOCK_INPUT, Labels::CCODE_WRAP_CALLS_CHECKBOX};
  auto labelWidth =
      std::ranges::max(labels | std::views::transform([](const auto& label) {
                         return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                       }));
  auto remainingWidth = ImGui::GetContentRegionAvail().x - labelWidth - 16; // 16 for spacing

  auto maxWidth =
      labelWidth + 32 + ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Input, "01234567890");
  maxWidth = std::max(maxWidth,
                      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::CCODE_OUTPUT_HINT));
  ImVec2 modalSize = ImVec2(maxWidth, -1.0f);

  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);

  if (ImGui::BeginPopupModal(Labels::CCODE_GENERATION_WINDOW_TITLE, &m_ShowCCodeGeneration,
                             ImGuiWindowFlags_NoResize)) {

    ImGui::Text(Labels::CCODE_COMMANDS_PER_BLOCK_INPUT);
    ImGui::SameLine(labelWidth + 16); // 16 for spacing
    ImGui::SetNextItemWidth(remainingWidth);
    ImGui::InputInt("###1", &m_CCodeParameters.CommandsPerBlock);
    ImGuiHelper::AddTooltip(Labels::CCODE_COMMANDS_PER_BLOCK_INPUT_HINT);

    ImGui::Text(Labels::CCODE_WRAP_CALLS_CHECKBOX);
    ImGui::SameLine(labelWidth + 16); // 16 for spacing
    ImGui::Checkbox("###2", &m_CCodeParameters.WrapAPICalls);
    ImGuiHelper::AddTooltip(Labels::CCODE_WRAP_CALLS_CHECKBOX_HINT);

    ImGui::Text(Labels::CCODE_OUTPUT_HINT);

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
    context.CaptureInQuietPeriod = false;
    context.CaptureMonitoredPid = 0;
    context.CaptureQuietPeriodStartTick = 0;
    break;
  default:
    break;
  }
}

void MainWindow::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  auto& context = Context::GetInstance();
  context.LauncherConfiguration.ToFile();

#ifdef _WIN32
  if (pathEvent.EventType == PathEvent::Type::GITS_BASE) {
    RegistryManager::Instance().LoadEntriesFromYamlFile(context.GetRegistryKeysYamlPath());
    return;
  }
#endif

  if (!pathEvent.Mode.has_value() || pathEvent.Mode == Mode::CAPTURE) {
    return;
  }

  const auto& eventMode = pathEvent.Mode.value();

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    const auto& streamPath = context.GetPathSafe(Path::INPUT_STREAM, eventMode);
    if (!streamPath.empty()) {
      context.ConfigurationForMode(eventMode).MetaData = LoadStreamMetaData(streamPath);
      EventBus::GetInstance().publish(ContextEvent::Type::MetadataLoaded, eventMode);
    }
  } else if (pathEvent.EventType == PathEvent::Type::GITS_LOG) {
    context.GITSLogEditor->SaveToFile(pathEvent.CustomPath.value());
  } else if (pathEvent.EventType == PathEvent::Type::CONFIG_EXPORT) {
    auto& configStr = context.ConfigurationForMode(eventMode).ModifiedGitsConfigurationStr;
    auto& filePath = pathEvent.CustomPath.value();
    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
      LOG_ERROR << "Failed to open file for saving: " << filePath;
    } else {
      outFile << configStr;
      outFile.close();
    }
  }
}
} // namespace gits::gui
