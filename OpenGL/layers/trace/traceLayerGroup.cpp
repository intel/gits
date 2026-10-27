// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerGroup.h"

#include "configurator.h"
#include "traceLayerAuto.h"

#include <filesystem>
#include <string>

namespace gits::opengl {

TraceLayerGroup::TraceLayerGroup() {
  const auto& config = Configurator::Get().common;
  if (!config.shared.trace.enabled || !config.shared.trace.print.preCalls) {
    return;
  }

  std::filesystem::path directory = config.player.outputTracePath;
  if (directory.empty()) {
    directory = std::filesystem::current_path();
  }
  std::filesystem::create_directories(directory);

  const auto base = directory / config.player.streamDir.filename();
  const std::string stem = base.string() + "_tracefile";
  std::string suffix;
  for (unsigned index = 1; std::filesystem::exists(stem + suffix + ".txt") ||
                           std::filesystem::exists(stem + "_pre" + suffix + ".txt");
       ++index) {
    suffix = std::to_string(index);
  }
  const std::string path = stem + "_pre" + suffix + ".txt";
  m_Flush = config.shared.trace.flushMethod != "off";
  if (config.shared.trace.flushMethod == "ipc") {
    m_PreStream = std::make_unique<FastOStringStream>(path);
  } else {
    m_PreStream = std::make_unique<FastOFileStream>(path);
  }

  // TODO: Mirror WARNING, ERROR, and TRACE via a removable log sink; lock m_Mutex and avoid
  // recursive trace writes while keeping the normal log.
}

TraceLayerGroup::~TraceLayerGroup() = default;

bool TraceLayerGroup::IsEnabled() const {
  return m_PreStream && m_PreStream->IsOpen();
}

std::unique_ptr<TraceLayer> TraceLayerGroup::CreateLayer() {
  return std::make_unique<TraceLayer>(*m_PreStream, m_Mutex, m_Flush);
}

} // namespace gits::opengl
