// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "messageBus.h"

#include <functional>
#include <limits>
#include <map>
#include <string>
#include <string_view>

namespace gits::opengl2 {

struct StatisticsConfig {
  std::string Output;
};

// Counts replayed calls per command name and per frame, and writes them on program exit.
class StatisticsService {
public:
  StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus);
  ~StatisticsService();
  StatisticsService(const StatisticsService&) = delete;
  StatisticsService& operator=(const StatisticsService&) = delete;

  // Commands whose key has STATE_RESTORE_KEY_MASK set are counted as state initialization.
  void Command(std::string_view name, GITSKey key);
  void FrameEnd();

private:
  void WriteResults();

  struct CallInfo {
    unsigned Num{};
    unsigned FrameNum{};
    unsigned MinFrameNum{std::numeric_limits<unsigned>::max()};
    unsigned MaxFrameNum{};
    unsigned CurrentFrameNum{};
  };

  const StatisticsConfig m_Cfg;
  gits::MessageBus& m_MsgBus;
  unsigned m_SubscriptionId{};
  unsigned m_FramesNum{};
  unsigned m_CallsNum{};
  unsigned m_StateInitCallsNum{};
  std::map<std::string, CallInfo, std::less<>> m_CallInfos;
};

} // namespace gits::opengl2
