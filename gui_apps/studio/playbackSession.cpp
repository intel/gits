// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playbackSession.h"

#include "baseContext.h"
#include "guiActions.h"
#include "studioLabels.h"

#include <chrono>
#include <utility>

namespace gits::gui {

PlaybackSession::~PlaybackSession() {
  Shutdown();
}

void PlaybackSession::Shutdown() {
  m_Reload = false;
  if (m_Connection) {
    if (m_Active) {
      m_Connection->Control.SendStop();
    }
    m_Connection->Control.Stop();
    m_Connection.reset();
  }
  if (m_Retiring.valid()) {
    m_Retiring.get();
  }
}

bool PlaybackSession::CanLoadReset() const {
  return !m_Retiring.valid() && !m_Stopping && (!m_Active || CanControl());
}

bool PlaybackSession::CanControl() const {
  return m_Active && !m_Stopping && m_Connection && m_Connection->Control.IsConnected() &&
         !m_Connection->Control.IsPlaybackDone() && !m_Connection->Control.IsCommandPending();
}

bool PlaybackSession::Send(const std::string& command) {
  if (!CanControl() || !m_Connection->Control.QueueCommand(command)) {
    return false;
  }
  if (command == "stop") {
    m_Stopping = true;
    m_Status = Labels::Status::PLAYBACK_STOPPING;
  }
  return true;
}

void PlaybackSession::LoadReset(const std::filesystem::path& stream,
                                const Configuration& configuration,
                                bool supported) {
  if (!CanLoadReset()) {
    return;
  }
  std::error_code error;
  if (!std::filesystem::is_regular_file(stream, error) ||
      !std::filesystem::is_regular_file(BaseContext::GetBaseInstance().GetGITSPlayerPath(),
                                        error)) {
    m_Status = Labels::Status::NO_STREAM_ERROR;
    return;
  }
  if (m_Active && !Send("stop")) {
    return;
  }
  m_Stream = stream;
  m_Configuration = configuration;
  m_Supported = supported;
  m_Reload = true;
}

void PlaybackSession::RetireConnection() {
  if (m_Connection) {
    m_Retiring = std::async(std::launch::async,
                            [connection = std::move(m_Connection)] { connection->Control.Stop(); });
  }
}

void PlaybackSession::Launch() {
  m_Reload = false;
  m_RemoteEnabled = m_Supported && m_Configuration->common.player.interactive;
  if (m_RemoteEnabled) {
    m_Connection = std::make_shared<Connection>();
    if (!m_Connection->Manager.m_statusPipe->create()) {
      m_Status = Labels::Status::PIPE_ALREADY_OWNED;
      m_Connection.reset();
      return;
    }
    m_Connection->Control.Start();
  }
  m_Output = std::make_shared<Output>();
  m_Frame = 0;
  m_Active = true;
  m_Stopping = false;
  m_Status = m_RemoteEnabled ? Labels::Status::LOADING_IPC
             : !m_Supported  ? Labels::Status::UNSUPPORTED_STREAM
                             : Labels::Status::NON_INTERACTIVE_MODE;
  try {
    gui_actions::PlaybackStreamAsync(
        m_Stream, *m_Configuration,
        [output = m_Output](bool success) { output->Result = success ? 1 : 2; },
        [output = m_Output](const std::string& text) {
          std::lock_guard<std::mutex> lock(output->Mutex);
          output->Log += text;
          constexpr size_t maxBufferedLog = 1024 * 1024;
          if (output->Log.size() > maxBufferedLog) {
            output->Log.erase(0, output->Log.size() - maxBufferedLog);
          }
        });
  } catch (const std::exception& error) {
    m_Status = std::string(Labels::Status::PLAYER_FAILED_PREFIX) + error.what();
    m_Output->Result = 2;
  }
}

void PlaybackSession::Update() {
  if (m_Output) {
    std::string log;
    {
      std::lock_guard<std::mutex> lock(m_Output->Mutex);
      log.swap(m_Output->Log);
    }
    if (!log.empty()) {
      BaseContext::GetBaseInstance().AddGITSLogText(log);
    }
  }
  if (m_Connection) {
    m_Frame = m_Connection->Control.GetLastFrame();
  }
  if (m_Active && m_Output && m_Output->Result != 0) {
    m_Active = false;
    m_Stopping = false;
    m_Status =
        m_Output->Result == 1 ? Labels::Status::SESSION_ENDED : Labels::Status::PLAYER_FAILED;
    RetireConnection();
  }
  if (m_Retiring.valid() &&
      m_Retiring.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    m_Retiring.get();
  }
  if (!m_Active && !m_Retiring.valid() && m_Reload) {
    Launch();
  }
  if (m_Active && m_Connection) {
    if (m_Connection->Control.CommandFailed()) {
      m_Stopping = false;
      m_Reload = false;
      m_Status = Labels::Status::COMMAND_FAILED;
    } else if (!m_Connection->Control.IsConnected()) {
      m_Status = Labels::Status::WAITING_CONNECTION;
    } else if (!m_Stopping) {
      switch (m_Connection->Control.GetPlaybackState()) {
      case Controller::PlaybackState::Paused:
        m_Status = Labels::Status::PAUSED;
        break;
      case Controller::PlaybackState::Running:
        m_Status = Labels::Status::RUNNING;
        break;
      case Controller::PlaybackState::Done:
        m_Status = Labels::Status::DONE;
        break;
      default:
        break;
      }
    }
  }
}

uint64_t PlaybackSession::Frame() const {
  return m_Frame;
}

Controller::PlaybackState PlaybackSession::State() const {
  return m_Connection ? m_Connection->Control.GetPlaybackState()
                      : Controller::PlaybackState::Disconnected;
}

const std::string& PlaybackSession::Status() const {
  return m_Status;
}

} // namespace gits::gui
