// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"
#include "reservedResourcesService.h"
#include "objectState.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"
#include "resourceResidencyService.h"
#include "subcaptureRecorder.h"
#include "hashUtils.h"
#include "resourceDump.h"

#include <unordered_map>
#include <mutex>

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
        m_ReservedResourcesService(reservedResourcesService),
        m_ResourceStateTracker(resourceStateTracker),
        m_GpuAddressService(gpuAddressService),
        m_Recorder(recorder) {}

  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void FenceSignal(GITSKey key, GITSKey fenceKey, UINT64 fenceValue);

  void StoreBufferRegion(GITSKey bufferKey, unsigned bufferOffset, unsigned bufferSize);
  void StoreBuffers(GITSKey commandKey, ID3D12GraphicsCommandList* commandList);

  void RestoreBuffersInitialization(std::vector<GITSKey>& commandKeys, GITSKey deviceKey);
  void MakeBuffersResident(GITSKey commandKey, ResourceResidencyService& residencyService);
  void RestoreBuffers(GITSKey commandKey, GITSKey commandListBarriersKey);
  void RestoreBuffersCleanup();

private:
  StateTrackingService& m_StateService;
  ReservedResourcesService& m_ReservedResourcesService;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;
  SubcaptureRecorder& m_Recorder;

  struct BufferRegion {
    unsigned Start{};
    unsigned End{};
  };
  std::unordered_map<GITSKey, std::vector<BufferRegion>> m_BufferRegionsByInputKey;

  struct InputBuffers {
    std::unordered_map<GITSKey, ResourceState*> Buffers;
    std::unordered_map<GITSKey, ReservedResourcesService::TiledResource> TiledResources;
  };
  std::unordered_map<GITSKey, std::unique_ptr<InputBuffers>> m_InputBuffers;

  std::unordered_map<std::pair<GITSKey, unsigned>, uint64_t, UnsignedPairHash>
      m_BufferHashesByKeyOffset;
  std::unordered_map<GITSKey, std::unordered_set<GITSKey>> m_TiledResourceUpdatesRestored;

  GITSKey m_CommandQueueKey{};
  GITSKey m_CommandAllocatorKey{};
  GITSKey m_CommandListKey{};
  GITSKey m_FenceKey{};
  GITSKey m_UploadBufferKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadBufferSize{};

private:
  class BufferInputDump : public ResourceDump {
  public:
    void DumpBuffer(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    GITSKey resourceKey,
                    unsigned offset,
                    unsigned size,
                    BarrierState resourceState,
                    GITSKey buildCallKey,
                    bool isMappable);

  public:
    struct InputBuffer {
      GITSKey BufferKey{};
      unsigned Offset{};
      unsigned BufferHash{};
      bool IsMappable{};
      std::unique_ptr<std::vector<char>> BufferData;
    };
    std::vector<InputBuffer>& GetInputBuffers(GITSKey buildKey) {
      return m_InputBuffersByBuildKey[buildKey];
    }

  private:
    std::unordered_map<GITSKey, std::vector<InputBuffer>> m_InputBuffersByBuildKey;
    std::mutex m_Mutex;

  protected:
    struct BufferInfo : public DumpInfo {
      GITSKey ResourceKey;
      GITSKey BuildCallKey;
      bool IsMappable;
    };
    void DumpBuffer(DumpInfo& dumpInfo, void* data) override;
  };
  BufferInputDump m_BufferInputDump;

private:
  size_t RestoreBuffer(const BufferInputDump::InputBuffer& restoreInfo, size_t uploadBufferOffset);
};

} // namespace DirectX
} // namespace gits
