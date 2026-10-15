// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "statisticsLayerAuto.h"

namespace gits::opengl2 {

StatisticsLayer::StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus)
    : Layer("StatisticsPlugin"), m_StatisticsService(cfg, msgBus) {}

void StatisticsLayer::Post(MappedDataMetaCommand& command) {
  m_StatisticsService.Command("MappedDataMeta", command.m_Key);
}

void StatisticsLayer::Post(WindowGeometryMetaCommand& command) {
  m_StatisticsService.Command("WindowGeometryMeta", command.m_Key);
}

void StatisticsLayer::Post(ClientArraysMetaCommand& command) {
  m_StatisticsService.Command("ClientArraysMeta", command.m_Key);
}

void StatisticsLayer::Post(UniformArrayMetaCommand& command) {
  m_StatisticsService.Command("UniformArrayMeta", command.m_Key);
}

void StatisticsLayer::Post(MappedTextureDataMetaCommand& command) {
  m_StatisticsService.Command("MappedTextureDataMeta", command.m_Key);
}

void StatisticsLayer::Post(PixelFormatMetaCommand& command) {
  m_StatisticsService.Command("PixelFormatMeta", command.m_Key);
}

void StatisticsLayer::Post(UnsupportedCommand& command) {
  m_StatisticsService.Command(command.GetName(), command.m_Key);
}

} // namespace gits::opengl2
