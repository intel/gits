// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRecorder.h"
#include "commandsAuto.h"

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <memory>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresSerializeService {
public:
  AccelerationStructuresSerializeService(StateTrackingService& stateService,
                                         SubcaptureRecorder& recorder);
  void buildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command);
  void copyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command);
  void setDevice(ID3D12Device* device, unsigned deviceKey) {
    device_ = device;
    deviceKey_ = deviceKey;
  }
  void restoreAccelerationStructures();
  void executeCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& command);
  void destroyResource(unsigned resourceKey);

private:
  StateTrackingService& stateService_;
  SubcaptureRecorder& recorder_;

  struct AccelerationStructure {
    unsigned callKey;
    unsigned key;
    unsigned offset;
    D3D12_GPU_VIRTUAL_ADDRESS address;
  };
  using AccelerationStructures = std::map<D3D12_GPU_VIRTUAL_ADDRESS, AccelerationStructure>;
  std::map<unsigned, AccelerationStructures> accelerationStructuresByCommandList_;
  AccelerationStructures accelerationStructures_;

  std::unordered_map<unsigned, std::unordered_set<D3D12_GPU_VIRTUAL_ADDRESS>>
      accelerationStructuresByResource_;

  bool serializeMode_{};
  ID3D12Device* device_{};
  ID3D12CommandQueue* commandQueue_{};
  ID3D12CommandAllocator* commandAllocator_{};
  ID3D12GraphicsCommandList4* commandList_{};
  ID3D12Fence* fence_{};
  UINT64 currentFenceValue_{};
  unsigned deviceKey_{};
  unsigned commandQueueKey_{};
  unsigned commandAllocatorKey_{};
  unsigned commandListKey_{};
  unsigned fenceKey_{};
  UINT64 recordedFenceValue_{};
};

} // namespace DirectX
} // namespace gits
