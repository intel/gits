// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configurationAuto.h"
#include "controller.h"
#include "ipcPipe.h"

#include <atomic>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace gits::gui {

class PlaybackSession {
public:
  ~PlaybackSession();
  void LoadReset(const std::filesystem::path& stream,
                 const Configuration& configuration,
                 bool supported);
  void Update();
  void Shutdown();
  bool Send(const std::string& command);
  bool CanControl() const;
  bool CanLoadReset() const;
  uint64_t Frame() const;
  Controller::PlaybackState State() const;
  const std::string& Status() const;

private:
  struct Connection {
    common::IPCManager Manager{"gits_player_ipc", false};
    Controller Control{Manager};
  };
  struct Output {
    std::atomic<int> Result{0};
    std::mutex Mutex;
    std::string Log;
  };

  void Launch();
  void RetireConnection();

  std::shared_ptr<Connection> m_Connection;
  std::shared_ptr<Output> m_Output;
  std::future<void> m_Retiring;
  std::filesystem::path m_Stream;
  std::optional<Configuration> m_Configuration;
  bool m_Supported = false;
  bool m_Active = false;
  bool m_Stopping = false;
  bool m_Reload = false;
  bool m_RemoteEnabled = false;
  uint64_t m_Frame = 0;
  std::string m_Status = "Load/Reset is required to launch a player session.";
};

} // namespace gits::gui
