// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "subcaptureRecorder.h"
#include "analyzerLayerAuto.h"

#include <memory>

namespace gits {
namespace DirectX {

/*
 * Encapsulates creation logic of subcapture-related Layers.
 * Validates relevant config and decides which layers to create.
 */
class SubcaptureFactory {
public:
  SubcaptureFactory();

  std::unique_ptr<Layer> getStateTrackingLayer() {
    return std::move(stateTrackingLayer_);
  }
  std::unique_ptr<Layer> getRecordingLayer() {
    return std::move(recordingLayer_);
  }
  std::unique_ptr<Layer> getCommandPreservationLayer() {
    return std::move(commandPreservationLayer_);
  }
  std::unique_ptr<Layer> getAnalyzerLayer() {
    return std::move(analyzerLayer_);
  }
  std::unique_ptr<Layer> getDirectStorageResourcesLayer() {
    return std::move(directStorageResourcesLayer_);
  }

private:
  std::unique_ptr<SubcaptureRecorder> recorder_;
  std::unique_ptr<Layer> stateTrackingLayer_;
  std::unique_ptr<Layer> recordingLayer_;
  std::unique_ptr<Layer> commandPreservationLayer_;
  std::unique_ptr<Layer> analyzerLayer_;
  std::unique_ptr<Layer> directStorageResourcesLayer_;
};

} // namespace DirectX
} // namespace gits
