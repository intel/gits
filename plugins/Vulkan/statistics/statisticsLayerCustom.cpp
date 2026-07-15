// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsLayerAuto.h"

namespace gits {
namespace vulkan {

StatisticsLayer::StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : Layer("StatisticsPlugin"), m_StatisticsService(cfg, msgBus) {}

void StatisticsLayer::Post(StateRestoreBeginCommand& c) {
  m_StatisticsService.StateRestoreBegin();
}

void StatisticsLayer::Post(StateRestoreEndCommand& c) {
  m_StatisticsService.StateRestoreEnd();
}

void StatisticsLayer::Post(MarkerUInt64Command& c) {}

void StatisticsLayer::Post(CreateWindowMetaCommand& command) {
  m_StatisticsService.Command("CreateWindowMeta");
}

void StatisticsLayer::Post(MappedDataMetaCommand& command) {
  m_StatisticsService.Command("MappedDataMeta");
}

void StatisticsLayer::Post(UpdateWindowMetaCommand& command) {
  m_StatisticsService.Command("UpdateWindowMeta");
}

void StatisticsLayer::Post(RestoreContentManifestCommand& command) {
  m_StatisticsService.Command("RestoreContentManifest");
}

void StatisticsLayer::Post(RestoreContentDataCommand& command) {
  m_StatisticsService.Command("RestoreContentData");
}

} // namespace vulkan
} // namespace gits
