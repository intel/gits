// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "remotePlaybackController.h"

#include "log.h"

#include <chrono>
#include <charconv>
#include <string_view>

namespace gits {

RemotePlaybackController::RemotePlaybackController(
    bool loopConfigured,
    bool interactiveStart,
    std::function<void(PlayerState)> onPlayerStateChanged,
    std::function<bool()> isPlayerAborted)
    : _interactiveStart(interactiveStart),
      _loopConfigured(loopConfigured),
      _onPlayerStateChanged(std::move(onPlayerStateChanged)),
      _isPlayerAborted(std::move(isPlayerAborted)),
      _ipcManager("gits_player_ipc", true),
      _mode(Mode::Paused),
      _remainingFrames(0),
      _targetFrame(0),
      _replayRequested(false) {
  if (!interactiveStart && _loopConfigured) {
    _mode = Mode::Loop;
    _replayRequested = true;
  }

  _ipcRunning = _interactiveStart;

  if (!_interactiveStart) {
    return;
  }

  _ipcCommandConnectThread = std::thread([this] {
    auto& pipe = *_ipcManager.m_commandPipe;
    while (_ipcRunning) {
      if (!_ipcConnected) {
        if ((!pipe.isValid() && !pipe.create()) || !pipe.connect()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
        _ipcConnected = true;
      }
      std::string command;
      if (pipe.read(command, 100)) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (command == "stop" || command == "abort") {
          _commands.clear();
          _commands.push_back(command);
        } else if (_commands.size() < 64) {
          _commands.push_back(command);
        }
      } else if (!pipe.isConnected()) {
        _ipcConnected = false;
        pipe.close();
      }
    }
  });

  _ipcStatusConnectThread = std::thread([this] {
    while (_ipcRunning) {
      if (_ipcStatusConnected) {
        if (!_ipcManager.m_statusPipe || !_ipcManager.m_statusPipe->isConnected()) {
          _ipcStatusConnected = false;
          if (_ipcManager.m_statusPipe) {
            _ipcManager.m_statusPipe->close();
          }
        }
      }

      if (!_ipcStatusConnected && _ipcManager.m_statusPipe->connect()) {
        _ipcStatusConnected = true;
        LOG_INFO << "IPC server status pipe connected";
      }
      if (_ipcStatusConnected) {
        std::string status;
        {
          std::lock_guard<std::mutex> lock(_mutex);
          const char* state = _reportedState == PlayerState::Running  ? "running"
                              : _reportedState == PlayerState::Paused ? "paused"
                                                                      : "done";
          status = std::string("state:") + state + ":" + std::to_string(_lastFrame);
        }
        sendIpcStatus(status);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
}

RemotePlaybackController::~RemotePlaybackController() {
  _ipcRunning = false;

  if (_ipcManager.m_commandPipe) {
    _ipcManager.m_commandPipe->close();
  }
  if (_ipcManager.m_statusPipe) {
    _ipcManager.m_statusPipe->close();
  }

  if (_ipcCommandConnectThread.joinable()) {
    _ipcCommandConnectThread.join();
  }
  if (_ipcStatusConnectThread.joinable()) {
    _ipcStatusConnectThread.join();
  }
}

void RemotePlaybackController::Start() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _mode = Mode::PlayOnce;
    _remainingFrames = 0;
    _targetFrame = 0;
    _replayRequested = true;
    _playbackDoneSent = false;
  }
  _onPlayerStateChanged(PlayerState::Running);
  _cv.notify_all();
}

void RemotePlaybackController::StartLoop() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _mode = _loopConfigured ? Mode::Loop : Mode::PlayOnce;
    _remainingFrames = 0;
    _targetFrame = 0;
    _replayRequested = true;
    _playbackDoneSent = false;
  }
  _onPlayerStateChanged(PlayerState::Running);
  _cv.notify_all();
}

void RemotePlaybackController::Pause() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _mode = Mode::Paused;
    _remainingFrames = 0;
    _targetFrame = 0;
    _replayRequested = false;
  }
  _onPlayerStateChanged(PlayerState::Paused);
}

void RemotePlaybackController::Stop() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _stopped = true;
    _mode = Mode::Paused;
    _replayRequested = false;
    _reportedState = PlayerState::Finished;
  }
  _onPlayerStateChanged(PlayerState::Finished);
  _cv.notify_all();
}

void RemotePlaybackController::Step(uint64_t frames) {
  if (frames == 0) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _mode = Mode::StepCount;
    _remainingFrames = frames;
    _targetFrame = 0;
    _replayRequested = true;
    _playbackDoneSent = false;
  }
  _onPlayerStateChanged(PlayerState::Running);
  _cv.notify_all();
}

void RemotePlaybackController::PlayUntilFrame(uint64_t frame) {
  if (frame <= _lastFrame) {
    Pause();
    return;
  }
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _mode = Mode::PlayUntilFrame;
    _remainingFrames = 0;
    _targetFrame = frame;
    _replayRequested = true;
    _playbackDoneSent = false;
  }
  _onPlayerStateChanged(PlayerState::Running);
  _cv.notify_all();
}

bool RemotePlaybackController::ShouldPauseAfterFrame(uint64_t frame) {
  std::lock_guard<std::mutex> lock(_mutex);

  switch (_mode) {
  case Mode::Paused:
    return true;
  case Mode::PlayOnce:
  case Mode::Loop:
    return false;
  case Mode::StepCount:
    if (_remainingFrames > 0) {
      --_remainingFrames;
    }
    if (_remainingFrames == 0) {
      _mode = Mode::Paused;
      _replayRequested = false;
      return true;
    }
    return false;
  case Mode::PlayUntilFrame:
    if (frame >= _targetFrame) {
      _mode = Mode::Paused;
      _replayRequested = false;
      return true;
    }
    return false;
  }

  return false;
}

bool RemotePlaybackController::ReplayImmediatelyAfterPass() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_mode == Mode::Loop) {
      return true;
    }
    _mode = Mode::Paused;
    _replayRequested = false;
  }
  _onPlayerStateChanged(PlayerState::Paused);
  return false;
}

bool RemotePlaybackController::WaitForReplayRequest() {
  std::unique_lock<std::mutex> lock(_mutex);

  if (_replayRequested) {
    return true;
  }

  _cv.wait(lock, [&] { return _replayRequested || _stopped || _isPlayerAborted(); });
  return !_stopped && !_isPlayerAborted();
}

void RemotePlaybackController::SendFrame(uint64_t frame) {
  SendFramePlayed(frame);
}

void RemotePlaybackController::SendFramePlayed(uint64_t frame) {
  std::lock_guard<std::mutex> lock(_mutex);
  _lastFrame = frame;
}

void RemotePlaybackController::NotifyPlaybackDone() {
  _playbackDoneSent = true;
  ReportState(PlayerState::Finished);
  _ipcRunning = false;
  if (_ipcStatusConnectThread.joinable()) {
    _ipcStatusConnectThread.join();
  }
  sendIpcStatus("state:done:" + std::to_string(_lastFrame));
}

void RemotePlaybackController::ReportState(PlayerState state) {
  std::lock_guard<std::mutex> lock(_mutex);
  _reportedState = state;
}

bool RemotePlaybackController::HasRemoteControl() const {
  return _remoteControl;
}

void RemotePlaybackController::UseLocalControl() {
  _remoteControl = false;
}

void RemotePlaybackController::ProcessPendingCommands() {
  std::deque<std::string> commands;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    commands.swap(_commands);
  }
  for (const auto& command : commands) {
    if (_stopped) {
      break;
    }
    processIpcCommand(command);
  }
}

void RemotePlaybackController::processIpcCommand(const std::string& command) {
  if (command == "abort" || command == "stop") {
    _remoteControl = true;
    Stop();
    return;
  }

  if (command == "start") {
    _remoteControl = true;
    Start();
    return;
  }

  if (command == "start-loop") {
    _remoteControl = true;
    StartLoop();
    return;
  }

  if (command == "pause") {
    _remoteControl = true;
    Pause();
    return;
  }

  if (command == "step") {
    _remoteControl = true;
    Step(1);
    return;
  }

  constexpr std::string_view stepPrefix = "step:";
  constexpr std::string_view playUntilPrefix = "play-until:";

  if (command.compare(0, stepPrefix.size(), stepPrefix) == 0) {
    uint64_t frames = 0;
    const auto begin = command.data() + stepPrefix.size();
    const auto end = command.data() + command.size();
    const auto result = std::from_chars(begin, end, frames);
    if (result.ec != std::errc{} || result.ptr != end || frames == 0) {
      return;
    }
    _remoteControl = true;
    Step(frames);
    return;
  }

  if (command.compare(0, playUntilPrefix.size(), playUntilPrefix) != 0) {
    return;
  }

  uint64_t targetFrame = 0;
  const auto begin = command.data() + playUntilPrefix.size();
  const auto end = command.data() + command.size();
  const auto result = std::from_chars(begin, end, targetFrame);
  if (result.ec != std::errc{} || result.ptr != end) {
    return;
  }
  _remoteControl = true;
  PlayUntilFrame(targetFrame);
}

bool RemotePlaybackController::sendIpcStatus(const std::string& status) {
  if (!_interactiveStart || !_ipcStatusConnected) {
    return false;
  }

  if (!_ipcManager.sendStatus(status)) {
    _ipcStatusConnected = false;
    if (_ipcManager.m_statusPipe) {
      _ipcManager.m_statusPipe->close();
    }
    return false;
  }

  return true;
}

} // namespace gits
