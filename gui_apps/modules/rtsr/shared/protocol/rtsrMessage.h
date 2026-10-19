// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace gits::rtsr {

// Wire framing: an 8 byte header followed by Len bytes of cereal binary payload.
// The header is written as raw bytes, so its layout is part of the protocol.
struct MessageHeader {
  RTSR_MESSAGE_TYPE Type{};
  int32_t Len{};
};
static_assert(sizeof(MessageHeader) == 8, "The message header layout is on the wire");

struct Message {
  MessageHeader Header{};
  std::vector<uint8_t> Data;
};

// The protocol carries no length bound of its own, so an implausible Len is
// either a desynchronised stream or a hostile peer. Reject rather than attempt a
// multi-gigabyte allocation. A single pipeline message carries every shader blob
// of one PSO, which is orders of magnitude below this.
constexpr int32_t MAX_MESSAGE_BYTES = 256 * 1024 * 1024;

inline bool IsPlausibleLength(int32_t length) {
  return length >= 0 && length <= MAX_MESSAGE_BYTES;
}

// Sink for diagnostics. The transport is shared between the plugin and the GUI,
// so it does not depend on either one's logging.
using LogSink = std::function<void(std::string_view)>;

using MessageCallback = std::function<void(const Message&)>;
using RequestCallback = std::function<Message(const Message&)>;

// Both handlers report an unregistered message type instead of throwing. RTSR
// throws from the receive thread, which has no handler and therefore terminates
// the whole traced application on a protocol mismatch.
class MessageHandler {
public:
  void RegisterCallback(RTSR_MESSAGE_TYPE type, MessageCallback callback);
  bool HandleMessage(const Message& message) const;

private:
  std::map<RTSR_MESSAGE_TYPE, MessageCallback> m_Callbacks;
};

class RequestHandler {
public:
  void RegisterCallback(RTSR_MESSAGE_TYPE type, RequestCallback callback);
  std::optional<Message> HandleRequest(const Message& message) const;

private:
  std::map<RTSR_MESSAGE_TYPE, RequestCallback> m_Callbacks;
};

} // namespace gits::rtsr
