// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/ui/rtsrEvents.h"
#include "rtsr/ui/rtsrModule.h"
#include "rtsr/ui/rtsrPerformanceStats.h"
#include "rtsr/ui/rtsrPlot.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace gits::gui {

// Where the fixed Y limit starts: one frame at 30 Hz, so a fixed scale has a
// recognisable reference point rather than an arbitrary one.
constexpr float DEFAULT_AXIS_Y_MAX = 33.3f;

// The Performance tab: present-to-present time per frame, the per-pipeline GPU
// cost inside the selected frame or range, and the numbers behind both.
class RtsrPerformancePanel {
public:
  // Not const: enabling measurement is a request sent to the client.
  explicit RtsrPerformancePanel(RtsrModule& module);

  void Render();
  void OnRtsrEvent(const RtsrEvent& event);

  // Drops the harvested run's timings and the view onto them. Called when the
  // next run starts, and by RtsrPanel when the module is disabled.
  void ClearSession();

  // Called when the user asks to see the selected pipeline on the Shaders tab.
  void SetGoToPipeline(std::function<void(rtsr::RTSR_ID)> callback);

private:
  void RefreshSeries();
  void RefreshWindow();
  void SortPipelines();
  void ResetView();

  void DrawToolbar();
  void DrawFrameControls();
  void DrawFramePlot();
  void DrawPipelineControls();
  void DrawPipelinePlot();
  void DrawDetails();

  void ThemeChangedCallback(const Event& e);

  RtsrModule& m_Module;
  std::function<void(rtsr::RTSR_ID)> m_GoToPipeline;

  // Raised by the event and consumed by the next render, so aggregation happens
  // once per arriving frame rather than once per drawn frame.
  bool m_SeriesDirty{true};
  bool m_WindowDirty{true};

  RtsrBarPlot m_FramePlot;
  std::vector<float> m_FramePositions;
  std::vector<float> m_FrameDurations;
  // Cached so the axis limits and the manual Y limit's upper bound do not walk
  // the series again on every drawn frame.
  PlotRange m_FrameDataRange{};
  // The client's frame numbers, used for the axis labels, the tooltip and to
  // follow a frame while the selection is sticky.
  std::vector<size_t> m_FrameNumbers;

  bool m_AutoScaleX{true};
  int m_VisibleFrames{static_cast<int>(FRAME_HISTORY)};
  bool m_AutoScaleY{true};
  float m_AxisYMax{DEFAULT_AXIS_Y_MAX};
  bool m_Sticky{false};

  RtsrBarPlot m_PipelinePlot;
  WindowAggregate m_Window;
  std::vector<rtsr::RTSR_ID> m_SortedPipelines;
  std::vector<float> m_PipelinePositions;
  std::vector<float> m_PipelineValues;
  PlotRange m_PipelineDataRange{};
  bool m_Accumulate{false};
  PipelineTimeStats::Field m_SortField{PipelineTimeStats::Field::TotalDuration};
  bool m_Descending{true};

  std::optional<rtsr::RTSR_ID> m_SelectedPipeline;

  bool m_Collecting{false};
  // Why the last collection request failed. Survives the toggle bouncing back.
  std::string m_Notice;
  // Compared each frame to notice that the frame selection moved, which is what
  // invalidates the window aggregate.
  PlotSelection m_LastFrameSelection;

  float m_WidthSortField = 0.0f;
  float m_WidthSortOrder = 0.0f;
};

} // namespace gits::gui
