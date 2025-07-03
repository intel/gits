// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRecorder.h"
#include "commandsAuto.h"
#include "accelerationStructuresBufferContentRestore.h"
#include "reservedResourcesService.h"
#include "objectState.h"

#include <unordered_map>
#include <memory>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresBuildService {
public:
  AccelerationStructuresBuildService(StateTrackingService& stateService,
                                     SubcaptureRecorder& recorder,
                                     ReservedResourcesService& reservedResourcesService);
  void buildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command);
  void copyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command);
  void setDeviceKey(unsigned deviceKey) {
    deviceKey_ = deviceKey;
    bufferContentRestore_.setDeviceKey(deviceKey);
  }
  void restoreAccelerationStructures();
  void executeCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& command);
  void commandQueueWait(ID3D12CommandQueueWaitCommand& command);
  void commandQueueSignal(ID3D12CommandQueueSignalCommand& command);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  StateTrackingService& stateService_;
  SubcaptureRecorder& recorder_;
  ReservedResourcesService& reservedResourcesService_;
  AccelerationStructuresBufferContentRestore bufferContentRestore_;

  struct RaytracingAccelerationStructureState {
    virtual ~RaytracingAccelerationStructureState() {}
    unsigned commandKey{};
    unsigned commandListKey{};
    bool buildState{};
    unsigned destKey{};
    unsigned destOffset{};
    unsigned sourceKey{};
    unsigned sourceOffset{};
  };

  struct BuildRaytracingAccelerationStructureState : public RaytracingAccelerationStructureState {
    std::unique_ptr<PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>> desc{};
    bool update{};
    std::unordered_map<unsigned, ResourceState*> buffers;
    std::vector<unsigned> uploadBuffers;
    std::unordered_map<unsigned, ReservedResourcesService::TiledResource> tiledResources;
  };

  struct CopyRaytracingAccelerationStructureState : public RaytracingAccelerationStructureState {
    D3D12_GPU_VIRTUAL_ADDRESS destAccelerationStructureData{};
    D3D12_GPU_VIRTUAL_ADDRESS sourceAccelerationStructureData{};
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode{};
  };

  using KeyOffset = std::pair<unsigned, unsigned>;
  std::set<KeyOffset> tlases_;

  std::unordered_map<unsigned, std::vector<RaytracingAccelerationStructureState*>>
      statesByCommandList_;

  std::map<unsigned, RaytracingAccelerationStructureState*> statesById_;

  std::map<KeyOffset, unsigned> stateByKeyOffset_;
  std::unordered_map<unsigned, unsigned> stateSourceByDest_;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> stateDestsBySource_;

  unsigned stateUniqueId_{};
  unsigned maxBuildScratchSpace_{};
  unsigned deviceKey_{};

  unsigned commandQueueKey_{};
  unsigned commandAllocatorKey_{};
  unsigned commandListKey_{};
  unsigned fenceKey_{};
  UINT64 recordedFenceValue_{};
  bool restored_{};
  bool serializeMode_{};
  bool restoreTLASes_{};

private:
  void storeState(RaytracingAccelerationStructureState* state);
  void removeState(unsigned stateId);
};

} // namespace DirectX
} // namespace gits
