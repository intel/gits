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

#include <fstream>
#include <string>
#include <filesystem>

#include "fastOStream.h"

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

  const std::filesystem::path filepathBase = filepath.string() + "_tracefile";
  const std::filesystem::path filepathBasePre = filepath.string() + "_tracefile_pre";
  std::string fileNum;
  const std::string fileExt = ".txt";

  for (int i = 1;; ++i) {
    if (!std::filesystem::exists(filepathBase.string() + fileNum + fileExt)) {
      break;
    }
    fileNum = std::to_string(i);
  }
  const std::string finalOutputPath = filepathBase.string() + fileNum + fileExt;
  const std::string finalOutputPathPre = filepathBasePre.string() + fileNum + fileExt;

  const auto& configCommon = Configurator::Get().common;

  if (configCommon.shared.trace.enabled) {

    m_TraceStream = std::make_unique<FastOFileStream>();
    m_TraceStreamPre = std::make_unique<FastOFileStream>();

    if (configCommon.shared.trace.print.postCalls) {
      if (configCommon.shared.trace.flushMethod == "ipc") {
        m_TraceStream = std::make_unique<FastOStringStream>(finalOutputPath);
      } else {
        m_TraceStream = std::make_unique<FastOFileStream>(finalOutputPath);
      }
    }
    if (configCommon.shared.trace.print.preCalls) {
      if (configCommon.shared.trace.flushMethod == "ipc") {
        m_TraceStreamPre = std::make_unique<FastOStringStream>(finalOutputPathPre);
      } else {
        m_TraceStreamPre = std::make_unique<FastOFileStream>(finalOutputPathPre);
      }
    }

    m_TraceLayer = std::make_unique<TraceLayer>(*m_TraceStreamPre, *m_TraceStream, m_TraceMutex,
                                                configCommon.shared.trace.flushMethod != "off");
  }

  // Log messages with LogLevel::TRACE to trace files
  auto traceMsg = [this](const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LogMessage>(m);
    if (!msg || msg->getLevel() != LogLevel::TRACE) {
      return;
    }

    std::lock_guard<std::mutex> lock(m_TraceMutex);

    auto* stream = m_TraceStream.get();
    auto* streamPre = m_TraceStreamPre.get();
    if (stream && stream->IsOpen()) {
      *stream << msg->getText() << '\n';
    }
    if (streamPre && streamPre->IsOpen()) {
      *streamPre << msg->getText() << '\n';
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
