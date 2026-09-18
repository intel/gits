// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "tools_lite.h"
#include "scheduler.h"

#include <string>
#include <memory>

namespace gits {
class CFrameRateCounter;

/**
   * @brief Main class of a player
   *
   * gits::CPlayer class is responsible for a process of playing
   * recorded library calls. Its behavior is specified by
   * gits::CPlayer::CBehavior class.
   */
class CPlayer : private gits::noncopyable {
public:
  enum TState {
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_FINISHED
  };

private:
  TState m_State;     /**< @brief defines current player state */
  bool m_Interactive; /**< @brief defines if player is running in interactive mode */
  StreamingContext m_Sc;

public:
  CPlayer();
  ~CPlayer();

  void Register(std::unique_ptr<CAction> action) {
    m_Sc.action.reset(action.release());
  }

  TState State() const;
  void Load(const std::filesystem::path& fileName);
  CScheduler& Scheduler() {
    return *m_Sc.scheduler;
  }
  void Play();
  void Key(unsigned code);
  void GLResourceCleanup();
  void GLContextsCleanup();
  void StatisticsPrint() const;
  void NotSupportedFunctionsPrint() const;
  int RenameAndRelaunch(const std::string& newPlayerName,
                        std::filesystem::path originalPlayerPath,
                        std::vector<std::string> args);
};
} // namespace gits
