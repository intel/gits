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
#include "log.h"
#include "messageBus.h"
#include "subcaptureSession.h"

namespace gits::opengl {

static_assert(stream::SubcaptureSession::STATE_RESTORE_KEY_MASK == STATE_RESTORE_KEY_MASK);

SubcaptureRecorder::SubcaptureRecorder() {
  const auto& cfg = Configurator::Get();
  auto& session = stream::SubcaptureSession::Get();
  // Configurator::PrepareSubcapturePath() has already expanded the path's placeholders.
  session.Open(cfg.common.player.subcapturePath, cfg.common.player.subcapture.compressionType);
  LOG_INFO << "Subcapture: recording to " << session.GetPath().string();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  try {
    FinishRecording();
  } catch (...) {
    topmost_exception_handler("SubcaptureRecorder::~SubcaptureRecorder");
  }
}

void SubcaptureRecorder::Record(const stream::CommandSerializer& serializer) {
  stream::SubcaptureSession::Get().Record(serializer);
}

void SubcaptureRecorder::FinishRecording() {
  auto& session = stream::SubcaptureSession::Get();
  if (!session.Close()) {
    return;
  }
  gits::MessageBus::get().publish({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                  std::make_shared<StreamSavedMessage>(session.GetPath().string()));
  LOG_INFO << "Subcapture: recording finished";
}

GITSKey SubcaptureRecorder::CreateStateRestoreKey() {
  return stream::SubcaptureSession::Get().CreateStateRestoreKey();
}

GITSKey SubcaptureRecorder::CreateStateRestoreObjectKey() {
  return stream::SubcaptureSession::Get().CreateStateRestoreObjectKey();
}

} // namespace gits::opengl
