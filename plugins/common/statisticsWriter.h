// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "messageBus.h"
#include "yaml-cpp/yaml.h"

#include <map>
#include <string>

namespace gits {

// Writes the statistics file of one API. The statistics plugins of the APIs in one process write
// one file together: a plugin writing a file that the plugin of another API wrote earlier in the
// process adds its ApiCalls section to the file. The summary of the file adds up the last summary
// each API wrote, so an API that writes again replaces its earlier calls in it.
class StatisticsWriter {
public:
  StatisticsWriter(std::string api, MessageBus& msgBus);
  ~StatisticsWriter();
  StatisticsWriter(const StatisticsWriter&) = delete;
  StatisticsWriter& operator=(const StatisticsWriter&) = delete;

  // calls is the sequence of the ApiCalls section of the API.
  void Write(const std::string& path, const StatisticsSummary& summary, const YAML::Node& calls);

private:
  const std::string m_Api;
  MessageBus& m_MsgBus;
  unsigned m_SubscriptionId{};
  // By path and API.
  std::map<std::string, std::map<std::string, StatisticsSummary>> m_SummariesOfOtherApis;
};

} // namespace gits
