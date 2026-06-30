// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "messageBus.h"

#include <string>
#include <map>

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

  void StateRestoreBegin();
  void StateRestoreEnd();
  void FrameEnd();
  void Command(const std::string& name);

private:
  void WriteResults();

private:
  const StatisticsConfig m_Cfg;
  gits::MessageBus& m_MsgBus;
  unsigned m_SubscriptionId{};
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
