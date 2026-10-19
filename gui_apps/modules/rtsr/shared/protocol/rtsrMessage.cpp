// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/protocol/rtsrMessage.h"

namespace gits::rtsr {

void MessageHandler::RegisterCallback(RTSR_MESSAGE_TYPE type, MessageCallback callback) {
  m_Callbacks[type] = std::move(callback);
}

bool MessageHandler::HandleMessage(const Message& message) const {
  const auto it = m_Callbacks.find(message.Header.Type);
  if (it == m_Callbacks.end()) {
    return false;
  }
  it->second(message);
  return true;
}

void RequestHandler::RegisterCallback(RTSR_MESSAGE_TYPE type, RequestCallback callback) {
  m_Callbacks[type] = std::move(callback);
}

std::optional<Message> RequestHandler::HandleRequest(const Message& message) const {
  const auto it = m_Callbacks.find(message.Header.Type);
  if (it == m_Callbacks.end()) {
    return std::nullopt;
  }
  return it->second(message);
}

} // namespace gits::rtsr
