// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationFactory.h"

namespace gits {
namespace DirectX {

ExecutionSerializationFactory::ExecutionSerializationFactory() {

  if (!Configurator::Get().directx.features.subcapture.enabled ||
      !Configurator::Get().directx.features.subcapture.executionSerialization) {
    return;
  }
  recorder_ = std::make_unique<ExecutionSerializationRecorder>();
  executionSerializationLayer_ = std::make_unique<ExecutionSerializationLayer>(*recorder_);
}

} // namespace DirectX
} // namespace gits
