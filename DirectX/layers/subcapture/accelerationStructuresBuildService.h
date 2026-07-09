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
#include "accelerationStructuresInputBuffersService.h"
#include "reservedResourcesService.h"
#include "objectState.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"
#include "hashUtils.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <map>

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
  }
  void RestoreAccelerationStructures();
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);
  void DestroyResource(unsigned commandKey, unsigned resourceKey);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  ReservedResourcesService& m_ReservedResourcesService;
  AccelerationStructuresInputBuffersService m_InputBuffersService;
  ResourceStateTracker& m_ResourceStateTracker;
  CapturePlayerGpuAddressService& m_GpuAddressService;

  struct RaytracingAccelerationStructureCommand {
    virtual ~RaytracingAccelerationStructureCommand() {}

    enum class CommandType {
      Build,
      Copy,
      NvAPIBuild,
      NvAPIOMM
    };
    std::unordered_map<unsigned, ResourceState*> Buffers;
    std::unordered_map<unsigned, ReservedResourcesService::TiledResource> TiledResources;
    unsigned CommandKey{};
    unsigned CommandListKey{};
    CommandType Type{};
    unsigned DestKey{};
    unsigned DestOffset{};
    unsigned SourceKey{};
    unsigned SourceOffset{};
    bool Update{};
    bool TlasBuild{};
  };

  struct BuildRaytracingAccelerationStructureCommand
      : public RaytracingAccelerationStructureCommand {
    std::unique_ptr<PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>> Desc{};
  };

  struct CopyRaytracingAccelerationStructureCommand
      : public RaytracingAccelerationStructureCommand {
    D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData{};
    D3D12_GPU_VIRTUAL_ADDRESS SourceAccelerationStructureData{};
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode{};
  };

  struct NvAPIBuildRaytracingAccelerationStructureExCommand
      : public RaytracingAccelerationStructureCommand {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>>
        Desc{};
  };

  struct NvAPIBuildRaytracingOpacityMicromapArrayCommand
      : public RaytracingAccelerationStructureCommand {
    std::unique_ptr<PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>> Desc{};
  };

  std::unordered_set<std::pair<unsigned, unsigned>, UnsignedPairHash> m_TlasesKeyOffsets;

  unsigned m_MaxBuildScratchSpace{};
  unsigned m_DeviceKey{};

  unsigned m_CommandQueueKey{};
  unsigned m_CommandAllocatorKey{};
  unsigned m_CommandListKey{};
  unsigned m_FenceKey{};
  unsigned m_ScratchResourceKey{};
  UINT64 m_RecordedFenceValue{};

  bool m_Restored{};
  bool m_SerializeMode{};
  bool m_RestoreTlas{};
  bool m_Optimize{};

private:
  void RestoreCommand(BuildRaytracingAccelerationStructureCommand* command);
  void RestoreCommand(CopyRaytracingAccelerationStructureCommand* command);
  void RestoreCommand(NvAPIBuildRaytracingAccelerationStructureExCommand* command);
  void RestoreCommand(NvAPIBuildRaytracingOpacityMicromapArrayCommand* command);
  void RecordExecuteCommandLists();

private:
  class OptimizationService {
  public:
    OptimizationService(StateTrackingService& stateService) : m_StateService(stateService) {}
    void AddCommand(unsigned commandListKey, RaytracingAccelerationStructureCommand* command) {
      m_CommandsByCommandList[commandListKey].emplace_back(command);
    }
    void OnExecute(std::vector<unsigned>& commandListKeys);
    void ProcessCommands();
    void Cleanup();

  public:
    struct CommandNode {
      unsigned Id{};
      std::unique_ptr<RaytracingAccelerationStructureCommand> Command{};
    };
    std::vector<CommandNode*>& GetCommands() {
      return m_RestoreCommands;
    }

  private:
    void StoreCommand(std::unique_ptr<RaytracingAccelerationStructureCommand>& command);

  private:
    StateTrackingService& m_StateService;
    unsigned m_CommandUniqueId{};
    std::unordered_map<unsigned,
                       std::vector<std::unique_ptr<RaytracingAccelerationStructureCommand>>>
        m_CommandsByCommandList;
    std::unordered_map<unsigned, std::unique_ptr<CommandNode>> m_CommandById;
    std::unordered_map<unsigned, RaytracingAccelerationStructureCommand*> m_CommandByBuildKey;
    std::vector<CommandNode*> m_RestoreCommands;
  };
  OptimizationService m_OptimizationService;

private:
  class BufferReleaseService {
  public:
    BufferReleaseService(SubcaptureRecorder& recorder) : m_Recorder(recorder) {}
    void AddBuffer(unsigned key);
    void AddRelease(unsigned commandKey, unsigned bufferKey);
    void ProcessReleases(unsigned commandKey);

  private:
    SubcaptureRecorder& m_Recorder;
    std::unordered_set<unsigned> m_Buffers;
    std::map<unsigned, unsigned> m_Releases;
  };
  BufferReleaseService m_BufferReleaseService;
};

} // namespace DirectX
} // namespace gits
