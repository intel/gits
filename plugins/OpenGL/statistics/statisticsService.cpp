// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsService.h"

#include "log.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>

namespace gits::opengl {

StatisticsService::StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus), m_Writer("OpenGL", msgBus) {
  m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                        [this](Topic, const MessagePtr&) { WriteResults(); });
  m_FrameEndSubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_FRAME_END},
                                                [this](Topic, const MessagePtr&) { FrameEnd(); });
}

StatisticsService::~StatisticsService() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
  m_MsgBus.unsubscribe(m_FrameEndSubscriptionId);
}

void StatisticsService::Command(std::string_view name, GITSKey key) {
  ++m_CallsNum;
  if (key & STATE_RESTORE_KEY_MASK) {
    ++m_StateInitCallsNum;
  }
  auto it = m_CallInfos.find(name);
  if (it == m_CallInfos.end()) {
    CallInfo info;
    // A command first called after some frames was called zero times in each of them.
    if (m_FramesNum > 0) {
      info.MinFrameNum = 0;
    }
    it = m_CallInfos.emplace(std::string(name), info).first;
  }
  ++it->second.Num;
  ++it->second.CurrentFrameNum;
}

void StatisticsService::FrameEnd() {
  ++m_FramesNum;
  for (auto& [name, info] : m_CallInfos) {
    if (info.CurrentFrameNum > 0) {
      ++info.FrameNum;
    }
    info.MinFrameNum = std::min(info.MinFrameNum, info.CurrentFrameNum);
    info.MaxFrameNum = std::max(info.MaxFrameNum, info.CurrentFrameNum);
    info.CurrentFrameNum = 0;
  }
}

void StatisticsService::WriteResults() {
  YAML::Node apiCalls;
  for (const auto& [name, info] : m_CallInfos) {
    YAML::Node call;
    call["Name"] = name;
    call["Num"] = info.Num;
    call["FrNum"] = info.FrameNum;
    call["MinPFr"] = info.MinFrameNum;
    call["MaxPFr"] = info.MaxFrameNum;
    apiCalls.push_back(call);
  }

  StatisticsSummary summary;
  summary.ApiFunctionsNum = static_cast<unsigned>(m_CallInfos.size());
  summary.FramesNum = m_FramesNum;
  summary.CallsNum = m_CallsNum;
  summary.StateInitCallsNum = m_StateInitCallsNum;
  m_Writer.Write(m_Cfg.Output, summary, apiCalls);
}

} // namespace gits::opengl
