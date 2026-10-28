// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"

#include "rtsr/ui/rtsrSelection.h"

#include <cstddef>
#include <string>
#include <vector>

// The one plot shape the performance view needs: a bar per data point, with
// hover and selection highlighting and click-to-select. RTSR's original carried
// a graph type enum with a single value, a second hit model that nothing
// selected, a marker overlay with no callers and an ImPlot flag member that was
// written but never read; none of that is here.

namespace gits::gui {

struct PlotRange {
  float MinX{};
  float MaxX{};
  float MinY{};
  float MaxY{};

  // Zeroed for empty data. RTSR's version dereferenced minmax_element on an
  // empty range and relied on every caller checking first.
  static PlotRange FromData(const std::vector<float>& x, const std::vector<float>& y);
};

class RtsrBarPlot {
public:
  enum class MouseInteraction {
    // The pointer is not over the plot at all.
    Outside,
    // Over the plot but not over a bar.
    NoBar,
    Hover,
    Click
  };

  struct MouseEvent {
    MouseInteraction Interaction{MouseInteraction::Outside};
    // Only meaningful for Hover and Click.
    size_t Index{};
  };

  explicit RtsrBarPlot(std::string seriesLabel);

  // Borrowed, and expected to stay alive and in step for as long as the plot is
  // drawn. The owning panel keeps both vectors.
  void SetData(const std::vector<float>* x, const std::vector<float>* y);

  // DataRange is what the data actually spans; AxisRange is what the axes are
  // pinned to. They differ whenever the user has fixed a scale by hand.
  void SetDataRange(const PlotRange& range);
  void SetAxisRange(const PlotRange& range);

  PlotSelection& Selection() {
    return m_Selection;
  }
  const PlotSelection& Selection() const {
    return m_Selection;
  }

  // While enabled, the axes are left alone so ImPlot's own pan and zoom work,
  // and hit testing is suspended.
  bool MouseNavigationEnabled() const {
    return m_MouseNavigation;
  }
  void DrawMouseNavigationToggle();

  // Call between ImPlot::BeginPlot and EndPlot, after SetupAxes.
  void Draw(size_t count);

  // Call after Draw, inside the same plot. Updates hover state and, on a click,
  // moves the selection.
  MouseEvent PollMouse();

  void Reset();

  void SetColors(const ImVec4& bars, const ImVec4& selection, const ImVec4& highlight);

private:
  void DrawSelectionHighlight();
  void DrawHoverHighlight();

  std::string m_SeriesLabel;
  const std::vector<float>* m_DataX{};
  const std::vector<float>* m_DataY{};
  PlotRange m_DataRange{};
  PlotRange m_AxisRange{};
  PlotSelection m_Selection{};

  size_t m_HoverIndex{};
  bool m_HasHover{false};
  bool m_MouseNavigation{false};

  ImVec4 m_ColorBars;
  ImVec4 m_ColorSelection;
  ImVec4 m_ColorHighlight;
};

} // namespace gits::gui
