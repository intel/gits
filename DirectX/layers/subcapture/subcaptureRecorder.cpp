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
#include "log2.h"

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

void SubcaptureRecorder::record(CToken* token) {
  CRecorder::Instance().Schedule(token);
}

void SubcaptureRecorder::frameEnd() {
  CRecorder::Instance().FrameEnd();
}

} // namespace DirectX
} // namespace gits
