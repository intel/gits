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
#include "showExecutionLayer.h"
#include "fastOStream.h"

#include <fstream>
#include <string>
#include <filesystem>

namespace gits {
namespace DirectX {

// Empty constructor and destructor needed due to the FastIO incomplete type in the header
TraceLayerGroup::TraceLayerGroup() {}
TraceLayerGroup::~TraceLayerGroup() {}

void TraceLayerGroup::loadLayers() {
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
  const std::filesystem::path filepathBaseExecution = filepath.string() + "_gpuexecution";
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
  const std::string finalOutputPathExecution = filepathBaseExecution.string() + fileNum + fileExt;

  const auto& configDirectX = Configurator::Get().directx;
  const auto flushMethod = configDirectX.features.trace.flushMethod == "file"
                               ? FastOStringStream::FlushMethod::File
                               : FastOStringStream::FlushMethod::Ipc;

  if (configDirectX.features.trace.enabled) {

    traceStream_ = std::make_unique<FastOFileStream>();
    traceStreamPre_ = std::make_unique<FastOFileStream>();

    if (configDirectX.features.trace.print.postCalls) {
      if (configDirectX.features.trace.flushMethod == "ipc") {
        traceStream_ = std::make_unique<FastOStringStream>(finalOutputPath);
      } else {
        traceStream_ = std::make_unique<FastOFileStream>(finalOutputPath);
      }
    }
    if (configDirectX.features.trace.print.preCalls) {
      if (configDirectX.features.trace.flushMethod == "ipc") {
        traceStreamPre_ = std::make_unique<FastOStringStream>(finalOutputPathPre);
      } else {
        traceStreamPre_ = std::make_unique<FastOFileStream>(finalOutputPathPre);
      }
    }

    addLayer(std::make_unique<TraceLayer>(*traceStreamPre_, *traceStream_, traceMutex_,
                                          configDirectX.features.trace.flushMethod != "off"));

    if (configDirectX.features.trace.print.gpuExecution) {
      showExecutionStream_ =
          std::make_unique<FastOStringStream>(finalOutputPathExecution, flushMethod);
      addLayer(std::make_unique<ShowExecutionLayer>(*showExecutionStream_));
    }
  }

  auto traceMsg = [this](const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LogMessage>(m);
    if (!msg || msg->getLevel() != LogLevel::TRACE) {
      return;
    }

    std::lock_guard<std::mutex> lock(traceMutex_);

    auto* stream = traceStream_.get();
    auto* streamPre = traceStreamPre_.get();
    if (stream && stream->isOpen()) {
      *stream << msg->getText() << '\n';
    }
    if (streamPre && streamPre->isOpen()) {
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

} // namespace DirectX
} // namespace gits
