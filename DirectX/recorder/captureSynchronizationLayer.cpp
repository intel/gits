// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureSynchronizationLayer.h"
#include "captureManager.h"

namespace gits {
namespace DirectX {

void CaptureSynchronizationLayer::pre(ID3D12FenceSignalCommand& command) {
  manager_.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::post(ID3D12FenceSignalCommand& command) {
  manager_.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::pre(ID3D12CommandQueueSignalCommand& command) {
  manager_.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::post(ID3D12CommandQueueSignalCommand& command) {
  manager_.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::pre(ID3D12CommandQueueWaitCommand& command) {
  manager_.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::post(ID3D12CommandQueueWaitCommand& command) {
  manager_.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::pre(ID3D12FenceGetCompletedValueCommand& command) {
  manager_.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::post(ID3D12FenceGetCompletedValueCommand& command) {
  manager_.getFenceService().getGlobalMutex().unlock();
}

} // namespace DirectX
} // namespace gits
