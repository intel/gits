// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "messageBus.h"
#include "statisticsService.h"

#include <string>

namespace gits {
namespace vulkan {

class StatisticsLayer : public Layer {
public:
  StatisticsLayer(const StatisticsConfig& cfg, gits::MessageBus& msgBus);
  ~StatisticsLayer() = default;
  StatisticsLayer(const StatisticsLayer&) = delete;
  StatisticsLayer& operator=(const StatisticsLayer&) = delete;

  void Post(StateRestoreBeginCommand& c) override;
  void Post(StateRestoreEndCommand& c) override;
  void Post(FrameEndCommand& c) override;
  void Post(MarkerUInt64Command& c) override;
  void Post(CreateWindowMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;
  void Post(UpdateWindowMetaCommand& command) override;
  %for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  void Post(${command.name}Command& command) override;
  % if define:
  #endif
  % endif
  %endfor

private:
  StatisticsService m_StatisticsService;
};

} // namespace vulkan
} // namespace gits
