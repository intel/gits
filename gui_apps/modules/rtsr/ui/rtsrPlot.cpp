// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrPlot.h"
#include "rtsr/ui/rtsrLabels.h"

#include "imGuiHelper.h"

#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <utility>

namespace gits::gui {

namespace {

constexpr double BAR_WIDTH = 1.0;
// Bars stay readable through a highlight rather than being hidden by it.
constexpr float SELECTION_ALPHA = 0.55f;
constexpr float HIGHLIGHT_ALPHA = 0.25f;

ImVec4 ThemeColor(ImGuiCol index, float alpha) {
  ImVec4 color = ImGui::GetStyleColorVec4(index);
  color.w = alpha;
  return color;
}

} // namespace

PlotRange PlotRange::FromData(const std::vector<float>& x, const std::vector<float>& y) {
  PlotRange range;
  if (x.empty() || y.empty()) {
    return range;
  }
  const auto xBounds = std::minmax_element(x.begin(), x.end());
  range.MinX = *xBounds.first;
  range.MaxX = *xBounds.second;
  const auto yBounds = std::minmax_element(y.begin(), y.end());
  range.MinY = *yBounds.first;
  range.MaxY = *yBounds.second;
  return range;
}

RtsrBarPlot::RtsrBarPlot(std::string seriesLabel) : m_SeriesLabel(std::move(seriesLabel)) {
  m_ColorBars = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
  m_ColorSelection = ThemeColor(ImGuiCol_TextSelectedBg, HIGHLIGHT_ALPHA);
  m_ColorHighlight = ThemeColor(ImGuiCol_PlotHistogramHovered, HIGHLIGHT_ALPHA);
}

void RtsrBarPlot::SetData(const std::vector<float>* x, const std::vector<float>* y) {
  m_DataX = x;
  m_DataY = y;
}

void RtsrBarPlot::SetDataRange(const PlotRange& range) {
  m_DataRange = range;
}

void RtsrBarPlot::SetAxisRange(const PlotRange& range) {
  m_AxisRange = range;
}

void RtsrBarPlot::DrawMouseNavigationToggle() {
  ImGui::Checkbox(RtsrLabels::MOUSE_NAVIGATION, &m_MouseNavigation);
  gits::ImGuiHelper::AddTooltip(RtsrLabels::MOUSE_NAVIGATION_HINT);
}

void RtsrBarPlot::Draw(size_t count) {
  if (m_DataX == nullptr || m_DataY == nullptr || count == 0) {
    return;
  }

  ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
  // Pinned every frame unless the user has taken over navigation, in which case
  // the limits are applied once and then left to ImPlot.
  const ImPlotCond condition = m_MouseNavigation ? ImPlotCond_Once : ImPlotCond_Always;
  ImPlot::SetupAxisLimits(ImAxis_X1, m_AxisRange.MinX, m_AxisRange.MaxX, condition);
  ImPlot::SetupAxisLimits(ImAxis_Y1, m_AxisRange.MinY, m_AxisRange.MaxY, condition);

  // Selection sits behind the bars, hover in front of them.
  ImPlot::PlotBars(m_SeriesLabel.c_str(), m_DataX->data(), m_DataY->data(), static_cast<int>(count),
                   BAR_WIDTH, {ImPlotProp_FillColor, m_ColorBars});
  DrawSelectionHighlight();
  DrawHoverHighlight();
}

void RtsrBarPlot::DrawSelectionHighlight() {
  if (!m_Selection.IsSelected() || m_DataX->empty()) {
    return;
  }

  const size_t begin = std::min(m_Selection.Begin(), m_DataX->size() - 1);
  const size_t end = std::min(m_Selection.End(), m_DataX->size() - 1);

  // One PlotBars call over the whole window. RTSR issued a second one for the
  // main index alone, which the window already covers.
  std::vector<float> x(m_DataX->begin() + static_cast<std::ptrdiff_t>(begin),
                       m_DataX->begin() + static_cast<std::ptrdiff_t>(end) + 1);
  std::vector<float> y(x.size(), m_AxisRange.MaxY);

  ImPlot::PlotBars("##selection", x.data(), y.data(), static_cast<int>(x.size()), BAR_WIDTH,
                   {ImPlotProp_FillColor, m_ColorSelection});
}

void RtsrBarPlot::DrawHoverHighlight() {
  if (!m_HasHover || m_HoverIndex >= m_DataX->size()) {
    return;
  }

  const float x = (*m_DataX)[m_HoverIndex];
  const float y = m_AxisRange.MaxY;

  ImPlot::PlotBars("##hover", &x, &y, 1, BAR_WIDTH, {ImPlotProp_FillColor, m_ColorHighlight});
}

RtsrBarPlot::MouseEvent RtsrBarPlot::PollMouse() {
  MouseEvent event;

  if (!ImPlot::IsPlotHovered() || m_MouseNavigation) {
    m_HasHover = false;
    return event;
  }

  event.Interaction = MouseInteraction::NoBar;
  if (m_DataX == nullptr || m_DataX->empty()) {
    m_HasHover = false;
    return event;
  }

  // Compared in plot space rather than by transforming every bar to pixels, and
  // scanning rather than deriving the index arithmetically, because the x values
  // are not required to be contiguous: pausing measurement leaves gaps.
  const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
  for (size_t i = 0; i < m_DataX->size(); ++i) {
    const double center = static_cast<double>((*m_DataX)[i]);
    if (mouse.x < center - BAR_WIDTH * 0.5 || mouse.x >= center + BAR_WIDTH * 0.5) {
      continue;
    }
    event.Index = i;
    event.Interaction = ImGui::IsMouseClicked(ImGuiMouseButton_Left) ? MouseInteraction::Click
                                                                     : MouseInteraction::Hover;
    break;
  }

  if (event.Interaction == MouseInteraction::Click) {
    m_Selection.SetIndex(event.Index);
    m_Selection.SetSelected(true);
  }

  m_HasHover = event.Interaction == MouseInteraction::Hover;
  if (m_HasHover) {
    m_HoverIndex = event.Index;
  }
  return event;
}

void RtsrBarPlot::Reset() {
  // Deliberately leaves m_MouseNavigation alone: it is a user preference, not
  // view state. RTSR's reset also cleared its show-hover and show-selection
  // flags, which the constructors set once and never restored, so highlighting
  // silently stopped working after a reset.
  m_DataRange = {};
  m_AxisRange = {};
  m_Selection.Reset();
  m_HoverIndex = 0;
  m_HasHover = false;
}

void RtsrBarPlot::SetColors(const ImVec4& bars, const ImVec4& selection, const ImVec4& highlight) {
  m_ColorBars = bars;
  m_ColorSelection = ImVec4(selection);
  m_ColorSelection.w = SELECTION_ALPHA;
  m_ColorHighlight = ImVec4(highlight);
  m_ColorHighlight.w = HIGHLIGHT_ALPHA;
}

} // namespace gits::gui
