// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerGroup.h"

#include "configurator.h"
#include "traceFiles.h"
#include "traceLayerAuto.h"

#include <filesystem>

namespace gits::opengl {

TraceLayerGroup::TraceLayerGroup() {
  const auto& config = Configurator::Get().common;
  if (!config.shared.trace.enabled) {
    return;
  }

  std::filesystem::path directory = config.player.outputTracePath;
  if (directory.empty()) {
    directory = std::filesystem::current_path();
  }
  std::filesystem::create_directories(directory);

  m_Files = TraceFiles::Acquire(directory / config.player.streamDir.filename());
  m_Flush = config.shared.trace.flushMethod != "off";

  // TODO: Mirror WARNING, ERROR, and TRACE via a removable log sink; lock the trace files mutex
  // and avoid recursive trace writes while keeping the normal log.
}

TraceLayerGroup::~TraceLayerGroup() = default;

bool TraceLayerGroup::TracesPreCalls() const {
  return m_Files && m_Files->Pre().IsOpen();
}

bool TraceLayerGroup::TracesPostCalls() const {
  return m_Files && m_Files->Post().IsOpen();
}

std::unique_ptr<TraceLayer> TraceLayerGroup::CreateLayer() {
  return std::make_unique<TraceLayer>(m_Files->Pre(), m_Files->Post(), m_Files->Mutex(), m_Flush);
}

} // namespace gits::opengl
