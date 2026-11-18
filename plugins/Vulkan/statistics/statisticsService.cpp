// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsService.h"
#include "yaml-cpp/yaml.h"

#include <algorithm>

namespace gits {
namespace vulkan {

StatisticsService::StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus), m_Writer("Vulkan", msgBus) {
  if (cfg.IsCapture) {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  } else {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
    m_FrameEndSubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_FRAME_END},
                                                  [this](Topic, const MessagePtr&) { FrameEnd(); });
  }
}

StatisticsService::~StatisticsService() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
  if (m_FrameEndSubscriptionId) {
    m_MsgBus.unsubscribe(*m_FrameEndSubscriptionId);
  }
}

void StatisticsService::Present() {
  if (m_Cfg.IsCapture) {
    FrameEnd();
  }
}

void StatisticsService::StateRestoreBegin() {
  m_StateRestore = true;
}

void StatisticsService::StateRestoreEnd() {
  m_StateInitCallsNum = m_CallsNum;
  m_StateRestore = false;
}

void StatisticsService::FrameEnd() {
  if (!m_StateRestore) {
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
}

void StatisticsService::Command(const std::string& name) {
  ++m_CallsNum;
  CallInfo& info = m_CallInfos[name];
  ++info.Num;
  ++info.CurrentFrameNum;
}

void StatisticsService::WriteResults() {
  YAML::Node apiCalls;
  for (auto& [name, info] : m_CallInfos) {
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

} // namespace vulkan
} // namespace gits
