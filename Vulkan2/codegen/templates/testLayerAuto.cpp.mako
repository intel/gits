// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "testLayerAuto.h"
#include "log.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif

void TestLayerAuto::Pre(${command.name}Command& command) {
  LOG_INFO << "Pre - ${command.name}";
}

void TestLayerAuto::Post(${command.name}Command& command) {
  LOG_INFO << "Post - ${command.name}";
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits