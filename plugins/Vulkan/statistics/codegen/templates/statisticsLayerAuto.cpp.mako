// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "statisticsLayerAuto.h"

namespace gits {
namespace vulkan {

%for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
void StatisticsLayer::Post(${command.name}Command& command) {
  m_StatisticsService.Command("${command.name}");
}
% if define:
#endif
% endif
%endfor

} // namespace vulkan
} // namespace gits
