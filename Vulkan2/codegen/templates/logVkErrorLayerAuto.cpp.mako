// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "logVkErrorLayerAuto.h"
#include "enumToStrAuto.h"
#include "log.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if command.return_type == 'VkResult':
% if define:
#ifdef ${define}
% endif

void LogVkErrorLayer::Pre(${command.name}Command& command) {
  m_PreReturn = command.m_Return.Value;
}

void LogVkErrorLayer::Post(${command.name}Command& command) {
  if (IsFailure(command.m_Return.Value)) {
    LOG_ERROR << command.m_Key << " ${command.name} failed " << toStr(command.m_Return.Value);
  }
}

% if define:
#endif
% endif
% endif
% endfor

} // namespace vulkan
} // namespace gits
