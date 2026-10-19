// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "pipelineTracker.h"
#include "rtsr/shared/client/rtsrClient.h"
#include "rtsr/shared/protocol/rtsrWireTypes.h"
#include "taskScheduler.h"
#include "utils/gpuTimer.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class Timer;

namespace gits {

class CGits;

namespace DirectX {

// Measures how long the GPU spends in each pipeline the tracker harvested, and
// reports one message per presented frame to the RTSR server.
//
// A pipeline is measured from the command that binds it until the next binding
// or the end of the command list, which is the granularity RTSREditor plotted
// and the one the performance panel's aggregate expects. Every duration comes
// from GPU timestamps; the only CPU clock used is present to present, which is
// the frame time itself. During playback that clock is the player's own, which
// stops while the stream is paused or being stepped, so navigating frames in
// Studio does not show up as one enormous frame. The wire's Begin fields are
// left zero rather than being
// filled with RTSR's mixture of a CPU submission time and a GPU offset, which
// nothing consumes and which cannot be compared between command lists.
//
// A frame is reported only once Present has closed it and every submission
// attributed to it has been read back behind a fence, so no measurement is ever
// taken from a query the GPU had not finished writing. RTSR instead read its
// buffers four presents later and hoped.
//
// Threading: the hooks run on whichever thread drives the graphics API, the
// enable request arrives on the transport's receive thread, and the readback
// runs on a worker thread. One mutex covers all of it; each hook holds it only
// long enough to record a query into the command list, and the worker never
// holds it while waiting on the GPU. Where the pipeline tracker's lock is also
// needed it is always taken inside this one, never the other way round.
class PerformanceTracker {
public:
  // During playback the frame clock comes from the player, which stops it while
  // paused; in a capture there is nothing to pause and the wall clock is used.
  PerformanceTracker(rtsr::RtsrClient& client,
                     PipelineTracker& pipelines,
                     CGits* gits,
                     bool playback);
  ~PerformanceTracker();

  PerformanceTracker(const PerformanceTracker&) = delete;
  PerformanceTracker& operator=(const PerformanceTracker&) = delete;

  // Starts or stops measuring, answering the server with what actually
  // happened. Runs on the receive thread, so it only flips state; the query
  // heaps are created by the first instrumented command list.
  rtsr::RTSR_RESULT SetEnabled(bool enable);

  // True while any hook has work to do: either measurement is on, or a command
  // list that was recorded while it was on has not finished its round trip.
  // Checked before the lock is taken, because these hooks sit on the most
  // frequent commands in a frame.
  bool IsActive() const {
    return m_Collecting.load(std::memory_order_relaxed) ||
           m_Live.load(std::memory_order_relaxed) != 0;
  }

  // Begins a recording: decides once whether it is instrumented, takes a fresh
  // timer and opens the command list's range. Called both after a reset and
  // after a creation that leaves the list ready to record, either of which can
  // bind a pipeline of its own, hence the initial pipeline key.
  void OnBeginRecording(GITSKey key,
                        GITSKey initialPipelineKey,
                        ID3D12GraphicsCommandList* commandList);

  // Closes the segment of the previously bound pipeline and opens one for this
  // pipeline, sharing a single timestamp between the two.
  void OnSetPipelineState(GITSKey key, GITSKey pipelineKey, ID3D12GraphicsCommandList* commandList);

  // Closes the last segment and the command list's range, then records the
  // resolve. Has to run before the command list is actually closed.
  void OnCommandListClose(GITSKey key, ID3D12GraphicsCommandList* commandList);

  // Attributes the submitted command lists to the frame being recorded, signals
  // a fence behind them and schedules the readback.
  void OnExecuteCommandLists(ID3D12CommandQueue* commandQueue,
                             GITSKey commandQueueKey,
                             const std::vector<GITSKey>& commandListKeys);

  // Closes the frame with its present to present time and opens the next.
  void OnPresent();

  // Forgets a command list or command queue. Recordings still awaiting readback
  // stay alive until the worker is done with them.
  void OnRelease(GITSKey key);

private:
  // Where a pipeline's GPU work begins and ends within one recording. The slots
  // index the ticks the timer reads back.
  struct Segment {
    GITSKey PipelineKey{};
    GpuTimer::Slot Begin{GpuTimer::INVALID_SLOT};
    GpuTimer::Slot End{GpuTimer::INVALID_SLOT};
  };

  // One recording of one command list. Held by shared pointer so a readback
  // still in flight survives the command list being released.
  struct Recording {
    GpuTimer Timer;
    std::vector<Segment> Segments;
    GpuTimer::Slot Begin{GpuTimer::INVALID_SLOT};
    GpuTimer::Slot End{GpuTimer::INVALID_SLOT};
    bool HasOpenSegment{false};
    // Set once the query budget is spent, which stops new segments without
    // losing the ones already recorded.
    bool Exhausted{false};
    // A recording is measured on its first submission only: a second one reuses
    // the same query slots, so its timestamps would overwrite the first's.
    unsigned Submitted{};
    unsigned Executed{};

    bool InFlight() const {
      return Submitted != Executed;
    }
  };

  struct CommandListRecordings {
    // Owns every recording ever made for this command list. One is reused as
    // soon as its submission has been read back.
    std::vector<std::shared_ptr<Recording>> Pool;
    // Being recorded now, between the reset and the close.
    std::shared_ptr<Recording> Open;
    // Closed and waiting to be submitted. Dropped when the list is reset,
    // because the recording it describes no longer exists.
    std::shared_ptr<Recording> Closed;
  };

  struct QueueFence {
    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    uint64_t Value{};
  };

  struct Frame {
    size_t Number{};
    size_t DurationNs{};
    bool Closed{false};
    // Submissions attributed to this frame that have not been read back.
    size_t Pending{};
    std::vector<rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> CommandBuffers;
  };

  // A command list with more pipeline bindings than this stops being measured
  // partway. Reaching it means the stream is far outside what shader editing is
  // used for, and an unbounded budget would let one recording claim query heaps
  // without limit.
  static constexpr size_t MAX_SEGMENTS = 4096;
  // Concurrent recordings of the same command list. Each needs its own query
  // slots, and needing more than this means submissions are not retiring.
  static constexpr size_t MAX_RECORDINGS_PER_COMMAND_LIST = 8;
  // Frames waiting to be completed. The oldest is dropped beyond this, which
  // bounds memory when a submission's fence never signals.
  static constexpr size_t MAX_PENDING_FRAMES = 64;
  // How long a readback waits for the GPU before giving up on a frame. Only
  // reached when the queue has stalled or the process is being torn down, and it
  // is what keeps the worker from blocking shutdown indefinitely.
  static constexpr uint32_t READBACK_TIMEOUT_MS = 5000;

  // Releases this command list's hold on its open and closed recordings. They
  // stay in the pool, and in flight ones stay alive through the worker's own
  // reference. Called under the lock.
  void Drop(CommandListRecordings& recordings);

  // Picks a reusable recording out of the pool, or adds one. Null when the
  // budget is spent. Called under the lock.
  std::shared_ptr<Recording> Acquire(CommandListRecordings& recordings);

  // Turns one read back recording into its wire form. Runs on the worker thread
  // without the lock, which is safe because a recording in flight can be neither
  // reused nor reset.
  rtsr::RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO Summarize(const Recording& recording,
                                                                    uint64_t frequency) const;

  // Waits for the submission, reads the timestamps back and files them under the
  // frame they belong to. Runs on the worker thread.
  void ReadBack(Microsoft::WRL::ComPtr<ID3D12Fence> fence,
                uint64_t fenceValue,
                uint64_t frequency,
                size_t frameNumber,
                std::vector<std::shared_ptr<Recording>> recordings);

  // Moves out every frame that is closed and has nothing outstanding, oldest
  // first, so the panel receives them in order. Called under the lock.
  void TakeCompleteFrames(std::vector<Frame>& complete);

  // Sends and empties the list. Called without the lock, because sending takes
  // the transport's queue mutex.
  void Send(std::vector<Frame>& frames);

  // The frame clock, in nanoseconds. Must only be called from the thread driving
  // the graphics API: during playback it reads the player's Timer, which that
  // same thread pauses and resumes and which carries no synchronisation.
  int64_t NowNs() const;

  rtsr::RtsrClient& m_Client;
  PipelineTracker& m_Pipelines;

  // The player's pause-aware clock, or null in a capture, where the wall clock
  // measured from this origin is used instead. Chosen once at construction.
  const Timer* m_PlaybackClock{};
  std::chrono::steady_clock::time_point m_WallClockOrigin;

  // Read without the lock by every hook, so measurement being off costs two
  // relaxed loads on the hot commands.
  std::atomic<bool> m_Collecting{false};
  // Instrumented recordings a command list still holds. Keeps the hooks alive
  // after measurement is switched off, so a list already carrying queries still
  // gets its resolve.
  std::atomic<size_t> m_Live{0};

  mutable std::mutex m_Mutex;

  std::unordered_map<GITSKey, CommandListRecordings> m_CommandLists;
  std::unordered_map<GITSKey, QueueFence> m_Queues;

  // Oldest first. The open frame, when there is one, is the last.
  std::deque<Frame> m_Frames;
  bool m_FrameOpen{false};
  // Never reset, so frame numbers only ever increase and a gap in the panel's
  // plot reads as a pause in measurement rather than a restart.
  size_t m_FrameNumber{0};
  int64_t m_LastPresentNs{};
  // Set when measurement is switched on, because the clock cannot be read from
  // the receive thread that asked for it. The next present takes the baseline.
  bool m_NeedBaseline{false};

  // Warnings that would otherwise repeat once per frame.
  bool m_ReportedSegmentLimit{false};
  bool m_ReportedRecordingLimit{false};
  bool m_ReportedFrameLimit{false};
  bool m_ReportedResubmission{false};

  // Set while the tracker is being destroyed so queued readbacks return at once
  // instead of waiting on a GPU that may never get there. Declared before the
  // scheduler, whose destructor runs first and drains those readbacks while the
  // rest of this object is still intact.
  std::atomic<bool> m_Draining{false};
  TaskScheduler m_Scheduler;
};

} // namespace DirectX
} // namespace gits
