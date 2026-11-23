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
#include "messageBus.h"

namespace gits {
namespace vulkan {

void LogVkErrorLayer::OnDeviceLost(const char* commandName, uint64_t commandKey) {
  if (!m_IsPlayer || m_DeviceLostHandled.exchange(true)) {
    return;
  }

  LOG_ERROR << "Device lost in " << commandName << " at command key " << commandKey
            << ". The device cannot be used again, so playback stops here - every command "
               "after this point would only report the same failure.";

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_CLOSE_PLAYER},
                            std::make_shared<ProgramMessage>());
}

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
    if (command.m_Return.Value == VK_ERROR_DEVICE_LOST) {
      OnDeviceLost("${command.name}", command.m_Key);
    }
  }
}

% if define:
#endif
% endif
% endif
% endfor

} // namespace vulkan
} // namespace gits
