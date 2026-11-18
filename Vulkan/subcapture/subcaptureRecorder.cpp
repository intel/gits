// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "configurator.h"
#include "exception.h"
#include "gits.h"
#include "log.h"
#include "messageBus.h"
#include "subcaptureSession.h"

#include <memory>

namespace gits {
namespace vulkan {

static_assert(stream::SubcaptureSession::STATE_RESTORE_KEY_MASK == STATE_RESTORE_KEY_MASK);

SubcaptureRecorder::SubcaptureRecorder(bool enabled) {
  const auto& cfg = Configurator::Get();

  if (!enabled || !cfg.common.player.subcapture.enabled ||
      cfg.common.player.subcapture.frames.empty()) {
    return;
  }

  // Configurator::PrepareSubcapturePath() has already been called by playerUtils.cpp
  // and resolved %f% and %r% in common.player.subcapturePath using the common
  // subcapture frames string (see configurator.cpp).  Use the prepared path directly.
  auto& session = stream::SubcaptureSession::Get();
  session.Open(cfg.common.player.subcapturePath, cfg.common.player.subcapture.compressionType);
  m_Enabled = true;
  LOG_INFO << "Vulkan subcapture: output stream opened at " << session.GetPath().string();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  try {
    FinishRecording();
  } catch (...) {
    topmost_exception_handler("SubcaptureRecorder::~SubcaptureRecorder");
  }
}

void SubcaptureRecorder::Record(const stream::CommandSerializer& serializer) {
  if (m_Enabled) {
    stream::SubcaptureSession::Get().Record(serializer);
  }
}

void SubcaptureRecorder::FinishRecording() {
  auto& session = stream::SubcaptureSession::Get();
  if (!m_Enabled || !session.Close()) {
    return;
  }
  CGits::Instance().GetMessageBus().publish(
      {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
      std::make_shared<StreamSavedMessage>(session.GetPath().string()));
  LOG_INFO << "Vulkan subcapture: recording finished";
}

bool SubcaptureRecorder::IsOpen() const {
  return m_Enabled && stream::SubcaptureSession::Get().IsOpen();
}

GITSKey SubcaptureRecorder::CreateStateRestoreKey() {
  return stream::SubcaptureSession::Get().CreateStateRestoreKey();
}

GITSKey SubcaptureRecorder::CreateStateRestoreObjectKey() {
  return stream::SubcaptureSession::Get().CreateStateRestoreObjectKey();
}

} // namespace vulkan
} // namespace gits
