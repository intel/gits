// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace DirectX {

class CaptureManager;

class CaptureSynchronizationLayer : public Layer {
public:
  CaptureSynchronizationLayer(CaptureManager& manager)
      : Layer("CaptureSynchronization"), m_Manager(manager) {}

  void Pre(ID3D12FenceSignalCommand& command) override;
  void Post(ID3D12FenceSignalCommand& command) override;
  void Pre(ID3D12CommandQueueSignalCommand& command) override;
  void Post(ID3D12CommandQueueSignalCommand& command) override;
  void Pre(ID3D12CommandQueueWaitCommand& command) override;
  void Post(ID3D12CommandQueueWaitCommand& command) override;
  void Pre(ID3D12FenceGetCompletedValueCommand& command) override;
  void Post(ID3D12FenceGetCompletedValueCommand& command) override;

private:
  CaptureManager& m_Manager;
};

} // namespace DirectX
} // namespace gits
