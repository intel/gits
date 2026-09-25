// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "capturePlayerGpuAddressService.h"
#include "gpuExecutionTracker.h"
#include "resourceStateTracker.h"

#include <queue>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

class RaytracingAssertionsLayer : public Layer {
public:
  RaytracingAssertionsLayer();
  ~RaytracingAssertionsLayer() override;

  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c)
      override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) override;

  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void Post(FrameEndCommand& c) override;

  void Post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void Post(ID3D12DeviceCreateHeapCommand& c) override;
  void Post(ID3D12Device4CreateHeap1Command& c) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void Post(ID3D12Device4CreateReservedResource1Command& c) override;
  void Post(ID3D12Device10CreateReservedResource2Command& c) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Post(IUnknownReleaseCommand& c) override;
  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& c) override;
  void Post(ID3D12GraphicsCommandListResetCommand& c) override;

private:
  template <typename T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;

  struct ResourceLocation {
    ObjectKey ResourceKey{};
    UINT64 Offset{};
  };

  struct Build {
    CommandKey Key{};
    ResourceLocation RecordedLocation{};
    D3D12_GPU_VIRTUAL_ADDRESS ReplayAddress{};
    UINT64 Size{};
    std::set<std::shared_ptr<Build>> Inputs;
    std::shared_ptr<Build> Overwriter;
    CommandKey DeletedBy{};
  };
  using BuildPtr = std::shared_ptr<Build>;

  class BufferPool {
  public:
    struct Releaser {
      BufferPool* Pool{};
      void operator()(ID3D12Resource* resource) const;
    };
    using Buffer = std::unique_ptr<ID3D12Resource, Releaser>;

    Buffer Acquire(ID3D12Device* device,
                   UINT64 size,
                   D3D12_HEAP_TYPE heapType,
                   D3D12_RESOURCE_STATES state);

  private:
    using BufferType = std::pair<D3D12_HEAP_TYPE, D3D12_RESOURCE_STATES>;
    std::map<BufferType, std::multimap<UINT64, ComPtr<ID3D12Resource>>> m_Free;
    std::unordered_map<ID3D12Resource*, BufferType> m_Types;
  };

  struct ReadbackBuffer {
    BufferPool::Buffer Buffer;
    UINT64 Size{};
  };

  struct SizeQuery {
    BufferPool::Buffer Uav;
    BufferPool::Buffer Readback;
  };

  struct PostbuildInfoOverride {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* Descs{};
    UINT Count{};
    size_t Size{};
    std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> Patched;
  };

  struct Operation {
    CommandKey Key{};
    std::optional<ResourceLocation> RecordedDestination;
    D3D12_GPU_VIRTUAL_ADDRESS DestinationReplayAddress{};
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> SourceReplayAddresses;
    bool InheritInputs{};
    std::vector<ReadbackBuffer> Instances;
    ReadbackBuffer SerializedHeader;
    SizeQuery PostbuildSizeQuery;
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> OmmArrayReplayAddresses;
  };

  struct QueueFence {
    ComPtr<ID3D12Fence> Fence;
    UINT64 Value{};
  };

  struct ValidationEvent : GpuExecutionTracker::Executable {
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue{};
    std::vector<Operation> Operations;
  };

  void ProcessReadyEvents(bool block = false);

  void ValidateOperation(
      const Operation& operation,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void ValidateOperationSources(const Operation& operation, std::set<BuildPtr>& inputs);
  void ValidateOperationTlasInputs(
      const Operation& operation,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings,
      std::set<BuildPtr>& inputs);
  void ValidateOperationSerialization(
      const Operation& operation,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings,
      std::set<BuildPtr>& inputs);
  void ValidateOperationOmmArrayInputs(const Operation& operation, std::set<BuildPtr>& inputs);
  void ValidateBuildDependency(CommandKey commandKey, const Build& build);
  static const Build* FindFirstInvalidBuild(const Build& build);
  void ValidateBuildFitsAllocation(CommandKey commandKey,
                                   D3D12_GPU_VIRTUAL_ADDRESS replayAddress,
                                   UINT64 sizeInBytes,
                                   const CapturePlayerGpuAddressService::ResourceInfo& resource);
  void ValidateBuildScratchSize(
      const ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void LogMissingBuild(CommandKey commandKey, D3D12_GPU_VIRTUAL_ADDRESS replayAddress);

  void LoadInstancePointers();
  ReadbackBuffer StageReadback(ID3D12GraphicsCommandList* commandList,
                               ObjectKey resourceKey,
                               UINT64 offset,
                               UINT64 size);
  SizeQuery CreateSizeQuery();
  void RecordSizeQuery(ID3D12GraphicsCommandList* commandList, const SizeQuery& query);

  BuildPtr FindBuild(D3D12_GPU_VIRTUAL_ADDRESS replayAddress) const;
  D3D12_GPU_VIRTUAL_ADDRESS GetReplayAddress(ObjectKey resourceKey, UINT64 offset) const;
  static D3D12_GPU_VIRTUAL_ADDRESS GetReplayAddressFromMappings(
      D3D12_GPU_VIRTUAL_ADDRESS captureAddress,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  CapturePlayerGpuAddressService::ResourceInfo& FindReplayResource(
      D3D12_GPU_VIRTUAL_ADDRESS replayAddress);
  CapturePlayerGpuAddressService::ResourceInfo& FindCaptureResource(
      D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
  void EraseBuildsInReplayRange(D3D12_GPU_VIRTUAL_ADDRESS start,
                                D3D12_GPU_VIRTUAL_ADDRESS end,
                                CommandKey deletedBy);
  std::pair<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS> GetBackingAllocationBounds(
      const CapturePlayerGpuAddressService::ResourceInfo& resource) const;

  template <typename Command, typename State>
  void AddResource(Command& c, State initialState, ObjectKey heapKey = 0);
  void StoreHeapReplayBounds(ObjectKey heapKey, ID3D12Heap* heap, UINT64 sizeInBytes);
  void StoreResourceReplayBounds(ObjectKey resourceKey, ID3D12Resource* resource);
  static D3D12_GPU_VIRTUAL_ADDRESS GetHeapGpuVirtualAddress(ID3D12Heap* heap);
  static std::vector<uint8_t> ReadBuffer(ID3D12Resource* resource, UINT64 size);

private:
  ComPtr<ID3D12Device> m_Device;
  GpuExecutionTracker m_ExecutionTracker;
  ResourceStateTracker m_ResourceStates;
  CapturePlayerGpuAddressService m_AddressService;
  BufferPool m_BufferPool;

  std::map<D3D12_GPU_VIRTUAL_ADDRESS, BuildPtr> m_Builds;

  std::unordered_map<ObjectKey, ID3D12Resource*> m_Resources;
  std::unordered_set<ObjectKey> m_PlacedResources;
  std::unordered_map<ObjectKey, ObjectKey> m_ResourceHeap;
  std::unordered_map<ObjectKey, std::pair<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS>>
      m_HeapReplayBounds;
  std::unordered_map<ObjectKey, std::pair<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS>>
      m_ResourceReplayBounds;

  std::unordered_map<ObjectKey, std::vector<Operation>> m_RecordedOperations;
  std::unordered_map<ObjectKey, QueueFence> m_QueueFences;
  std::queue<std::unique_ptr<ValidationEvent>> m_PendingValidationEvents;

  PostbuildInfoOverride m_PostbuildInfoOverride;

  std::unordered_map<CommandKey, std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> m_InstancePointers;
  bool m_InstancePointersLoaded{};
};

} // namespace DirectX
} // namespace gits
