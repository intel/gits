// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
  void BuildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void SetDevice(ID3D12Device* device, ObjectKey deviceKey) {
    m_Device = device;
    m_DeviceKey = deviceKey;
  }
  void RestoreAccelerationStructures();
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void DestroyResource(ObjectKey resourceKey);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;

  struct AccelerationStructure {
    CommandKey CallKey{};
    ObjectKey Key{};
    unsigned Offset{};
    D3D12_GPU_VIRTUAL_ADDRESS Address{};
  };
  using AccelerationStructures = std::map<D3D12_GPU_VIRTUAL_ADDRESS, AccelerationStructure>;
  std::map<ObjectKey, AccelerationStructures> m_AccelerationStructuresByCommandList;
  AccelerationStructures m_AccelerationStructures;

  std::unordered_map<ObjectKey, std::unordered_set<D3D12_GPU_VIRTUAL_ADDRESS>>
      m_AccelerationStructuresByResource;

  bool m_SerializeMode{};
  ID3D12Device* m_Device{};
  ID3D12CommandQueue* m_CommandQueue{};
  ID3D12CommandAllocator* m_CommandAllocator{};
  ID3D12GraphicsCommandList4* m_CommandList{};
  ID3D12Fence* m_Fence{};
  UINT64 m_CurrentFenceValue{};
  ObjectKey m_DeviceKey{};
  ObjectKey m_CommandQueueKey{};
  ObjectKey m_CommandAllocatorKey{};
  ObjectKey m_CommandListKey{};
  ObjectKey m_FenceKey{};
  UINT64 m_RecordedFenceValue{};
};

} // namespace DirectX
} // namespace gits
