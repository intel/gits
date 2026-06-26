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
#include "commandsCustom.h"
#include "accelerationStructuresBufferContentRestore.h"
#include "reservedResourcesService.h"
#include "objectState.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <set>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresBuildService {
public:
  AccelerationStructuresBuildService(StateTrackingService& stateService,
                                     SubcaptureRecorder& recorder,
                                     ReservedResourcesService& reservedResourcesService,
                                     ResourceStateTracker& resourceStateTracker,
                                     CapturePlayerGpuAddressService& gpuAddressService);
  void BuildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void NvapiBuildAccelerationStructureEx(
      NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void NvapiBuildOpacityMicromapArray(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);
  void SetDeviceKey(unsigned deviceKey) {
    m_DeviceKey = deviceKey;
    m_BufferContentRestore.SetDeviceKey(deviceKey);
  }
  void RestoreAccelerationStructures();
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  ReservedResourcesService& m_ReservedResourcesService;
  AccelerationStructuresBufferContentRestore m_BufferContentRestore;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;

  struct RaytracingAccelerationStructureState {
    virtual ~RaytracingAccelerationStructureState() {}

    enum class StateKind {
      Build,
      Copy,
      NvAPIBuild,
      NvAPIOMM
    };
    std::unordered_map<unsigned, ResourceState*> Buffers;
    std::unordered_map<unsigned, ReservedResourcesService::TiledResource> TiledResources;
    unsigned CommandKey{};
    unsigned CommandListKey{};
    StateKind Kind{};
    unsigned DestKey{};
    unsigned DestOffset{};
    unsigned SourceKey{};
    unsigned SourceOffset{};
    bool FoundInAnalysis{};
  };

  struct BuildRaytracingAccelerationStructureState : public RaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>> Desc{};
    bool Update{};
  };

  struct CopyRaytracingAccelerationStructureState : public RaytracingAccelerationStructureState {
    D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData{};
    D3D12_GPU_VIRTUAL_ADDRESS SourceAccelerationStructureData{};
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode{};
  };

  struct NvAPIBuildRaytracingAccelerationStructureExState
      : public RaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>>
        Desc{};
    bool Update{};
  };

  struct NvAPIBuildRaytracingOpacityMicromapArrayState
      : public RaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>> Desc{};
  };

  std::set<std::pair<unsigned, unsigned>> m_TlasesKeyOffsets;

  unsigned m_MaxBuildScratchSpace{};
  unsigned m_DeviceKey{};

  struct CommandListKeys {
    unsigned CommandQueueKey{};
    unsigned CommandAllocatorKey{};
    unsigned CommandListKey{};
  };
  CommandListKeys m_CommandListCopyKeys{};
  CommandListKeys m_CommandListDirectKeys{};

  unsigned m_FenceKey{};
  unsigned m_UploadBufferKey{};
  unsigned m_ScratchResourceKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadBufferSize{};

  std::map<std::pair<unsigned, unsigned>, uint64_t> m_BufferHashesByKeyOffset;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> m_TiledResourceUpdatesRestored;

  bool m_Restored{};
  bool m_SerializeMode{};
  bool m_RestoreTlas{};
  bool m_Optimize{};
  bool m_OptimizeRaytracing{};

private:
  void InitUploadBuffer();
  void RestoreState(BuildRaytracingAccelerationStructureState* state);
  void RestoreState(CopyRaytracingAccelerationStructureState* state);
  void RestoreState(NvAPIBuildRaytracingAccelerationStructureExState* state);
  void RestoreState(NvAPIBuildRaytracingOpacityMicromapArrayState* state);
  void RecordExecuteCommandLists(const CommandListKeys& keys);
  size_t RestoreBuffer(
      const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& restoreInfo,
      size_t uploadBufferOffset);

private:
  class InputBufferService {
  public:
    InputBufferService(StateTrackingService& stateService,
                       AccelerationStructuresBufferContentRestore& bufferContentRestore,
                       ReservedResourcesService& reservedResourcesService,
                       ResourceStateTracker& resourceStateTracker,
                       CapturePlayerGpuAddressService& gpuAddressService)
        : m_StateService(stateService),
          m_BufferContentRestore(bufferContentRestore),
          m_ReservedResourcesService(reservedResourcesService),
          m_ResourceStateTracker(resourceStateTracker),
          m_GpuAddressService(gpuAddressService) {}
    void AddBufferRegion(unsigned key, unsigned offset, unsigned size);
    void StoreBuffers(unsigned commandKey,
                      ID3D12GraphicsCommandList* commandList,
                      RaytracingAccelerationStructureState* state);

  private:
    void StoreBuffer(unsigned inputKey,
                     unsigned inputOffset,
                     unsigned size,
                     unsigned commandKey,
                     ID3D12GraphicsCommandList* commandList,
                     RaytracingAccelerationStructureState* state);

  private:
    StateTrackingService& m_StateService;
    AccelerationStructuresBufferContentRestore& m_BufferContentRestore;
    ReservedResourcesService& m_ReservedResourcesService;
    ResourceStateTracker& m_ResourceStateTracker;
    CapturePlayerGpuAddressService& m_GpuAddressService;
    struct BufferRegion {
      unsigned Start{};
      unsigned End{};
    };
    std::unordered_map<unsigned, std::vector<BufferRegion>> m_BufferRegionsByInputKey;
  };

  class OptimizationService {
  public:
    OptimizationService(StateTrackingService& stateService,
                        AccelerationStructuresBufferContentRestore& bufferContentRestore)
        : m_StateService(stateService), m_BufferContentRestore(bufferContentRestore) {}
    void AddState(unsigned commandListKey, RaytracingAccelerationStructureState* state);
    void StoreStatesOnExecute(std::vector<unsigned>& commandListKeys);
    void Optimize();
    std::map<unsigned, RaytracingAccelerationStructureState*>& GetStates() {
      return m_StatesById;
    }

  private:
    void StoreState(RaytracingAccelerationStructureState* state);
    void RemoveState(unsigned stateId, bool removeSource = false);
    void RemoveSourcesWithoutDestinations();
    void CompleteSourcesFromAnalysis();

  private:
    StateTrackingService& m_StateService;
    AccelerationStructuresBufferContentRestore& m_BufferContentRestore;
    unsigned m_StateUniqueId{};
    std::unordered_map<unsigned, std::vector<RaytracingAccelerationStructureState*>>
        m_StatesByCommandList;
    std::map<unsigned, RaytracingAccelerationStructureState*> m_StatesById;
    std::map<std::pair<unsigned, unsigned>, std::set<unsigned>> m_StateByKeyOffset;
    std::unordered_map<unsigned, unsigned> m_StateSourceByDest;
    std::unordered_map<unsigned, std::unordered_set<unsigned>> m_StateDestsBySource;
    std::unordered_set<unsigned> m_SourcesWithoutDestinations;
  };
  OptimizationService m_OptimizationService;
};

} // namespace DirectX
} // namespace gits
