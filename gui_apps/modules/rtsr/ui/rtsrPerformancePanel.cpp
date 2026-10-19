// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrPerformancePanel.h"
#include "rtsr/ui/rtsrLabels.h"

#include "imGuiHelper.h"

#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <utility>

namespace gits::gui {

namespace {

using LB = RtsrLabels;

constexpr float CONTROL_WIDTH = 220.0f;
// Headroom above the tallest bar, so the highest frame is not flush with the
// top of the plot.
constexpr float AXIS_HEADROOM = 1.12f;
// Any axis needs a non-degenerate span; ImPlot rejects an inverted or empty one.
constexpr float MIN_AXIS_SPAN = 0.001f;

std::string Milliseconds(float value) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << value << " ms";
  return oss.str();
}

std::string Percent(float value) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value << " %";
  return oss.str();
}

// The x axis carries dense positions so that a pause in measurement does not
// leave a hole in the plot, but the ticks should still read as frame numbers.
int FormatFrameNumber(double value, char* buffer, int size, void* userData) {
  const auto* numbers = static_cast<const std::vector<size_t>*>(userData);
  const auto index = static_cast<long long>(std::llround(value));
  if (numbers == nullptr || index < 0 || static_cast<size_t>(index) >= numbers->size()) {
    return snprintf(buffer, size, " ");
  }
  return snprintf(buffer, size, "%zu", (*numbers)[static_cast<size_t>(index)]);
}

void DetailRow(const char* label, const std::string& value) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(label);
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(value.c_str());
}

const PipelineTimeStats::Field PipelineSortFields[] = {
    PipelineTimeStats::Field::TotalDuration, PipelineTimeStats::Field::MinDuration,
    PipelineTimeStats::Field::MaxDuration, PipelineTimeStats::Field::Occurrences};

const char* FieldText(PipelineTimeStats::Field field) {
  switch (field) {
  case PipelineTimeStats::Field::TotalDuration:
    return LB::FIELD_TOTAL_DURATION;
  case PipelineTimeStats::Field::MinDuration:
    return LB::FIELD_MIN_DURATION;
  case PipelineTimeStats::Field::MaxDuration:
    return LB::FIELD_MAX_DURATION;
  case PipelineTimeStats::Field::Occurrences:
    return LB::FIELD_OCCURRENCES;
  }
  return LB::UNKNOWN;
}

} // namespace

RtsrPerformancePanel::RtsrPerformancePanel(RtsrModule& module)
    : m_Module(module), m_FramePlot(LB::FRAME_AXIS_Y), m_PipelinePlot(LB::PIPELINE_AXIS_X) {
  m_FramePlot.SetData(&m_FramePositions, &m_FrameDurations);
  m_PipelinePlot.SetData(&m_PipelinePositions, &m_PipelineValues);

  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&RtsrPerformancePanel::ThemeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ThemeChanged});
}

void RtsrPerformancePanel::SetGoToPipeline(std::function<void(rtsr::RTSR_ID)> callback) {
  m_GoToPipeline = std::move(callback);
}

void RtsrPerformancePanel::OnRtsrEvent(const RtsrEvent& event) {
  switch (event.EventType) {
  case RtsrEvent::Type::TimeStatsUpdated:
    m_SeriesDirty = true;
    break;

  case RtsrEvent::Type::SessionOpened:
    // The aggregator has been emptied for the new run, so a selection into the
    // previous one's frames has to go with it.
    ClearSession();
    break;

  case RtsrEvent::Type::SessionClosed:
    // The frames the run produced are kept, so the view that points into them
    // is kept as well. Only the client is gone, and with it any collecting it
    // was doing on our behalf.
    m_Collecting = false;
    m_Notice.clear();
    break;

  default:
    break;
  }
}

void RtsrPerformancePanel::ClearSession() {
  ResetView();
  m_Collecting = false;
  m_Notice.clear();
}

// **********************************************************************************************
// * Deriving what the plots draw                                                               *
// **********************************************************************************************

void RtsrPerformancePanel::RefreshSeries() {
  m_SeriesDirty = false;

  // A sticky selection is anchored to a frame number, so the number is resolved
  // before the series is replaced and the position is looked up again after.
  std::optional<size_t> stickyFrame;
  if (m_Sticky && m_FramePlot.Selection().IsSelected()) {
    const size_t index = m_FramePlot.Selection().Index();
    if (index < m_FrameNumbers.size()) {
      stickyFrame = m_FrameNumbers[index];
    }
  }

  m_Module.Performance().ReadFrameSeries(m_FramePositions, m_FrameDurations, m_FrameNumbers);
  m_FramePlot.Selection().SetPointCount(m_FramePositions.size());

  if (stickyFrame.has_value()) {
    if (const auto index = m_Module.Performance().IndexOfFrame(*stickyFrame)) {
      m_FramePlot.Selection().SetIndex(*index);
    } else {
      // The frame has scrolled out of the history.
      m_FramePlot.Selection().SetSelected(false);
    }
  }

  m_FrameDataRange = PlotRange::FromData(m_FramePositions, m_FrameDurations);
  m_FramePlot.SetDataRange(m_FrameDataRange);
  m_WindowDirty = true;
}

void RtsrPerformancePanel::RefreshWindow() {
  m_WindowDirty = false;

  const auto& selection = m_FramePlot.Selection();
  if (!selection.IsSelected() || m_FramePositions.empty()) {
    m_Window = {};
    m_SortedPipelines.clear();
    m_PipelinePositions.clear();
    m_PipelineValues.clear();
    return;
  }

  m_Window = m_Module.Performance().ReadWindow(selection.Begin(), selection.End());
  SortPipelines();
}

void RtsrPerformancePanel::SortPipelines() {
  m_SortedPipelines.clear();
  m_SortedPipelines.reserve(m_Window.Pipelines.size());
  for (const auto& [id, stats] : m_Window.Pipelines) {
    m_SortedPipelines.push_back(id);
  }

  std::sort(m_SortedPipelines.begin(), m_SortedPipelines.end(),
            [this](rtsr::RTSR_ID lhs, rtsr::RTSR_ID rhs) {
              const float a = m_Window.Pipelines.at(lhs).Get(m_SortField);
              const float b = m_Window.Pipelines.at(rhs).Get(m_SortField);
              // Ties broken by id, so the order does not shuffle between frames.
              if (a == b) {
                return lhs < rhs;
              }
              return m_Descending ? a > b : a < b;
            });

  m_PipelinePositions.resize(m_SortedPipelines.size());
  m_PipelineValues.resize(m_SortedPipelines.size());

  float running = 0.0f;
  for (size_t i = 0; i < m_SortedPipelines.size(); ++i) {
    const float value = m_Window.Pipelines.at(m_SortedPipelines[i]).Get(m_SortField);
    running += value;
    m_PipelinePositions[i] = static_cast<float>(i);
    m_PipelineValues[i] = m_Accumulate ? running : value;
  }

  m_PipelinePlot.Selection().SetPointCount(m_SortedPipelines.size());
  m_PipelineDataRange = PlotRange::FromData(m_PipelinePositions, m_PipelineValues);
  m_PipelinePlot.SetDataRange(m_PipelineDataRange);

  // Keep the highlighted bar on the pipeline the user picked, which sorting may
  // have moved somewhere else.
  if (m_SelectedPipeline.has_value()) {
    const auto it =
        std::find(m_SortedPipelines.begin(), m_SortedPipelines.end(), *m_SelectedPipeline);
    if (it == m_SortedPipelines.end()) {
      m_SelectedPipeline.reset();
      m_PipelinePlot.Selection().SetSelected(false);
    } else {
      m_PipelinePlot.Selection().SetIndex(
          static_cast<size_t>(std::distance(m_SortedPipelines.begin(), it)));
    }
  }
}

void RtsrPerformancePanel::ResetView() {
  m_FramePlot.Reset();
  m_PipelinePlot.Reset();
  m_AutoScaleX = true;
  m_VisibleFrames = static_cast<int>(FRAME_HISTORY);
  m_AutoScaleY = true;
  m_AxisYMax = DEFAULT_AXIS_Y_MAX;
  m_Accumulate = false;
  m_SortField = PipelineTimeStats::Field::TotalDuration;
  m_Descending = true;
  m_SelectedPipeline.reset();
  m_LastFrameSelection = PlotSelection{};
  m_SeriesDirty = true;
}

// **********************************************************************************************
// * Rendering                                                                                  *
// **********************************************************************************************

void RtsrPerformancePanel::Render() {
  if (m_SeriesDirty) {
    RefreshSeries();
  }
  if (m_WindowDirty || m_FramePlot.Selection() != m_LastFrameSelection) {
    m_LastFrameSelection = m_FramePlot.Selection();
    RefreshWindow();
  }

  DrawToolbar();
  if (!m_Notice.empty()) {
    ImGui::TextUnformatted(m_Notice.c_str());
  }
  ImGui::Separator();

  if (m_FramePositions.empty()) {
    ImGui::TextUnformatted(LB::PERF_EMPTY);
    //return;
  }

  const float detailsWidth = ImGui::GetFontSize() * 17.0f;
  const float frameHeight = ImGui::GetContentRegionAvail().y * 0.5f;

  ImGui::BeginChild("RtsrFramePane", ImVec2(0.0f, frameHeight),
                    ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
  DrawFrameControls();
  DrawFramePlot();
  ImGui::EndChild();

  if (ImGui::BeginTable("RtsrPerfPanes", 2, ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("##plot", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##details", ImGuiTableColumnFlags_WidthFixed, detailsWidth);

    ImGui::TableNextColumn();
    ImGui::BeginChild("RtsrPipelinePane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Border);
    if (m_FramePlot.Selection().IsSelected() && !m_SortedPipelines.empty()) {
      DrawPipelineControls();
      DrawPipelinePlot();
    } else {
      ImGui::TextUnformatted(LB::PERF_NO_FRAME);
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();
    ImGui::BeginChild("RtsrDetailsPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Border);
    DrawDetails();
    ImGui::EndChild();

    ImGui::EndTable();
  }
}

void RtsrPerformancePanel::DrawToolbar() {
  const bool wasCollecting = m_Collecting;
  const ImGuiCol tint = wasCollecting ? ImGuiCol_ButtonActive : ImGuiCol_Button;
  ImGuiHelper::PushButtonStyle(
      wasCollecting ? ImGuiHelper::ButtonStyle::BUTTON_STYLE_WARNING
                    : (m_Module.IsSessionOpen() ? ImGuiHelper::ButtonStyle::BUTTON_STYLE_SUCCESS
                                                : ImGuiHelper::ButtonStyle::BUTTON_STYLE_FAILURE));
  // A finished run keeps its timings on screen, but there is no longer a client
  // to ask for more.
  ImGui::BeginDisabled(!m_Module.IsSessionOpen());
  auto label = m_Module.IsSessionOpen() ? (wasCollecting ? LB::COLLECT_END : LB::COLLECT_START)
                                        : LB::COLLECT_DISABLED;
  if (ImGui::Button(label)) {
    m_Notice.clear();
    // The button reports what the client answered, not what was asked for, so a
    // plugin without timing support cannot leave it looking enabled.
    if (m_Module.EnablePerformanceMeasurements(!wasCollecting)) {
      m_Collecting = !wasCollecting;
    } else {
      m_Notice = LB::COLLECT_REFUSED;
    }
  }
  ImGui::EndDisabled();
  ImGuiHelper::PopButtonStyle();
  gits::ImGuiHelper::AddTooltip(LB::COLLECT_HINT);

  ImGui::SameLine();
  if (ImGui::Button(LB::RESET_VIEW)) {
    ResetView();
  }
  gits::ImGuiHelper::AddTooltip(LB::RESET_VIEW_HINT);
}

void RtsrPerformancePanel::DrawFrameControls() {
  m_FramePlot.DrawMouseNavigationToggle();
  ImGui::SameLine();

  ImGui::BeginDisabled(m_FramePlot.MouseNavigationEnabled());

  ImGui::Checkbox(LB::AUTOSCALE_X, &m_AutoScaleX);
  gits::ImGuiHelper::AddTooltip(LB::AUTOSCALE_X_HINT);

  ImGui::SameLine();
  ImGui::BeginDisabled(m_AutoScaleX);
  ImGui::SetNextItemWidth(CONTROL_WIDTH);
  const int maxFrames = std::max<int>(static_cast<int>(m_FramePositions.size()), 1);
  ImGui::SliderInt(LB::FRAME_WINDOW, &m_VisibleFrames, 1, maxFrames, "%d",
                   ImGuiSliderFlags_AlwaysClamp);
  gits::ImGuiHelper::AddTooltip(LB::FRAME_WINDOW_HINT);
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::Checkbox(LB::AUTOSCALE_Y, &m_AutoScaleY);
  gits::ImGuiHelper::AddTooltip(LB::AUTOSCALE_Y_HINT);

  ImGui::SameLine();
  ImGui::BeginDisabled(m_AutoScaleY);
  ImGui::SetNextItemWidth(CONTROL_WIDTH);
  // Bounded by the data rather than by the current value, so dragging the
  // slider cannot keep pushing its own upper limit further out.
  const float axisYCeiling = std::max(DEFAULT_AXIS_Y_MAX, m_FrameDataRange.MaxY * AXIS_HEADROOM);
  ImGui::SliderFloat(LB::AXIS_Y_MAX, &m_AxisYMax, DEFAULT_AXIS_Y_MAX, axisYCeiling, "%.2f",
                     ImGuiSliderFlags_AlwaysClamp);
  gits::ImGuiHelper::AddTooltip(LB::AXIS_Y_MAX_HINT);
  ImGui::EndDisabled();

  ImGui::EndDisabled();

  // Selection controls.
  bool selected = m_FramePlot.Selection().IsSelected();
  if (ImGui::Checkbox(LB::FRAME_SELECTION, &selected)) {
    m_FramePlot.Selection().SetSelected(selected);
  }
  gits::ImGuiHelper::AddTooltip(LB::FRAME_SELECTION_HINT);

  ImGui::SameLine();
  ImGui::BeginDisabled(!selected);

  ImGui::SetNextItemWidth(CONTROL_WIDTH);
  int frame = static_cast<int>(m_FramePlot.Selection().Index());
  const int lastFrame = std::max<int>(static_cast<int>(m_FramePositions.size()) - 1, 0);
  if (ImGui::SliderInt("##RtsrFrame", &frame, 0, lastFrame, "", ImGuiSliderFlags_AlwaysClamp)) {
    m_FramePlot.Selection().SetIndex(static_cast<size_t>(frame));
  }
  ImGui::SameLine();
  if (!m_FrameNumbers.empty()) {
    const size_t position = std::min(m_FramePlot.Selection().Index(), m_FrameNumbers.size() - 1);
    ImGui::Text("%zu", m_FrameNumbers[position]);
  }

  ImGui::SameLine();
  if (ImGui::Button(LB::PREVIOUS)) {
    m_FramePlot.Selection().TrySetIndex(m_FramePlot.Selection().Index() - 1);
  }
  gits::ImGuiHelper::AddTooltip(LB::PREVIOUS_FRAME_HINT);
  ImGui::SameLine();
  if (ImGui::Button(LB::NEXT)) {
    m_FramePlot.Selection().TrySetIndex(m_FramePlot.Selection().Index() + 1);
  }
  gits::ImGuiHelper::AddTooltip(LB::NEXT_FRAME_HINT);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(CONTROL_WIDTH);
  int length = static_cast<int>(m_FramePlot.Selection().WindowLength());
  const int maxLength =
      std::max<int>(static_cast<int>(m_FramePlot.Selection().MaxWindowLength()), 1);
  if (ImGui::SliderInt(LB::SELECTION_LENGTH, &length, 1, maxLength, "%d",
                       ImGuiSliderFlags_AlwaysClamp)) {
    m_FramePlot.Selection().SetWindowLength(static_cast<size_t>(length));
  }
  gits::ImGuiHelper::AddTooltip(LB::SELECTION_LENGTH_HINT);

  ImGui::SameLine();
  ImGui::Checkbox(LB::STICKY_SELECTION, &m_Sticky);
  gits::ImGuiHelper::AddTooltip(LB::STICKY_SELECTION_HINT);

  ImGui::EndDisabled();
}

void RtsrPerformancePanel::DrawFramePlot() {
  const PlotRange& data = m_FrameDataRange;

  PlotRange axis;
  axis.MinX =
      m_AutoScaleX ? data.MinX - 0.5f : data.MaxX - static_cast<float>(m_VisibleFrames) + 0.5f;
  axis.MaxX = data.MaxX + 0.5f;
  axis.MinY = 0.0f;
  axis.MaxY = m_AutoScaleY ? data.MaxY * AXIS_HEADROOM : m_AxisYMax;
  axis.MaxY = std::max(axis.MaxY, MIN_AXIS_SPAN);
  axis.MaxX = std::max(axis.MaxX, axis.MinX + MIN_AXIS_SPAN);
  m_FramePlot.SetAxisRange(axis);

  if (!ImPlot::BeginPlot(LB::FRAME_PLOT, ImVec2(-1.0f, -1.0f), ImPlotFlags_NoLegend)) {
    return;
  }

  ImPlot::SetupAxes(LB::FRAME_AXIS_X, LB::FRAME_AXIS_Y, ImPlotAxisFlags_NoHighlight,
                    ImPlotAxisFlags_NoHighlight);
  ImPlot::SetupAxisFormat(ImAxis_X1, FormatFrameNumber, &m_FrameNumbers);

  m_FramePlot.Draw(m_FramePositions.size());

  const auto mouse = m_FramePlot.PollMouse();
  if (mouse.Interaction == RtsrBarPlot::MouseInteraction::Hover &&
      mouse.Index < m_FrameNumbers.size()) {
    ImGui::BeginTooltip();
    ImGui::Text("%s %zu", LB::FRAME_AXIS_X, m_FrameNumbers[mouse.Index]);
    ImGui::TextUnformatted(Milliseconds(m_FrameDurations[mouse.Index]).c_str());
    ImGui::EndTooltip();
  }

  ImPlot::EndPlot();
}

void RtsrPerformancePanel::DrawPipelineControls() {
  m_PipelinePlot.DrawMouseNavigationToggle();

  ImGui::SameLine();
  if (ImGui::Checkbox(LB::ACCUMULATE, &m_Accumulate)) {
    m_WindowDirty = true;
  }
  gits::ImGuiHelper::AddTooltip(LB::ACCUMULATE_HINT);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(m_WidthSortField);
  if (ImGui::BeginCombo(LB::PIPELINE_PROPERTY, FieldText(m_SortField))) {
    for (const auto field : PipelineSortFields) {
      if (ImGui::Selectable(FieldText(field), field == m_SortField)) {
        m_SortField = field;
        m_WindowDirty = true;
      }
    }
    ImGui::EndCombo();
  }
  gits::ImGuiHelper::AddTooltip(LB::PIPELINE_PROPERTY_HINT);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(m_WidthSortOrder);
  if (ImGui::BeginCombo(LB::SORT, m_Descending ? LB::SORT_DESCENDING : LB::SORT_ASCENDING)) {
    if (ImGui::Selectable(LB::SORT_DESCENDING, m_Descending)) {
      m_Descending = true;
      m_WindowDirty = true;
    }
    if (ImGui::Selectable(LB::SORT_ASCENDING, !m_Descending)) {
      m_Descending = false;
      m_WindowDirty = true;
    }
    ImGui::EndCombo();
  }

  ImGui::SetNextItemWidth(CONTROL_WIDTH);
  int index = static_cast<int>(m_PipelinePlot.Selection().Index());
  const int last = std::max<int>(static_cast<int>(m_SortedPipelines.size()) - 1, 0);
  if (ImGui::SliderInt(LB::SELECTED_PIPELINE, &index, 0, last, "%d",
                       ImGuiSliderFlags_AlwaysClamp)) {
    m_PipelinePlot.Selection().SetIndex(static_cast<size_t>(index));
    m_PipelinePlot.Selection().SetSelected(true);
    m_SelectedPipeline = m_SortedPipelines[static_cast<size_t>(index)];
  }

  ImGui::SameLine();
  if (ImGui::Button(LB::PREVIOUS) &&
      m_PipelinePlot.Selection().TrySetIndex(m_PipelinePlot.Selection().Index() - 1)) {
    m_SelectedPipeline = m_SortedPipelines[m_PipelinePlot.Selection().Index()];
  }
  gits::ImGuiHelper::AddTooltip(LB::PREVIOUS_PIPELINE_HINT);
  ImGui::SameLine();
  if (ImGui::Button(LB::NEXT) &&
      m_PipelinePlot.Selection().TrySetIndex(m_PipelinePlot.Selection().Index() + 1)) {
    m_SelectedPipeline = m_SortedPipelines[m_PipelinePlot.Selection().Index()];
  }
  gits::ImGuiHelper::AddTooltip(LB::NEXT_PIPELINE_HINT);
}

void RtsrPerformancePanel::DrawPipelinePlot() {
  const PlotRange& data = m_PipelineDataRange;

  PlotRange axis;
  axis.MinX = data.MinX - 0.5f;
  axis.MaxX = std::max(data.MaxX + 0.5f, axis.MinX + MIN_AXIS_SPAN);
  axis.MinY = 0.0f;
  axis.MaxY = std::max(data.MaxY * AXIS_HEADROOM, MIN_AXIS_SPAN);
  m_PipelinePlot.SetAxisRange(axis);

  const std::string title = std::string(FieldText(m_SortField)) + " per pipeline";
  if (!ImPlot::BeginPlot(title.c_str(), ImVec2(-1.0f, -1.0f), ImPlotFlags_NoLegend)) {
    return;
  }

  // The tick labels would be positions in the sorted order, which mean nothing.
  ImPlot::SetupAxes(LB::PIPELINE_AXIS_X, FieldText(m_SortField),
                    ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoTickLabels,
                    ImPlotAxisFlags_NoHighlight);

  m_PipelinePlot.Draw(m_PipelinePositions.size());

  const auto mouse = m_PipelinePlot.PollMouse();
  if (mouse.Index < m_SortedPipelines.size()) {
    if (mouse.Interaction == RtsrBarPlot::MouseInteraction::Hover) {
      ImGui::BeginTooltip();
      ImGui::Text("%s %llu", LB::ROW_ID,
                  static_cast<unsigned long long>(m_SortedPipelines[mouse.Index]));
      ImGui::TextUnformatted(Milliseconds(m_PipelineValues[mouse.Index]).c_str());
      ImGui::EndTooltip();
    } else if (mouse.Interaction == RtsrBarPlot::MouseInteraction::Click) {
      m_SelectedPipeline = m_SortedPipelines[mouse.Index];
    }
  }

  ImPlot::EndPlot();
}

void RtsrPerformancePanel::DrawDetails() {
  constexpr auto flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                         ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;

  if (ImGui::BeginTable("RtsrPerfTotals", 2, flags)) {
    ImGui::TableSetupColumn(LB::DETAILS_SELECTION);
    ImGui::TableSetupColumn(LB::DETAILS_VALUE);
    ImGui::TableHeadersRow();
    DetailRow(LB::ROW_FRAMES, std::to_string(m_Window.FrameCount));
    DetailRow(LB::ROW_DURATION_CPU, Milliseconds(m_Window.CpuDuration));
    DetailRow(LB::ROW_DURATION_GPU, Milliseconds(m_Window.GpuDuration));
    DetailRow(LB::ROW_PIPELINES, std::to_string(m_Window.Pipelines.size()));
    ImGui::EndTable();
  }

  const PipelineTimeStats* stats = nullptr;
  if (m_SelectedPipeline.has_value()) {
    const auto it = m_Window.Pipelines.find(*m_SelectedPipeline);
    if (it != m_Window.Pipelines.end()) {
      stats = &it->second;
    }
  }

  ImGui::Spacing();
  if (ImGui::BeginTable("RtsrPerfPipeline", 2, flags)) {
    ImGui::TableSetupColumn(LB::DETAILS_PIPELINE);
    ImGui::TableSetupColumn(LB::DETAILS_VALUE);
    ImGui::TableHeadersRow();
    DetailRow(LB::ROW_ID, stats != nullptr ? std::to_string(*m_SelectedPipeline)
                                           : std::string(LB::NOT_AVAILABLE));
    DetailRow(LB::ROW_OCCURRENCES, stats != nullptr ? std::to_string(stats->Occurrences)
                                                    : std::string(LB::NOT_AVAILABLE));
    DetailRow(LB::ROW_DURATION, stats != nullptr ? Milliseconds(stats->TotalDuration)
                                                 : std::string(LB::NOT_AVAILABLE));
    DetailRow(LB::ROW_SHARE,
              stats != nullptr ? Percent(stats->ShareOfFrame) : std::string(LB::NOT_AVAILABLE));
    ImGui::EndTable();
  }

  // Extremes only say something once more than one frame is aggregated.
  if (stats != nullptr && m_Window.FrameCount > 1) {
    ImGui::Spacing();
    if (ImGui::BeginTable("RtsrPerfExtremes", 2, flags)) {
      ImGui::TableSetupColumn(LB::DETAILS_EXTREMES);
      ImGui::TableSetupColumn(LB::DETAILS_VALUE);
      ImGui::TableHeadersRow();
      DetailRow(LB::ROW_MIN_DURATION, Milliseconds(stats->MinDuration));
      DetailRow(LB::ROW_MAX_DURATION, Milliseconds(stats->MaxDuration));
      ImGui::EndTable();
    }
  }

  ImGui::Spacing();
  ImGui::BeginDisabled(stats == nullptr || !m_GoToPipeline);
  if (ImGui::Button(LB::GO_TO_PIPELINE) && stats != nullptr && m_GoToPipeline) {
    m_GoToPipeline(*m_SelectedPipeline);
  }
  ImGui::EndDisabled();
  gits::ImGuiHelper::AddTooltip(LB::GO_TO_PIPELINE_HINT);
}

void RtsrPerformancePanel::ThemeChangedCallback(const Event& e) {
  const ImVec4& bars = ImGui::GetStyleColorVec4(ImGuiCol_Button);
  const ImVec4& selection = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
  const ImVec4& highlight = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
  m_FramePlot.SetColors(bars, selection, highlight);
  m_PipelinePlot.SetColors(bars, selection, highlight);

  m_WidthSortField = 0.0f;
  for (const auto field : PipelineSortFields) {
    m_WidthSortField = std::max<float>(
        m_WidthSortField, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Combo, FieldText(field)));
  }
  m_WidthSortOrder = 0.0f;
  m_WidthSortOrder = std::max<float>(
      m_WidthSortOrder, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Combo, LB::SORT_DESCENDING));
  m_WidthSortOrder = std::max<float>(
      m_WidthSortOrder, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Combo, LB::SORT_ASCENDING));
}
} // namespace gits::gui
