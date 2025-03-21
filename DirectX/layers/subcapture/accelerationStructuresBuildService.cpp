// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBuildService.h"
#include "stateTrackingService.h"
#include "arguments.h"
#include "argumentEncoders.h"
#include "argumentDecoders.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "reservedResourcesService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

AccelerationStructuresBuildService::AccelerationStructuresBuildService(
    StateTrackingService& stateService,
    SubcaptureRecorder& recorder,
    ReservedResourcesService& reservedResourcesService)
    : stateService_(stateService),
      recorder_(recorder),
      reservedResourcesService_(reservedResourcesService),
      bufferContentRestore_(stateService) {
  commandListKey_ = stateService_.getUniqueObjectKey();
  serializeMode_ = Config::Get().directx.features.subcapture.serializeAccelerationStructures;
  restoreTLASes_ = Config::Get().directx.features.subcapture.restoreTLASes;
}

void AccelerationStructuresBuildService::buildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (serializeMode_ &&
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    return;
  }
  if (!restoreTLASes_ &&
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  if (restored_) {
    return;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.pDesc_.value->Inputs;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.object_.value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
  if (info.ScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.ScratchDataSizeInBytes;
  }
  if (info.UpdateScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.UpdateScratchDataSizeInBytes;
  }

  BuildRaytracingAccelerationStructureState* state =
      new BuildRaytracingAccelerationStructureState();
  state->commandKey = c.key;
  state->commandListKey = commandListKey_;
  state->buildState = true;
  state->destKey = c.pDesc_.destAccelerationStructureKey;
  state->destOffset = c.pDesc_.destAccelerationStructureOffset;
  state->update = c.pDesc_.value->Inputs.Flags &
                  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  if (serializeMode_ &&
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    tlases_.insert(std::make_pair(state->destKey, state->destOffset));
  }

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  auto storeBuffer = [&](unsigned inputIndex, D3D12_GPU_VIRTUAL_ADDRESS address, unsigned size) {
    unsigned inputKey = c.pDesc_.inputKeys[inputIndex];
    stateService_.keepState(inputKey);
    ResourceState* bufferState = static_cast<ResourceState*>(stateService_.getState(inputKey));
    unsigned offset = address - bufferState->gpuVirtualAddress;
    unsigned uploadResourceKey = stateService_.getUniqueObjectKey();
    state->uploadBuffers.push_back(uploadResourceKey);
    D3D12_RESOURCE_STATES resourceState = bufferState->isGenericRead
                                              ? D3D12_RESOURCE_STATE_GENERIC_READ
                                              : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bufferContentRestore_.storeBuffer(
        c.object_.value, commandListKey_, static_cast<ID3D12Resource*>(bufferState->object),
        inputKey, offset, size, resourceState, c.key, bufferState->isMappable, uploadResourceKey);
    state->buffers[inputKey] = bufferState;
    ReservedResourcesService::TiledResource* tiledResource =
        reservedResourcesService_.getTiledResource(inputKey);
    if (tiledResource) {
      auto it = state->tiledResources.find(inputKey);
      if (it == state->tiledResources.end()) {
        state->tiledResources[inputKey] = *tiledResource;
      }
      for (ReservedResourcesService::Tile& tile : tiledResource->tiles) {
        if (tile.heapKey) {
          stateService_.keepState(tile.heapKey);
        }
      }
    }
  };

  stateService_.keepState(c.pDesc_.destAccelerationStructureKey);

  unsigned inputIndex = 0;
  if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.NumDescs) {
    if (inputs.NumDescs) {
      unsigned size = inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      storeBuffer(inputIndex, inputs.InstanceDescs, size);
    }
  } else {
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.pDesc_.value->Inputs.pGeometryDescs[i]
              : *c.pDesc_.value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        if (desc.Triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          storeBuffer(inputIndex, desc.Triangles.Transform3x4, size);
        }
        ++inputIndex;
        if (desc.Triangles.IndexBuffer && desc.Triangles.IndexCount) {
          unsigned size = desc.Triangles.IndexCount *
                          (desc.Triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          storeBuffer(inputIndex, desc.Triangles.IndexBuffer, size);
        }
        ++inputIndex;
        if (desc.Triangles.VertexBuffer.StartAddress && desc.Triangles.VertexCount) {
          unsigned stride = desc.Triangles.VertexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.Triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          unsigned size = desc.Triangles.VertexCount * stride;
          storeBuffer(inputIndex, desc.Triangles.VertexBuffer.StartAddress, size);
        }
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        if (desc.AABBs.AABBs.StartAddress && desc.AABBs.AABBCount) {
          unsigned size = desc.AABBs.AABBCount * desc.AABBs.AABBs.StrideInBytes;
          storeBuffer(inputIndex, desc.AABBs.AABBs.StartAddress, size);
        }
        ++inputIndex;
      }
    }
  }

  statesByCommandList_[c.object_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::copyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (serializeMode_) {
    auto it = tlases_.find(std::make_pair(c.DestAccelerationStructureData_.interfaceKey,
                                          c.DestAccelerationStructureData_.offset));
    if (it == tlases_.end()) {
      return;
    }
  }
  CopyRaytracingAccelerationStructureState* state = new CopyRaytracingAccelerationStructureState();
  state->commandKey = c.key;
  state->commandListKey = commandListKey_;
  state->destAccelerationStructureData = c.DestAccelerationStructureData_.value;
  state->destAccelerationStructureKey = c.DestAccelerationStructureData_.interfaceKey;
  state->destAccelerationStructureOffset = c.DestAccelerationStructureData_.offset;
  state->sourceAccelerationStructureData = c.SourceAccelerationStructureData_.value;
  state->sourceAccelerationStructureKey = c.SourceAccelerationStructureData_.interfaceKey;
  state->sourceAccelerationStructureOffset = c.SourceAccelerationStructureData_.offset;
  state->mode = c.Mode_.value;

  stateService_.keepState(c.DestAccelerationStructureData_.interfaceKey);

  statesByCommandList_[c.object_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::restoreAccelerationStructures() {
  if (states_.empty()) {
    return;
  }

  bufferContentRestore_.waitUntilDumped();

  {
    commandQueueKey_ = stateService_.getUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.key = stateService_.getUniqueCommandKey();
    createCommandQueue.object_.key = deviceKey_;
    createCommandQueue.pDesc_.value = &commandQueueDesc;
    createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
    createCommandQueue.ppCommandQueue_.key = commandQueueKey_;
    stateService_.recorder_.record(new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

    commandAllocatorKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.key = stateService_.getUniqueCommandKey();
    createCommandAllocator.object_.key = deviceKey_;
    createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
    createCommandAllocator.ppCommandAllocator_.key = commandAllocatorKey_;
    stateService_.recorder_.record(
        new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.key = stateService_.getUniqueCommandKey();
    createCommandList.object_.key = deviceKey_;
    createCommandList.nodeMask_.value = 0;
    createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
    createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.pInitialState_.value = nullptr;
    createCommandList.riid_.value = IID_ID3D12CommandList;
    createCommandList.ppCommandList_.key = commandListKey_;
    stateService_.recorder_.record(new ID3D12DeviceCreateCommandListWriter(createCommandList));

    fenceKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.key = stateService_.getUniqueCommandKey();
    createFence.object_.key = deviceKey_;
    createFence.InitialValue_.value = 0;
    createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
    createFence.riid_.value = IID_ID3D12Fence;
    createFence.ppFence_.key = fenceKey_;
    stateService_.recorder_.record(new ID3D12DeviceCreateFenceWriter(createFence));
  }

  unsigned scratchResourceKey = stateService_.getUniqueObjectKey();

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = maxBuildScratchSpace_;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  ID3D12DeviceCreateCommittedResourceCommand createScratch;
  createScratch.key = stateService_.getUniqueCommandKey();
  createScratch.object_.key = deviceKey_;
  createScratch.pHeapProperties_.value = &heapProperties;
  createScratch.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
  createScratch.pDesc_.value = &resourceDesc;
  createScratch.InitialResourceState_.value = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  createScratch.pOptimizedClearValue_.value = nullptr;
  createScratch.riidResource_.value = IID_ID3D12Resource;
  createScratch.ppvResource_.key = scratchResourceKey;
  recorder_.record(new ID3D12DeviceCreateCommittedResourceWriter(createScratch));

  ID3D12ResourceGetGPUVirtualAddressCommand getAddress{};
  getAddress.key = stateService_.getUniqueCommandKey();
  getAddress.object_.key = scratchResourceKey;
  recorder_.record(new ID3D12ResourceGetGPUVirtualAddressWriter(getAddress));

  // remove previous builds to the same destination
  std::map<std::pair<unsigned, unsigned>, std::unordered_set<unsigned>>
      restoreBuildsByDestKeyOffset;
  std::map<std::pair<unsigned, unsigned>, std::unordered_set<unsigned>> copiedBuildsByKeyOffset;
  for (auto& itState : states_) {
    if (itState->buildState) {
      BuildRaytracingAccelerationStructureState* state =
          static_cast<BuildRaytracingAccelerationStructureState*>(itState.get());
      if (!state->update) {
        auto itRestoreBuilds =
            restoreBuildsByDestKeyOffset.find(std::pair(state->destKey, state->destOffset));
        if (itRestoreBuilds != restoreBuildsByDestKeyOffset.end()) {
          restoreBuildsByDestKeyOffset.erase(itRestoreBuilds);
        }
      }
      restoreBuildsByDestKeyOffset[std::pair(state->destKey, state->destOffset)].insert(
          state->commandKey);
    } else {
      CopyRaytracingAccelerationStructureState* state =
          static_cast<CopyRaytracingAccelerationStructureState*>(itState.get());

      auto itSourceRestoreBuilds = restoreBuildsByDestKeyOffset.find(std::pair(
          state->sourceAccelerationStructureKey, state->sourceAccelerationStructureOffset));
      if (itSourceRestoreBuilds != restoreBuildsByDestKeyOffset.end()) {
        auto& copiedBuilds = copiedBuildsByKeyOffset[std::pair(
            state->sourceAccelerationStructureKey, state->sourceAccelerationStructureOffset)];
        for (unsigned buildKey : itSourceRestoreBuilds->second) {
          copiedBuilds.insert(buildKey);
        }
      }

      auto itDestRestoreBuilds = restoreBuildsByDestKeyOffset.find(
          std::pair(state->destAccelerationStructureKey, state->destAccelerationStructureOffset));
      if (itDestRestoreBuilds != restoreBuildsByDestKeyOffset.end()) {
        restoreBuildsByDestKeyOffset.erase(itDestRestoreBuilds);
      }
    }
  }

  std::map<std::pair<unsigned, unsigned>, uint64_t> bufferHashesByKeyOffset;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> tiledResourceUpdatesRestored;

  for (auto& itState : states_) {
    if (itState->buildState) {
      BuildRaytracingAccelerationStructureState* state =
          static_cast<BuildRaytracingAccelerationStructureState*>(itState.get());

      auto itRestoreBuilds =
          restoreBuildsByDestKeyOffset.find(std::pair(state->destKey, state->destOffset));
      if (itRestoreBuilds == restoreBuildsByDestKeyOffset.end() ||
          itRestoreBuilds->second.find(state->commandKey) == itRestoreBuilds->second.end()) {
        auto itCopiedBuilds =
            copiedBuildsByKeyOffset.find(std::pair(state->destKey, state->destOffset));
        if (itCopiedBuilds == copiedBuildsByKeyOffset.end() ||
            itCopiedBuilds->second.find(state->commandKey) == itCopiedBuilds->second.end()) {
          continue;
        }
      }

      std::unordered_set<unsigned> restoredBuffers;
      std::unordered_set<unsigned> uploadBuffers;

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          bufferContentRestore_.getRestoreInfos(state->commandKey);
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.bufferKey, info.offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.bufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.bufferKey, info.offset)] = info.bufferHash;
        restoredBuffers.insert(info.bufferKey);
        uploadBuffers.insert(info.uploadResourceKey);

        for (auto& itTiledResource : state->tiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.bufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.updateId) == it->second.end()) {
            reservedResourcesService_.updateTileMappings(itTiledResource.second, commandQueueKey_,
                                                         nullptr);
            tiledResourceUpdatesRestored[info.bufferKey].insert(itTiledResource.second.updateId);
          }
        }

        for (CommandWriter* command : info.restoreCommands) {
          recorder_.record(command);
        }
      }

      for (auto& it : state->buffers) {
        if (!it.second->isMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.key = stateService_.getUniqueCommandKey();
          barrierCommand.object_.key = commandListKey_;
          barrierCommand.NumBarriers_.value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.pBarriers_.value = &barrier;
          barrierCommand.pBarriers_.size = 1;
          barrierCommand.pBarriers_.resourceKeys.resize(1);
          barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrierCommand.pBarriers_.resourceKeys[0] = it.first;
          stateService_.recorder_.record(
              new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
        }
      }

      PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> descArg;
      unsigned offset{};
      decode(state->descEncoded.get(), offset, descArg);

      descArg.scratchAccelerationStructureKey = scratchResourceKey;
      descArg.scratchAccelerationStructureOffset = 0;

      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand build;
      build.key = state->commandKey;
      build.object_.key = commandListKey_;
      build.pDesc_ = std::move(descArg);
      build.NumPostbuildInfoDescs_.value = 0;
      build.pPostbuildInfoDescs_.value = nullptr;
      recorder_.record(
          new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureWriter(build));

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListKey_;
        stateService_.recorder_.record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListKey_;
        stateService_.recorder_.record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.recorder_.record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorKey_;
        stateService_.recorder_.record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListKey_;
        commandListReset.pAllocator_.key = commandAllocatorKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.recorder_.record(new ID3D12GraphicsCommandListResetWriter(commandListReset));

        for (unsigned uploadResourceKey : uploadBuffers) {
          IUnknownReleaseCommand releaseCommand;
          releaseCommand.key = stateService_.getUniqueCommandKey();
          releaseCommand.object_.key = uploadResourceKey;
          stateService_.recorder_.record(new IUnknownReleaseWriter(releaseCommand));
        }
      }
    } else {
      CopyRaytracingAccelerationStructureState* state =
          static_cast<CopyRaytracingAccelerationStructureState*>(itState.get());

      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copy;
      copy.key = state->commandKey;
      copy.object_.key = commandListKey_;
      copy.DestAccelerationStructureData_.value = state->destAccelerationStructureData;
      copy.DestAccelerationStructureData_.interfaceKey = state->destAccelerationStructureKey;
      copy.DestAccelerationStructureData_.offset = state->destAccelerationStructureOffset;
      copy.SourceAccelerationStructureData_.value = state->sourceAccelerationStructureData;
      copy.SourceAccelerationStructureData_.interfaceKey = state->sourceAccelerationStructureKey;
      copy.SourceAccelerationStructureData_.offset = state->sourceAccelerationStructureOffset;
      copy.Mode_.value = state->mode;
      stateService_.recorder_.record(
          new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureWriter(copy));

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListKey_;
        stateService_.recorder_.record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListKey_;
        stateService_.recorder_.record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.recorder_.record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorKey_;
        stateService_.recorder_.record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListKey_;
        commandListReset.pAllocator_.key = commandAllocatorKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.recorder_.record(new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }
    }
  }

  IUnknownReleaseCommand release{};
  release.key = stateService_.getUniqueCommandKey();
  release.object_.key = scratchResourceKey;
  stateService_.recorder_.record(new IUnknownReleaseWriter(release));

  restored_ = true;
}

void AccelerationStructuresBuildService::executeCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (restored_) {
    return;
  }
  for (unsigned commandListKey : c.ppCommandLists_.keys) {
    auto itStates = statesByCommandList_.find(commandListKey);
    if (itStates != statesByCommandList_.end()) {
      for (auto& it : itStates->second) {
        states_.push_back(std::move(it));
      }
      statesByCommandList_.erase(itStates);
    }
  }
  bufferContentRestore_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                            c.ppCommandLists_.value, c.NumCommandLists_.value);
}

void AccelerationStructuresBuildService::commandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  bufferContentRestore_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AccelerationStructuresBuildService::commandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  bufferContentRestore_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AccelerationStructuresBuildService::fenceSignal(unsigned key,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  bufferContentRestore_.fenceSignal(key, fenceKey, fenceValue);
}

} // namespace DirectX
} // namespace gits
