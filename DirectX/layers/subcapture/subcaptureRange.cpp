// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRange.h"

#include <string>

namespace gits {
namespace DirectX {

SubcaptureRange::SubcaptureRange() {
  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      config.directx.features.subcapture.executionSerialization) {
    return;
  }

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      startFrame_ = std::stoi(frames.substr(0, pos));
      endFrame_ = std::stoi(frames.substr(pos + 1));
    } else {
      startFrame_ = std::stoi(frames);
      endFrame_ = startFrame_;
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Configurator::Get().directx.features.subcapture.frames + "'");
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
  } else if (startFrame_ == 1) {
    trimmingMode_ = true;
    inFrameRange_ = true;
  }
}

void SubcaptureRange::frameEnd(bool stateRestore) {
  executionCount_ = 0;
  zeroOrFirstFrame_ = false;

  if (!stateRestore) {
    ++currentFrame_;
  }

  if (currentFrame_ >= startFrame_ && currentFrame_ <= endFrame_) {
    inFrameRange_ = true;
  } else {
    inFrameRange_ = false;
  }
}

bool SubcaptureRange::isFrameRangeStart(bool stateRestore) {
  if (stateRestore) {
    return false;
  }
  return currentFrame_ == startFrame_ - 1;
}

void SubcaptureRange::executionStart() {
  ++executionCount_;
  insideExecution_ = true;
}

void SubcaptureRange::executionEnd() {
  insideExecution_ = false;
}

bool SubcaptureRange::isExecutionRangeStart() {
  if (zeroOrFirstFrame_) {
    return false;
  }
  if (!executionRangeStart_ || !inFrameRange_) {
    return false;
  }
  return executionCount_ == executionRangeStart_ - 1;
}

bool SubcaptureRange::inRange() {
  if (zeroOrFirstFrame_ && !trimmingMode_) {
    return false;
  }
  if (!executionRangeStart_ || !inFrameRange_) {
    return inFrameRange_;
  }
  if (!insideExecution_ && executionCount_ == executionRangeEnd_) {
    return false;
  }
  return executionCount_ >= executionRangeStart_ && executionCount_ <= executionRangeEnd_;
}

bool SubcaptureRange::commandListSubcapture() {
  return executionRangeStart_;
}

} // namespace DirectX
} // namespace gits
