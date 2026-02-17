// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   player.h
 *
 * @brief Declaration of function calls player.
 *
 */

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
  TState _state;     /**< @brief defines current player state */
  bool _interactive; /**< @brief defines if player is running in interactive mode */
  StreamingContext _sc;

public:
  CPlayer();
  ~CPlayer();

  void Register(std::unique_ptr<CAction> action) {
    _sc.action.reset(action.release());
  }

  TState State() const;
  void Load(const std::filesystem::path& fileName);
  CScheduler& Scheduler() {
    return *_sc.scheduler;
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
