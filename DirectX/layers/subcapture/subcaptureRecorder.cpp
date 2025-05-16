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
}

SubcaptureRecorder::~SubcaptureRecorder() {
  if (isRunning()) {
    Log(ERR) << "Subcapture recorder terminated prematurely";
  }
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
