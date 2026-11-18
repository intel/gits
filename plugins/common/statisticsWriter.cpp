// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsWriter.h"

#include "log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace gits {

namespace {

std::string NormalizedPath(const std::string& path) {
  return std::filesystem::absolute(path).lexically_normal().string();
}

} // namespace

StatisticsWriter::StatisticsWriter(std::string api, MessageBus& msgBus)
    : m_Api(std::move(api)), m_MsgBus(msgBus) {
  m_SubscriptionId = m_MsgBus.subscribe(
      {PUBLISHER_PLUGIN, TOPIC_STATISTICS_WRITTEN}, [this](Topic, const MessagePtr& message) {
        const auto written = std::dynamic_pointer_cast<StatisticsWrittenMessage>(message);
        if (written && written->getApi() != m_Api) {
          m_SummariesOfOtherApis[written->getPath()][written->getApi()] = written->getSummary();
        }
      });
}

StatisticsWriter::~StatisticsWriter() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
}

void StatisticsWriter::Write(const std::string& path,
                             const StatisticsSummary& summary,
                             const YAML::Node& calls) {
  const std::string normalizedPath = NormalizedPath(path);
  StatisticsSummary total = summary;
  YAML::Node output;
  const auto others = m_SummariesOfOtherApis.find(normalizedPath);
  if (others != m_SummariesOfOtherApis.end()) {
    try {
      output = YAML::LoadFile(path);
      for (const auto& [api, other] : others->second) {
        total.ApiFunctionsNum += other.ApiFunctionsNum;
        total.FramesNum = std::max(total.FramesNum, other.FramesNum);
        total.CallsNum += other.CallsNum;
        total.StateInitCallsNum += other.StateInitCallsNum;
      }
    } catch (const YAML::Exception& e) {
      LOG_ERROR << "StatisticsService - failed to read the statistics of other APIs from " << path
                << ", they are overwritten: " << e.what();
      output = YAML::Node();
    }
  }

  const unsigned applicationCallsNum = total.CallsNum - total.StateInitCallsNum;
  YAML::Node statistics = output["Statistics"];
  statistics["ApiFunctionsNum"] = total.ApiFunctionsNum;
  statistics["FramesNum"] = total.FramesNum;
  statistics["CallsNum"] = total.CallsNum;
  statistics["StateInitCallsNum"] = total.StateInitCallsNum;
  statistics["ApplicationCallsNum"] = applicationCallsNum;
  statistics["AvgAppCallsNumPerFrame"] = applicationCallsNum / std::max(total.FramesNum, 1U);
  output["ApiCalls"][m_Api] = calls;

  {
    std::ofstream stream(path);
    GITS_ASSERT(stream.good(), "StatisticsService - failed to create file: " + path);
    stream << output;
  }
  LOG_INFO << "Statistics printed into " << path << " file.";
  m_MsgBus.publish({PUBLISHER_PLUGIN, TOPIC_STATISTICS_WRITTEN},
                   std::make_shared<StatisticsWrittenMessage>(m_Api, normalizedPath, summary));
}

} // namespace gits
