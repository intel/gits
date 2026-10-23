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

RaytracingAssertionsLayer::RaytracingAssertionsLayer() : Layer("RaytracingAssertions") {}

RaytracingAssertionsLayer::~RaytracingAssertionsLayer() {
  ProcessReadyEvents(true);
}

void RaytracingAssertionsLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  m_BuildSkipped = c.Skip;
  if (c.Skip) {
    return;
  }
  RecordBuildScratch(c);
  const auto& desc = *c.m_pDesc.Value;
  const bool update =
      desc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  Operation operation;
  operation.Key = c.Key;
  operation.RecordedDestination = ResourceLocation{c.m_pDesc.DestAccelerationStructureKey,
                                                   c.m_pDesc.DestAccelerationStructureOffset};
  operation.DestinationLocation = GetHeapLocation(c.m_pDesc.DestAccelerationStructureKey,
                                                  c.m_pDesc.DestAccelerationStructureOffset);
  if (update) {
    operation.SourceLocations.push_back(GetHeapLocation(
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
          operation.OmmArrayLocations.push_back(
              GetHeapLocation(c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]));
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
  GITS_ASSERT(c.Skip == m_BuildSkipped,
              "RaytracingAssertionsLayer: Skip modified between RTAS build Pre and Post, "
              "possibly by another enabled feature");
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
    operation.SourceLocations.push_back(GetHeapLocation(sourceKey, sourceOffset));
  }
  if (mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE ||
      mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT ||
      mode == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE) {
    operation.RecordedDestination = ResourceLocation{c.m_DestAccelerationStructureData.InterfaceKey,
                                                     c.m_DestAccelerationStructureData.Offset};
    operation.DestinationLocation = GetHeapLocation(c.m_DestAccelerationStructureData.InterfaceKey,
                                                    c.m_DestAccelerationStructureData.Offset);
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
    operation.SourceLocations.push_back(
        GetHeapLocation(c.m_pSourceAccelerationStructureData.InterfaceKeys[i],
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

void RaytracingAssertionsLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (c.Skip) {
    return;
  }
  std::vector<const ScratchUsage*> concurrentScratch;
  for (UINT i = 0; i < c.m_NumCommandLists.Value; ++i) {
    auto recorded = m_RecordedScratch.find(c.m_ppCommandLists.Keys[i]);
    if (recorded == m_RecordedScratch.end()) {
      continue;
    }
    for (const ScratchEvent& event : recorded->second) {
      if (const auto* usage = std::get_if<ScratchUsage>(&event)) {
        ValidateBuildScratch(*usage, concurrentScratch);
        continue;
      }
      concurrentScratch.clear();
    }
  }
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
  m_HeapSizes[c.m_ppvHeap.Key] = c.m_pDesc.Value->SizeInBytes;
}

void RaytracingAssertionsLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || !c.m_ppvHeap.Value || !*c.m_ppvHeap.Value ||
      !c.m_pDesc.Value) {
    return;
  }
  m_HeapSizes[c.m_ppvHeap.Key] = c.m_pDesc.Value->SizeInBytes;
}

void RaytracingAssertionsLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  AddResource(c, c.m_InitialState.Value, c.m_pHeap.Key, c.m_HeapOffset.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  AddResource(c, c.m_InitialState.Value, c.m_pHeap.Key, c.m_HeapOffset.Value);
}

void RaytracingAssertionsLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  AddResource(c, c.m_InitialLayout.Value, c.m_pHeap.Key, c.m_HeapOffset.Value);
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

void RaytracingAssertionsLayer::Post(IUnknownReleaseCommand& c) {
  if (c.Skip || c.m_Result.Value != 0) {
    return;
  }
  ProcessReadyEvents();
  EraseBuilds(c.m_Object.Key, c.Key);
  m_HeapSizes.erase(c.m_Object.Key);
  m_ResourceHeapLocations.erase(c.m_Object.Key);
  m_Resources.erase(c.m_Object.Key);
  m_AddressService.DestroyInterface(c.m_Object.Key);
  m_RecordedOperations.erase(c.m_Object.Key);
  m_RecordedScratch.erase(c.m_Object.Key);
  m_QueueFences.erase(c.m_Object.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStates.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value, c.m_NumBarriers.Value,
                                   c.m_pBarriers.ResourceKeys.data());
  RecordScratchBarrier(c.m_Object.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStates.ResourceBarrier(c.m_Object.Value, c.m_pBarrierGroups.Value,
                                   c.m_NumBarrierGroups.Value,
                                   c.m_pBarrierGroups.ResourceKeys.data());
  RecordScratchBarrier(c.m_Object.Key);
}

void RaytracingAssertionsLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (c.Skip || FAILED(c.m_Result.Value)) {
    return;
  }
  m_RecordedOperations.erase(c.m_Object.Key);
  m_RecordedScratch.erase(c.m_Object.Key);
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
    for (const Operation& operation : validationEvent.Operations) {
      ValidateOperation(operation);
    }
    m_PendingValidationEvents.pop();
  }
}

void RaytracingAssertionsLayer::ValidateOperation(const Operation& operation) {
  std::set<BuildPtr> inputs;
  ValidateOperationSources(operation, inputs);
  ValidateOperationTlasInputs(operation, inputs);
  ValidateOperationSerialization(operation, inputs);
  ValidateOperationOmmArrayInputs(operation, inputs);

  if (!operation.RecordedDestination) {
    return;
  }

  BuildPtr newBuild = std::make_shared<Build>();
  newBuild->Key = operation.Key;
  newBuild->RecordedLocation = *operation.RecordedDestination;
  newBuild->Location = operation.DestinationLocation;
  newBuild->Inputs = std::move(inputs);
  {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC size{};
    const std::vector<uint8_t> data =
        ReadBuffer(operation.PostbuildSizeQuery.Readback.get(), sizeof(size));
    std::memcpy(&size, data.data(), sizeof(size));
    GITS_ASSERT(size.CurrentSizeInBytes, "RaytracingAssertionsLayer: RTAS size zero");
    newBuild->Size = size.CurrentSizeInBytes;
  }

  ValidateBuildFitsAllocation(*newBuild);

  std::map<UINT64, BuildPtr>& heapBuilds = m_Builds[newBuild->Location.HeapKey];
  const UINT64 newStart = newBuild->Location.Offset;
  const UINT64 newEnd = newStart + newBuild->Size;
  const auto scanEnd = heapBuilds.lower_bound(newEnd);
  for (auto it = heapBuilds.begin(); it != scanEnd; ++it) {
    const auto& [existingStart, existingBuild] = *it;
    if (!existingBuild->Overwriter && newStart < existingStart + existingBuild->Size) {
      existingBuild->Overwriter = newBuild;
    }
  }

  heapBuilds[newStart] = newBuild;
}

void RaytracingAssertionsLayer::ValidateOperationSources(const Operation& operation,
                                                         std::set<BuildPtr>& inputs) {
  for (const HeapLocation& sourceLocation : operation.SourceLocations) {
    BuildPtr build = FindBuild(sourceLocation);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      if (operation.InheritInputs) {
        inputs.insert(build->Inputs.begin(), build->Inputs.end());
      }
    } else {
      LogMissingBuild(operation.Key, sourceLocation);
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationTlasInputs(const Operation& operation,
                                                            std::set<BuildPtr>& inputs) {
  std::set<HeapLocation> seenLocations;
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
      const std::optional<HeapLocation> blasLocation =
          FindBuildHeapLocation(instance.AccelerationStructure);
      if (blasLocation && !seenLocations.insert(*blasLocation).second) {
        continue;
      }
      BuildPtr build = blasLocation ? FindBuild(*blasLocation) : nullptr;
      if (build) {
        ValidateBuildDependency(operation.Key, *build);
        inputs.insert(std::move(build));
      } else {
        LOG_ERROR << "RaytracingAssertionsLayer: TLAS build " << keyToStr(operation.Key)
                  << " INSTANCE " << i + 1 << " (InstanceID " << instance.InstanceID
                  << ") references missing BLAS at capture address 0x" << std::hex
                  << instance.AccelerationStructure << std::dec;
        if (blasLocation) {
          LogMissingBuild(operation.Key, *blasLocation);
        } else {
          GITS_ASSERT(false, "RaytracingAssertionsLayer: unresolved BLAS capture address");
        }
      }
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationOmmArrayInputs(const Operation& operation,
                                                                std::set<BuildPtr>& inputs) {
  std::set<HeapLocation> seenLocations;
  for (const HeapLocation& ommArrayLocation : operation.OmmArrayLocations) {
    if (!seenLocations.insert(ommArrayLocation).second) {
      continue;
    }
    BuildPtr build = FindBuild(ommArrayLocation);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      inputs.insert(std::move(build));
    } else {
      LogMissingBuild(operation.Key, ommArrayLocation);
    }
  }
}

void RaytracingAssertionsLayer::ValidateOperationSerialization(const Operation& operation,
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

  std::set<HeapLocation> seenLocations;
  for (UINT64 i = 0; i < header.NumBottomLevelAccelerationStructurePointersAfterHeader; ++i) {
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress{};
    std::memcpy(&captureAddress, data.data() + sizeof(header) + i * sizeof(captureAddress),
                sizeof(captureAddress));
    if (!captureAddress) {
      continue;
    }
    const std::optional<HeapLocation> blasLocation = FindBuildHeapLocation(captureAddress);
    if (!blasLocation) {
      LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(operation.Key)
                << " serialized RTAS BLAS pointer " << i + 1
                << " references unresolved capture address 0x" << std::hex << captureAddress
                << std::dec;
      GITS_ASSERT(false, "RaytracingAssertionsLayer: unresolved BLAS capture address");
      continue;
    }
    if (!seenLocations.insert(*blasLocation).second) {
      continue;
    }
    BuildPtr build = FindBuild(*blasLocation);
    if (build) {
      ValidateBuildDependency(operation.Key, *build);
      inputs.insert(std::move(build));
    } else {
      LogMissingBuild(operation.Key, *blasLocation);
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
    const uint64_t overlapStart = std::max(invalid->Location.Offset, overwriter.Location.Offset);
    const uint64_t overlapEnd = std::min(invalid->Location.Offset + invalid->Size,
                                         overwriter.Location.Offset + overwriter.Size);
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

void RaytracingAssertionsLayer::ValidateBuildFitsAllocation(const Build& build) {
  const ResourceLocation& resource = build.RecordedLocation;
  const auto resourceIt = m_Resources.find(resource.ResourceKey);
  GITS_ASSERT(resourceIt != m_Resources.end(),
              "RaytracingAssertionsLayer: RTAS build destination resource not found");
  const UINT64 resourceBytes = resourceIt->second->GetDesc().Width;
  const bool placed = build.Location.HeapKey != resource.ResourceKey;
  const UINT64 buildEnd = resource.Offset + build.Size;

  if (buildEnd > resourceBytes) {
    std::stringstream message;
    message << "RaytracingAssertionsLayer: " << keyToStr(build.Key) << " RTAS build in O"
            << keyToStr(resource.ResourceKey) << " offset " << resource.Offset << " size "
            << build.Size << " exceeds " << (placed ? "placed" : "committed") << " buffer ("
            << resourceBytes << " bytes) by " << buildEnd - resourceBytes << " bytes";

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
  const auto heapSizeIt = m_HeapSizes.find(build.Location.HeapKey);
  GITS_ASSERT(heapSizeIt != m_HeapSizes.end(), "RaytracingAssertionsLayer: heap size not found");
  const UINT64 heapBytes = heapSizeIt->second;
  const UINT64 heapEnd = build.Location.Offset + build.Size;
  if (heapEnd > heapBytes) {
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(build.Key) << " RTAS build in O"
              << keyToStr(resource.ResourceKey) << " offset " << resource.Offset << " size "
              << build.Size << " at heap O" << keyToStr(build.Location.HeapKey) << " offset "
              << build.Location.Offset << " exceeds heap (" << heapBytes << " bytes) by "
              << heapEnd - heapBytes << " bytes";
    GITS_ASSERT(false, "RaytracingAssertionsLayer: RTAS build exceeds heap");
  }
}

void RaytracingAssertionsLayer::LogMissingBuild(CommandKey commandKey,
                                                const HeapLocation& location) {
  LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(commandKey) << " uses RTAS in O"
            << keyToStr(location.HeapKey) << " offset " << location.Offset;
  GITS_ASSERT(false, "RaytracingAssertionsLayer: used RTAS was erased or never built");
}

void RaytracingAssertionsLayer::RecordBuildScratch(
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

  ScratchUsage usage;
  usage.Key = c.Key;
  usage.ResourceKey = scratchKey;
  usage.Offset = c.m_pDesc.ScratchAccelerationStructureOffset;
  usage.ResourceSize = scratch->second->GetDesc().Width;
  usage.Location = GetHeapLocation(scratchKey, usage.Offset);
  usage.Update = inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  usage.RequiredSize =
      usage.Update ? liveInfo.UpdateScratchDataSizeInBytes : liveInfo.ScratchDataSizeInBytes;
  m_RecordedScratch[c.m_Object.Key].emplace_back(std::move(usage));
}

void RaytracingAssertionsLayer::RecordScratchBarrier(ObjectKey commandListKey) {
  std::vector<ScratchEvent>& events = m_RecordedScratch[commandListKey];
  if (events.empty() || !std::holds_alternative<ScratchBarrier>(events.back())) {
    events.emplace_back(ScratchBarrier{});
  }
}

void RaytracingAssertionsLayer::ValidateBuildScratch(
    const ScratchUsage& usage, std::vector<const ScratchUsage*>& concurrentScratch) {
  ValidateBuildScratchSize(usage);
  if (!usage.RequiredSize) {
    return;
  }
  for (const ScratchUsage* concurrent : concurrentScratch) {
    ValidateBuildScratchOverlap(usage, *concurrent);
  }
  concurrentScratch.push_back(&usage);
}

void RaytracingAssertionsLayer::ValidateBuildScratchSize(const ScratchUsage& usage) {
  const UINT64 availableScratch =
      usage.Offset <= usage.ResourceSize ? usage.ResourceSize - usage.Offset : 0;
  if (usage.Offset > usage.ResourceSize || usage.RequiredSize > availableScratch) {
    const UINT64 exceeding = usage.RequiredSize > availableScratch
                                 ? usage.RequiredSize - availableScratch
                                 : usage.RequiredSize;
    LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(usage.Key) << " scratch buffer O"
              << keyToStr(usage.ResourceKey) << " has " << usage.ResourceSize << " bytes at offset "
              << usage.Offset << " but live prebuild info requires " << usage.RequiredSize
              << " bytes for " << (usage.Update ? "update" : "build") << " scratch, exceeding by "
              << exceeding << " bytes";
    GITS_ASSERT(false,
                "RaytracingAssertionsLayer: scratch buffer too small for live prebuild info");
  }
}

void RaytracingAssertionsLayer::ValidateBuildScratchOverlap(const ScratchUsage& usage,
                                                            const ScratchUsage& concurrent) {
  if (usage.Location.HeapKey != concurrent.Location.HeapKey) {
    return;
  }
  const UINT64 overlapStart = std::max(usage.Location.Offset, concurrent.Location.Offset);
  const UINT64 overlapEnd = std::min(usage.Location.Offset + usage.RequiredSize,
                                     concurrent.Location.Offset + concurrent.RequiredSize);
  if (overlapStart >= overlapEnd) {
    return;
  }
  LOG_ERROR << "RaytracingAssertionsLayer: " << keyToStr(usage.Key) << " scratch in O"
            << keyToStr(usage.ResourceKey) << " offset " << usage.Offset << " size "
            << usage.RequiredSize << " overlaps scratch of " << keyToStr(concurrent.Key) << " in O"
            << keyToStr(concurrent.ResourceKey) << " offset " << concurrent.Offset << " size "
            << concurrent.RequiredSize << " by " << overlapEnd - overlapStart
            << " bytes without a barrier between the builds";
  GITS_ASSERT(false, "RaytracingAssertionsLayer: concurrent RTAS builds overlap in scratch buffer");
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
    const HeapLocation& location) const {
  const auto heapBuilds = m_Builds.find(location.HeapKey);
  if (heapBuilds == m_Builds.end()) {
    return nullptr;
  }
  const auto buildIt = heapBuilds->second.find(location.Offset);
  return buildIt != heapBuilds->second.end() ? buildIt->second : nullptr;
}

RaytracingAssertionsLayer::HeapLocation RaytracingAssertionsLayer::GetHeapLocation(
    ObjectKey resourceKey, UINT64 offset) const {
  const auto it = m_ResourceHeapLocations.find(resourceKey);
  if (it == m_ResourceHeapLocations.end()) {
    LOG_ERROR << "RaytracingAssertionsLayer: no heap location for O" << keyToStr(resourceKey);
  }
  GITS_ASSERT(it != m_ResourceHeapLocations.end(),
              "RaytracingAssertionsLayer: unresolved RTAS resource heap location");
  return {it->second.HeapKey, it->second.Offset + offset};
}

std::optional<RaytracingAssertionsLayer::HeapLocation> RaytracingAssertionsLayer::
    FindBuildHeapLocation(D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  const CapturePlayerGpuAddressService::ResourceInfo* resource =
      TryFindCaptureResource(captureAddress);
  if (!resource || !resource->CaptureStart) {
    return std::nullopt;
  }
  return GetHeapLocation(resource->Key, captureAddress - resource->CaptureStart);
}

CapturePlayerGpuAddressService::ResourceInfo& RaytracingAssertionsLayer::FindCaptureResource(
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  CapturePlayerGpuAddressService::ResourceInfo* info = TryFindCaptureResource(captureAddress);
  if (!info) {
    LOG_ERROR << "RaytracingAssertionsLayer: unresolved capture GPUVA 0x" << std::hex
              << captureAddress << std::dec;
    GITS_ASSERT(false, "RaytracingAssertionsLayer: unresolved capture GPU address");
  }
  GITS_ASSERT(info->CaptureStart, "RaytracingAssertionsLayer: capture GPUVA not registered");
  return *info;
}

CapturePlayerGpuAddressService::ResourceInfo* RaytracingAssertionsLayer::TryFindCaptureResource(
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_AddressService.GetResourceInfoByCaptureAddress(captureAddress, true);
  if (info) {
    return info;
  }
  info = m_AddressService.GetResourceInfoByCaptureAddress(captureAddress, false);
  if (info) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "RaytracingAssertionsLayer: capture GPUVA 0x" << std::hex << captureAddress
                << std::dec << " not resolved as RTAS resource, resolved as non-RTAS resource O"
                << keyToStr(info->Key);
      logged = true;
    }
  }
  return info;
}

void RaytracingAssertionsLayer::EraseBuilds(ObjectKey heapKey, CommandKey deletedBy) {
  const auto heapBuilds = m_Builds.find(heapKey);
  if (heapBuilds == m_Builds.end()) {
    return;
  }
  for (const auto& [offset, build] : heapBuilds->second) {
    build->DeletedBy = deletedBy;
  }
  m_Builds.erase(heapBuilds);
}

template <typename Command, typename State>
void RaytracingAssertionsLayer::AddResource(Command& c,
                                            State initialState,
                                            ObjectKey heapKey,
                                            UINT64 heapOffset) {
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
    m_ResourceHeapLocations[key] = {heapKey, heapOffset};
  } else {
    m_ResourceHeapLocations[key] = {key, 0};
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
