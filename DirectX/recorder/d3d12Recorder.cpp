// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "d3d12Recorder.h"
#include "captureManager.h"
#include "log.h"

namespace gits {
namespace DirectX {

void RecorderWrapper::ExchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                                 DXGIDispatchTable& wrapperTable) {
  CaptureManager::get().exchangeDXGIDispatchTables(systemTable, wrapperTable);
}

void RecorderWrapper::ExchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                                  D3D12DispatchTable& wrapperTable) {
  CaptureManager::get().exchangeD3D12DispatchTables(systemTable, wrapperTable);
}

} // namespace DirectX
} // namespace gits

std::unique_ptr<gits::DirectX::RecorderWrapper> g_recorderWrapper;

gits::DirectX::IRecorderWrapper* STDCALL GITSRecorderD3D12() {
  if (g_recorderWrapper == nullptr) {
    try {
      g_recorderWrapper = std::make_unique<gits::DirectX::RecorderWrapper>();
    } catch (const std::exception& ex) {
      LOG_ERROR << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return g_recorderWrapper.get();
}
