// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "navBar.h"

#include <format>

#include "imgui.h"
#include "imguiHelper.h"
#include "studioContext.h"
#include "studioLabels.h"
#include "moduleRegistry.h"

namespace Settings {
static const constexpr auto PADDING = 8.0f;

static const constexpr auto COLOR_NAVBAR = IM_COL32(255, 255, 255, 255);
static const constexpr auto COLOR_CIRCLE = IM_COL32(0, 255, 0, 255);
static const constexpr auto COLOR_MARKER_1 = IM_COL32(255, 0, 0, 200);
static const constexpr auto COLOR_MARKER_2 = IM_COL32(0, 0, 255, 200);
static const constexpr auto COLOR_REGION = IM_COL32(128, 128, 128, 128);

static const constexpr auto NAVBAR_HEIGHT = 24.0f;
static const constexpr auto NAVBAR_HEIGHT_HALF = NAVBAR_HEIGHT * 0.5f;

static const constexpr auto NAVBAR_MARKER_HEIGHT = 24.0f;
static const constexpr auto NAVBAR_MARKER_WIDTH = 8.0f;
static const constexpr auto NAVBAR_MARKER_THICKNESS = 4.0f;

static const constexpr auto NAVBAR_CIRCLE_RADIUS_MAIN = 16.0f;
static const constexpr auto NAVBAR_CIRCLE_RADIUS_2 = 4.0f;
static const constexpr auto NAVBAR_CIRCLE_THICKNESS = 2.0f;

} // namespace Settings

namespace {

void DrawMarker(bool start, ImVec2 position, ImU32 color) {
  // Note: Maybe not use the full height of the navbar, so calculate an offset here
  auto bottom = position + ImVec2(0.0, ::Settings::NAVBAR_MARKER_HEIGHT);
  auto side = position + ImVec2(::Settings::NAVBAR_MARKER_WIDTH * (start ? 1.0f : -1.0f), 0.0f);

  ImGui::GetWindowDrawList()->AddLine(position, bottom, color, ::Settings::NAVBAR_MARKER_THICKNESS);
  ImGui::GetWindowDrawList()->AddLine(position, side, color, ::Settings::NAVBAR_MARKER_THICKNESS);
  ImGui::GetWindowDrawList()->AddLine(bottom, bottom + side - position, color,
                                      ::Settings::NAVBAR_MARKER_THICKNESS);
}

} // anonymous namespace

namespace gits::gui {

NavBar::NavBar(bool enableSection) : m_SectionEnabled(enableSection) {
  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&NavBar::ActionCallback, this, std::placeholders::_1));
}

void NavBar::Render() {
  auto& context = StudioContext::GetInstance();
  auto& playHead = context.PlayHeadState;
  auto& playback = context.Playback;
  const bool canControl = playback.CanControl();
  const bool paused = playback.State() == Controller::PlaybackState::Paused;

  const auto totalFramesStr = playHead.TotalFrames == 0
                                  ? std::string("  -- %")
                                  : std::format("{:5.1f} %", playHead.Pos() * 100.0f);
  const auto totalFramesText = totalFramesStr.c_str();
  const auto totalFramesTextWidth =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, totalFramesText);

  const auto frameText =
      std::format("{} / {}", playHead.CurrentFrame,
                  playHead.TotalFrames == 0 ? "?" : std::to_string(playHead.TotalFrames));
  ImGui::TextUnformatted(frameText.c_str());
  ImGui::SameLine();

  const auto availableWidth = ImGui::GetContentRegionAvail().x;
  const float widthTimeLineArea = availableWidth - totalFramesTextWidth - ::Settings::PADDING -
                                  2 * ::Settings::NAVBAR_CIRCLE_RADIUS_MAIN;
  const auto vecWidthNavBar = ImVec2(widthTimeLineArea, 0.0f);

  const auto timeLineBasePos =
      ImGui::GetCursorPos() + ImVec2(::Settings::NAVBAR_CIRCLE_RADIUS_MAIN, 0.0f);

  ImVec2 timeLineOffset = ImVec2(0.0f, ::Settings::NAVBAR_HEIGHT_HALF);
  ImVec2 timeLineStart = timeLineBasePos + timeLineOffset;
  ImVec2 timeLineEnd = timeLineStart + vecWidthNavBar;

  // Interaction area: hover tooltip + drag to set section markers.
  const auto cursorPosBeforeTimeline = ImGui::GetCursorPos();
  ImGui::SetCursorPos(timeLineBasePos);
  ImGui::InvisibleButton("###timeline_hover",
                         ImVec2(widthTimeLineArea, ::Settings::NAVBAR_MARKER_HEIGHT));
  const bool isTimelineHovered = ImGui::IsItemHovered();
  const bool isTimelineActivated = ImGui::IsItemActivated();
  const bool isTimelineActive = ImGui::IsItemActive();

  const auto timelineItemMin = ImGui::GetItemRectMin();
  const auto timelineItemSize = ImGui::GetItemRectSize();

  auto normalizedPosOnTimeline = [&]() -> float {
    if (timelineItemSize.x <= 0.0f) {
      return 0.0f;
    }
    return (std::clamp)((ImGui::GetMousePos().x - timelineItemMin.x) / timelineItemSize.x, 0.0f,
                        1.0f);
  };

  if (m_SectionEnabled) {
    if (isTimelineHovered || isTimelineActive) {
      const float normalizedPos = normalizedPosOnTimeline();
      m_HoveredTimelinePercentage = normalizedPos * 100.0f;
      m_HoveredTimelineFrame = static_cast<int>(playHead.TotalFrames * normalizedPos);
      ImGui::SetTooltip("Frame: %i (%.1f %%)", m_HoveredTimelineFrame, m_HoveredTimelinePercentage);
    } else {
      m_HoveredTimelinePercentage = -1.0f;
      m_HoveredTimelineFrame = -1;
    }

    // Drag to define the section: mouse-down anchors the start, dragging extends to the end.
    if (isTimelineActivated) {
      m_DragAnchor = normalizedPosOnTimeline();
      m_SectionActive = true;
    }
    if (isTimelineActive) {
      const float currentPos = normalizedPosOnTimeline();
      const float selStart = (std::min)(m_DragAnchor, currentPos);
      const float selEnd = (std::max)(m_DragAnchor, currentPos);
      playHead.SetMarkerStart(
          static_cast<int>(selStart * static_cast<float>(playHead.TotalFrames)));
      playHead.SetMarkerEnd(static_cast<int>(selEnd * static_cast<float>(playHead.TotalFrames)));
    }
  }
  ImGui::SetCursorPos(cursorPosBeforeTimeline);

  // Region background
  if (m_SectionActive && m_SectionEnabled) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        timeLineBasePos + vecWidthNavBar * playHead.MarkerStartPos(),
        timeLineBasePos + vecWidthNavBar * playHead.MarkerEndPos() +
            ImVec2(0.0f, ::Settings::NAVBAR_MARKER_HEIGHT),
        ::Settings::COLOR_REGION);
  }
  // timeline
  ImGui::GetWindowDrawList()->AddLine(timeLineStart, timeLineEnd, ::Settings::COLOR_NAVBAR, 2.0f);

  // region marker
  if (m_SectionActive) {
    ::DrawMarker(true, timeLineBasePos + vecWidthNavBar * playHead.MarkerStartPos(),
                 ::Settings::COLOR_MARKER_1);
    ::DrawMarker(false, timeLineBasePos + vecWidthNavBar * playHead.MarkerEndPos(),
                 ::Settings::COLOR_MARKER_2);
  }

  // playhead
  if (playHead.TotalFrames > 0) {
    ImVec2 circleCenter = timeLineBasePos + timeLineOffset + vecWidthNavBar * playHead.Pos();
    ImGui::GetWindowDrawList()->AddCircle(circleCenter, ::Settings::NAVBAR_CIRCLE_RADIUS_MAIN,
                                          ::Settings::COLOR_CIRCLE, 16,
                                          ::Settings::NAVBAR_CIRCLE_THICKNESS);
    ImGui::GetWindowDrawList()->AddCircle(circleCenter, ::Settings::NAVBAR_CIRCLE_RADIUS_2,
                                          ::Settings::COLOR_CIRCLE, 16,
                                          ::Settings::NAVBAR_CIRCLE_THICKNESS);

    ImGui::SetCursorPos(timeLineBasePos + ImVec2(widthTimeLineArea + ::Settings::PADDING +
                                                     ::Settings::NAVBAR_CIRCLE_RADIUS_MAIN,
                                                 0.0f));
    ImGui::TextUnformatted(totalFramesText);
  } else {
    ImGui::NewLine();
  }

  // Navbar buttons
  ImGui::BeginDisabled(!playback.CanLoadReset());
  auto msg = (playback.CanLoadReset() && playback.Frame() > 0) ? Labels::NavBar::RESET_STREAM
                                                               : Labels::NavBar::LOAD_STREAM;
  if (ImGui::Button(msg)) {
    auto& configuration = context.ConfigurationForAction(GitsAction::PLAYBACK);
    auto& config = configuration.ModifiedGitsConfiguration;
    config.common.player.interactive = true;
    auto launchConfig = config;
    ModuleRegistry::GetInstance().AddActivePlugins(GitsAction::PLAYBACK,
                                                   configuration.MetaData.StreamApi, launchConfig);
    playback.LoadReset(context.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK), launchConfig,
                       true);
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGuiHelper::Seperator();
  ImGui::SameLine();

  ImGui::BeginDisabled(!canControl);
  auto playBtnMsg =
      !canControl ? Labels::NavBar::START_PLAYBACK
                  : (paused ? Labels::NavBar::START_PLAYBACK : Labels::NavBar::PAUSE_PLAYBACK);
  if (ImGui::Button(playBtnMsg)) {
    if (paused) {
      playback.Send("start");
    } else {
      playback.Send("pause");
    }
  }
  ImGui::EndDisabled();
  ImGui::SameLine();

  ImGui::BeginDisabled(!canControl);
  if (ImGui::Button(Labels::NavBar::STOP_PLAYBACK)) {
    playback.Send("stop");
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGuiHelper::Seperator();
  ImGui::SameLine();

  if (m_PlayUntilFrame <= playHead.CurrentFrame) {
    m_PlayUntilFrame = playHead.CurrentFrame + 1;
  }
  const bool validTarget = m_PlayUntilFrame >= 0 &&
                           static_cast<uint64_t>(m_PlayUntilFrame) > playHead.CurrentFrame &&
                           (playHead.TotalFrames == 0 ||
                            static_cast<uint64_t>(m_PlayUntilFrame) <= playHead.TotalFrames);
  ImGui::BeginDisabled(!canControl || !paused || !validTarget);
  if (ImGui::Button(Labels::NavBar::PLAY_UNTIL)) {
    playback.Send("play-until:" + std::to_string(m_PlayUntilFrame));
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  float widthUntilInputInt =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::InputInt, std::to_string(m_PlayUntilFrame));

  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthUntilInputInt);
  const int64_t inputStep = 1;
  ImGui::InputScalar("###00", ImGuiDataType_S64, &m_PlayUntilFrame, &inputStep);
  ImGui::SameLine();

  ImGuiHelper::Seperator();
  ImGui::SameLine();
  ImGui::BeginDisabled(!canControl || !paused || m_StepSize <= 0);
  if (ImGui::Button(Labels::NavBar::STEP_PLAYBACK)) {
    playback.Send("step:" + std::to_string(m_StepSize));
  }
  ImGui::EndDisabled();

  float widthInputInt =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::InputInt, std::to_string(m_StepSize));

  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthInputInt);
  ImGui::InputScalar("Size", ImGuiDataType_S64, &m_StepSize, &inputStep);

  if (m_SectionEnabled) {
    ImGui::SameLine();
    ImGuiHelper::Seperator();
    ImGui::SameLine();
    ImGui::Text(Labels::NavBar::SECTION_LABEL);
    ImGui::SameLine();
    ImGui::Button(Labels::NavBar::MARK_START);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(widthInputInt);
    int currentStart = static_cast<int>(playHead.MarkerStart);
    ImGui::InputInt("###01", &currentStart, 1, 10);
    playHead.SetMarkerStart(currentStart < 0 ? 0 : currentStart);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthInputInt);
    int currentEnd = static_cast<int>(playHead.MarkerEnd);
    ImGui::InputInt("###02", &currentEnd, 1, 10);
    playHead.SetMarkerEnd(currentEnd < 0 ? 0 : currentEnd);

    ImGui::SameLine();
    ImGui::Button(Labels::NavBar::MARK_END);
    ImGui::SameLine();
    ImGui::Checkbox("Active", &m_SectionActive);
  }
  // Status text
  ImGui::TextUnformatted(playback.Status().c_str());
  //ImGui::SameLine();
  // right align the next buttons
  //auto width = ImGui::GetContentRegionAvail().x;
  //auto sizes = 0.0f;
  //sizes += 64 + ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "Show Frames");
  //sizes += 32 + ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, "|");
  //sizes += 8 + ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "Subcapture");
  //sizes += 8 + ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "Extract Resources");

  //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - sizes);
  //ImGui::Button("Show Frames");
  //ImGui::SameLine();
  //ImGuiHelper::Seperator(true);
  //ImGui::Button("Create Subcapture");
  //ImGui::SameLine();
  //ImGui::Button("Extract Resources");
}

void NavBar::ActionCallback(const Event& e) {
  const ActionEvent& event = static_cast<const ActionEvent&>(e);
  auto& context = StudioContext::GetInstance();

  if (event.EventType == ActionEvent::Type::StudioAnalysisFinished) {
    if (context.PlayHeadState.TotalFrames != 0) {
      m_PlayUntilFrame = context.PlayHeadState.TotalFrames;
    } else if (context.Analysis.Screenshots) {
      m_PlayUntilFrame = context.PlayHeadState.TotalFrames;
    }
  }
}
} // namespace gits::gui
