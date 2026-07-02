// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "reservedResourcesService.h"
#include "objectState.h"
#include "accelerationStructuresBufferContentRestore.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"
#include "resourceResidencyService.h"
#include "subcaptureRecorder.h"
#include "hashUtils.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresInputBuffersService {
public:
  AccelerationStructuresInputBuffersService(StateTrackingService& stateService,
                                            ReservedResourcesService& reservedResourcesService,
                                            ResourceStateTracker& resourceStateTracker,
                                            CapturePlayerGpuAddressService& gpuAddressService,
                                            SubcaptureRecorder& recorder)
      : m_StateService(stateService),
        m_BufferContentRestore(stateService),
        m_ReservedResourcesService(reservedResourcesService),
        m_ResourceStateTracker(resourceStateTracker),
        m_GpuAddressService(gpuAddressService),
        m_Recorder(recorder) {}

  void SetDeviceKey(unsigned deviceKey);
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

  void StoreBufferRegion(unsigned bufferKey, unsigned bufferOffset, unsigned bufferSize);
  void StoreBuffers(unsigned commandKey, ID3D12GraphicsCommandList* commandList);

  void RestoreBuffersInitialization(std::vector<unsigned>& commandKeys);
  void MakeBuffersResident(unsigned commandKey, ResourceResidencyService& residencyService);
  void RestoreBuffers(unsigned commandKey, unsigned commandListBarriersKey);
  void RestoreBuffersCleanup();

private:
  size_t RestoreBuffer(
      const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& restoreInfo,
      size_t uploadBufferOffset);

private:
  StateTrackingService& m_StateService;
  AccelerationStructuresBufferContentRestore m_BufferContentRestore;
  ReservedResourcesService& m_ReservedResourcesService;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;
  SubcaptureRecorder& m_Recorder;

  struct BufferRegion {
    unsigned Start{};
    unsigned End{};
  };
  std::unordered_map<unsigned, std::vector<BufferRegion>> m_BufferRegionsByInputKey;

  struct InputBuffers {
    std::unordered_map<unsigned, ResourceState*> Buffers;
    std::unordered_map<unsigned, ReservedResourcesService::TiledResource> TiledResources;
  };
  std::unordered_map<unsigned, std::unique_ptr<InputBuffers>> m_InputBuffers;

  std::unordered_map<std::pair<unsigned, unsigned>, uint64_t, UnsignedPairHash>
      m_BufferHashesByKeyOffset;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> m_TiledResourceUpdatesRestored;

  unsigned m_CommandQueueKey{};
  unsigned m_CommandAllocatorKey{};
  unsigned m_CommandListKey{};
  unsigned m_FenceKey{};
  unsigned m_UploadBufferKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadBufferSize{};
  unsigned m_DeviceKey{};
};

} // namespace DirectX
} // namespace gits
