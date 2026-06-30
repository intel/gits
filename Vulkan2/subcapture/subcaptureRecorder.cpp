// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "configurator.h"
#include "gits.h"
#include "log.h"
#include "messageBus.h"

#include <filesystem>
#include <memory>

namespace gits {
namespace vulkan {

SubcaptureRecorder::SubcaptureRecorder(bool enabled) {
  const auto& cfg = Configurator::Get();

  if (!enabled || !cfg.common.features.subcapture.enabled ||
      cfg.common.features.subcapture.frames.empty()) {
    return;
  }

  // Configurator::PrepareSubcapturePath() has already been called by playerUtils.cpp
  // and resolved %f% and %r% in common.player.subcapturePath using the common
  // subcapture frames string (see configurator.cpp).  Use the prepared path directly.
  m_StreamPath = cfg.common.player.subcapturePath;

  m_Writer = std::make_unique<stream::StreamWriter>(m_StreamPath,
                                                    cfg.common.features.subcapture.compressionType);
  LOG_INFO << "Vulkan2 subcapture: output stream opened at " << m_StreamPath.string();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  FinishRecording();
}

void SubcaptureRecorder::Record(const stream::CommandSerializer& serializer) {
  if (!m_Writer) {
    return;
  }
  m_Writer->Record(serializer);
}

void SubcaptureRecorder::FinishRecording() {
  if (m_Finished || !m_Writer) {
    return;
  }
  m_Writer->Close();
  CGits::Instance().GetMessageBus().publish(
      {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
      std::make_shared<StreamSavedMessage>(m_StreamPath.string()));
  m_Finished = true;
  LOG_INFO << "Vulkan2 subcapture: recording finished";
}

} // namespace vulkan
} // namespace gits
