// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

// Turns the per-frame timing messages the client sends into the aggregates the
// performance tab plots. Nothing here knows about ImGui.
//
// The wire carries nanoseconds; everything below is milliseconds.

namespace gits::gui {

// How many frames are kept. The oldest is evicted beyond this, which is what
// makes the frame plot a rolling window.
constexpr size_t FRAME_HISTORY = 600;

struct PipelineTimeStats {
  // Times the pipeline was bound within the aggregated range.
  size_t Occurrences{};
  float TotalDuration{};
  float MinDuration{};
  float MaxDuration{};
  float AverageDuration{};
  // Percent of the aggregated range's total GPU time.
  float ShareOfFrame{};

  // The metrics the user can plot and sort by. Average duration and share of
  // frame are shown in the details table but are not sort keys, which is what
  // RTSREditor offered too.
  enum class Field {
    TotalDuration,
    MinDuration,
    MaxDuration,
    Occurrences
  };

  float Get(Field field) const;
};

// One kept frame.
struct FrameSample {
  // The client's frame number, not a position. Pausing measurement leaves gaps
  // in it, which is why the plot uses the position and shows this in tooltips.
  size_t FrameNumber{};
  float PresentToPresent{};
  // Summed pipeline durations, so it is GPU time attributable to pipelines
  // rather than a measured frame time.
  float GpuDuration{};
  std::unordered_map<rtsr::RTSR_ID, PipelineTimeStats> Pipelines;
};

// Everything the pipelines plot and the details tables need for one selection,
// computed in a single pass under one lock.
struct WindowAggregate {
  size_t FrameCount{};
  float CpuDuration{};
  float GpuDuration{};
  std::unordered_map<rtsr::RTSR_ID, PipelineTimeStats> Pipelines;
};

class PerformanceStats {
public:
  // Called on the transport's receive thread.
  void Add(const rtsr::RTSR_REPORT_TIME_STATS_MESSAGE& stats);
  void Clear();

  // Frame plot series. The x values are positions rather than frame numbers, so
  // a gap in measurement shows as a jump in frameNumbers instead of a hole in
  // the plot.
  void ReadFrameSeries(std::vector<float>& positions,
                       std::vector<float>& presentToPresent,
                       std::vector<size_t>& frameNumbers) const;

  // Position of a frame number, so a sticky selection can follow one frame as
  // the window scrolls. Empty once that frame has been evicted.
  std::optional<size_t> IndexOfFrame(size_t frameNumber) const;

  // Aggregates the inclusive position range. Out of range positions are
  // clamped; an empty history yields an empty aggregate.
  WindowAggregate ReadWindow(size_t begin, size_t end) const;

private:
  mutable std::mutex m_Mutex;
  std::deque<FrameSample> m_Frames;
};

} // namespace gits::gui
