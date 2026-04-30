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
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& Command);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& Command);
  void NvapiBuildAccelerationStructureEx(
      NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& Command);
  void NvapiBuildOpacityMicromapArray(
      NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& Command);
  void SetDeviceKey(unsigned DeviceKey) {
    m_DeviceKey = DeviceKey;
    m_BufferContentRestore.SetDeviceKey(DeviceKey);
  }
  void RestoreAccelerationStructures();
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& Command);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& Command);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& Command);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  ReservedResourcesService& m_ReservedResourcesService;
  AccelerationStructuresBufferContentRestore m_BufferContentRestore;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;

  struct RaytracingAccelerationStructureState {
    enum class StateKind {
      Build,
      Copy,
      NvAPIBuild,
      NvAPIOMM
    };

    virtual ~RaytracingAccelerationStructureState() {}
    unsigned CommandKey{};
    unsigned CommandListKey{};
    StateKind Kind{};
    unsigned DestKey{};
    unsigned DestOffset{};
    unsigned SourceKey{};
    unsigned SourceOffset{};
    bool FoundInAnalysis{};
  };

  struct BufferBackedRaytracingAccelerationStructureState
      : public RaytracingAccelerationStructureState {
    std::unordered_map<unsigned, ResourceState*> Buffers;
    std::unordered_map<unsigned, ReservedResourcesService::TiledResource> TiledResources;
  };

  struct BuildRaytracingAccelerationStructureState
      : public BufferBackedRaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>> Desc{};
    bool Update{};
  };

  struct CopyRaytracingAccelerationStructureState : public RaytracingAccelerationStructureState {
    D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData{};
    D3D12_GPU_VIRTUAL_ADDRESS SourceAccelerationStructureData{};
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode{};
  };

  struct NvAPIBuildRaytracingAccelerationStructureExState
      : public BufferBackedRaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>>
        Desc{};
    bool Update{};
  };

  struct NvAPIBuildRaytracingOpacityMicromapArrayState
      : public BufferBackedRaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>> Desc{};
  };

  struct Interval {
    unsigned Start{};
    unsigned End{};
  };

  using KeyOffset = std::pair<unsigned, unsigned>;
  std::set<KeyOffset> m_Tlases;

  std::unordered_map<unsigned, std::vector<RaytracingAccelerationStructureState*>>
      m_StatesByCommandList;

  std::map<unsigned, RaytracingAccelerationStructureState*> m_StatesById;

  std::map<KeyOffset, std::set<unsigned>> m_StateByKeyOffset;
  std::unordered_map<unsigned, unsigned> m_StateSourceByDest;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> m_StateDestsBySource;
  std::unordered_set<unsigned> m_SourcesWithoutDestinations;

  unsigned m_StateUniqueId{};
  unsigned m_MaxBuildScratchSpace{};
  unsigned m_DeviceKey{};

  unsigned m_CommandQueueCopyKey{};
  unsigned m_CommandAllocatorCopyKey{};
  unsigned m_CommandListCopyKey{};
  unsigned m_CommandQueueDirectKey{};
  unsigned m_CommandAllocatorDirectKey{};
  unsigned m_CommandListDirectKey{};
  unsigned m_FenceKey{};
  unsigned m_UploadBufferKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadBufferSize{};
  bool m_Restored{};
  bool m_SerializeMode{};
  bool m_RestoreTlas{};
  bool m_Optimize{};

  void StoreState(RaytracingAccelerationStructureState* state);
  unsigned GetState(unsigned key, unsigned offset);
  void RemoveState(unsigned stateId, bool removeSource = false);
  void InitUploadBuffer();
  size_t RestoreBuffer(
      const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& restoreInfo,
      size_t uploadBufferOffset);
  void Optimize();
  void RemoveSourcesWithoutDestinations();
  void CompleteSourcesFromAnalysis();
  void StoreBuffer(unsigned inputKey,
                   unsigned inputOffset,
                   unsigned size,
                   unsigned commandKey,
                   ID3D12GraphicsCommandList* commandList,
                   BufferBackedRaytracingAccelerationStructureState* state);
  std::vector<Interval> MergeIntervals(const std::vector<Interval>& intervals);
  void InsertIfNotResident(unsigned ResourceKey, std::set<unsigned>& residencyKeys);
  std::optional<unsigned> GetResidencyKeyForNotResidentResource(unsigned key);
  void RecordMakeResident(const std::set<unsigned>& keys);
  void RecordEvict(const std::set<unsigned>& keys);
};

} // namespace DirectX
} // namespace gits
