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

namespace gits::opengl {

SubcaptureRecorder::SubcaptureRecorder() {
  const auto& cfg = Configurator::Get();
  // Configurator::PrepareSubcapturePath() has already expanded the path's placeholders.
  m_StreamPath = cfg.common.player.subcapturePath;
  m_Writer = std::make_unique<stream::StreamWriter>(m_StreamPath,
                                                    cfg.common.player.subcapture.compressionType);
  LOG_INFO << "Subcapture: recording to " << m_StreamPath.string();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  try {
    FinishRecording();
  } catch (...) {
    topmost_exception_handler("SubcaptureRecorder::~SubcaptureRecorder");
  }
}

void SubcaptureRecorder::Record(const stream::CommandSerializer& serializer) {
  if (m_Writer) {
    m_Writer->Record(serializer);
  }
}

void SubcaptureRecorder::FinishRecording() {
  if (!m_Writer) {
    return;
  }
  m_Writer->Close();
  m_Writer.reset();
  gits::MessageBus::get().publish({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                  std::make_shared<StreamSavedMessage>(m_StreamPath.string()));
  LOG_INFO << "Subcapture: recording finished";
}

} // namespace gits::opengl
