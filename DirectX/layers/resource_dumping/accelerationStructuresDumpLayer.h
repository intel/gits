// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "accelerationStructuresDump.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

class AccelerationStructuresDumpLayer : public Layer {
public:
  AccelerationStructuresDumpLayer();
  void pre(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void pre(ID3D12DeviceCreateCommandListCommand& c) override;
  void pre(ID3D12Device4CreateCommandList1Command& c) override;
  void pre(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;

private:
  void extractKeys(const std::string& keyString, std::unordered_set<unsigned>& keySet);

private:
  std::wstring dumpPath_;
  AccelerationStructuresDump accelerationStructuresDump_;
  std::unordered_set<unsigned> callKeys_;
};

} // namespace DirectX
} // namespace gits
