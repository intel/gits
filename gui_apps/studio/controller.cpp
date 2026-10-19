// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "controller.h"

#include "ipcPipe.h"

#include <chrono>
#include <charconv>
#include <cstdint>
#include <string_view>

namespace {
constexpr auto kConnectRetryInterval = std::chrono::milliseconds(500);
constexpr auto kConnectedPollInterval = std::chrono::milliseconds(250);
constexpr auto kCommandConnectWait = std::chrono::seconds(5);
constexpr auto kCommandConnectPollInterval = std::chrono::milliseconds(50);
} // namespace

namespace gits::gui {

Controller::Controller(gits::common::IPCManager& ipcManager) : m_IPCManager(ipcManager) {}

Controller::~Controller() {
  Stop();
}

void Controller::Start() {
  if (m_Running.exchange(true)) {
    return;
  }

  m_ConnectionThread = std::thread(&Controller::ConnectLoop, this);
  m_StatusThread = std::thread(&Controller::StatusLoop, this);
}

void Controller::Stop() {
  if (!m_Running.exchange(false)) {
    return;
  }

  if (m_IPCManager.m_commandPipe) {
    m_IPCManager.m_commandPipe->close();
  }
  if (m_IPCManager.m_statusPipe) {
    m_IPCManager.m_statusPipe->close();
  }

  if (m_ConnectionThread.joinable()) {
    m_ConnectionThread.join();
  }
  if (m_StatusThread.joinable()) {
    m_StatusThread.join();
  }

  m_IPCManager.m_commandPipe->close();
  m_IPCManager.m_statusPipe->close();
  {
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    m_QueuedCommand.clear();
  }
  m_CommandPending = false;
  m_HasStatus = false;

  m_Connected = false;
  m_StatusConnected = false;
  m_PlaybackState = PlaybackState::Disconnected;
}

bool Controller::SendStart() {
  return SendCommand("start");
}

bool Controller::SendStartLoop() {
  return SendCommand("start-loop");
}

bool Controller::SendStop() {
  return SendCommand("stop");
}

bool Controller::SendPause() {
  return SendCommand("pause");
}

bool Controller::SendPlayUntil(uint64_t frame) {
  return SendCommand("play-until:" + std::to_string(frame));
}

bool Controller::QueueCommand(const std::string& command) {
  std::lock_guard<std::mutex> lock(m_QueueMutex);
  if (!m_Running || !IsConnected() || command.empty() || m_CommandPending) {
    return false;
  }
  m_QueuedCommand = command;
  m_CommandFailed = false;
  m_CommandPending = true;
  return true;
}

bool Controller::IsCommandPending() const {
  return m_CommandPending;
}

bool Controller::CommandFailed() const {
  return m_CommandFailed;
}

bool Controller::SendAbort() {
  return SendCommand("abort");
}

bool Controller::SendStep() {
  return SendCommand("step");
}

bool Controller::SendStep(uint64_t frames) {
  if (frames == 0) {
    return false;
  }
  return SendCommand("step:" + std::to_string(frames));
}

bool Controller::SendCommand(const std::string& command) {
  std::lock_guard<std::mutex> lock(m_SendMutex);
  if (command.empty()) {
    return false;
  }

  if (!m_Connected && !WaitForCommandConnection(kCommandConnectWait)) {
    return false;
  }

  if (!m_IPCManager.sendCommand(command)) {
    if (m_IPCManager.m_commandPipe) {
      m_IPCManager.m_commandPipe->close();
    }
    m_Connected = false;

    // Do not retry: the player may have received a command even if the write failed.
    return false;
  }

  return true;
}

void Controller::SetFrameCallback(std::function<void(uint64_t)> callback) {
  std::lock_guard<std::mutex> lock(m_CallbackMutex);
  m_FrameCallback = std::move(callback);
}

void Controller::SetPlaybackDoneCallback(std::function<void()> callback) {
  std::lock_guard<std::mutex> lock(m_CallbackMutex);
  m_PlaybackDoneCallback = std::move(callback);
}

void Controller::ResetPlaybackDone() {
  m_PlaybackDone = false;
  m_PlaybackState = PlaybackState::Paused;
}

bool Controller::IsConnected() const {
  return m_Connected && m_StatusConnected && m_HasStatus;
}

bool Controller::IsPlaybackDone() const {
  return m_PlaybackDone;
}

Controller::PlaybackState Controller::GetPlaybackState() const {
  return m_PlaybackState;
}

uint64_t Controller::GetLastFrame() const {
  return m_LastFrame;
}

double Controller::GetTimeSinceLastFrame() const {
  const auto lastTime = m_LastFrameTime.load();
  if (lastTime.count() == 0) {
    return -1.0;
  }
  auto elapsed = std::chrono::steady_clock::now().time_since_epoch() - lastTime;
  return std::chrono::duration<double>(elapsed).count();
}

void Controller::ConnectLoop() {
  while (m_Running) {
    std::string command;
    {
      std::lock_guard<std::mutex> lock(m_QueueMutex);
      command.swap(m_QueuedCommand);
    }
    if (!command.empty()) {
      m_CommandFailed = !IsConnected() || !SendCommand(command);
      m_CommandPending = false;
    }
    if (m_Connected) {
      {
        std::lock_guard<std::mutex> lock(m_SendMutex);
        if (!m_IPCManager.m_commandPipe || !m_IPCManager.m_commandPipe->isConnected()) {
          if (m_IPCManager.m_commandPipe) {
            m_IPCManager.m_commandPipe->close();
          }
          m_Connected = false;
          m_PlaybackState = PlaybackState::Disconnected;
        }
      }

      std::this_thread::sleep_for(kConnectedPollInterval);
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(m_SendMutex);
      m_Connected = TryConnectCommandPipe();
    }

    if (!m_Connected) {
      std::this_thread::sleep_for(kConnectRetryInterval);
    }
  }
}

void Controller::StatusLoop() {
  while (m_Running) {
    if (!m_StatusConnected) {
      m_StatusConnected = TryConnectStatusPipe();
      if (!m_StatusConnected) {
        std::this_thread::sleep_for(kConnectRetryInterval);
        continue;
      }
    }

    std::string status;
    if (m_IPCManager.m_statusPipe && m_IPCManager.m_statusPipe->read(status, 100)) {
      HandleStatusMessage(status);
      continue;
    }

    if (!m_IPCManager.m_statusPipe->isConnected()) {
      m_IPCManager.m_statusPipe->close();
      m_StatusConnected = false;
      m_HasStatus = false;
      m_PlaybackState = PlaybackState::Disconnected;
    }
  }
}

bool Controller::TryConnectCommandPipe() {
  if (!m_IPCManager.m_commandPipe) {
    return false;
  }

  if (m_IPCManager.m_commandPipe->isValid()) {
    return true;
  }

  return m_IPCManager.m_commandPipe->connect();
}

bool Controller::TryConnectStatusPipe() {
  if (!m_IPCManager.m_statusPipe) {
    return false;
  }

  if (!m_IPCManager.m_statusPipe->isValid() && !m_IPCManager.m_statusPipe->create()) {
    return false;
  }

  if (m_IPCManager.m_statusPipe->isConnected()) {
    return true;
  }

  return m_IPCManager.m_statusPipe->connect();
}

bool Controller::WaitForCommandConnection(std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;

  while (std::chrono::steady_clock::now() < deadline) {
    if (m_Connected) {
      return true;
    }

    m_Connected = TryConnectCommandPipe();
    if (m_Connected) {
      return true;
    }

    if (!m_Running) {
      return false;
    }

    std::this_thread::sleep_for(kCommandConnectPollInterval);
  }

  return m_Connected;
}

void Controller::HandleStatusMessage(const std::string& status) {
  if (status.rfind("state:", 0) == 0) {
    const auto separator = status.find(':', 6);
    if (separator == std::string::npos) {
      return;
    }
    const auto state = status.substr(6, separator - 6);
    PlaybackState playbackState;
    if (state == "paused") {
      playbackState = PlaybackState::Paused;
    } else if (state == "running") {
      playbackState = PlaybackState::Running;
    } else if (state == "done") {
      playbackState = PlaybackState::Done;
    } else {
      return;
    }
    uint64_t frame = 0;
    const auto end = status.data() + status.size();
    const auto parsed = std::from_chars(status.data() + separator + 1, end, frame);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      return;
    }
    m_LastFrame = frame;
    m_LastFrameTime = std::chrono::steady_clock::now().time_since_epoch();
    m_PlaybackDone = playbackState == PlaybackState::Done;
    m_PlaybackState = playbackState;
    m_HasStatus = true;
    std::function<void(uint64_t)> callback;
    {
      std::lock_guard<std::mutex> lock(m_CallbackMutex);
      callback = m_FrameCallback;
    }
    if (callback) {
      callback(frame);
    }
    return;
  }
  if (status == "playback:done") {
    m_PlaybackDone = true;
    m_PlaybackState = PlaybackState::Done;

    std::function<void()> callback;
    {
      std::lock_guard<std::mutex> lock(m_CallbackMutex);
      callback = m_PlaybackDoneCallback;
    }
    if (callback) {
      callback();
    }
    return;
  }

  constexpr std::string_view kFramePrefix = "frame:";
  constexpr std::string_view kFramePlayedPrefix = "frame-played:";
  size_t prefixSize = 0;

  if (status.compare(0, kFramePrefix.size(), kFramePrefix) == 0) {
    prefixSize = kFramePrefix.size();
  } else if (status.compare(0, kFramePlayedPrefix.size(), kFramePlayedPrefix) == 0) {
    prefixSize = kFramePlayedPrefix.size();
  } else {
    return;
  }

  uint64_t frame = 0;
  const auto end = status.data() + status.size();
  const auto parsed = std::from_chars(status.data() + prefixSize, end, frame);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    return;
  }

  m_LastFrame = frame;
  m_LastFrameTime = std::chrono::steady_clock::now().time_since_epoch();
  std::function<void(uint64_t)> callback;
  {
    std::lock_guard<std::mutex> lock(m_CallbackMutex);
    callback = m_FrameCallback;
  }
  if (callback) {
    callback(frame);
  }
}

} // namespace gits::gui
