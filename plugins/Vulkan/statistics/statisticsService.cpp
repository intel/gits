// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsService.h"
#include "yaml-cpp/yaml.h"

#include <fstream>

namespace gits {
namespace vulkan {

StatisticsService::StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus) {
  if (cfg.IsCapture) {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  } else {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  }
}

StatisticsService::~StatisticsService() {}

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
  std::ofstream stream(m_Cfg.Output);
  GITS_ASSERT(stream.good(), "StatisticsService - failed to create file: " + m_Cfg.Output);

  YAML::Node output;
  YAML::Node stats = output["Statistics"];
  stats["ApiFunctionsNum"] = m_CallInfos.size();
  stats["FramesNum"] = m_FramesNum;
  stats["CallsNum"] = m_CallsNum;
  stats["StateInitCallsNum"] = m_StateInitCallsNum;
  stats["ApplicationCallsNum"] = m_CallsNum - m_StateInitCallsNum;
  stats["AvgAppCallsNumPerFrame"] = (m_CallsNum - m_StateInitCallsNum) / std::max(m_FramesNum, 1U);

  YAML::Node apiCalls = output["ApiCalls"]["Vulkan"];
  for (auto& [name, info] : m_CallInfos) {
    YAML::Node call;
    call["Name"] = name;
    call["Num"] = info.Num;
    call["FrNum"] = info.FrameNum;
    call["MinPFr"] = info.MinFrameNum;
    call["MaxPFr"] = info.MaxFrameNum;
    apiCalls.push_back(call);
  }

  stream << output;
  LOG_INFO << "Statistics printed into " << m_Cfg.Output << " file.";
}

} // namespace vulkan
} // namespace gits
