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
#include <fstream>

namespace gits::opengl {

StatisticsService::StatisticsService(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus) {
  m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                        [this](Topic, const MessagePtr&) { WriteResults(); });
}

StatisticsService::~StatisticsService() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
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
  std::ofstream stream(m_Cfg.Output);
  GITS_ASSERT(stream.good(), "StatisticsService - failed to create file: " + m_Cfg.Output);

  const unsigned applicationCallsNum = m_CallsNum - m_StateInitCallsNum;
  YAML::Node output;
  YAML::Node stats = output["Statistics"];
  stats["ApiFunctionsNum"] = m_CallInfos.size();
  stats["FramesNum"] = m_FramesNum;
  stats["CallsNum"] = m_CallsNum;
  stats["StateInitCallsNum"] = m_StateInitCallsNum;
  stats["ApplicationCallsNum"] = applicationCallsNum;
  stats["AvgAppCallsNumPerFrame"] = applicationCallsNum / std::max(m_FramesNum, 1U);

  YAML::Node apiCalls = output["ApiCalls"]["OpenGL"];
  for (const auto& [name, info] : m_CallInfos) {
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

} // namespace gits::opengl
