// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrPerformanceStats.h"

#include <algorithm>
#include <limits>

namespace gits::gui {

namespace {

// The wire is in nanoseconds.
constexpr float TO_MILLISECONDS = 1.0e-6f;

float Milliseconds(size_t nanoseconds) {
  return static_cast<float>(nanoseconds) * TO_MILLISECONDS;
}

} // namespace

float PipelineTimeStats::Get(Field field) const {
  switch (field) {
  case Field::TotalDuration:
    return TotalDuration;
  case Field::MinDuration:
    return MinDuration;
  case Field::MaxDuration:
    return MaxDuration;
  case Field::Occurrences:
    return static_cast<float>(Occurrences);
  }
  return 0.0f;
}

void PerformanceStats::Add(const rtsr::RTSR_REPORT_TIME_STATS_MESSAGE& stats) {
  FrameSample sample;
  sample.FrameNumber = stats.FrameNumber;
  sample.PresentToPresent = Milliseconds(stats.Duration);

  // Every individual duration per pipeline, so the min, max and mean within
  // this frame are exact rather than derived from the running total.
  std::unordered_map<rtsr::RTSR_ID, std::vector<float>> durations;

  for (const auto& commandBuffer : stats.CommandBufferStats) {
    for (const auto& pipeline : commandBuffer.PipelineStats) {
      const float duration = Milliseconds(pipeline.Duration);

      auto& entry = sample.Pipelines[pipeline.Id];
      ++entry.Occurrences;
      entry.TotalDuration += duration;

      durations[pipeline.Id].push_back(duration);
      sample.GpuDuration += duration;
    }
  }

  for (auto& [id, entry] : sample.Pipelines) {
    const auto& samples = durations[id];
    if (!samples.empty()) {
      const auto bounds = std::minmax_element(samples.begin(), samples.end());
      entry.MinDuration = *bounds.first;
      entry.MaxDuration = *bounds.second;
      entry.AverageDuration = entry.TotalDuration / static_cast<float>(samples.size());
    }
    entry.ShareOfFrame =
        sample.GpuDuration > 0.0f ? 100.0f * entry.TotalDuration / sample.GpuDuration : 0.0f;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Frames.push_back(std::move(sample));
  // Eviction is a pop, because a frame owns its own per-pipeline stats. RTSR
  // additionally maintained a pipeline-to-frames index and running totals that
  // had to be unwound entry by entry on every eviction; nothing here reads
  // those, so they are not kept.
  while (m_Frames.size() > FRAME_HISTORY) {
    m_Frames.pop_front();
  }
}

void PerformanceStats::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Frames.clear();
}

void PerformanceStats::ReadFrameSeries(std::vector<float>& positions,
                                       std::vector<float>& presentToPresent,
                                       std::vector<size_t>& frameNumbers) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  positions.clear();
  presentToPresent.clear();
  frameNumbers.clear();
  positions.reserve(m_Frames.size());
  presentToPresent.reserve(m_Frames.size());
  frameNumbers.reserve(m_Frames.size());

  for (size_t i = 0; i < m_Frames.size(); ++i) {
    positions.push_back(static_cast<float>(i));
    presentToPresent.push_back(m_Frames[i].PresentToPresent);
    frameNumbers.push_back(m_Frames[i].FrameNumber);
  }
}

std::optional<size_t> PerformanceStats::IndexOfFrame(size_t frameNumber) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (size_t i = 0; i < m_Frames.size(); ++i) {
    if (m_Frames[i].FrameNumber == frameNumber) {
      return i;
    }
  }
  return std::nullopt;
}

WindowAggregate PerformanceStats::ReadWindow(size_t begin, size_t end) const {
  WindowAggregate aggregate;

  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_Frames.empty() || begin >= m_Frames.size()) {
    return aggregate;
  }
  end = std::min(end, m_Frames.size() - 1);
  if (end < begin) {
    return aggregate;
  }

  // Per-frame averages are summed here and divided once at the end, so the
  // result is the mean of the per-frame means.
  std::unordered_map<rtsr::RTSR_ID, float> summedAverages;

  for (size_t i = begin; i <= end; ++i) {
    const auto& frame = m_Frames[i];
    ++aggregate.FrameCount;
    aggregate.CpuDuration += frame.PresentToPresent;
    aggregate.GpuDuration += frame.GpuDuration;

    for (const auto& [id, frameEntry] : frame.Pipelines) {
      auto [it, inserted] = aggregate.Pipelines.try_emplace(id);
      auto& entry = it->second;
      if (inserted) {
        entry.MinDuration = std::numeric_limits<float>::max();
        entry.MaxDuration = std::numeric_limits<float>::lowest();
      }
      entry.Occurrences += frameEntry.Occurrences;
      entry.TotalDuration += frameEntry.TotalDuration;
      // True extremes across the selection. RTSR divided these by the frame
      // count, which turns a minimum into neither a minimum nor a mean.
      entry.MinDuration = std::min(entry.MinDuration, frameEntry.MinDuration);
      entry.MaxDuration = std::max(entry.MaxDuration, frameEntry.MaxDuration);
      summedAverages[id] += frameEntry.AverageDuration;
    }
  }

  const auto frameCount = static_cast<float>(aggregate.FrameCount);
  for (auto& [id, entry] : aggregate.Pipelines) {
    entry.AverageDuration = frameCount > 0.0f ? summedAverages[id] / frameCount : 0.0f;
    entry.ShareOfFrame =
        aggregate.GpuDuration > 0.0f ? 100.0f * entry.TotalDuration / aggregate.GpuDuration : 0.0f;
  }

  return aggregate;
}

} // namespace gits::gui
