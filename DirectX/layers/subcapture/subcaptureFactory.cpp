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
#include "directXApi.h"
#include "directStorageResourcesLayer.h"

namespace gits {
namespace DirectX {

SubcaptureFactory::SubcaptureFactory() {

  if (!Config::Get().directx.features.subcapture.enabled) {
    return;
  }

  const std::string& frames = Config::Get().directx.features.subcapture.frames;
  try {
    int startFrame = std::stoi(frames);
    if (startFrame == 1) {
      Log(ERR) << "Subcapture from frame 1 is not supported";
      exit(EXIT_FAILURE);
    }
  } catch (...) {
    Log(ERR) << "Invalid subcapture range: '" + Config::Get().directx.features.subcapture.frames +
                    "'";
    exit(EXIT_FAILURE);
  }

  std::ifstream analysisFile(AnalyzerLayer::getAnalysisFileName());
  if (analysisFile) {
    recorder_ = std::make_unique<SubcaptureRecorder>();
    stateTrackingLayer_ = std::make_unique<StateTrackingLayer>(*recorder_);
    recordingLayer_ = std::make_unique<RecordingLayer>(*recorder_);
    directStorageResourcesLayer_ = std::make_unique<DirectStorageResourcesLayer>();
  } else {
    const_cast<gits::Config&>(Config::Get()).directx.features.subcapture.enabled = false;
    analyzerLayer_ = std::make_unique<AnalyzerLayer>();
    Log(INFO) << "SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.";
  }
}

} // namespace DirectX
} // namespace gits
