// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "orderingRecorder.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>

namespace gits {
namespace stream {

// Recording state shared by the recorders of all APIs in a process. Commands of every API are
// written to one stream in the order of one command key sequence, so commands that synchronize
// work between APIs replay in their recorded order.
class RecordingSession {
public:
  // The session is never destroyed: the recorder closes the stream on program exit, and recorders
  // of other APIs may still record while static objects are destroyed.
  static RecordingSession& Get();
  RecordingSession(const RecordingSession&) = delete;
  RecordingSession& operator=(const RecordingSession&) = delete;

  OrderingRecorder& GetOrderingRecorder() {
    return m_Recorder;
  }

  // Reserves `count` consecutive keys and returns the first one. Keys start at 1.
  uint64_t CreateCommandKeyRange(uint64_t count) {
    return m_CommandKey.fetch_add(count, std::memory_order_relaxed) + 1;
  }
  uint64_t CreateCommandKey() {
    return CreateCommandKeyRange(1);
  }

  // The publisher records the host writes to mapped memory that another API may import, and makes
  // them visible to that API. An empty function removes the publisher.
  void SetHostWritePublisher(std::function<void()> publisher);
  // Called by the recorder of an API that imports memory, before a command that may read it and
  // before the key of that command is reserved.
  void PublishHostWrites();

private:
  RecordingSession() = default;

  OrderingRecorder m_Recorder;
  std::atomic<uint64_t> m_CommandKey{0};
  std::atomic<bool> m_HasHostWritePublisher{false};
  std::mutex m_HostWritePublisherMutex;
  std::function<void()> m_HostWritePublisher;
};

} // namespace stream
} // namespace gits
