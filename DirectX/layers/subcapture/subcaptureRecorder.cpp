// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "directXApiIfaceSubcapture.h"
#include "gits.h"

#include <filesystem>
#include <string>

namespace gits {
namespace DirectX {

SubcaptureRecorder::SubcaptureRecorder() {

  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      config.directx.features.subcapture.executionSerialization) {
    return;
  }

  std::string commandListExecutions = config.directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    size_t pos = commandListExecutions.find("-");
    if (pos != std::string::npos) {
      executionRangeStart_ = std::stoi(commandListExecutions.substr(0, pos));
      executionRangeEnd_ = std::stoi(commandListExecutions.substr(pos + 1));
    } else {
      executionRangeStart_ = executionRangeEnd_ = std::stoi(commandListExecutions);
    }

    std::filesystem::path subcapturePath = config.common.player.subcapturePath;
    std::string path = subcapturePath.parent_path().string();
    path += "/" + config.common.player.streamDir.filename().string();
    path += "_frames_" + config.directx.features.subcapture.frames;
    path += "_executions_" + commandListExecutions;
    const_cast<std::filesystem::path&>(config.common.player.subcapturePath) = path;
  }

  CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<ApisIface::Api3d>(new DirectXApiIfaceSubcapture()));
  CRecorder::Instance();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  try {
    if (isRunning()) {
      Log(ERR) << "Subcapture recorder terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("SubcaptureRecorder::~SubcaptureRecorder");
  }
}

void SubcaptureRecorder::record(CToken* token) {
  CRecorder::Instance().Schedule(token);
}

void SubcaptureRecorder::frameEnd() {
  executionCount_ = 0;
  CRecorder::Instance().FrameEnd();
}

void SubcaptureRecorder::executionStart() {
  ++executionCount_;
  insideExecution_ = true;
}

void SubcaptureRecorder::executionEnd() {
  insideExecution_ = false;
}

bool SubcaptureRecorder::isExecutionRangeStart() {
  if (!executionRangeStart_ || !CRecorder::Instance().Running()) {
    return false;
  }
  return executionCount_ == executionRangeStart_ - 1;
}

bool SubcaptureRecorder::isRunning() {
  bool frameRunning = CRecorder::Instance().Running();
  if (!executionRangeStart_ || !frameRunning) {
    return frameRunning;
  }
  if (!insideExecution_) {
    return false;
  }
  return executionCount_ >= executionRangeStart_ && executionCount_ <= executionRangeEnd_;
}

} // namespace DirectX
} // namespace gits
