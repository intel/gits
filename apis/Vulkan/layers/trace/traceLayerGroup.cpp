// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerGroup.h"
#include "traceLayerAuto.h"
#include "configurator.h"
#include "streamHeader.h"
#include "messageBus.h"

#include <filesystem>
#include <string>

#include "fastOStream.h"
#include "traceFiles.h"

namespace gits {
namespace vulkan {

TraceLayerGroup::TraceLayerGroup() {
  std::filesystem::path outputTracePath = Configurator::Get().common.player.outputTracePath;

  if (!outputTracePath.empty() && !std::filesystem::exists(outputTracePath)) {
    std::filesystem::create_directory(outputTracePath);
  }

  if (outputTracePath.empty()) {
    outputTracePath = std::filesystem::current_path();
  }

  std::filesystem::path filepath;
  if (Configurator::IsPlayer()) {
    const std::string streamDir = Configurator::Get().common.player.streamDir.filename().string();
    filepath = outputTracePath / streamDir;
  } else {
    std::filesystem::path appName = stream::StreamHeader::Get().GetApplicationName();
    appName.replace_extension();
    filepath = outputTracePath / appName;
  }

  const auto& configCommon = Configurator::Get().common;
  if (!configCommon.shared.trace.enabled) {
    return;
  }

  m_TraceFiles = TraceFiles::Acquire(filepath);
  m_TraceLayer =
      std::make_unique<TraceLayer>(m_TraceFiles->Pre(), m_TraceFiles->Post(), m_TraceFiles->Mutex(),
                                   configCommon.shared.trace.flushMethod != "off");

  // Log messages with LogLevel::TRACE to trace files
  auto traceMsg = [weakFiles = std::weak_ptr<TraceFiles>(m_TraceFiles)](const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LogMessage>(m);
    if (!msg || msg->getLevel() != LogLevel::TRACE) {
      return;
    }
    const auto files = weakFiles.lock();
    if (!files) {
      return;
    }

    std::lock_guard<std::mutex> lock(files->Mutex());
    if (files->Post().IsOpen()) {
      files->Post() << msg->getText() << '\n';
    }
    if (files->Pre().IsOpen()) {
      files->Pre() << msg->getText() << '\n';
    }
  };
  gits::MessageBus& msgBus = gits::MessageBus::get();
  msgBus.subscribe({PUBLISHER_RECORDER, TOPIC_LOG},
                   [traceMsg](Topic t, const MessagePtr& m) { traceMsg(m); });
  msgBus.subscribe({PUBLISHER_PLAYER, TOPIC_LOG},
                   [traceMsg](Topic t, const MessagePtr& m) { traceMsg(m); });
  msgBus.subscribe({PUBLISHER_PLUGIN, TOPIC_LOG},
                   [traceMsg](Topic t, const MessagePtr& m) { traceMsg(m); });
}

TraceLayerGroup::~TraceLayerGroup() {}

} // namespace vulkan
} // namespace gits
