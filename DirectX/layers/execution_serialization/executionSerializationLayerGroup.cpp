// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationLayerGroup.h"
#include "log.h"
#include "configurator.h"
#include "executionSerializationLayerAuto.h"

namespace gits {
namespace DirectX {

void ExecutionSerializationLayerGroup::loadLayers() {

  if (!Configurator::Get().directx.features.subcapture.enabled ||
      !Configurator::Get().directx.features.subcapture.executionSerialization) {
    return;
  }

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  try {
    int startFrame = std::stoi(frames);
    if (startFrame != 1) {
      LOG_ERROR << "Execution serialization must start from frame 1";
      exit(EXIT_FAILURE);
    }
  } catch (...) {
    LOG_ERROR << "Invalid execution serialization range: '" +
                     Configurator::Get().directx.features.subcapture.frames + "'";
    exit(EXIT_FAILURE);
  }

  recorder_ = std::make_unique<ExecutionSerializationRecorder>();
  addLayer(std::make_unique<ExecutionSerializationLayer>(*recorder_));
}

} // namespace DirectX
} // namespace gits
