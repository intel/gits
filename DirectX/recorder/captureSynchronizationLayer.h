// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      : Layer("CaptureSynchronization"), manager_(manager) {}

  void pre(ID3D12FenceSignalCommand& command) override;
  void post(ID3D12FenceSignalCommand& command) override;
  void pre(ID3D12CommandQueueSignalCommand& command) override;
  void post(ID3D12CommandQueueSignalCommand& command) override;
  void pre(ID3D12CommandQueueWaitCommand& command) override;
  void post(ID3D12CommandQueueWaitCommand& command) override;
  void pre(ID3D12FenceGetCompletedValueCommand& command) override;
  void post(ID3D12FenceGetCompletedValueCommand& command) override;

private:
  CaptureManager& manager_;
};

} // namespace DirectX
} // namespace gits
