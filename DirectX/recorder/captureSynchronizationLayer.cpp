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

void CaptureSynchronizationLayer::Pre(ID3D12FenceSignalCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::Post(ID3D12FenceSignalCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::Pre(ID3D12CommandQueueSignalCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::Post(ID3D12CommandQueueSignalCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::Pre(ID3D12CommandQueueWaitCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::Post(ID3D12CommandQueueWaitCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().unlock();
}

void CaptureSynchronizationLayer::Pre(ID3D12FenceGetCompletedValueCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().lock();
}

void CaptureSynchronizationLayer::Post(ID3D12FenceGetCompletedValueCommand& command) {
  m_Manager.getFenceService().getGlobalMutex().unlock();
}

} // namespace DirectX
} // namespace gits
