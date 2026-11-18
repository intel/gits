// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingSession.h"

#include <utility>

namespace gits {
namespace stream {

RecordingSession& RecordingSession::Get() {
  static RecordingSession* session = new RecordingSession();
  return *session;
}

void RecordingSession::SetHostWritePublisher(std::function<void()> publisher) {
  std::lock_guard<std::mutex> lock(m_HostWritePublisherMutex);
  m_HostWritePublisher = std::move(publisher);
  m_HasHostWritePublisher.store(static_cast<bool>(m_HostWritePublisher), std::memory_order_release);
}

void RecordingSession::PublishHostWrites() {
  if (!m_HasHostWritePublisher.load(std::memory_order_acquire)) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_HostWritePublisherMutex);
  if (m_HostWritePublisher) {
    m_HostWritePublisher();
  }
}

} // namespace stream
} // namespace gits
