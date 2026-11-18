// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingSession.h"

namespace gits {
namespace stream {

RecordingSession& RecordingSession::Get() {
  static RecordingSession* session = new RecordingSession();
  return *session;
}

} // namespace stream
} // namespace gits
