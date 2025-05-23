// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationRecorder.h"
#include "directXApiIfaceExecutionSerialization.h"

#include <filesystem>

namespace gits {
namespace DirectX {

ExecutionSerializationRecorder::ExecutionSerializationRecorder() {

  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      !config.directx.features.subcapture.executionSerialization) {
    return;
  }

  std::string subcapturePath = config.common.player.subcapturePath.string();
  subcapturePath += "_serialized";
  const_cast<std::filesystem::path&>(config.common.player.subcapturePath) = subcapturePath;

  CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<ApisIface::Api3d>(new DirectXApiIfaceExecutionSerialization()));
  CRecorder::Instance();
}

void ExecutionSerializationRecorder::record(CToken* token) {
  CRecorder::Instance().Schedule(token);
}

void ExecutionSerializationRecorder::frameEnd() {
  CRecorder::Instance().FrameEnd();
}

bool ExecutionSerializationRecorder::isRunning() {
  return CRecorder::Instance().Running();
}

} // namespace DirectX
} // namespace gits
