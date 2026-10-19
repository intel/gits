// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "ipcPipe.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace gits {

class RemotePlaybackController {
public:
  enum class PlayerState {
    Running,
    Paused,
    Finished
  };

  enum class Mode {
    Paused,
    PlayOnce,
    Loop,
    StepCount,
    PlayUntilFrame
  };

  RemotePlaybackController(bool loopConfigured,
                           bool interactiveStart,
                           std::function<void(PlayerState)> onPlayerStateChanged,
                           std::function<bool()> isPlayerAborted);
  ~RemotePlaybackController();

  void Start();
  void StartLoop();
  void Pause();
  void Stop();
  void Step(uint64_t frames);
  void PlayUntilFrame(uint64_t frame);

  // Returns true when playback should pause after the just-played frame.
  bool ShouldPauseAfterFrame(uint64_t frame);

  // Returns true when playback pass should be replayed immediately.
  bool ReplayImmediatelyAfterPass();

  // Blocks while paused, returns false if abort condition is met.
  bool WaitForReplayRequest();

  void SendFrame(uint64_t frame);
  void SendFramePlayed(uint64_t frame);
  void NotifyPlaybackDone();
  void ProcessPendingCommands();
  bool HasRemoteControl() const;
  void UseLocalControl();
  void ReportState(PlayerState state);

private:
  void processIpcCommand(const std::string& command);
  bool sendIpcStatus(const std::string& status);

  bool _interactiveStart;
  bool _loopConfigured;
  std::function<void(PlayerState)> _onPlayerStateChanged;
  std::function<bool()> _isPlayerAborted;

  common::IPCManager _ipcManager;
  std::thread _ipcCommandConnectThread;
  std::thread _ipcStatusConnectThread;
  std::atomic<bool> _ipcConnected{false};
  std::atomic<bool> _ipcStatusConnected{false};
  std::atomic<bool> _ipcRunning{false};
  std::atomic<bool> _playbackDoneSent{false};

  Mode _mode;
  uint64_t _remainingFrames;
  uint64_t _targetFrame;
  bool _replayRequested;
  bool _remoteControl = false;
  bool _stopped = false;
  uint64_t _lastFrame = 0;
  PlayerState _reportedState = PlayerState::Paused;
  std::deque<std::string> _commands;

  mutable std::mutex _mutex;
  std::condition_variable _cv;
};

} // namespace gits
