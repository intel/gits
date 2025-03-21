// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "argumentDecoders.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

StateTrackingService::~StateTrackingService() {
  for (auto& it : statesByKey_) {
    delete it.second;
  }
}

void StateTrackingService::restoreState() {
  unsigned frameNumber = CGits::Instance().CurrentFrame();
  recorder_.record(new CTokenFrameNumber(CToken::ID_INIT_START, frameNumber));

  for (auto& it : statesByKey_) {
    restoreState(it.second);
  }
  residencyService_.restoreResidency();
  xessStateService_.restoreState();
  descriptorService_.restoreState();
  accelerationStructuresSerializeService_.restoreAccelerationStructures();
  accelerationStructuresBuildService_.restoreAccelerationStructures();
  reservedResourcesService_.restoreContent();
  resourceContentRestore_.restoreContent();
  resourceStateTrackingService_.restoreResourceStates();
  mapStateService_.restoreMapState();
  commandListService_.restoreCommandLists();
  restoreReferenceCount();

  swapChainService_.restoreBackBufferSequence();
  recorder_.record(new CTokenFrameNumber(CToken::ID_INIT_END, frameNumber));
  // one Present after ID_INIT_END to enable PIX first frame capture in gits interactive mode
  swapChainService_.recordSwapChainPresent();

  copyAuxiliaryFiles();
}

void StateTrackingService::copyAuxiliaryFiles() {
  std::filesystem::path streamDir = Config::Get().common.player.streamDir;
  std::filesystem::path subcapturePath = Config::Get().common.player.subcapturePath;
  if (std::filesystem::exists(streamDir / "raytracingArraysOfPointers.dat")) {
    std::filesystem::copy(streamDir / "raytracingArraysOfPointers.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "executeIndirectRaytracing.txt")) {
    std::filesystem::copy(streamDir / "executeIndirectRaytracing.txt", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "resourcePlacementData.dat")) {
    std::filesystem::copy(streamDir / "resourcePlacementData.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
}

void StateTrackingService::keepState(unsigned objectKey) {
  auto it = statesByKey_.find(objectKey);
  GITS_ASSERT(it != statesByKey_.end())
  it->second->keepDestroyed = true;
  if (it->second->id == ObjectState::D3D12_PLACEDRESOURCE) {
    keepState(static_cast<D3D12PlacedResourceState*>(it->second)->heapKey);
  } else if (it->second->id == ObjectState::D3D12_PLACEDRESOURCE1) {
    keepState(static_cast<D3D12PlacedResource1State*>(it->second)->heapKey);
  } else if (it->second->id == ObjectState::D3D12_PLACEDRESOURCE2) {
    keepState(static_cast<D3D12PlacedResource2State*>(it->second)->heapKey);
  }
}

void StateTrackingService::restoreState(ObjectState* state) {

  if (state->restored) {
    return;
  }

  switch (state->id) {
  case ObjectState::DXGI_FACTORY:
    restoreDXGIFactory(static_cast<DXGIFactoryState*>(state));
    break;
  case ObjectState::DXGI_ADAPTER:
    restoreDXGIAdapter(static_cast<DXGIAdapterState*>(state));
    break;
  case ObjectState::DXGI_ADAPTERBYLUID:
    restoreDXGIAdapterByLuid(static_cast<DXGIAdapterByLuidState*>(state));
    break;
  case ObjectState::DXGI_GETPARENT:
    restoreDXGIGetParent(static_cast<DXGIGetParentState*>(state));
    break;
  case ObjectState::DXGI_SWAPCHAIN:
    restoreSwapChain(static_cast<DXGISwapChainState*>(state));
    break;
  case ObjectState::DXGI_SWAPCHAINFORHWND:
    restoreSwapChainForHwnd(static_cast<DXGISwapChainForHwndState*>(state));
    break;
  case ObjectState::DXGI_SWAPCHAINBUFFER:
    restoreSwapChainBuffer(static_cast<DXGISwapChainBufferState*>(state));
    break;
  case ObjectState::D3D12_DEVICE:
    restoreD3D12Device(static_cast<D3D12DeviceState*>(state));
    break;
  case ObjectState::D3D12_COMMANDQUEUE:
    restoreD3D12CommandQueue(static_cast<D3D12CommandQueueState*>(state));
    break;
  case ObjectState::D3D12_COMMANDQUEUE1:
    restoreD3D12CommandQueue1(static_cast<D3D12CommandQueue1State*>(state));
    break;
  case ObjectState::D3D12_DESCRIPTORHEAP:
    restoreD3D12DescriptorHeap(static_cast<D3D12DescriptorHeapState*>(state));
    break;
  case ObjectState::D3D12_HEAP:
    restoreD3D12Heap(static_cast<D3D12HeapState*>(state));
    break;
  case ObjectState::D3D12_HEAP1:
    restoreD3D12Heap1(static_cast<D3D12Heap1State*>(state));
    break;
  case ObjectState::D3D12_HEAPFROMADDRESS:
    restoreD3D12HeapFromAddress(static_cast<D3D12HeapFromAddressState*>(state));
    break;
  case ObjectState::D3D12_QUERYHEAP:
    restoreD3D12QueryHeap(static_cast<D3D12QueryHeapState*>(state));
    break;
  case ObjectState::D3D12_COMMANDALLOCATOR:
    restoreD3D12CommandAllocator(static_cast<D3D12CommandAllocatorState*>(state));
    break;
  case ObjectState::D3D12_ROOTSIGNATURE:
    restoreD3D12RootSignature(static_cast<D3D12RootSignatureState*>(state));
    break;
  case ObjectState::D3D12_PIPELINELIBRARY:
    restoreD3D12PipelineLibrary(static_cast<D3D12PipelineLibraryState*>(state));
    break;
  case ObjectState::D3D12_LOADPIPELINESTATE:
    restoreD3D12LoadPipelineState(static_cast<D3D12LoadPipelineState*>(state));
    break;
  case ObjectState::D3D12_LOADGRAPHICSPIPELINESTATE:
    restoreD3D12LoadGraphicsPipelineState(static_cast<D3D12LoadGraphicsPipelineState*>(state));
    break;
  case ObjectState::D3D12_LOADCOMPUTEPIPELINESTATE:
    restoreD3D12LoadComputePipelineState(static_cast<D3D12LoadComputePipelineState*>(state));
    break;
  case ObjectState::D3D12_COMMANDSIGNATURE:
    restoreD3D12CommandSignature(static_cast<D3D12CommandSignatureState*>(state));
    break;
  case ObjectState::D3D12_GRAPHICSPIPELINESTATE:
    restoreD3D12GraphicsPipelineState(static_cast<D3D12GraphicsPipelineState*>(state));
    break;
  case ObjectState::D3D12_COMPUTEPIPELINESTATE:
    restoreD3D12ComputePipelineState(static_cast<D3D12ComputePipelineState*>(state));
    break;
  case ObjectState::D3D12_PIPELINESTATESTREAMSTATE:
    restoreD3D12PipelineStateStreamState(static_cast<D3D12PipelineStateStreamState*>(state));
    break;
  case ObjectState::D3D12_STATEOBJECTSTATE:
    restoreD3D12StateObjectState(static_cast<D3D12StateObjectState*>(state));
    break;
  case ObjectState::D3D12_ADDTOSTATEOBJECTSTATE:
    restoreD3D12AddToStateObjectState(static_cast<D3D12AddToStateObjectState*>(state));
    break;
  case ObjectState::D3D12_STATEOBJECTPROPERTIESSTATE:
    restoreD3D12StateObjectPropertiesState(static_cast<D3D12StateObjectPropertiesState*>(state));
    break;
  case ObjectState::D3D12_COMMANDLIST:
    restoreD3D12CommandList(static_cast<D3D12CommandListState*>(state));
    break;
  case ObjectState::D3D12_COMMANDLIST1:
    restoreD3D12CommandList1(static_cast<D3D12CommandList1State*>(state));
    break;
  case ObjectState::D3D12_COMMITTEDRESOURCE:
    restoreD3D12CommittedResource(static_cast<D3D12CommittedResourceState*>(state));
    break;
  case ObjectState::D3D12_COMMITTEDRESOURCE1:
    restoreD3D12CommittedResource1(static_cast<D3D12CommittedResource1State*>(state));
    break;
  case ObjectState::D3D12_COMMITTEDRESOURCE2:
    restoreD3D12CommittedResource2(static_cast<D3D12CommittedResource2State*>(state));
    break;
  case ObjectState::D3D12_COMMITTEDRESOURCE3:
    restoreD3D12CommittedResource3(static_cast<D3D12CommittedResource3State*>(state));
    break;
  case ObjectState::D3D12_PLACEDRESOURCE:
    restoreD3D12PlacedResource(static_cast<D3D12PlacedResourceState*>(state));
    break;
  case ObjectState::D3D12_PLACEDRESOURCE1:
    restoreD3D12PlacedResource1(static_cast<D3D12PlacedResource1State*>(state));
    break;
  case ObjectState::D3D12_RESERVEDRESOURCE:
    restoreD3D12ReservedResource(static_cast<D3D12ReservedResourceState*>(state));
    break;
  case ObjectState::D3D12_RESERVEDRESOURCE1:
    restoreD3D12ReservedResource1(static_cast<D3D12ReservedResource1State*>(state));
    break;
  case ObjectState::D3D12_FENCE:
    restoreD3D12Fence(static_cast<D3D12FenceState*>(state));
    break;
  case ObjectState::D3D12_INTC_DEVICEEXTENSIONCONTEXT:
    restoreD3D12INTCDeviceExtensionContext(
        static_cast<D3D12INTCDeviceExtensionContextState*>(state));
    break;
  case ObjectState::D3D12_INTC_DEVICEEXTENSIONCONTEXT1:
    restoreD3D12INTCDeviceExtensionContext1(
        static_cast<D3D12INTCDeviceExtensionContext1State*>(state));
    break;
  case ObjectState::D3D12_INTC_COMMITTEDRESOURCE:
    restoreD3D12INTCCommittedResource(static_cast<D3D12INTCCommittedResourceState*>(state));
    break;
  case ObjectState::D3D12_INTC_PLACEDRESOURCE:
    restoreD3D12INTCPlacedResource(static_cast<D3D12INTCPlacedResourceState*>(state));
    break;
  case ObjectState::D3D12_INTC_COMPUTEPIPELINESTATE:
    restoreD3D12INTCComputePipelineState(static_cast<D3D12INTCComputePipelineState*>(state));
    break;
  case ObjectState::DSTORAGE_FACTORY:
    restoreDStorageFactoryState(static_cast<DStorageFactoryState*>(state));
    break;
  case ObjectState::DSTORAGE_FILE:
    restoreDStorageFileState(static_cast<DStorageFileState*>(state));
    break;
  case ObjectState::DSTORAGE_CUSTOMDECOMPRESSIONQUEUE:
    restoreDStorageCustomDecompressionQueueState(
        static_cast<DStorageCustomDecompressionQueueState*>(state));
    break;
  case ObjectState::DSTORAGE_QUEUE:
    restoreDStorageQueueState(static_cast<DStorageQueueState*>(state));
    break;
  case ObjectState::DSTORAGE_STATUSARRAY:
    restoreDStorageStatusArrayState(static_cast<DStorageStatusArrayState*>(state));
    break;
  }

  if (!state->name.empty()) {
    ID3D12ObjectSetNameCommand c;
    c.key = getUniqueCommandKey();
    c.object_.key = state->key;
    c.Name_.value = state->name.data();
    recorder_.record(new ID3D12ObjectSetNameWriter(c));
  }

  state->restored = true;
}

void StateTrackingService::storeState(ObjectState* state) {
  auto it = statesByKey_.find(state->key);
  if (it == statesByKey_.end()) {
    statesByKey_[state->key] = state;
  }
  if (!state->refCount) {
    state->refCount = 1;
  }
}

void StateTrackingService::storeINTCFeature(INTC_D3D12_FEATURE feature) {
  intcFeature_ = feature;
}

void StateTrackingService::releaseObject(unsigned key, ULONG result) {
  auto itState = statesByKey_.find(key);
  if (itState == statesByKey_.end()) {
    return;
  }

  itState->second->refCount = result;

  if (result != 0) {
    return;
  }

  itState->second->destroyed = true;

  if (!itState->second->keepDestroyed) {
    if (itState->second->id == ObjectState::D3D12_PIPELINELIBRARY) {
      return;
    }

    unsigned parentKey = itState->second->parentKey;
    if (parentKey) {
      auto itParentState = statesByKey_.find(parentKey);
      if (itParentState != statesByKey_.end()) {
        itParentState->second->object->AddRef();
        ULONG refCount = itParentState->second->object->Release();
        if (refCount == 1) {
          delete itParentState->second;
          statesByKey_.erase(itParentState);
        }
      }
    }

    unsigned childKey = itState->second->childKey;
    if (childKey) {
      auto itChildState = statesByKey_.find(childKey);
      if (itChildState != statesByKey_.end()) {
        delete itChildState->second;
        statesByKey_.erase(itChildState);
      }
    }

    delete itState->second;
    statesByKey_.erase(itState);
  }
}

void StateTrackingService::setReferenceCount(unsigned objectKey, ULONG referenceCount) {
  auto itState = statesByKey_.find(objectKey);
  if (itState == statesByKey_.end()) {
    return;
  }
  itState->second->refCount = referenceCount;
}

ObjectState* StateTrackingService::getState(unsigned key) {
  auto it = statesByKey_.find(key);
  if (it == statesByKey_.end()) {
    return nullptr;
  }
  return it->second;
}

void StateTrackingService::restoreReferenceCount() {
  for (auto& it : statesByKey_) {
    ObjectState* state = it.second;
    if (!state->object) {
      continue;
    }
    if (it.second->destroyed) {
      IUnknownReleaseCommand c;
      c.key = getUniqueCommandKey();
      c.object_.key = it.second->key;
      recorder_.record(new IUnknownReleaseWriter(c));
      continue;
    }

    int refCount = 0;
    if (state->refCount <= 0 || state->id == ObjectState::DXGI_ADAPTER ||
        state->id == ObjectState::DXGI_FACTORY || state->id == ObjectState::DXGI_SWAPCHAINBUFFER ||
        state->id == ObjectState::D3D12_PIPELINELIBRARY ||
        state->id == ObjectState::D3D12_LOADGRAPHICSPIPELINESTATE ||
        state->id == ObjectState::D3D12_LOADCOMPUTEPIPELINESTATE ||
        state->id == ObjectState::D3D12_LOADPIPELINESTATE ||
        state->id == ObjectState::D3D12_ADDTOSTATEOBJECTSTATE ||
        state->id == ObjectState::D3D12_ROOTSIGNATURE) {
      refCount = state->refCount;
    } else {
      state->object->AddRef();
      refCount = state->object->Release();
    }

    for (int i = 1; i < refCount; ++i) {
      IUnknownAddRefCommand c;
      c.key = getUniqueCommandKey();
      c.object_.key = state->key;
      recorder_.record(new IUnknownAddRefWriter(c));
    }
  }
}

D3D12_RESOURCE_STATES StateTrackingService::getResourceInitialState(
    ResourceState& state, D3D12_RESOURCE_DIMENSION dimension) {
  if (state.isMappable || state.isBarrierRestricted) {
    return resourceStateTrackingService_.getResourceState(state.key);
  }
  if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return D3D12_RESOURCE_STATE_COMMON;
  }
  return D3D12_RESOURCE_STATE_COPY_DEST;
}

D3D12_BARRIER_LAYOUT StateTrackingService::getResourceInitialLayout(
    ResourceState& state, D3D12_RESOURCE_DIMENSION dimension) {
  if (state.isMappable) {
    return resourceStateTrackingService_.getResourceLayout(state.key);
  }
  if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return D3D12_BARRIER_LAYOUT_COMMON;
  }
  return D3D12_BARRIER_LAYOUT_COPY_DEST;
}

void StateTrackingService::restoreResidencyPriority(unsigned deviceKey,
                                                    unsigned objectKey,
                                                    D3D12_RESIDENCY_PRIORITY residencyPriority) {
  if (!residencyPriority) {
    return;
  }
  ID3D12Device1SetResidencyPriorityCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = deviceKey;
  c.NumObjects_.value = 1;
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  c.ppObjects_.value = &fakePtr;
  c.ppObjects_.size = 1;
  c.ppObjects_.keys.push_back(objectKey);
  c.pPriorities_.size = 1;
  c.pPriorities_.value = &residencyPriority;
  recorder_.record(new ID3D12Device1SetResidencyPriorityWriter(c));
}

void StateTrackingService::restoreDXGIFactory(DXGIFactoryState* state) {
  CreateDXGIFactory2Command c;
  c.key = getUniqueCommandKey();
  c.Flags_.value = state->flags;
  c.riid_.value = state->iid;
  c.ppFactory_.key = state->key;
  recorder_.record(new CreateDXGIFactory2Writer(c));
}

void StateTrackingService::restoreDXGIAdapter(DXGIAdapterState* state) {
  IDXGIFactory6EnumAdapterByGpuPreferenceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.Adapter_.value = state->adapter;
  c.GpuPreference_.value = state->gpuPreference;
  c.riid_.value = state->iid;
  c.ppvAdapter_.key = state->key;
  recorder_.record(new IDXGIFactory6EnumAdapterByGpuPreferenceWriter(c));
}

void StateTrackingService::restoreDXGIAdapterByLuid(DXGIAdapterByLuidState* state) {
  IDXGIFactory4EnumAdapterByLuidCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.AdapterLuid_.value = state->adapterLuid;
  c.riid_.value = state->iid;
  c.ppvAdapter_.key = state->key;
  recorder_.record(new IDXGIFactory4EnumAdapterByLuidWriter(c));
}

void StateTrackingService::restoreDXGIGetParent(DXGIGetParentState* state) {
  IDXGIObjectGetParentCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->objectKey;
  c.riid_.value = state->iid;
  c.ppParent_.key = state->key;
  recorder_.record(new IDXGIObjectGetParentWriter(c));
}

void StateTrackingService::restoreSwapChain(DXGISwapChainState* state) {
  CreateWindowMetaCommand createWindowCommand;
  createWindowCommand.key = getUniqueCommandKey();
  createWindowCommand.hWnd_.value = state->desc.OutputWindow;
  createWindowCommand.width_.value = state->desc.BufferDesc.Width;
  createWindowCommand.height_.value = state->desc.BufferDesc.Height;
  recorder_.record(new CreateWindowMetaWriter(createWindowCommand));

  IDXGIFactoryCreateSwapChainCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->factoryKey;
  c.pDevice_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.ppSwapChain_.key = state->key;
  recorder_.record(new IDXGIFactoryCreateSwapChainWriter(c));

  swapChainService_.setSwapChain(state->key, state->swapChain, state->desc.BufferCount);
}

void StateTrackingService::restoreSwapChainForHwnd(DXGISwapChainForHwndState* state) {
  CreateWindowMetaCommand createWindowCommand;
  createWindowCommand.key = getUniqueCommandKey();
  createWindowCommand.hWnd_.value = state->hWnd;
  createWindowCommand.width_.value = state->desc.Width;
  createWindowCommand.height_.value = state->desc.Height;
  recorder_.record(new CreateWindowMetaWriter(createWindowCommand));

  IDXGIFactory2CreateSwapChainForHwndCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->factoryKey;
  c.pDevice_.key = state->deviceKey;
  c.hWnd_.value = state->hWnd;
  c.pDesc_.value = &state->desc;
  c.pFullscreenDesc_.value = state->isFullscreenDesc ? &state->fullscreenDesc : nullptr;
  c.pRestrictToOutput_.key = state->restrictToOutputKey;
  c.ppSwapChain_.key = state->key;
  recorder_.record(new IDXGIFactory2CreateSwapChainForHwndWriter(c));

  swapChainService_.setSwapChain(state->key, state->swapChain, state->desc.BufferCount);
}

void StateTrackingService::restoreSwapChainBuffer(DXGISwapChainBufferState* state) {
  IDXGISwapChainGetBufferCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->swapChainKey;
  c.Buffer_.value = state->buffer;
  c.riid_.value = state->iid;
  c.ppSurface_.key = state->key;
  recorder_.record(new IDXGISwapChainGetBufferWriter(c));
}

void StateTrackingService::restoreD3D12Device(D3D12DeviceState* state) {
  D3D12CreateDeviceCommand c;
  c.key = getUniqueCommandKey();
  c.pAdapter_.key = state->adapterKey;
  c.MinimumFeatureLevel_.value = state->minimumFeatureLevel;
  c.riid_.value = state->iid;
  c.ppDevice_.key = state->key;
  recorder_.record(new D3D12CreateDeviceWriter(c));

  deviceKey_ = state->key;
}

void StateTrackingService::restoreD3D12CommandQueue(D3D12CommandQueueState* state) {
  ID3D12DeviceCreateCommandQueueCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.riid_.value = state->iid;
  c.ppCommandQueue_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateCommandQueueWriter(c));
}

void StateTrackingService::restoreD3D12CommandQueue1(D3D12CommandQueue1State* state) {
  ID3D12Device9CreateCommandQueue1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.CreatorID_.value = state->creatorId;
  c.riid_.value = state->iid;
  c.ppCommandQueue_.key = state->key;
  recorder_.record(new ID3D12Device9CreateCommandQueue1Writer(c));
}

void StateTrackingService::restoreD3D12DescriptorHeap(D3D12DescriptorHeapState* state) {
  ID3D12DeviceCreateDescriptorHeapCommand createCommand;
  createCommand.key = getUniqueCommandKey();
  createCommand.object_.key = state->deviceKey;
  createCommand.pDescriptorHeapDesc_.value = &state->desc;
  createCommand.riid_.value = state->iid;
  createCommand.ppvHeap_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(createCommand));

  if (state->gpuDescriptorHandle.ptr) {
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand getCommand;
    getCommand.key = getUniqueCommandKey();
    getCommand.object_.key = state->key;
    getCommand.result_.value = state->gpuDescriptorHandle;
    recorder_.record(new ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartWriter(getCommand));
  }
}

void StateTrackingService::restoreD3D12Heap(D3D12HeapState* state) {
  ID3D12DeviceCreateHeapCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.riid_.value = state->iid;
  c.ppvHeap_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateHeapWriter(c));

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
}

void StateTrackingService::restoreD3D12Heap1(D3D12Heap1State* state) {
  ID3D12Device4CreateHeap1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.pProtectedSession_.key = state->key;
  c.riid_.value = state->iid;
  c.ppvHeap_.key = state->key;
  recorder_.record(new ID3D12Device4CreateHeap1Writer(c));

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
}

void StateTrackingService::restoreD3D12HeapFromAddress(D3D12HeapFromAddressState* state) {
  CreateHeapAllocationMetaCommand allocCommand;
  allocCommand.key = getUniqueCommandKey();
  allocCommand.heap_.key = state->key;
  allocCommand.address_.value = state->address;
  allocCommand.data_.size = state->buffer.size();
  allocCommand.data_.value = state->buffer.data();
  recorder_.record(new CreateHeapAllocationMetaWriter(allocCommand));

  ID3D12Device3OpenExistingHeapFromAddressCommand openHeapCommand;
  openHeapCommand.key = getUniqueCommandKey();
  openHeapCommand.object_.key = state->deviceKey;
  openHeapCommand.pAddress_.value = state->address;
  openHeapCommand.riid_.value = state->iid;
  openHeapCommand.ppvHeap_.key = state->key;
  recorder_.record(new ID3D12Device3OpenExistingHeapFromAddressWriter(openHeapCommand));
}

void StateTrackingService::restoreD3D12QueryHeap(D3D12QueryHeapState* state) {
  ID3D12DeviceCreateQueryHeapCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.riid_.value = state->iid;
  c.ppvHeap_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateQueryHeapWriter(c));
}

void StateTrackingService::restoreD3D12CommandAllocator(D3D12CommandAllocatorState* state) {
  ID3D12DeviceCreateCommandAllocatorCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.type_.value = state->type;
  c.riid_.value = state->iid;
  c.ppCommandAllocator_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateCommandAllocatorWriter(c));
}

void StateTrackingService::restoreD3D12RootSignature(D3D12RootSignatureState* state) {
  ID3D12DeviceCreateRootSignatureCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.nodeMask_.value = state->nodeMask;
  c.pBlobWithRootSignature_.value = state->blob.get();
  c.pBlobWithRootSignature_.size = state->blobSize;
  c.blobLengthInBytes_.value = state->blobSize;
  c.riid_.value = state->iid;
  c.ppvRootSignature_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateRootSignatureWriter(c));
}

void StateTrackingService::restoreD3D12PipelineLibrary(D3D12PipelineLibraryState* state) {
  ID3D12Device1CreatePipelineLibraryCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pLibraryBlob_.value = state->blob.get();
  c.pLibraryBlob_.size = state->blobSize;
  c.BlobLength_.value = state->blobSize;
  c.riid_.value = state->iid;
  c.ppPipelineLibrary_.key = state->key;
  recorder_.record(new ID3D12Device1CreatePipelineLibraryWriter(c));
}

void StateTrackingService::restoreD3D12LoadPipelineState(D3D12LoadPipelineState* state) {
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12PipelineLibrary1LoadPipelineCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->pipelineLibraryKey;
  c.pName_.value = const_cast<wchar_t*>(state->name.c_str());
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12PipelineLibrary1LoadPipelineWriter(c));
}

void StateTrackingService::restoreD3D12LoadGraphicsPipelineState(
    D3D12LoadGraphicsPipelineState* state) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12PipelineLibraryLoadGraphicsPipelineCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->pipelineLibraryKey;
  c.pName_.value = const_cast<wchar_t*>(state->name.c_str());
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12PipelineLibraryLoadGraphicsPipelineWriter(c));
}

void StateTrackingService::restoreD3D12LoadComputePipelineState(
    D3D12LoadComputePipelineState* state) {
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12PipelineLibraryLoadComputePipelineCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->pipelineLibraryKey;
  c.pName_.value = const_cast<wchar_t*>(state->name.c_str());
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12PipelineLibraryLoadComputePipelineWriter(c));
}

void StateTrackingService::restoreD3D12CommandSignature(D3D12CommandSignatureState* state) {
  PointerArgument<D3D12_COMMAND_SIGNATURE_DESC> descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12DeviceCreateCommandSignatureCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_ = descArg;
  c.pRootSignature_.key = state->rootSignatureKey;
  c.riid_.value = state->iid;
  c.ppvCommandSignature_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateCommandSignatureWriter(c));
}

void StateTrackingService::restoreD3D12GraphicsPipelineState(D3D12GraphicsPipelineState* state) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12DeviceCreateGraphicsPipelineStateCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateGraphicsPipelineStateWriter(c));
}

void StateTrackingService::restoreD3D12ComputePipelineState(D3D12ComputePipelineState* state) {
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12DeviceCreateComputePipelineStateCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateComputePipelineStateWriter(c));
}

void StateTrackingService::restoreD3D12PipelineStateStreamState(
    D3D12PipelineStateStreamState* state) {
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12Device2CreatePipelineStateCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_ = descArg;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new ID3D12Device2CreatePipelineStateWriter(c));
}

void StateTrackingService::restoreD3D12StateObjectState(D3D12StateObjectState* state) {
  D3D12_STATE_OBJECT_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12Device5CreateStateObjectCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_ = std::move(descArg);
  c.riid_.value = state->iid;
  c.ppStateObject_.key = state->key;
  recorder_.record(new ID3D12Device5CreateStateObjectWriter(c));
}

void StateTrackingService::restoreD3D12AddToStateObjectState(D3D12AddToStateObjectState* state) {
  D3D12_STATE_OBJECT_DESC_Argument descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  ID3D12Device7AddToStateObjectCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pAddition_ = std::move(descArg);
  c.pStateObjectToGrowFrom_.key = state->stateObjectToGrowFromKey;
  c.riid_.value = state->iid;
  c.ppNewStateObject_.key = state->key;
  recorder_.record(new ID3D12Device7AddToStateObjectWriter(c));
}

void StateTrackingService::restoreD3D12StateObjectPropertiesState(
    D3D12StateObjectPropertiesState* state) {
  IUnknownQueryInterfaceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->stateObjectKey;
  c.ppvObject_.key = state->key;
  c.riid_.value = IID_ID3D12StateObjectProperties;
  recorder_.record(new IUnknownQueryInterfaceWriter(c));

  for (auto& shaderIdentifier : state->shaderIdentifiers) {
    ID3D12StateObjectPropertiesGetShaderIdentifierCommand c;
    c.key = getUniqueCommandKey();
    c.object_.key = state->key;
    c.pExportName_.value = const_cast<wchar_t*>(shaderIdentifier.first.c_str());
    c.result_.value = shaderIdentifier.second.data();
    recorder_.record(new ID3D12StateObjectPropertiesGetShaderIdentifierWriter(c));
  }
}

void StateTrackingService::restoreD3D12CommandList(D3D12CommandListState* state) {

  ObjectState* allocatorState = getState(state->allocatorKey);
  restoreState(allocatorState);
  if (state->initialStateKey) {
    ObjectState* initialState = getState(state->initialStateKey);
    restoreState(initialState);
  }

  ID3D12CommandAllocatorResetCommand reset;
  reset.key = getUniqueCommandKey();
  reset.object_.key = state->allocatorKey;
  recorder_.record(new ID3D12CommandAllocatorResetWriter(reset));

  ID3D12DeviceCreateCommandListCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.nodeMask_.value = state->nodeMask;
  c.type_.value = state->type;
  c.pCommandAllocator_.key = state->allocatorKey;
  c.pInitialState_.key = state->initialStateKey;
  c.riid_.value = state->iid;
  c.ppCommandList_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateCommandListWriter(c));

  ID3D12GraphicsCommandListCloseCommand close;
  close.key = getUniqueCommandKey();
  close.object_.key = state->key;
  recorder_.record(new ID3D12GraphicsCommandListCloseWriter(close));
}

void StateTrackingService::restoreD3D12CommandList1(D3D12CommandList1State* state) {

  ID3D12Device4CreateCommandList1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.nodeMask_.value = state->nodeMask;
  c.type_.value = state->type;
  c.flags_.value = state->flags;
  c.riid_.value = state->iid;
  c.ppCommandList_.key = state->key;
  recorder_.record(new ID3D12Device4CreateCommandList1Writer(c));
}

void StateTrackingService::restoreD3D12CommittedResource(D3D12CommittedResourceState* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12DeviceCreateCommittedResourceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeapProperties_.value = &state->heapProperties;
  c.HeapFlags_.value = state->heapFlags;
  c.pDesc_.value = &state->desc;
  c.InitialResourceState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riidResource_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateCommittedResourceWriter(c));

  if (state->refCount > 0) {
    resourceContentRestore_.addCommittedResourceState(state);
  }

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12CommittedResource1(D3D12CommittedResource1State* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12Device4CreateCommittedResource1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeapProperties_.value = &state->heapProperties;
  c.HeapFlags_.value = state->heapFlags;
  c.pDesc_.value = &state->desc;
  c.InitialResourceState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riidResource_.value = state->iid;
  c.pProtectedSession_.key = state->protectedSessionKey;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12Device4CreateCommittedResource1Writer(c));

  if (state->refCount > 0) {
    resourceContentRestore_.addCommittedResourceState(state);
  }

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12CommittedResource2(D3D12CommittedResource2State* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12Device8CreateCommittedResource2Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeapProperties_.value = &state->heapProperties;
  c.HeapFlags_.value = state->heapFlags;
  c.pDesc_.value = &state->desc;
  c.InitialResourceState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riidResource_.value = state->iid;
  c.pProtectedSession_.key = state->protectedSessionKey;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12Device8CreateCommittedResource2Writer(c));

  if (state->refCount > 0) {
    resourceContentRestore_.addCommittedResourceState(state);
  }

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12CommittedResource3(D3D12CommittedResource3State* state) {
  D3D12_BARRIER_LAYOUT initialLayout = getResourceInitialLayout(*state, state->desc.Dimension);

  ID3D12Device10CreateCommittedResource3Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeapProperties_.value = &state->heapProperties;
  c.HeapFlags_.value = state->heapFlags;
  c.pDesc_.value = &state->desc;
  c.InitialLayout_.value = initialLayout;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.pProtectedSession_.key = state->protectedSessionKey;
  c.NumCastableFormats_.value = state->castableFormats.size();
  c.pCastableFormats_.value = state->castableFormats.data();
  c.pCastableFormats_.size = state->castableFormats.size();
  c.riidResource_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12Device10CreateCommittedResource3Writer(c));

  if (state->refCount > 0) {
    resourceContentRestore_.addCommittedResourceState(state);
  }

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12PlacedResource(D3D12PlacedResourceState* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    if (state->refCount > 0) {
      resourceContentRestore_.addPlacedResourceState(state);
    }
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12DeviceCreatePlacedResourceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeap_.key = state->heapKey;
  c.HeapOffset_.value = state->heapOffset;
  c.pDesc_.value = &state->desc;
  c.InitialState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riid_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12DeviceCreatePlacedResourceWriter(c));

  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12PlacedResource1(D3D12PlacedResource1State* state) {
  auto it = statesByKey_.find(state->heapKey);
  GITS_ASSERT(it != statesByKey_.end());

  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    if (state->refCount > 0) {
      resourceContentRestore_.addPlacedResourceState(state);
    }
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12Device8CreatePlacedResource1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pHeap_.key = state->heapKey;
  c.HeapOffset_.value = state->heapOffset;
  c.pDesc_.value = &state->desc;
  c.InitialState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riid_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12Device8CreatePlacedResource1Writer(c));

  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12ReservedResource(D3D12ReservedResourceState* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12DeviceCreateReservedResourceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.InitialState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riid_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateReservedResourceWriter(c));

  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreD3D12ReservedResource1(D3D12ReservedResource1State* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*state, state->desc.Dimension);
  }

  ID3D12Device4CreateReservedResource1Command c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.InitialState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.pProtectedSession_.key = state->protectedSessionKey;
  c.riid_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new ID3D12Device4CreateReservedResource1Writer(c));

  restoreGpuVirtualAddress(state);
}

void StateTrackingService::restoreGpuVirtualAddress(ResourceState* state) {
  if (state->gpuVirtualAddress) {
    ID3D12ResourceGetGPUVirtualAddressCommand c;
    c.key = getUniqueCommandKey();
    c.object_.key = state->key;
    c.result_.value = state->gpuVirtualAddress;
    recorder_.record(new ID3D12ResourceGetGPUVirtualAddressWriter(c));
  }
}

void StateTrackingService::restoreD3D12Fence(D3D12FenceState* state) {

  std::array<UINT64, 3>& fenceValues = fenceTrackingService_.getFenceValue(state->key);
  bool incremental = fenceTrackingService_.incremental(fenceValues);

  ID3D12DeviceCreateFenceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.InitialValue_.value = incremental ? fenceValues[0] : fenceValues[2];
  c.Flags_.value = state->flags;
  c.riid_.value = state->iid;
  c.ppFence_.key = state->key;
  recorder_.record(new ID3D12DeviceCreateFenceWriter(c));

  if (!incremental) {
    ID3D12FenceSignalCommand c1;
    c1.key = getUniqueCommandKey();
    c1.object_.key = state->key;
    c1.Value_.value = fenceValues[1];
    recorder_.record(new ID3D12FenceSignalWriter(c1));

    ID3D12FenceSignalCommand c0;
    c0.key = getUniqueCommandKey();
    c0.object_.key = state->key;
    c0.Value_.value = fenceValues[0];
    recorder_.record(new ID3D12FenceSignalWriter(c0));
  }
}

void StateTrackingService::restoreD3D12INTCDeviceExtensionContext(
    D3D12INTCDeviceExtensionContextState* state) {
  INTC_D3D12_CreateDeviceExtensionContextCommand c;
  c.key = getUniqueCommandKey();
  c.pDevice_.key = state->deviceKey;
  c.ppExtensionContext_.key = state->key;

  PointerArgument<INTCExtensionInfo> extensionInfoArg;
  unsigned offset{};
  decode(state->extensionInfoEncoded.get(), offset, extensionInfoArg);
  c.pExtensionInfo_ = extensionInfoArg;

  if (state->isExtensionAppInfo) {
    PointerArgument<INTCExtensionAppInfo> extensionAppInfoArg;
    unsigned offset{};
    decode(state->extensionAppInfoEncoded.get(), offset, extensionAppInfoArg);
    c.pExtensionAppInfo_ = extensionAppInfoArg;
  } else {
    c.pExtensionAppInfo_.value = nullptr;
  }

  recorder_.record(new INTC_D3D12_CreateDeviceExtensionContextWriter(c));
  if (intcFeature_.EmulatedTyped64bitAtomics) {
    INTC_D3D12_SetFeatureSupportCommand c;
    c.key = getUniqueCommandKey();
    c.pExtensionContext_.key = state->key;
    c.pFeature_.value = &intcFeature_;
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(c));
  }
}

void StateTrackingService::restoreD3D12INTCDeviceExtensionContext1(
    D3D12INTCDeviceExtensionContext1State* state) {
  INTC_D3D12_CreateDeviceExtensionContext1Command c;
  c.key = getUniqueCommandKey();
  c.pDevice_.key = state->deviceKey;
  c.ppExtensionContext_.key = state->key;

  PointerArgument<INTCExtensionInfo> extensionInfoArg;
  unsigned offset{};
  decode(state->extensionInfoEncoded.get(), offset, extensionInfoArg);
  c.pExtensionInfo_ = extensionInfoArg;

  if (state->isExtensionAppInfo) {
    PointerArgument<INTCExtensionAppInfo1> extensionAppInfoArg;
    unsigned offset{};
    decode(state->extensionAppInfoEncoded.get(), offset, extensionAppInfoArg);
    c.pExtensionAppInfo_ = extensionAppInfoArg;
  } else {
    c.pExtensionAppInfo_.value = nullptr;
  }

  recorder_.record(new INTC_D3D12_CreateDeviceExtensionContext1Writer(c));

  if (intcFeature_.EmulatedTyped64bitAtomics) {
    INTC_D3D12_SetFeatureSupportCommand c;
    c.key = getUniqueCommandKey();
    c.pExtensionContext_.key = state->key;
    c.pFeature_.value = &intcFeature_;
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(c));
  }
}

void StateTrackingService::restoreD3D12INTCCommittedResource(
    D3D12INTCCommittedResourceState* state) {
  D3D12_RESOURCE_STATES initialState =
      state->descIntc.pD3D12Desc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
          ? D3D12_RESOURCE_STATE_COMMON
          : D3D12_RESOURCE_STATE_COPY_DEST;
  if (state->isMappable) {
    initialState = resourceStateTrackingService_.getResourceState(state->key);
  }

  INTC_D3D12_CreateCommittedResourceCommand c;
  c.key = getUniqueCommandKey();
  c.pExtensionContext_.key = state->extensionContextKey;
  c.pHeapProperties_.value = &state->heapProperties;
  c.HeapFlags_.value = state->heapFlags;
  c.pDesc_.value = &state->descIntc;
  c.pDesc_.value->pD3D12Desc = &state->desc;
  c.InitialResourceState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riidResource_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new INTC_D3D12_CreateCommittedResourceWriter(c));

  resourceContentRestore_.addCommittedResourceState(state);

  restoreResidencyPriority(state->deviceKey, state->key, state->residencyPriority);
}

void StateTrackingService::restoreD3D12INTCPlacedResource(D3D12INTCPlacedResourceState* state) {
  D3D12_RESOURCE_STATES initialState = state->initialResourceState;
  initialState = state->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
                     ? D3D12_RESOURCE_STATE_COMMON
                     : D3D12_RESOURCE_STATE_COPY_DEST;
  if (state->isMappable) {
    initialState = resourceStateTrackingService_.getResourceState(state->key);
  }

  INTC_D3D12_CreatePlacedResourceCommand c;
  c.key = getUniqueCommandKey();
  c.pExtensionContext_.key = state->extensionContextKey;
  c.pHeap_.key = state->heapKey;
  c.HeapOffset_.value = state->heapOffset;
  c.pDesc_.value = &state->descIntc;
  c.pDesc_.value->pD3D12Desc = &state->desc;
  c.InitialState_.value = initialState;
  c.pOptimizedClearValue_.value = state->isClearValue ? &state->clearValue : nullptr;
  c.riid_.value = state->iid;
  c.ppvResource_.key = state->key;
  recorder_.record(new INTC_D3D12_CreatePlacedResourceWriter(c));

  resourceContentRestore_.addPlacedResourceState(state);
}

void StateTrackingService::restoreD3D12INTCComputePipelineState(
    D3D12INTCComputePipelineState* state) {
  PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> descArg;
  unsigned offset{};
  decode(state->descEncoded.get(), offset, descArg);

  INTC_D3D12_CreateComputePipelineStateCommand c;
  c.key = getUniqueCommandKey();
  c.pExtensionContext_.key = state->extensionContextKey;
  c.pDesc_ = descArg;
  c.pDesc_.cs = c.pDesc_.value->CS.pShaderBytecode;
  c.pDesc_.compileOptions = c.pDesc_.value->CompileOptions;
  c.pDesc_.internalOptions = c.pDesc_.value->InternalOptions;
  c.riid_.value = state->iid;
  c.ppPipelineState_.key = state->key;
  recorder_.record(new INTC_D3D12_CreateComputePipelineStateWriter(c));
}

void StateTrackingService::restoreDStorageFactoryState(DStorageFactoryState* state) {
  DStorageGetFactoryCommand c;
  c.key = getUniqueCommandKey();
  c.riid_.value = state->iid;
  c.ppv_.key = state->key;
  recorder_.record(new DStorageGetFactoryWriter(c));
}

void StateTrackingService::restoreDStorageFileState(DStorageFileState* state) {
  IDStorageFactoryOpenFileCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.path_.value = const_cast<wchar_t*>(state->path.c_str());
  c.riid_.value = state->iid;
  c.ppv_.key = state->key;
  recorder_.record(new IDStorageFactoryOpenFileWriter(c));
}

void StateTrackingService::restoreDStorageQueueState(DStorageQueueState* state) {
  IDStorageFactoryCreateQueueCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.desc_.value = &state->desc;
  c.desc_.name = state->name.c_str();
  c.desc_.value->Name = c.desc_.name;
  c.desc_.deviceKey = state->deviceKey;
  c.riid_.value = state->iid;
  c.ppv_.key = state->key;
  recorder_.record(new IDStorageFactoryCreateQueueWriter(c));
}

void StateTrackingService::restoreDStorageCustomDecompressionQueueState(
    DStorageCustomDecompressionQueueState* state) {
  IUnknownQueryInterfaceCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.riid_.value = state->iid;
  c.ppvObject_.key = state->key;
  recorder_.record(new IUnknownQueryInterfaceWriter(c));
}

void StateTrackingService::restoreDStorageStatusArrayState(DStorageStatusArrayState* state) {
  IDStorageFactoryCreateStatusArrayCommand c;
  c.key = getUniqueCommandKey();
  c.object_.key = state->parentKey;
  c.capacity_.value = state->capacity;
  c.name_.value = const_cast<char*>(state->name.c_str());
  c.ppv_.key = state->key;
  recorder_.record(new IDStorageFactoryCreateStatusArrayWriter(c));
}

void StateTrackingService::SwapChainService::setSwapChain(unsigned key,
                                                          IDXGISwapChain* swapChain,
                                                          unsigned backBuffersCount) {
  swapChainKey_ = key;
  swapChain_ = swapChain;
  backBuffersCount_ = backBuffersCount;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
  HRESULT hr = swapChain_->QueryInterface(IID_PPV_ARGS(&swapChain3));
  GITS_ASSERT(hr == S_OK);
  backBufferShift_ = swapChain3->GetCurrentBackBufferIndex();
}

void StateTrackingService::SwapChainService::restoreBackBufferSequence() {

  // taking into account one Present that will be always recorded later
  int backBufferShift = backBufferShift_ - 1;
  if (backBufferShift < 0) {
    backBufferShift += backBuffersCount_;
  }
  for (int i = 0; i < backBufferShift; ++i) {
    recordSwapChainPresent();
  }
}

void StateTrackingService::SwapChainService::recordSwapChainPresent() {
  IDXGISwapChainPresentCommand presentCommand;
  presentCommand.key = stateService_.getUniqueCommandKey();
  presentCommand.object_.key = swapChainKey_;
  presentCommand.SyncInterval_.value = 0;
  presentCommand.Flags_.value = 0;
  stateService_.recorder_.record(new IDXGISwapChainPresentWriter(presentCommand));
}

} // namespace DirectX
} // namespace gits
