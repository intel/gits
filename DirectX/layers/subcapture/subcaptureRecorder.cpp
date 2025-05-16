// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "directXApiIfaceSubcapture.h"

#include <filesystem>
#include <string>

namespace gits {
namespace DirectX {

SubcaptureRecorder::SubcaptureRecorder() {

  if (!Configurator::Get().directx.features.subcapture.enabled ||
      Configurator::Get().directx.features.subcapture.executionSerialization) {
    return;
  }
  CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<ApisIface::Api3d>(new DirectXApiIfaceSubcapture()));
  CRecorder::Instance();

  std::string commandListExecutions =
      Configurator::Get().directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    size_t pos = commandListExecutions.find("-");
    if (pos != std::string::npos) {
      executionRangeStart_ = std::stoi(commandListExecutions.substr(0, pos));
      executionRangeEnd_ = std::stoi(commandListExecutions.substr(pos + 1));
    } else {
      executionRangeStart_ = executionRangeEnd_ = std::stoi(commandListExecutions);
    }
  }
}

SubcaptureRecorder::~SubcaptureRecorder() {
  if (isRunning()) {
    Log(ERR) << "Subcapture recorder terminated prematurely";
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
