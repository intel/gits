// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "messageBus.h"
#include "statisticsWriter.h"

#include <string>
#include <map>
#include <optional>

namespace gits {
namespace vulkan {

struct StatisticsConfig {
  std::string Output;
  bool IsCapture{};
};

class StatisticsService {
public:
  StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus);
  ~StatisticsService();
  StatisticsService(const StatisticsService&) = delete;
  StatisticsService& operator=(const StatisticsService&) = delete;

  void StateRestoreBegin();
  void StateRestoreEnd();
  // The recorder ends a frame at each present. The player ends one at each frame end of the
  // stream, which in a stream with two APIs is recorded by the API that presents.
  void Present();
  void Command(const std::string& name);

private:
  void FrameEnd();
  void WriteResults();

private:
  const StatisticsConfig m_Cfg;
  gits::MessageBus& m_MsgBus;
  StatisticsWriter m_Writer;
  unsigned m_SubscriptionId{};
  std::optional<unsigned> m_FrameEndSubscriptionId;
  unsigned m_FramesNum{0};
  unsigned m_CallsNum{};
  unsigned m_StateInitCallsNum{};
  bool m_StateRestore{false};

  struct CallInfo {
    unsigned Num{};
    unsigned FrameNum{};
    unsigned MinFrameNum{std::numeric_limits<unsigned>::max()};
    unsigned MaxFrameNum{};
    unsigned CurrentFrameNum{};
  };
  std::map<std::string, CallInfo> m_CallInfos;
};

} // namespace vulkan
} // namespace gits
