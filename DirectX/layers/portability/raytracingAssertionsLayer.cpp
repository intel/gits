// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingAssertionsLayer.h"
#include "resourceStateEnhanced.h"
#include "configurationLib.h"
#include "log.h"
#include "to_string/toStr.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace gits {
namespace DirectX {

RaytracingAssertionsLayer::RaytracingAssertionsLayer() : Layer("RaytracingAssertions") {
  m_AddressService.EnablePlayerAddressLookup();
}

RaytracingAssertionsLayer::~RaytracingAssertionsLayer() {
  ProcessReadyEvents(true);
}

void RaytracingAssertionsLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.Skip) {
    return;
  }
  ValidateBuildScratchSize(c);
  const auto& desc = *c.m_pDesc.Value;
  const bool update =
      desc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  Operation operation;
  operation.Key = c.Key;
  operation.RecordedDestination = ResourceLocation{c.m_pDesc.DestAccelerationStructureKey,
                                                   c.m_pDesc.DestAccelerationStructureOffset};
  operation.DestinationReplayAddress = GetReplayAddress(c.m_pDesc.DestAccelerationStructureKey,
                                                        c.m_pDesc.DestAccelerationStructureOffset);
  if (update) {
    operation.SourceReplayAddresses.push_back(GetReplayAddress(
        c.m_pDesc.SourceAccelerationStructureKey, c.m_pDesc.SourceAccelerationStructureOffset));
  }
  if (desc.Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
      desc.Inputs.NumDescs) {
    if (desc.Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      LoadInstancePointers();
      auto pointers = m_InstancePointers.find(c.Key);
      GITS_ASSERT(pointers != m_InstancePointers.end(),
                  "RaytracingAssertionsLayer: missing raytracingArraysOfPointers.dat entry");
      GITS_ASSERT(pointers->second.size() == desc.Inputs.NumDescs,
                  "RaytracingAssertionsLayer: array of pointers count mismatch");
      const std::set<D3D12_GPU_VIRTUAL_ADDRESS> unique(pointers->second.begin(),
                                                       pointers->second.end());
      for (D3D12_GPU_VIRTUAL_ADDRESS captureAddress : unique) {
        const auto& resource = FindCaptureResource(captureAddress);
        operation.Instances.push_back(StageReadback(c.m_Object.Value, resource.Key,
                                                    captureAddress - resource.CaptureStart,
                                                    sizeof(D3D12_RAYTRACING_INSTANCE_DESC)));
      }
    } else {
      GITS_ASSERT(!c.m_pDesc.InputKeys.empty() && !c.m_pDesc.InputOffsets.empty(),
                  "RaytracingAssertionsLayer: missing TLAS instances key");
      ReadbackBuffer readbackBuffer =
          StageReadback(c.m_Object.Value, c.m_pDesc.InputKeys[0], c.m_pDesc.InputOffsets[0],
                        UINT64{desc.Inputs.NumDescs} * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
      operation.Instances.push_back(std::move(readbackBuffer));
    }
  } else if (desc.Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    const auto& inputs = desc.Inputs;
    unsigned inputIndex = 0;
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      const D3D12_RAYTRACING_GEOMETRY_DESC& geometry =
          inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY ? inputs.pGeometryDescs[i]
                                                            : *inputs.ppGeometryDescs[i];
      if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        inputIndex += 3;
      } else if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        ++inputIndex;
      } else if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (geometry.OmmTriangles.pTriangles) {
          inputIndex += 3;
        }
        if (geometry.OmmTriangles.pOmmLinkage) {
          ++inputIndex;
          GITS_ASSERT(inputIndex < c.m_pDesc.InputKeys.size(),
                      "RaytracingAssertionsLayer: missing OMM array input key");
          operation.OmmArrayReplayAddresses.push_back(GetReplayAddress(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]));
          ++inputIndex;
        }
      }
    }
  }

  operation.PostbuildSizeQuery = CreateSizeQuery();

  if (desc.Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    m_PostbuildInfoOverride = {};
  } else {
    auto& postbuildInfoOverride = m_PostbuildInfoOverride;
    postbuildInfoOverride.Descs = c.m_pPostbuildInfoDescs.Value;
    postbuildInfoOverride.Count = c.m_NumPostbuildInfoDescs.Value;
    postbuildInfoOverride.Size = c.m_pPostbuildInfoDescs.Size;
    postbuildInfoOverride.Patched.assign(postbuildInfoOverride.Descs,
                                         postbuildInfoOverride.Descs + postbuildInfoOverride.Count);
    postbuildInfoOverride.Patched.push_back(
        {operation.PostbuildSizeQuery.Uav->GetGPUVirtualAddress(),
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE});
    c.m_pPostbuildInfoDescs.Value = postbuildInfoOverride.Patched.data();
    c.m_pPostbuildInfoDescs.Size = postbuildInfoOverride.Patched.size();
    c.m_NumPostbuildInfoDescs.Value = static_cast<UINT>(postbuildInfoOverride.Patched.size());
  }

  m_RecordedOperations[c.m_Object.Key].push_back(std::move(operation));
}

void RaytracingAssertionsLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  auto& postbuildInfoOverride = m_PostbuildInfoOverride;
  if (!postbuildInfoOverride.Patched.empty()) {
    c.m_pPostbuildInfoDescs.Value = postbuildInfoOverride.Descs;
    c.m_pPostbuildInfoDescs.Size = postbuildInfoOverride.Size;
    c.m_NumPostbuildInfoDescs.Value = postbuildInfoOverride.Count;
    postbuildInfoOverride.Patched.clear();
  }
  if (!c.Skip) {
    Operation& operation = m_RecordedOperations[c.m_Object.Key].back();
    if (c.m_pDesc.Value->Inputs.Type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      c.m_Object.Value->ResourceBarrier(1, &barrier);
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC info{
          operation.PostbuildSizeQuery.Uav->GetGPUVirtualAddress(),
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE};
      const D3D12_GPU_VIRTUAL_ADDRESS dest = c.m_pDesc.Value->DestAccelerationStructureData;
      c.m_Object.Value->EmitRaytracingAccelerationStructurePostbuildInfo(&info, 1, &dest);
    }
    RecordSizeQuery(c.m_Object.Value, operation.PostbuildSizeQuery);
  }
}

void RaytracingAssertionsLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (c.Skip) {
    return;
  }
  const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode = c.m_Mode.Value;

  Operation operation;
  operation.Key = c.Key;
  const ObjectKey sourceKey = c.m_SourceAccelerationStructureData.InterfaceKey;
  const UINT64 sourceOffset = c.m_SourceAccelerationStructureData.Offset;
  if (mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE) {
    const UINT64 size = m_Resources.at(sourceKey)->GetDesc().Width - sourceOffset;
    operation.SerializedHeader = StageReadback(c.m_Object.Value, sourceKey, sourceOffset, size);
  } else {
    operation.SourceReplayAddresses.push_back(GetReplayAddress(sourceKey, sourceOffset));
  }
  if (mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE ||
      mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT ||
      mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE) {
    operation.RecordedDestination = ResourceLocation{c.m_DestAccelerationStructureData.InterfaceKey,
                                                     c.m_DestAccelerationStructureData.Offset};
    operation.DestinationReplayAddress = GetReplayAddress(
        c.m_DestAccelerationStructureData.InterfaceKey, c.m_DestAccelerationStructureData.Offset);
    operation.InheritInputs = mode != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE;
    operation.PostbuildSizeQuery = CreateSizeQuery();
  }
  m_RecordedOperations[c.m_Object.Key].push_back(std::move(operation));
}

void RaytracingAssertionsLayer::Post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (c.Skip) {
    return;
  }
  const Operation& operation = m_RecordedOperations[c.m_Object.Key].back();
  if (!operation.RecordedDestination) {
    return;
  }
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  c.m_Object.Value->ResourceBarrier(1, &barrier);
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC info{
      operation.PostbuildSizeQuery.Uav->GetGPUVirtualAddress(),
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE};
  c.m_Object.Value->EmitRaytracingAccelerationStructurePostbuildInfo(
      &info, 1, &c.m_DestAccelerationStructureData.Value);
  RecordSizeQuery(c.m_Object.Value, operation.PostbuildSizeQuery);
}

void RaytracingAssertionsLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (c.Skip) {
    return;
  }
  Operation operation;
  operation.Key = c.Key;
  for (UINT i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    operation.SourceReplayAddresses.push_back(
        GetReplayAddress(c.m_pSourceAccelerationStructureData.InterfaceKeys[i],
                         c.m_pSourceAccelerationStructureData.Offsets[i]));
  }
  m_RecordedOperations[c.m_Object.Key].push_back(std::move(operation));
}

void RaytracingAssertionsLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (c.Skip) {
    return;
  }
  LOG_ERROR << "RaytracingAssertionsLayer: NvAPI RTAS build not handled";
}

void RaytracingAssertionsLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (c.Skip) {
    return;
  }
  LOG_ERROR << "RaytracingAssertionsLayer: NvAPI cluster operation not handled";
}

void RaytracingAssertionsLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStates.ExecuteCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
      c.m_NumCommandLists.Value);
  auto validationEvent = std::make_unique<ValidationEvent>();
  for (UINT i = 0; i < c.m_NumCommandLists.Value; ++i) {
    auto recorded = m_RecordedOperations.find(c.m_ppCommandLists.Keys[i]);
    if (recorded == m_RecordedOperations.end()) {
      continue;
    }
    GITS_ASSERT(!recorded->second.empty(),
                "RaytracingAssertionsLayer: re-executed command list with RTAS operations");
    std::move(recorded->second.begin(), recorded->second.end(),
              std::back_inserter(validationEvent->Operations));
    recorded->second.clear();
  }
  if (validationEvent->Operations.empty()) {
    ProcessReadyEvents();
    return;
  }

  QueueFence& queueFence = m_QueueFences[c.m_Object.Key];
  if (!queueFence.Fence) {
    HRESULT hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queueFence.Fence));
    GITS_ASSERT(hr == S_OK);
  }
  validationEvent->Fence = queueFence.Fence;
  validationEvent->FenceValue = ++queueFence.Value;
  HRESULT hr = c.m_Object.Value->Signal(validationEvent->Fence.Get(), validationEvent->FenceValue);
  GITS_ASSERT(hr == S_OK);

  m_ExecutionTracker.Execute(c.Key, c.m_Object.Key, validationEvent.release());
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_ExecutionTracker.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_ExecutionTracker.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_ExecutionTracker.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(ID3D12FenceSignalCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_ExecutionTracker.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_ExecutionTracker.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(FrameEndCommand& c) {
  ProcessReadyEvents();
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  AddResource(c, c.m_InitialResourceState.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  AddResource(c, c.m_InitialResourceState.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  AddResource(c, c.m_InitialResourceState.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  AddResource(c, c.m_InitialLayout.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || !c.m_ppvHeap.Value || !*c.m_ppvHeap.Value ||
      !c.m_pDesc.Value) {
    return;
  }
  StoreHeapReplayBounds(c.m_ppvHeap.Key, static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value),
                        c.m_pDesc.Value->SizeInBytes);
}

void RaytracingAssertionsLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || !c.m_ppvHeap.Value || !*c.m_ppvHeap.Value ||
      !c.m_pDesc.Value) {
    return;
  }
  StoreHeapReplayBounds(c.m_ppvHeap.Key, static_cast<ID3D12Heap*>(*c.m_ppvHeap.Value),
                        c.m_pDesc.Value->SizeInBytes);
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  AddResource(c, c.m_InitialState.Value, c.m_pHeap.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  AddResource(c, c.m_InitialState.Value, c.m_pHeap.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  AddResource(c, c.m_InitialLayout.Value, c.m_pHeap.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  AddResource(c, c.m_InitialState.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  AddResource(c, c.m_InitialState.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  AddResource(c, c.m_InitialLayout.Value);
}

void RaytracingAssertionsLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (c.Skip || !c.m_Result.Value) {
    return;
  }
  auto it = m_Resources.find(c.m_Object.Key);
  if (it == m_Resources.end()) {
    return;
  }
  m_AddressService.AddGpuCaptureAddress(it->second, c.m_Object.Key, it->second->GetDesc().Width,
                                        c.m_Result.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (c.Skip || !c.m_Result.Value) {
    return;
  }
  auto it = m_Resources.find(c.m_Object.Key);
  if (it == m_Resources.end()) {
    return;
  }
  m_AddressService.AddGpuPlayerAddress(it->second, c.m_Object.Key, it->second->GetDesc().Width,
                                       it->second->GetGPUVirtualAddress());
}

void RaytracingAssertionsLayer::Post(IUnknownReleaseCommand& c) {
  if (c.Skip || c.m_Result.Value != 0) {
    return;
  }
  if (auto heapBounds = m_HeapReplayBounds.find(c.m_Object.Key);
      heapBounds != m_HeapReplayBounds.end()) {
    ProcessReadyEvents();
    EraseBuildsInReplayRange(heapBounds->second.first, heapBounds->second.second, c.Key);
    m_HeapReplayBounds.erase(heapBounds);
  }
  if (auto resourceBounds = m_ResourceReplayBounds.find(c.m_Object.Key);
      resourceBounds != m_ResourceReplayBounds.end()) {
    ProcessReadyEvents();
    EraseBuildsInReplayRange(resourceBounds->second.first, resourceBounds->second.second, c.Key);
    m_ResourceReplayBounds.erase(resourceBounds);
  }
  auto resource = m_Resources.find(c.m_Object.Key);
  if (resource != m_Resources.end()) {
    m_PlacedResources.erase(c.m_Object.Key);
    m_ResourceHeap.erase(c.m_Object.Key);
    m_Resources.erase(resource);
  }
  ProcessReadyEvents();
  m_AddressService.DestroyInterface(c.m_Object.Key);
  m_RecordedOperations.erase(c.m_Object.Key);
  m_QueueFences.erase(c.m_Object.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStates.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value, c.m_NumBarriers.Value,
                                   c.m_pBarriers.ResourceKeys.data());
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStates.ResourceBarrier(c.m_Object.Value, c.m_pBarrierGroups.Value,
                                   c.m_NumBarrierGroups.Value,
                                   c.m_pBarrierGroups.ResourceKeys.data());
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_RecordedOperations.erase(c.m_Object.Key);
}

void RaytracingAssertionsLayer::ProcessReadyEvents(bool block) {
  for (auto* executable : m_ExecutionTracker.GetReadyExecutables()) {
    m_PendingValidationEvents.emplace(static_cast<ValidationEvent*>(executable));
  }
  m_ExecutionTracker.GetReadyExecutables().clear();

  while (!m_PendingValidationEvents.empty()) {
    const ValidationEvent& validationEvent = *m_PendingValidationEvents.front();
    UINT64 fenceValue = validationEvent.Fence->GetCompletedValue();
    GITS_ASSERT(fenceValue != UINT64_MAX, "RaytracingAssertionsLayer: device removed");
    if (fenceValue < validationEvent.FenceValue) {
      if (!block) {
        return;
      }
      validationEvent.Fence->SetEventOnCompletion(validationEvent.FenceValue, nullptr);
      GITS_ASSERT(validationEvent.Fence->GetCompletedValue() != UINT64_MAX,
                  "RaytracingAssertionsLayer: device removed");
    }
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
    m_AddressService.GetMappings(gpuAddressMappings);
    for (const Operation& operation : validationEvent.Operations) {
      ValidateOperation(operation, gpuAddressMappings);
    }
    m_PendingValidationEvents.pop();
  }
}

void RaytracingAssertionsLayer::ValidateOperation(
    const Operation& operation,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  std::set<BuildPtr> inputs;
  ValidateOperationSources(operation, inputs);
  ValidateOperationTlasInputs(operation, gpuAddressMappings, inputs);
  ValidateOperationSerialization(operation, gpuAddressMappings, inputs);
  ValidateOperationOmmArrayInputs(operation, inputs);

  if (!operation.RecordedDestination) {
    return;
  }

  BuildPtr newBuild = std::make_shared<Build>();
  newBuild->Key = operation.Key;
  newBuild->ReplayAddress = operation.DestinationReplayAddress;
  newBuild->RecordedLocation = *operation.RecordedDestination;
  newBuild->Inputs = std::move(inputs);
  {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC size{};
    const std::vector<uint8_t> data =
        ReadBuffer(operation.PostbuildSizeQuery.Readback.get(), sizeof(size));
    std::memcpy(&size, data.data(), sizeof(size));
    GITS_ASSERT(size.CurrentSizeInBytes, "RaytracingAssertionsLayer: RTAS size zero");
    newBuild->Size = size.CurrentSizeInBytes;
  }

  const CapturePlayerGpuAddressService::ResourceInfo& destinationResource =
      FindReplayResource(newBuild->ReplayAddress);
  {
    const UINT64 destinationOffset = newBuild->ReplayAddress - destinationResource.PlayerStart;
    if (destinationResource.Key != newBuild->RecordedLocation.ResourceKey ||
        destinationOffset != newBuild->RecordedLocation.Offset) {
      static bool logged = false;
      if (!logged) {
        LOG_WARNING << "RaytracingAssertionsLayer: " << keyToStr(newBuild->Key)
                    << " RTAS build replay address resolves to O"
                    << keyToStr(destinationResource.Key) << " offset " << destinationOffset
                    << " but was recorded as O" << keyToStr(newBuild->RecordedLocation.ResourceKey)
                    << " offset " << newBuild->RecordedLocation.Offset;
        logged = true;
      }
    }
    ValidateBuildFitsAllocation(newBuild->Key, newBuild->ReplayAddress, newBuild->Size,
                                destinationResource);
  }

  {
    const auto [scanStart, scanEnd] = GetBackingAllocationBounds(destinationResource);
    for (auto it = m_Builds.lower_bound(scanStart); it != m_Builds.lower_bound(scanEnd); ++it) {
      const auto& [existingReplayAddress, existingBuild] = *it;
      if (existingBuild->Overwriter) {
        continue;
      }
      if (existingReplayAddress < newBuild->ReplayAddress + newBuild->Size &&
          newBuild->ReplayAddress < existingReplayAddress + existingBuild->Size) {
        existingBuild->Overwriter = newBuild;
      }
    }
  }

  m_Builds[newBuild->ReplayAddress] = newBuild;
}

void RaytracingAssertionsLayer::ValidateOperationSources(const Operation& operation,
                                                         std::set<BuildPtr>& inputs) {
  for (D3D12_GPU_VIRTUAL_ADDRESS sourceReplayAddress : operation.SourceReplayAddresses) {
    BuildPtr build = FindBuild(sourceReplayAddress);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      if (operation.InheritInputs) {
        inputs.insert(build->Inputs.begin(), build->Inputs.end());
      }
    } else {
      LogMissingBuild(operation.Key, sourceReplayAddress);
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationTlasInputs(
    const Operation& operation,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings,
    std::set<BuildPtr>& inputs) {
  std::set<D3D12_GPU_VIRTUAL_ADDRESS> seenReplayAddresses;
  for (const ReadbackBuffer& readbackBuffer : operation.Instances) {
    constexpr UINT64 stride = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    const std::vector<uint8_t> data = ReadBuffer(readbackBuffer.Buffer.get(), readbackBuffer.Size);
    GITS_ASSERT(data.size() % stride == 0);
    for (UINT64 i = 0; i < data.size() / stride; ++i) {
      D3D12_RAYTRACING_INSTANCE_DESC instance{};
      std::memcpy(&instance, data.data() + i * stride, sizeof(instance));
      if (!instance.AccelerationStructure) {
        continue;
      }
      const D3D12_GPU_VIRTUAL_ADDRESS replayBlasAddress =
          GetReplayAddressFromMappings(instance.AccelerationStructure, gpuAddressMappings);
      if (!seenReplayAddresses.insert(replayBlasAddress).second) {
        continue;
      }
      BuildPtr build = FindBuild(replayBlasAddress);
      if (build) {
        ValidateBuildDependency(operation.Key, *build);
        inputs.insert(std::move(build));
      } else {
        LogMissingBuild(operation.Key, replayBlasAddress);
      }
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationOmmArrayInputs(const Operation& operation,
                                                                std::set<BuildPtr>& inputs) {
  std::set<D3D12_GPU_VIRTUAL_ADDRESS> seenReplayAddresses;
  for (D3D12_GPU_VIRTUAL_ADDRESS ommArrayReplayAddress : operation.OmmArrayReplayAddresses) {
    if (!seenReplayAddresses.insert(ommArrayReplayAddress).second) {
      continue;
    }
    BuildPtr build = FindBuild(ommArrayReplayAddress);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      inputs.insert(std::move(build));
    } else {
      LogMissingBuild(operation.Key, ommArrayReplayAddress);
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationSerialization(
    const Operation& operation,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings,
    std::set<BuildPtr>& inputs) {
  if (!operation.SerializedHeader.Buffer) {
    return;
  }
  const std::vector<uint8_t> data =
      ReadBuffer(operation.SerializedHeader.Buffer.get(), operation.SerializedHeader.Size);
  D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER header{};
  GITS_ASSERT(data.size() >= sizeof(header),
              "RaytracingAssertionsLayer: truncated serialized RTAS");
  std::memcpy(&header, data.data(), sizeof(header));
  const UINT64 pointersSize = header.NumBottomLevelAccelerationStructurePointersAfterHeader *
                              sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
  GITS_ASSERT(header.SerializedSizeInBytesIncludingHeader <= data.size() &&
                  sizeof(header) + pointersSize <= header.SerializedSizeInBytesIncludingHeader,
              "RaytracingAssertionsLayer: invalid serialized RTAS header");

  std::set<D3D12_GPU_VIRTUAL_ADDRESS> seenReplayAddresses;
  for (UINT64 i = 0; i < header.NumBottomLevelAccelerationStructurePointersAfterHeader; ++i) {
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress{};
    std::memcpy(&captureAddress, data.data() + sizeof(header) + i * sizeof(captureAddress),
                sizeof(captureAddress));
    if (!captureAddress) {
      continue;
    }
    const D3D12_GPU_VIRTUAL_ADDRESS replayBlasAddress =
        GetReplayAddressFromMappings(captureAddress, gpuAddressMappings);
    if (!seenReplayAddresses.insert(replayBlasAddress).second) {
      continue;
    }
    BuildPtr build = FindBuild(replayBlasAddress);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      inputs.insert(std::move(build));
    } else {
      LogMissingBuild(operation.Key, replayBlasAddress);
    }
  }
}

void RaytracingAssertionsLayer::ValidateBuildDependency(CommandKey commandKey, const Build& build) {
  const Build* invalid = FindFirstInvalidBuild(build);
  if (!invalid) {
    return;
  }
  if (invalid->DeletedBy) {
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " uses RTAS in O"
              << keyToStr(build.RecordedLocation.ResourceKey) << " offset "
              << build.RecordedLocation.Offset << " that depends on RTAS built by "
              << keyToStr(invalid->Key) << " in O"
              << keyToStr(invalid->RecordedLocation.ResourceKey) << " offset "
              << invalid->RecordedLocation.Offset << " deleted by " << keyToStr(invalid->DeletedBy);
    GITS_ASSERT(false, "RaytracingAssertionsLayer: deleted RTAS was used");
  } else if (invalid->Overwriter) {
    const Build& overwriter = *invalid->Overwriter;
    const uint64_t overlapStart = std::max(invalid->ReplayAddress, overwriter.ReplayAddress);
    const uint64_t overlapEnd = std::min(invalid->ReplayAddress + invalid->Size,
                                         overwriter.ReplayAddress + overwriter.Size);
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " uses RTAS in O"
              << keyToStr(build.RecordedLocation.ResourceKey) << " offset "
              << build.RecordedLocation.Offset << " that depends on RTAS built by "
              << keyToStr(invalid->Key) << " in O"
              << keyToStr(invalid->RecordedLocation.ResourceKey) << " offset "
              << invalid->RecordedLocation.Offset << " size " << invalid->Size << " overwritten by "
              << keyToStr(overwriter.Key) << " in O"
              << keyToStr(overwriter.RecordedLocation.ResourceKey) << " offset "
              << overwriter.RecordedLocation.Offset << " size " << overwriter.Size
              << " overlapping " << overlapEnd - overlapStart << " bytes";
    GITS_ASSERT(false, "RaytracingAssertionsLayer: overwritten RTAS was used");
  }
}

const RaytracingAssertionsLayer::Build* RaytracingAssertionsLayer::FindFirstInvalidBuild(
    const Build& build) {
  if (build.DeletedBy || build.Overwriter) {
    return &build;
  }
  for (const BuildPtr& input : build.Inputs) {
    if (const Build* invalid = FindFirstInvalidBuild(*input)) {
      return invalid;
    }
  }
  return nullptr;
}

void RaytracingAssertionsLayer::ValidateBuildFitsAllocation(
    CommandKey commandKey,
    D3D12_GPU_VIRTUAL_ADDRESS replayAddress,
    UINT64 sizeInBytes,
    const CapturePlayerGpuAddressService::ResourceInfo& resource) {
  const UINT64 resourceOffset = replayAddress - resource.PlayerStart;
  const UINT64 resourceBytes = resource.Resource->GetDesc().Width;
  const bool placed = m_PlacedResources.contains(resource.Key);
  const UINT64 buildEnd = resourceOffset + sizeInBytes;

  if (buildEnd > resourceBytes) {
    std::stringstream message;
    message << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " RTAS build in O"
            << keyToStr(resource.Key) << " offset " << resourceOffset << " size " << sizeInBytes
            << " exceeds " << (placed ? "placed" : "committed") << " buffer (" << resourceBytes
            << " bytes) by " << buildEnd - resourceBytes << " bytes";

    if (!placed) {
      LOG_ERROR << message.str();
      GITS_ASSERT(false, "RaytracingAssertionsLayer: RTAS build exceeds committed resource");
    } else {
      LOG_WARNING << message.str();
    }
  }

  if (!placed) {
    return;
  }
  const auto heapIt = m_ResourceHeap.find(resource.Key);
  GITS_ASSERT(heapIt != m_ResourceHeap.end(),
              "RaytracingAssertionsLayer: placed resource missing heap");
  const auto heapBoundsIt = m_HeapReplayBounds.find(heapIt->second);
  GITS_ASSERT(heapBoundsIt != m_HeapReplayBounds.end(),
              "RaytracingAssertionsLayer: heap missing replay bounds");
  const D3D12_GPU_VIRTUAL_ADDRESS replayEnd = replayAddress + sizeInBytes;
  GITS_ASSERT(replayAddress >= heapBoundsIt->second.first,
              "RaytracingAssertionsLayer: RTAS build starts before heap replay range");
  if (replayEnd > heapBoundsIt->second.second) {
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " RTAS build in O"
              << keyToStr(resource.Key) << " offset " << resourceOffset << " size " << sizeInBytes
              << " exceeds heap O" << keyToStr(heapIt->second) << " replay range [0x" << std::hex
              << heapBoundsIt->second.first << ", 0x" << heapBoundsIt->second.second << ")"
              << std::dec << " by " << replayEnd - heapBoundsIt->second.second << " bytes";
    GITS_ASSERT(false, "RaytracingAssertionsLayer: RTAS build exceeds heap");
  }
}

void RaytracingAssertionsLayer::ValidateBuildScratchSize(
    const ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  const ObjectKey scratchKey = c.m_pDesc.ScratchAccelerationStructureKey;
  if (!scratchKey) {
    return;
  }
  const auto scratch = m_Resources.find(scratchKey);
  if (scratch == m_Resources.end()) {
    return;
  }

  ComPtr<ID3D12Device5> device;
  HRESULT hr = c.m_Object.Value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  const auto& inputs = c.m_pDesc.Value->Inputs;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO liveInfo{};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &liveInfo);

  const bool update =
      inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  const UINT64 requiredScratch =
      update ? liveInfo.UpdateScratchDataSizeInBytes : liveInfo.ScratchDataSizeInBytes;
  const UINT64 offset = c.m_pDesc.ScratchAccelerationStructureOffset;
  const UINT64 scratchBytes = scratch->second->GetDesc().Width;
  const UINT64 availableScratch = offset <= scratchBytes ? scratchBytes - offset : 0;
  if (offset > scratchBytes || requiredScratch > availableScratch) {
    const UINT64 exceeding =
        requiredScratch > availableScratch ? requiredScratch - availableScratch : requiredScratch;
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(c.Key) << " scratch buffer O"
              << keyToStr(scratchKey) << " has " << scratchBytes << " bytes at offset " << offset
              << " but live prebuild info requires " << requiredScratch << " bytes for "
              << (update ? "update" : "build") << " scratch, exceeding by " << exceeding
              << " bytes";
    GITS_ASSERT(false,
                "RaytracingAssertionsLayer: scratch buffer too small for live prebuild info");
  }
}

void RaytracingAssertionsLayer::LogMissingBuild(CommandKey commandKey,
                                                D3D12_GPU_VIRTUAL_ADDRESS replayAddress) {
  const CapturePlayerGpuAddressService::ResourceInfo& resource = FindReplayResource(replayAddress);
  const UINT64 offset = replayAddress - resource.PlayerStart;
  LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " uses RTAS in O"
            << keyToStr(resource.Key) << " offset " << offset;
  GITS_ASSERT(false, "RaytracingAssertionsLayer: used RTAS was erased or never built");
}

void RaytracingAssertionsLayer::LoadInstancePointers() {
  if (m_InstancePointersLoaded) {
    return;
  }
  m_InstancePointersLoaded = true;
  std::filesystem::path streamDir = Configurator::Get().common.player.streamDir;
  std::ifstream stream(streamDir / "raytracingArraysOfPointers.dat", std::ios::binary);
  CommandKey key{};
  while (stream.read(reinterpret_cast<char*>(&key), sizeof(key))) {
    unsigned count{};
    stream.read(reinterpret_cast<char*>(&count), sizeof(count));
    auto& addresses = m_InstancePointers[key];
    addresses.resize(count);
    stream.read(reinterpret_cast<char*>(addresses.data()),
                count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    if (!static_cast<bool>(stream)) {
      LOG_ERROR << "RaytracingAssertionsLayer: truncated raytracingArraysOfPointers.dat";
    }
  }
}

RaytracingAssertionsLayer::ReadbackBuffer RaytracingAssertionsLayer::StageReadback(
    ID3D12GraphicsCommandList* commandList, ObjectKey resourceKey, UINT64 offset, UINT64 size) {
  auto it = m_Resources.find(resourceKey);
  GITS_ASSERT(it != m_Resources.end(), "RaytracingAssertionsLayer: staged resource not found");
  ID3D12Resource* resource = it->second;
  GITS_ASSERT(size && offset + size <= resource->GetDesc().Width,
              "RaytracingAssertionsLayer: staged range exceeds the buffer");
  ReadbackBuffer readbackBuffer;
  readbackBuffer.Buffer = m_BufferPool.Acquire(m_Device.Get(), size, D3D12_HEAP_TYPE_READBACK,
                                               D3D12_RESOURCE_STATE_COPY_DEST);
  readbackBuffer.Size = size;
  ResourceStateEnhanced resourceState(commandList, resource,
                                      m_ResourceStates.GetResourceState(commandList, resourceKey));
  resourceState.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);
  commandList->CopyBufferRegion(readbackBuffer.Buffer.get(), 0, resource, offset, size);
  resourceState.RevertState();
  return readbackBuffer;
}

RaytracingAssertionsLayer::SizeQuery RaytracingAssertionsLayer::CreateSizeQuery() {
  constexpr UINT64 size =
      sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
  SizeQuery query;
  query.Uav = m_BufferPool.Acquire(m_Device.Get(), size, D3D12_HEAP_TYPE_DEFAULT,
                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  query.Readback = m_BufferPool.Acquire(m_Device.Get(), size, D3D12_HEAP_TYPE_READBACK,
                                        D3D12_RESOURCE_STATE_COPY_DEST);
  return query;
}

void RaytracingAssertionsLayer::RecordSizeQuery(ID3D12GraphicsCommandList* commandList,
                                                const SizeQuery& query) {
  if (!query.Uav) {
    return;
  }
  constexpr UINT64 size =
      sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC);
  auto transition = [&](D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = query.Uav.get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    commandList->ResourceBarrier(1, &barrier);
  };
  transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
  commandList->CopyBufferRegion(query.Readback.get(), 0, query.Uav.get(), 0, size);
  transition(D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

RaytracingAssertionsLayer::BuildPtr RaytracingAssertionsLayer::FindBuild(
    D3D12_GPU_VIRTUAL_ADDRESS replayAddress) const {
  const auto buildIt = m_Builds.find(replayAddress);
  return buildIt != m_Builds.end() ? buildIt->second : nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingAssertionsLayer::GetReplayAddress(ObjectKey resourceKey,
                                                                      UINT64 offset) const {
  const auto it = m_Resources.find(resourceKey);
  if (it == m_Resources.end()) {
    LOG_ERROR << "RaytracingAssertionsLayer: no replay GPUVA for O" << keyToStr(resourceKey);
  }
  GITS_ASSERT(it != m_Resources.end(),
              "RaytracingAssertionsLayer: unresolved RTAS resource replay address");
  return it->second->GetGPUVirtualAddress() + offset;
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingAssertionsLayer::GetReplayAddressFromMappings(
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  int first = 0;
  int last = static_cast<int>(gpuAddressMappings.size()) - 1;
  while (first <= last) {
    const int mid = first + (last - first) / 2;
    const auto& mapping = gpuAddressMappings[mid];
    if (captureAddress >= mapping.CaptureStart &&
        captureAddress < mapping.CaptureStart + mapping.Size) {
      return mapping.PlayerStart + (captureAddress - mapping.CaptureStart);
    }
    if (captureAddress >= mapping.CaptureStart + mapping.Size) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  GITS_ASSERT(!captureAddress, "RaytracingAssertionsLayer: unresolved instance BLAS GPUVA");
  return 0;
}

CapturePlayerGpuAddressService::ResourceInfo& RaytracingAssertionsLayer::FindReplayResource(
    D3D12_GPU_VIRTUAL_ADDRESS replayAddress) {
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_AddressService.GetResourceInfoByPlayerAddress(replayAddress, true);
  if (!info) {
    LOG_ERROR << "RaytracingAssertionsLayer: unresolved replay GPUVA 0x" << std::hex
              << replayAddress << std::dec;
    GITS_ASSERT(false, "RaytracingAssertionsLayer: unresolved replay GPU address");
  }
  GITS_ASSERT(info->PlayerStart, "RaytracingAssertionsLayer: replay GPUVA not registered");
  GITS_ASSERT(info->Resource, "RaytracingAssertionsLayer: replay GPUVA resource not registered");
  return *info;
}

CapturePlayerGpuAddressService::ResourceInfo& RaytracingAssertionsLayer::FindCaptureResource(
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_AddressService.GetResourceInfoByCaptureAddress(captureAddress, true);
  if (!info) {
    LOG_ERROR << "RaytracingAssertionsLayer: unresolved capture GPUVA 0x" << std::hex
              << captureAddress << std::dec;
    GITS_ASSERT(false, "RaytracingAssertionsLayer: unresolved capture GPU address");
  }
  GITS_ASSERT(info->CaptureStart, "RaytracingAssertionsLayer: capture GPUVA not registered");
  return *info;
}

void RaytracingAssertionsLayer::EraseBuildsInReplayRange(D3D12_GPU_VIRTUAL_ADDRESS start,
                                                         D3D12_GPU_VIRTUAL_ADDRESS end,
                                                         CommandKey deletedBy) {
  GITS_ASSERT(end > start);
  for (auto it = m_Builds.lower_bound(start); it != m_Builds.lower_bound(end);) {
    it->second->DeletedBy = deletedBy;
    it = m_Builds.erase(it);
  }
}

std::pair<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS> RaytracingAssertionsLayer::
    GetBackingAllocationBounds(const CapturePlayerGpuAddressService::ResourceInfo& resource) const {
  if (!m_PlacedResources.contains(resource.Key)) {
    const D3D12_GPU_VIRTUAL_ADDRESS start = resource.PlayerStart;
    return {start, start + resource.Resource->GetDesc().Width};
  }
  const auto heapIt = m_ResourceHeap.find(resource.Key);
  GITS_ASSERT(heapIt != m_ResourceHeap.end(),
              "RaytracingAssertionsLayer: placed resource missing heap");
  const auto heapBoundsIt = m_HeapReplayBounds.find(heapIt->second);
  GITS_ASSERT(heapBoundsIt != m_HeapReplayBounds.end(),
              "RaytracingAssertionsLayer: heap missing replay bounds");
  return heapBoundsIt->second;
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingAssertionsLayer::GetHeapGpuVirtualAddress(ID3D12Heap* heap) {
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC dummyDesc{};
  dummyDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  dummyDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  dummyDesc.Width = 1;
  dummyDesc.Height = 1;
  dummyDesc.DepthOrArraySize = 1;
  dummyDesc.MipLevels = 1;
  dummyDesc.Format = DXGI_FORMAT_UNKNOWN;
  dummyDesc.SampleDesc = {1, 0};
  dummyDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  dummyDesc.Flags = heapDesc.Flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER
                        ? D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER
                        : D3D12_RESOURCE_FLAG_NONE;

  ComPtr<ID3D12Device> device;
  HRESULT res = heap->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(res == S_OK);

  D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD ||
      heapDesc.Properties.Type == D3D12_HEAP_TYPE_GPU_UPLOAD) {
    initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
  } else if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK) {
    initialState = D3D12_RESOURCE_STATE_COPY_DEST;
  }

  ComPtr<ID3D12Resource> dummyResource;
  res = device->CreatePlacedResource(heap, 0, &dummyDesc, initialState, nullptr,
                                     IID_PPV_ARGS(&dummyResource));
  GITS_ASSERT(res == S_OK);

  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = dummyResource->GetGPUVirtualAddress();
  GITS_ASSERT(gpuAddress);

  return gpuAddress;
}

void RaytracingAssertionsLayer::StoreHeapReplayBounds(ObjectKey heapKey,
                                                      ID3D12Heap* heap,
                                                      UINT64 sizeInBytes) {
  if (!sizeInBytes) {
    return;
  }
  if (heap->GetDesc().Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  const D3D12_GPU_VIRTUAL_ADDRESS start = GetHeapGpuVirtualAddress(heap);
  m_HeapReplayBounds[heapKey] = {start, start + sizeInBytes};
}

void RaytracingAssertionsLayer::StoreResourceReplayBounds(ObjectKey resourceKey,
                                                          ID3D12Resource* resource) {
  const UINT64 width = resource->GetDesc().Width;
  if (!width) {
    return;
  }
  const D3D12_GPU_VIRTUAL_ADDRESS start = resource->GetGPUVirtualAddress();
  m_ResourceReplayBounds[resourceKey] = {start, start + width};
}

template <typename Command, typename State>
void RaytracingAssertionsLayer::AddResource(Command& c, State initialState, ObjectKey heapKey) {
  if (c.Skip || FAILED(c.m_Result.Value) || !c.m_ppvResource.Value || !*c.m_ppvResource.Value) {
    return;
  }
  auto* resource = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
  if (resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  const ObjectKey key = c.m_ppvResource.Key;
  if (!m_Device) {
    HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&m_Device));
    GITS_ASSERT(hr == S_OK);
  }
  if (heapKey) {
    m_AddressService.CreatePlacedResource(heapKey, key, c.m_pDesc.Value->Flags, initialState);
    m_PlacedResources.insert(key);
    m_ResourceHeap[key] = heapKey;
  } else {
    StoreResourceReplayBounds(key, resource);
  }
  m_Resources[key] = resource;
  m_ResourceStates.AddResource(resource, key, initialState);
}

RaytracingAssertionsLayer::BufferPool::Buffer RaytracingAssertionsLayer::BufferPool::Acquire(
    ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state) {
  const BufferType type{heapType, state};
  auto& free = m_Free[type];
  ComPtr<ID3D12Resource> buffer;
  if (auto it = free.lower_bound(size); it != free.end()) {
    buffer = std::move(it->second);
    free.erase(it);
  } else {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = heapType;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
      desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                 state, nullptr, IID_PPV_ARGS(&buffer));
    GITS_ASSERT(hr == S_OK);
    m_Types[buffer.Get()] = type;
  }
  return Buffer(buffer.Detach(), Releaser{this});
}

void RaytracingAssertionsLayer::BufferPool::Releaser::operator()(ID3D12Resource* resource) const {
  ComPtr<ID3D12Resource> released;
  released.Attach(resource);
  Pool->m_Free[Pool->m_Types.at(resource)].emplace(resource->GetDesc().Width, std::move(released));
}

std::vector<uint8_t> RaytracingAssertionsLayer::ReadBuffer(ID3D12Resource* resource, UINT64 size) {
  void* data{};
  D3D12_RANGE readRange{0, static_cast<SIZE_T>(size)};
  HRESULT hr = resource->Map(0, &readRange, &data);
  GITS_ASSERT(hr == S_OK);
  std::vector<uint8_t> result(static_cast<const uint8_t*>(data),
                              static_cast<const uint8_t*>(data) + size);
  D3D12_RANGE writtenRange{0, 0};
  resource->Unmap(0, &writtenRange);
  return result;
}

} // namespace DirectX
} // namespace gits
