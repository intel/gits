// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "directXApi.h"

#include <filesystem>

namespace gits {
namespace DirectX {

SubcaptureRecorder::SubcaptureRecorder() {

  const Config& config = Config::Get();
  if (!config.directx.features.subcapture.enabled) {
    return;
  }
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new DirectXApi()));
  CRecorder::Instance();
  initialized_ = true;
}

void SubcaptureRecorder::record(CToken* token) {
  std::lock_guard<std::mutex> lock(mutex_);
  CRecorder::Instance().Schedule(token);
}

void SubcaptureRecorder::frameEnd() {
  std::lock_guard<std::mutex> lock(mutex_);
  CRecorder::Instance().FrameEnd();
}

bool SubcaptureRecorder::isRunning() {
  return CRecorder::Instance().Running();
}

} // namespace DirectX
} // namespace gits
