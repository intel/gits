// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "layerAuto.h"
#include "messageBus.h"
#include "statisticsService.h"

namespace gits::opengl2 {

class StatisticsLayer : public Layer {
public:
  StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus);
  StatisticsLayer(const StatisticsLayer&) = delete;
  StatisticsLayer& operator=(const StatisticsLayer&) = delete;

  void Post(MappedDataMetaCommand& command) override;
  void Post(WindowGeometryMetaCommand& command) override;
  void Post(ClientArraysMetaCommand& command) override;
  void Post(UniformArrayMetaCommand& command) override;
  void Post(MappedTextureDataMetaCommand& command) override;
  void Post(PbufferPixelFormatMetaCommand& command) override;
  void Post(UnsupportedCommand& command) override;
% for cmd in commands:
% if cmd.enabled:
% if cmd.name in frame_end_commands:
  void Pre(${cmd.name}Command& command) override;
% endif
  void Post(${cmd.name}Command& command) override;
% endif
% endfor

private:
  StatisticsService m_StatisticsService;
  bool m_SwapEndsFrame{};
};

} // namespace gits::opengl2
