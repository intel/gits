// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace gits::common {
class IPCManager;
}

namespace gits::gui {

class Controller {
public:
  enum class PlaybackState {
    Disconnected,
    Paused,
    Starting,
    Running,
    Done
  };

  explicit Controller(gits::common::IPCManager& ipcManager);
  ~Controller();

  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;

  void Start();
  void Stop();

  bool SendStart();
  bool SendStartLoop();
  bool SendStop();
  bool SendPause();
  bool SendPlayUntil(uint64_t frame);
  bool SendAbort();
  bool SendStep();
  bool SendStep(uint64_t frames);
  bool SendCommand(const std::string& command);
  bool QueueCommand(const std::string& command);
  bool IsCommandPending() const;
  bool CommandFailed() const;
  void SetFrameCallback(std::function<void(uint64_t)> callback);
  void SetPlaybackDoneCallback(std::function<void()> callback);
  void ResetPlaybackDone();

  bool IsConnected() const;
  bool IsPlaybackDone() const;
  PlaybackState GetPlaybackState() const;
  uint64_t GetLastFrame() const;
  double GetTimeSinceLastFrame() const;

private:
  void ConnectLoop();
  void StatusLoop();
  bool TryConnectCommandPipe();
  bool TryConnectStatusPipe();
  bool WaitForCommandConnection(std::chrono::milliseconds timeout);
  void HandleStatusMessage(const std::string& status);

  gits::common::IPCManager& m_IPCManager;
  std::thread m_ConnectionThread;
  std::thread m_StatusThread;
  std::atomic<bool> m_Running{false};
  std::atomic<bool> m_Connected{false};
  std::atomic<bool> m_StatusConnected{false};
  std::atomic<bool> m_PlaybackDone{false};
  std::atomic<PlaybackState> m_PlaybackState{PlaybackState::Disconnected};
  std::atomic<uint64_t> m_LastFrame{0};
  std::atomic<std::chrono::steady_clock::time_point::duration> m_LastFrameTime{};
  std::function<void(uint64_t)> m_FrameCallback;
  std::function<void()> m_PlaybackDoneCallback;
  mutable std::mutex m_CallbackMutex;
  mutable std::mutex m_SendMutex;
  mutable std::mutex m_QueueMutex;
  std::string m_QueuedCommand;
  std::atomic<bool> m_CommandPending{false};
  std::atomic<bool> m_CommandFailed{false};
  std::atomic<bool> m_HasStatus{false};
};

} // namespace gits::gui
