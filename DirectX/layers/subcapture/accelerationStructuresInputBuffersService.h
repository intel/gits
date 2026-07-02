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
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

  void StoreBufferRegion(unsigned bufferKey, unsigned bufferOffset, unsigned bufferSize);
  void StoreBuffers(unsigned commandKey, ID3D12GraphicsCommandList* commandList);

  void RestoreBuffersInitialization(std::vector<unsigned>& commandKeys, unsigned deviceKey);
  void MakeBuffersResident(unsigned commandKey, ResourceResidencyService& residencyService);
  void RestoreBuffers(unsigned commandKey, unsigned commandListBarriersKey);
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

private:
  class BufferInputDump : public ResourceDump {
  public:
    void DumpBuffer(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned resourceKey,
                    unsigned offset,
                    unsigned size,
                    BarrierState resourceState,
                    unsigned buildCallKey,
                    bool isMappable);

  public:
    struct InputBuffer {
      unsigned BufferKey{};
      unsigned Offset{};
      unsigned BufferHash{};
      bool IsMappable{};
      std::unique_ptr<std::vector<char>> BufferData;
    };
    std::vector<InputBuffer>& GetInputBuffers(unsigned buildKey) {
      return m_InputBuffersByBuildKey[buildKey];
    }

  private:
    std::unordered_map<unsigned, std::vector<InputBuffer>> m_InputBuffersByBuildKey;
    std::mutex m_Mutex;

  protected:
    struct BufferInfo : public DumpInfo {
      unsigned ResourceKey;
      unsigned BuildCallKey;
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
