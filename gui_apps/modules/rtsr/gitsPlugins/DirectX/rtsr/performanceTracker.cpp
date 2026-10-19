// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "performanceTracker.h"

#include "gits.h"
#include "log.h"
#include "timer.h"

#include <windows.h>

#include <algorithm>
#include <utility>

namespace gits {
namespace DirectX {

namespace {

// Bundles cannot carry queries at all, and timestamp support on copy queues is
// optional, so only the two types RTSR measured are instrumented.
bool IsQueryable(D3D12_COMMAND_LIST_TYPE type) {
  return type == D3D12_COMMAND_LIST_TYPE_DIRECT || type == D3D12_COMMAND_LIST_TYPE_COMPUTE;
}

// Blocks until the queue has passed the value, or gives up. Returns whether the
// GPU actually got there, which is what decides if the timestamps behind it may
// be read.
bool AwaitFence(ID3D12Fence* fence, uint64_t value, uint32_t timeoutMs) {
  if (fence == nullptr) {
    return false;
  }
  if (fence->GetCompletedValue() >= value) {
    return true;
  }

  HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (event == nullptr) {
    return false;
  }

  bool signalled = false;
  if (SUCCEEDED(fence->SetEventOnCompletion(value, event))) {
    signalled = WaitForSingleObject(event, timeoutMs) == WAIT_OBJECT_0;
  }
  CloseHandle(event);

  return signalled || fence->GetCompletedValue() >= value;
}

// Whether both ends of a pair were recorded and read back, which is what
// separates something that could not be measured from something that measured
// zero. A segment that ran out of query slots has to be left out rather than
// reported as having cost nothing.
bool IsMeasured(const std::vector<GpuTimer::Tick>& ticks,
                GpuTimer::Slot begin,
                GpuTimer::Slot end) {
  return begin != GpuTimer::INVALID_SLOT && end != GpuTimer::INVALID_SLOT && begin < ticks.size() &&
         end < ticks.size() && ticks[begin] <= ticks[end];
}

// Nanoseconds between two recorded timestamps. Computed in floating point
// because a tick difference scaled by a billion overflows 64 bits for long
// enough command lists.
size_t ElapsedNs(const std::vector<GpuTimer::Tick>& ticks,
                 GpuTimer::Slot begin,
                 GpuTimer::Slot end,
                 uint64_t frequency) {
  if (frequency == 0) {
    return 0;
  }
  const double elapsed = static_cast<double>(ticks[end] - ticks[begin]);
  return static_cast<size_t>(elapsed * 1.0e9 / static_cast<double>(frequency));
}

} // namespace

PerformanceTracker::PerformanceTracker(rtsr::RtsrClient& client,
                                       PipelineTracker& pipelines,
                                       CGits* gits,
                                       bool playback)
    : m_Client(client),
      m_Pipelines(pipelines),
      // The player pauses and resumes this timer around every pause, step and
      // frame navigation, so reading it instead of the wall clock is what keeps
      // that time out of the frame durations. It is never started in a capture,
      // where the wall clock is correct anyway because nothing can pause.
      m_PlaybackClock(playback && gits != nullptr ? &gits->Timers().playback : nullptr),
      m_WallClockOrigin(std::chrono::steady_clock::now()),
      m_Scheduler("rtsr-readback") {}

PerformanceTracker::~PerformanceTracker() {
  m_Collecting.store(false);
  // The scheduler's destructor runs whatever is still queued before joining its
  // thread. This makes each of those readbacks return immediately instead of
  // waiting on a queue that may already have been abandoned.
  m_Draining.store(true);
}

// **********************************************************************************************
// * Measurement state                                                                          *
// **********************************************************************************************

rtsr::RTSR_RESULT PerformanceTracker::SetEnabled(bool enable) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  if (enable == m_Collecting.load()) {
    return rtsr::RTSR_RESULT_SUCCESS;
  }

  if (enable) {
    // No frame is opened and no clock is read here. This runs on the receive
    // thread, and the frame clock belongs to the thread driving the graphics
    // API, which is also the one that pauses it. The next present takes the
    // baseline, so measurement starts at a frame boundary and the first bar is
    // a real frame rather than however long the user took to click.
    m_NeedBaseline = true;
    m_Collecting.store(true);
    LOG_INFO << "Rtsr - the server asked for pipeline timings, measuring from the next frame";
  } else {
    m_Collecting.store(false);
    m_NeedBaseline = false;
    // The open frame was never presented, so it has no present to present time
    // and is dropped. Frames already closed still flush as their readbacks land.
    if (m_FrameOpen && !m_Frames.empty()) {
      m_Frames.pop_back();
      m_FrameOpen = false;
    }
    LOG_INFO << "Rtsr - the server asked to stop measuring pipeline timings";
  }

  return rtsr::RTSR_RESULT_SUCCESS;
}

// **********************************************************************************************
// * Recording                                                                                  *
// **********************************************************************************************

void PerformanceTracker::Drop(CommandListRecordings& recordings) {
  if (recordings.Open) {
    recordings.Open.reset();
    m_Live.fetch_sub(1, std::memory_order_relaxed);
  }
  if (recordings.Closed) {
    recordings.Closed.reset();
    m_Live.fetch_sub(1, std::memory_order_relaxed);
  }
}

std::shared_ptr<PerformanceTracker::Recording> PerformanceTracker::Acquire(
    CommandListRecordings& recordings) {
  for (const auto& recording : recordings.Pool) {
    if (!recording->InFlight()) {
      return recording;
    }
  }

  if (recordings.Pool.size() >= MAX_RECORDINGS_PER_COMMAND_LIST) {
    if (!m_ReportedRecordingLimit) {
      m_ReportedRecordingLimit = true;
      LOG_WARNING << "Rtsr - a command list is being re-recorded faster than its submissions "
                     "retire; some timings will be missing";
    }
    return nullptr;
  }

  recordings.Pool.push_back(std::make_shared<Recording>());
  return recordings.Pool.back();
}

void PerformanceTracker::OnBeginRecording(GITSKey key,
                                          GITSKey initialPipelineKey,
                                          ID3D12GraphicsCommandList* commandList) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  // Whatever this command list held describes contents that no longer exist.
  if (const auto existing = m_CommandLists.find(key); existing != m_CommandLists.end()) {
    Drop(existing->second);
  }

  // Sampled once per recording. A command list therefore either carries queries
  // from its first command to its close or carries none at all, which is what
  // keeps a resolve from being recorded without the timestamps it resolves.
  if (!m_Collecting.load(std::memory_order_relaxed) || commandList == nullptr ||
      !IsQueryable(commandList->GetType())) {
    return;
  }

  auto& recordings = m_CommandLists[key];
  auto recording = Acquire(recordings);
  if (!recording) {
    return;
  }

  recording->Timer.Reset();
  recording->Segments.clear();
  recording->End = GpuTimer::INVALID_SLOT;
  recording->HasOpenSegment = false;
  recording->Exhausted = false;
  recording->Submitted = 0;
  recording->Executed = 0;

  recording->Begin = recording->Timer.InsertTimestamp(commandList);
  if (recording->Begin == GpuTimer::INVALID_SLOT) {
    // Without an opening timestamp there is no command list range to report, so
    // the recording is left out entirely rather than half measured.
    return;
  }

  recordings.Open = std::move(recording);
  m_Live.fetch_add(1, std::memory_order_relaxed);

  // A reset or a creation can bind a pipeline itself, and everything drawn until
  // the next binding belongs to it. The command list's opening timestamp doubles
  // as that segment's start, so this costs no extra query. An untracked or
  // absent pipeline has a key the tracker does not know and opens nothing.
  Recording& open = *recordings.Open;
  if (m_Pipelines.IsTracked(initialPipelineKey)) {
    open.Segments.push_back(Segment{initialPipelineKey, open.Begin, GpuTimer::INVALID_SLOT});
    open.HasOpenSegment = true;
  }
}

void PerformanceTracker::OnSetPipelineState(GITSKey key,
                                            GITSKey pipelineKey,
                                            ID3D12GraphicsCommandList* commandList) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  const auto it = m_CommandLists.find(key);
  if (it == m_CommandLists.end() || !it->second.Open) {
    return;
  }
  Recording& recording = *it->second.Open;

  if (!recording.Exhausted && recording.Segments.size() >= MAX_SEGMENTS) {
    recording.Exhausted = true;
    if (!m_ReportedSegmentLimit) {
      m_ReportedSegmentLimit = true;
      LOG_WARNING << "Rtsr - a command list binds more than " << MAX_SEGMENTS
                  << " pipelines; the rest of it is not measured";
    }
  }

  // Only pipelines the server was told about are worth a segment: the panel has
  // no name and no shader to show for any other.
  const bool opensSegment = !recording.Exhausted && m_Pipelines.IsTracked(pipelineKey);
  if (!recording.HasOpenSegment && !opensSegment) {
    return;
  }

  // One timestamp serves as both the end of the previous pipeline's segment and
  // the start of this one, which is half the queries a pair would need.
  const GpuTimer::Slot slot = recording.Timer.InsertTimestamp(commandList);

  if (recording.HasOpenSegment) {
    recording.Segments.back().End = slot;
    recording.HasOpenSegment = false;
  }

  if (slot == GpuTimer::INVALID_SLOT) {
    recording.Exhausted = true;
    return;
  }

  if (opensSegment) {
    recording.Segments.push_back(Segment{pipelineKey, slot, GpuTimer::INVALID_SLOT});
    recording.HasOpenSegment = true;
  }
}

void PerformanceTracker::OnCommandListClose(GITSKey key, ID3D12GraphicsCommandList* commandList) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  const auto it = m_CommandLists.find(key);
  if (it == m_CommandLists.end() || !it->second.Open) {
    return;
  }
  CommandListRecordings& recordings = it->second;
  Recording& recording = *recordings.Open;

  const GpuTimer::Slot slot = recording.Timer.InsertTimestamp(commandList);
  if (recording.HasOpenSegment) {
    recording.Segments.back().End = slot;
    recording.HasOpenSegment = false;
  }
  recording.End = slot;

  // Has to be the last thing recorded: the resolve copies whatever the
  // timestamps above will hold once the GPU reaches them.
  recording.Timer.InsertResolve(commandList);

  if (recordings.Closed) {
    recordings.Closed.reset();
    m_Live.fetch_sub(1, std::memory_order_relaxed);
  }
  // The same recording moves from one slot to the other, so the live count is
  // unchanged.
  recordings.Closed = std::move(recordings.Open);
}

// **********************************************************************************************
// * Submission and readback                                                                    *
// **********************************************************************************************

void PerformanceTracker::OnExecuteCommandLists(ID3D12CommandQueue* commandQueue,
                                               GITSKey commandQueueKey,
                                               const std::vector<GITSKey>& commandListKeys) {
  if (commandQueue == nullptr) {
    return;
  }

  std::vector<std::shared_ptr<Recording>> recordings;
  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  uint64_t fenceValue = 0;
  uint64_t frequency = 0;
  size_t frameNumber = 0;

  {
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (const GITSKey key : commandListKeys) {
      const auto it = m_CommandLists.find(key);
      if (it == m_CommandLists.end() || !it->second.Closed) {
        continue;
      }
      const auto& recording = it->second.Closed;
      if (recording->Segments.empty()) {
        continue;
      }
      if (recording->Submitted != 0) {
        if (!m_ReportedResubmission) {
          m_ReportedResubmission = true;
          LOG_WARNING << "Rtsr - a command list is submitted more than once per recording; only "
                         "its first execution is measured";
        }
        continue;
      }
      ++recording->Submitted;
      recordings.push_back(recording);
    }

    if (recordings.empty()) {
      return;
    }

    // Undoes the submission marks on every path that cannot go through with the
    // readback, so those recordings stay reusable instead of looking in flight
    // forever.
    const auto abandon = [&recordings]() {
      for (const auto& recording : recordings) {
        --recording->Submitted;
      }
    };

    if (FAILED(commandQueue->GetTimestampFrequency(&frequency)) || frequency == 0) {
      abandon();
      return;
    }

    auto& queue = m_Queues[commandQueueKey];
    if (!queue.Fence) {
      Microsoft::WRL::ComPtr<ID3D12Device> device;
      if (FAILED(commandQueue->GetDevice(IID_PPV_ARGS(&device))) ||
          FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue.Fence)))) {
        LOG_WARNING << "Rtsr - could not create a fence for a command queue, its command lists "
                       "will not be measured";
        m_Queues.erase(commandQueueKey);
        abandon();
        return;
      }
    }

    // Signalled after the application's own submission on the same queue, so
    // reaching this value means every timestamp above it has been written.
    fence = queue.Fence;
    fenceValue = ++queue.Value;
    if (FAILED(commandQueue->Signal(fence.Get(), fenceValue))) {
      abandon();
      return;
    }

    // The frame being recorded owns this submission. Zero means measurement was
    // switched off between the recording and the submission: the readback still
    // runs, so the recordings become reusable, but its results are discarded.
    if (m_FrameOpen && !m_Frames.empty()) {
      Frame& frame = m_Frames.back();
      frameNumber = frame.Number;
      frame.Pending += recordings.size();
    }
  }

  m_Scheduler.Schedule([this, fence, fenceValue, frequency, frameNumber,
                        recordings = std::move(recordings)]() mutable {
    ReadBack(std::move(fence), fenceValue, frequency, frameNumber, std::move(recordings));
  });
}

rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO PerformanceTracker::Summarize(
    const Recording& recording, uint64_t frequency) const {
  rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO info;

  const std::vector<GpuTimer::Tick> ticks = recording.Timer.Gather();
  if (ticks.empty()) {
    return info;
  }

  if (IsMeasured(ticks, recording.Begin, recording.End)) {
    info.Duration = ElapsedNs(ticks, recording.Begin, recording.End, frequency);
  }

  info.PipelineStats.reserve(recording.Segments.size());
  for (const Segment& segment : recording.Segments) {
    if (!IsMeasured(ticks, segment.Begin, segment.End)) {
      continue;
    }
    rtsr::RTSR_PIPELINE_TIME_STATS_INFO pipeline;
    pipeline.Id = segment.PipelineKey;
    pipeline.Duration = ElapsedNs(ticks, segment.Begin, segment.End, frequency);
    info.PipelineStats.push_back(pipeline);
  }

  return info;
}

void PerformanceTracker::ReadBack(Microsoft::WRL::ComPtr<ID3D12Fence> fence,
                                  uint64_t fenceValue,
                                  uint64_t frequency,
                                  size_t frameNumber,
                                  std::vector<std::shared_ptr<Recording>> recordings) {
  std::vector<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> commandBuffers;

  // The wait is not skipped when the frame is gone: a recording only becomes
  // reusable once the GPU is done with it, whether or not anyone wants its
  // numbers. The one exception is teardown, where the queue may never get there.
  const bool finished = !m_Draining.load(std::memory_order_relaxed) &&
                        AwaitFence(fence.Get(), fenceValue, READBACK_TIMEOUT_MS);

  if (finished && frameNumber != 0) {
    for (const auto& recording : recordings) {
      auto info = Summarize(*recording, frequency);
      if (!info.PipelineStats.empty()) {
        commandBuffers.push_back(std::move(info));
      }
    }
  }

  std::vector<Frame> complete;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Marks the recordings reusable whether or not anything was read, so a
    // dropped frame or a stalled queue does not retire a command list's timers.
    for (const auto& recording : recordings) {
      ++recording->Executed;
    }

    for (Frame& frame : m_Frames) {
      if (frame.Number != frameNumber) {
        continue;
      }
      frame.Pending -= std::min(frame.Pending, recordings.size());
      for (auto& info : commandBuffers) {
        frame.CommandBuffers.push_back(std::move(info));
      }
      break;
    }

    TakeCompleteFrames(complete);
  }

  Send(complete);
}

// **********************************************************************************************
// * Frames                                                                                     *
// **********************************************************************************************

int64_t PerformanceTracker::NowNs() const {
  if (m_PlaybackClock != nullptr) {
    return m_PlaybackClock->Get();
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() -
                                                              m_WallClockOrigin)
      .count();
}

void PerformanceTracker::OnPresent() {
  std::vector<Frame> complete;

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_Collecting.load(std::memory_order_relaxed)) {
      return;
    }

    const int64_t now = NowNs();

    if (m_NeedBaseline) {
      // Measurement was just switched on. This present is the start of the
      // first measured frame, not the end of one, so nothing is closed here.
      m_NeedBaseline = false;
      m_LastPresentNs = now;
      m_Frames.push_back(Frame{++m_FrameNumber});
      m_FrameOpen = true;
      return;
    }

    const int64_t elapsed = now - m_LastPresentNs;
    m_LastPresentNs = now;

    if (m_FrameOpen && !m_Frames.empty()) {
      Frame& frame = m_Frames.back();
      frame.Closed = true;
      // Never negative: both readings come from the same monotonic clock on this
      // thread, and a paused Timer only ever holds its value.
      frame.DurationNs = static_cast<size_t>(std::max<int64_t>(elapsed, 0));
    }

    // A frame with no measured pipeline is still reported: its duration is the
    // frame time, and leaving it out would show as a hole in the plot rather
    // than as a frame in which nothing tracked was drawn.
    m_Frames.push_back(Frame{++m_FrameNumber});
    m_FrameOpen = true;

    while (m_Frames.size() > MAX_PENDING_FRAMES) {
      if (!m_ReportedFrameLimit) {
        m_ReportedFrameLimit = true;
        LOG_WARNING << "Rtsr - GPU timings are not coming back fast enough; the oldest measured "
                       "frames are being dropped";
      }
      m_Frames.pop_front();
    }

    TakeCompleteFrames(complete);
  }

  Send(complete);
}

void PerformanceTracker::TakeCompleteFrames(std::vector<Frame>& complete) {
  // Oldest first, stopping at the first frame that is still waiting, so the
  // panel never receives frames out of order.
  while (!m_Frames.empty()) {
    Frame& frame = m_Frames.front();
    if (!frame.Closed || frame.Pending != 0) {
      break;
    }
    complete.push_back(std::move(frame));
    m_Frames.pop_front();
  }
}

void PerformanceTracker::Send(std::vector<Frame>& frames) {
  for (Frame& frame : frames) {
    rtsr::RTSR_REPORT_TIME_STATS_MESSAGE message;
    message.FrameNumber = frame.Number;
    message.Duration = frame.DurationNs;
    message.CommandBufferStats = std::move(frame.CommandBuffers);
    // Begin stays zero, here and in every nested entry. See the class comment.
    m_Client.ReportTimeStats(std::move(message));
  }
  frames.clear();
}

// **********************************************************************************************
// * Lifetime                                                                                   *
// **********************************************************************************************

void PerformanceTracker::OnRelease(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  const auto commandList = m_CommandLists.find(key);
  if (commandList != m_CommandLists.end()) {
    Drop(commandList->second);
    // Recordings still awaiting readback survive: the worker holds its own
    // reference to each one it was given.
    m_CommandLists.erase(commandList);
  }

  m_Queues.erase(key);
}

} // namespace DirectX
} // namespace gits
