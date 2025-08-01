// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureFactory.h"
#include "stateTrackingLayer.h"
#include "recordingLayerAuto.h"
#include "commandPreservationLayer.h"
#include "directStorageResourcesLayer.h"
#include "analyzerResults.h"
#include "gits.h"

namespace gits {
namespace DirectX {

SubcaptureFactory::SubcaptureFactory() {

  if (!Configurator::Get().directx.features.subcapture.enabled ||
      Configurator::Get().directx.features.subcapture.executionSerialization) {
    return;
  }

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  const std::string& executions =
      Configurator::Get().directx.features.subcapture.commandListExecutions;
  try {
    if (executions.empty()) {
      int startFrame = std::stoi(frames);
      if (startFrame == 1) {
        Log(ERR) << "Subcapture from frame 1 is not supported";
        exit(EXIT_FAILURE);
      }
    } else {
      if (frames.find("-") != std::string::npos) {
        Log(ERR) << "Subcapture of command list executions must have one frame range";
        exit(EXIT_FAILURE);
      }
    }
  } catch (...) {
    Log(ERR) << "Invalid subcapture range: '" +
                    Configurator::Get().directx.features.subcapture.frames + "'";
    exit(EXIT_FAILURE);
  }

  subcaptureRange_ = std::make_unique<SubcaptureRange>();

  if (AnalyzerResults::isAnalysis()) {
    recorder_ = std::make_unique<SubcaptureRecorder>();
    stateTrackingLayer_ = std::make_unique<StateTrackingLayer>(*recorder_, *subcaptureRange_);
    recordingLayer_ = std::make_unique<RecordingLayer>(*recorder_, *subcaptureRange_);
    commandPreservationLayer_ = std::make_unique<CommandPreservationLayer>();
    directStorageResourcesLayer_ = std::make_unique<DirectStorageResourcesLayer>();
  } else {
    const_cast<gits::Configuration&>(Configurator::Get()).directx.features.subcapture.enabled =
        false;
    AnalyzerLayer* analyzerLayer = new AnalyzerLayer(*subcaptureRange_);
    analyzerLayer_.reset(analyzerLayer);
    objectUsageNotifier_ = analyzerLayer;
    Log(INFO) << "SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.";
  }
}

} // namespace DirectX
} // namespace gits
