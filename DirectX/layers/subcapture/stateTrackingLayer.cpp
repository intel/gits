// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingLayer.h"
#include "argumentEncoders.h"
#include "descriptorService.h"
#include "commandWritersAuto.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

StateTrackingLayer::StateTrackingLayer(SubcaptureRecorder& recorder)
    : Layer("StateTracking"),
      stateService_(recorder,
                    fenceTrackingService_,
                    mapStateService_,
                    resourceStateTrackingService_,
                    reservedResourcesService_,
                    descriptorService_,
                    commandListService_,
                    xessStateService_,
                    accelerationStructuresSerializeService_,
                    accelerationStructuresBuildService_),
      recorder_(recorder),
      mapStateService_(stateService_),
      resourceStateTrackingService_(stateService_),
      reservedResourcesService_(stateService_),
      descriptorService_(stateService_),
      commandListService_(stateService_),
      xessStateService_(stateService_, recorder),
      accelerationStructuresSerializeService_(stateService_, recorder_),
      accelerationStructuresBuildService_(stateService_, recorder_, reservedResourcesService_) {

  const std::string& frames = Config::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      startFrame_ = std::stoi(frames.substr(0, pos));
    } else {
      startFrame_ = std::stoi(frames);
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Config::Get().directx.features.subcapture.frames + "'");
  }
}

bool StateTrackingLayer::isResourceHeapMappable(unsigned heapKey) {
  ObjectState* state = stateService_.getState(heapKey);
  if (state->id == ObjectState::D3D12_HEAP) {
    D3D12HeapState* heapState = static_cast<D3D12HeapState*>(state);
    return isResourceHeapMappable(heapState->desc.Properties);
  } else if (state->id == ObjectState::D3D12_HEAP1) {
    D3D12Heap1State* heapState = static_cast<D3D12Heap1State*>(state);
    return isResourceHeapMappable(heapState->desc.Properties);
  } else if (state->id == ObjectState::D3D12_HEAPFROMADDRESS) {
    return true;
  } else {
    GITS_ASSERT(0 && "Unexpected state type");
  }
  return false;
}

void StateTrackingLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.Flags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  unsigned currentFrame = CGits::Instance().CurrentFrame();
  if (currentFrame == startFrame_ - 1) {
    Log(INFOV) << "Start subcapture frame " << currentFrame + 1 << " call " << c.key;
    gpuExecutionFlusher_.flushCommandQueues();
    stateService_.restoreState();
  }
}

void StateTrackingLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  unsigned currentFrame = CGits::Instance().CurrentFrame();
  if (currentFrame == startFrame_ - 1) {
    Log(INFOV) << "Start subcapture frame " << currentFrame + 1 << " call " << c.key;
    gpuExecutionFlusher_.flushCommandQueues();
    stateService_.restoreState();
  }
}

void StateTrackingLayer::pre(IUnknownReleaseCommand& c) {
  stateService_.releaseObject(c.object_.key, c.result_.value);
  if (c.result_.value == 0) {
    mapStateService_.destroyResource(c.object_.key);
    resourceStateTrackingService_.destroyResource(c.object_.key);
    heapAllocationStateService_.destroyHeap(c.object_.key);
    reservedResourcesService_.destroyObject(c.object_.key);
    descriptorService_.removeState(c.object_.key);
    commandListService_.removeCommandList(c.object_.key);
    xessStateService_.destroyDevice(c.object_.key);
    accelerationStructuresSerializeService_.destroyResource(c.object_.key);

    auto it = resourceHeaps_.find(c.object_.key);
    if (it != resourceHeaps_.end()) {
      for (unsigned resourceKey : it->second) {
        stateService_.releaseObject(resourceKey, 0);
      }
    }

    gpuExecutionFlusher_.destroyCommandQueue(c.object_.key);
  }
}

void StateTrackingLayer::post(IUnknownAddRefCommand& c) {
  stateService_.addRefObject(c.object_.key, c.result_.value);
}

void StateTrackingLayer::post(IUnknownQueryInterfaceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  IID riid = c.riid_.value;
  if (riid == IID_ID3D12StateObjectProperties) {
    D3D12StateObjectPropertiesState* state = new D3D12StateObjectPropertiesState();
    state->stateObjectKey = c.object_.key;
    state->key = c.ppvObject_.key;

    ObjectState* parentState = stateService_.getState(state->stateObjectKey);
    parentState->childKey = state->key;

    stateService_.storeState(state);
  } else if ((riid == __uuidof(IDStorageCustomDecompressionQueue)) ||
             (riid == __uuidof(IDStorageCustomDecompressionQueue1))) {
    DStorageCustomDecompressionQueueState* state = new DStorageCustomDecompressionQueueState();
    state->parentKey = c.object_.key;
    state->key = c.ppvObject_.key;
    state->iid = riid;

    ObjectState* parentState = stateService_.getState(state->parentKey);
    parentState->childKey = state->key;

    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(CreateDXGIFactoryCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIFactoryState* state = new DXGIFactoryState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->flags = 0;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateDXGIFactory1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIFactoryState* state = new DXGIFactoryState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->flags = 0;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateDXGIFactory2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIFactoryState* state = new DXGIFactoryState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->flags = c.Flags_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactoryEnumAdaptersCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIAdapterState* state = new DXGIAdapterState();
  state->parentKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = *c.ppAdapter_.value;
  state->adapter = c.Adapter_.value;
  state->gpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;
  state->iid = IID_IDXGIAdapter;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory1EnumAdapters1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIAdapterState* state = new DXGIAdapterState();
  state->parentKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = *c.ppAdapter_.value;
  state->adapter = c.Adapter_.value;
  state->gpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;
  state->iid = IID_IDXGIAdapter;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIAdapterState* state = new DXGIAdapterState();
  state->parentKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->adapter = c.Adapter_.value;
  state->gpuPreference = c.GpuPreference_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIAdapterByLuidState* state = new DXGIAdapterByLuidState();
  state->parentKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->adapterLuid = c.AdapterLuid_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIObjectGetParentCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGIGetParentState* state = new DXGIGetParentState();
  state->objectKey = c.object_.key;
  state->key = c.ppParent_.key;
  state->object = static_cast<IUnknown*>(*c.ppParent_.value);
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(D3D12CreateDeviceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12DeviceState* state = new D3D12DeviceState();
  state->adapterKey = 0; //c.pAdapter_.key;
  state->key = c.ppDevice_.key;
  state->object = static_cast<IUnknown*>(*c.ppDevice_.value);
  state->minimumFeatureLevel = c.MinimumFeatureLevel_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandQueueState* state = new D3D12CommandQueueState();
  state->deviceKey = c.object_.key;
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->desc = *c.pDesc_.value;
  state->iid = c.riid_.value;
  state->commandQueue = static_cast<ID3D12CommandQueue*>(*c.ppCommandQueue_.value);
  stateService_.storeState(state);
  accelerationStructuresSerializeService_.setDevice(c.object_.value, c.object_.key);
  accelerationStructuresBuildService_.setDeviceKey(c.object_.key);

  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(ID3D12Device9CreateCommandQueue1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandQueue1State* state = new D3D12CommandQueue1State();
  state->deviceKey = c.object_.key;
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->desc = *c.pDesc_.value;
  state->creatorId = c.CreatorID_.value;
  state->iid = c.riid_.value;
  state->commandQueue = static_cast<ID3D12CommandQueue*>(*c.ppCommandQueue_.value);
  stateService_.storeState(state);

  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGISwapChainState* state = new DXGISwapChainState();
  state->deviceKey = c.pDevice_.key;
  state->key = c.ppSwapChain_.key;
  state->object = *c.ppSwapChain_.value;
  state->factoryKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->swapChain = *c.ppSwapChain_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGISwapChainForHwndState* state = new DXGISwapChainForHwndState();
  state->deviceKey = c.pDevice_.key;
  state->key = c.ppSwapChain_.key;
  state->object = *c.ppSwapChain_.value;
  state->factoryKey = c.object_.key;
  state->hWnd = c.hWnd_.value;
  state->desc = *c.pDesc_.value;
  if (state->isFullscreenDesc = c.pFullscreenDesc_.value ? true : false) {
    state->fullscreenDesc = *c.pFullscreenDesc_.value;
  }
  state->restrictToOutputKey = c.pRestrictToOutput_.key;
  state->swapChain = *c.ppSwapChain_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainResizeBuffersCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* objectState = stateService_.getState(c.object_.key);
  if (objectState->id == ObjectState::DXGI_SWAPCHAINFORHWND) {
    DXGISwapChainForHwndState* state = static_cast<DXGISwapChainForHwndState*>(objectState);
    state->desc.Width = c.Width_.value;
    state->desc.Height = c.Height_.value;
    state->desc.Format = c.NewFormat_.value;
    state->desc.Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      state->desc.BufferCount = c.BufferCount_.value;
    }
  } else if (objectState->id == ObjectState::DXGI_SWAPCHAIN) {
    DXGISwapChainState* state = static_cast<DXGISwapChainState*>(objectState);
    state->desc.BufferDesc.Width = c.Width_.value;
    state->desc.BufferDesc.Height = c.Height_.value;
    state->desc.BufferDesc.Format = c.NewFormat_.value;
    state->desc.Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      state->desc.BufferCount = c.BufferCount_.value;
    }
  }
}

void StateTrackingLayer::post(ID3D12ObjectSetNameCommand& c) {
  ObjectState* state = stateService_.getState(c.object_.key);
  if (state == nullptr) {
    Log(ERR) << "StateTrackingLayer: SetName failed. Cannot find object O" << c.object_.key << ".";
    return;
  }
  state->name = c.Name_.value;
}

void StateTrackingLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12DescriptorHeapState* state = new D3D12DescriptorHeapState();
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->desc = *c.pDescriptorHeapDesc_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12HeapState* state = new D3D12HeapState();
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->desc = *c.pDesc_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device4CreateHeap1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12Heap1State* state = new D3D12Heap1State();
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->desc = *c.pDesc_.value;
  state->protectedSessionKey = c.pProtectedSession_.key;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12QueryHeapState* state = new D3D12QueryHeapState();
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->desc = *c.pDesc_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateHeapAllocationMetaCommand& c) {
  D3D12HeapFromAddressState* state = new D3D12HeapFromAddressState();
  state->key = c.heap_.key;
  state->address = c.address_.value;
  state->buffer.resize(c.data_.size);
  memcpy(state->buffer.data(), c.data_.value, c.data_.size);
  heapAllocationStateService_.setHeapState(state);
}

void StateTrackingLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  D3D12HeapFromAddressState* state = heapAllocationStateService_.getHeapState(c.ppvHeap_.key);
  state->deviceKey = c.object_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DXGISwapChainBufferState* state = new DXGISwapChainBufferState();
  state->swapChainKey = c.object_.key;
  state->key = c.ppSurface_.key;
  state->object = static_cast<IUnknown*>(*c.ppSurface_.value);
  state->buffer = c.Buffer_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(0, static_cast<ID3D12Resource*>(*c.ppSurface_.value),
                                            state->key, D3D12_RESOURCE_STATE_COMMON, false);
}

void StateTrackingLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  D3D12RenderTargetViewState* state = new D3D12RenderTargetViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  D3D12DepthStencilViewState* state = new D3D12DepthStencilViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandAllocatorState* state = new D3D12CommandAllocatorState();
  state->deviceKey = c.object_.key;
  state->key = c.ppCommandAllocator_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandAllocator_.value);
  state->type = c.type_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12RootSignatureState* state = new D3D12RootSignatureState();
  state->deviceKey = c.object_.key;
  state->key = c.ppvRootSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvRootSignature_.value);
  state->nodeMask = c.nodeMask_.value;
  state->blob.reset(new char[c.blobLengthInBytes_.value]);
  memcpy(state->blob.get(), c.pBlobWithRootSignature_.value, c.blobLengthInBytes_.value);
  state->blobSize = c.blobLengthInBytes_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12PipelineLibraryState* state = new D3D12PipelineLibraryState();
  state->deviceKey = c.object_.key;
  state->key = c.ppPipelineLibrary_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineLibrary_.value);
  state->blob.reset(new char[c.BlobLength_.value]);
  memcpy(state->blob.get(), c.pLibraryBlob_.value, c.BlobLength_.value);
  state->blobSize = c.BlobLength_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  ObjectState* state = stateService_.getState(c.ppPipelineState_.key);
  if (state) {
    ++state->refCount;
  } else {
    D3D12LoadPipelineState* state = new D3D12LoadPipelineState();
    state->pipelineLibraryKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->name = c.pName_.value;
    state->iid = c.riid_.value;

    state->descEncoded.reset(new char[getSize(c.pDesc_)]);
    unsigned offset{};
    encode(state->descEncoded.get(), offset, c.pDesc_);

    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  ObjectState* state = stateService_.getState(c.ppPipelineState_.key);
  if (state) {
    ++state->refCount;
  } else {
    D3D12LoadGraphicsPipelineState* state = new D3D12LoadGraphicsPipelineState();
    state->pipelineLibraryKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->name = c.pName_.value;
    state->iid = c.riid_.value;

    state->descEncoded.reset(new char[getSize(c.pDesc_)]);
    unsigned offset{};
    encode(state->descEncoded.get(), offset, c.pDesc_);

    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  ObjectState* state = stateService_.getState(c.ppPipelineState_.key);
  if (state) {
    ++state->refCount;
  } else {
    D3D12LoadComputePipelineState* state = new D3D12LoadComputePipelineState();
    state->pipelineLibraryKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->name = c.pName_.value;
    state->iid = c.riid_.value;

    state->descEncoded.reset(new char[getSize(c.pDesc_)]);
    unsigned offset{};
    encode(state->descEncoded.get(), offset, c.pDesc_);

    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandSignatureState* state = new D3D12CommandSignatureState();
  state->deviceKey = c.object_.key;
  state->key = c.ppvCommandSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvCommandSignature_.value);
  state->rootSignatureKey = c.pRootSignature_.key;
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12GraphicsPipelineState* state = new D3D12GraphicsPipelineState();
  state->deviceKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12ComputePipelineState* state = new D3D12ComputePipelineState();
  state->deviceKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12PipelineStateStreamState* state = new D3D12PipelineStateStreamState();
  state->deviceKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12StateObjectState* state = new D3D12StateObjectState();
  state->deviceKey = c.object_.key;
  state->key = c.ppStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppStateObject_.value);
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  stateService_.storeState(state);
  accelerationStructuresSerializeService_.setDevice(c.object_.value, c.object_.key);
  accelerationStructuresBuildService_.setDeviceKey(c.object_.key);
}

void StateTrackingLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12AddToStateObjectState* state = new D3D12AddToStateObjectState();
  state->deviceKey = c.object_.key;
  state->key = c.ppNewStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppNewStateObject_.value);
  state->stateObjectToGrowFromKey = c.pStateObjectToGrowFrom_.key;
  state->iid = c.riid_.value;

  state->descEncoded.reset(new char[getSize(c.pAddition_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pAddition_);

  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandListState* state = new D3D12CommandListState();
  state->deviceKey = c.object_.key;
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->nodeMask = c.nodeMask_.value;
  state->type = c.type_.value;
  state->allocatorKey = c.pCommandAllocator_.key;
  state->initialStateKey = c.pInitialState_.key;
  state->iid = c.riid_.value;
  stateService_.storeState(state);

  commandListService_.addCommandList(state);
}

void StateTrackingLayer::post(ID3D12Device4CreateCommandList1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommandList1State* state = new D3D12CommandList1State();
  state->deviceKey = c.object_.key;
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->nodeMask = c.nodeMask_.value;
  state->type = c.type_.value;
  state->flags = c.flags_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);

  commandListService_.addCommandList(state);
}

void StateTrackingLayer::pre(ID3D12DeviceCreateCommittedResourceCommand& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommittedResourceState* state = new D3D12CommittedResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapProperties = *c.pHeapProperties_.value;
  state->heapFlags = c.HeapFlags_.value;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialResourceState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riidResource_.value;
  state->isMappable = isResourceHeapMappable(state->heapProperties);
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateCommittedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommittedResource1State* state = new D3D12CommittedResource1State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapProperties = *c.pHeapProperties_.value;
  state->heapFlags = c.HeapFlags_.value;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialResourceState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->protectedSessionKey = c.pProtectedSession_.key;
  state->iid = c.riidResource_.value;
  state->isMappable = isResourceHeapMappable(state->heapProperties);
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);
  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device8CreateCommittedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommittedResource2State* state = new D3D12CommittedResource2State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapProperties = *c.pHeapProperties_.value;
  state->heapFlags = c.HeapFlags_.value;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialResourceState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->protectedSessionKey = c.pProtectedSession_.key;
  state->iid = c.riidResource_.value;
  state->isMappable = isResourceHeapMappable(state->heapProperties);
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateCommittedResource3Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12CommittedResource3State* state = new D3D12CommittedResource3State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapProperties = *c.pHeapProperties_.value;
  state->heapFlags = c.HeapFlags_.value;
  state->desc = *c.pDesc_.value;
  state->initialLayout = c.InitialLayout_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->protectedSessionKey = c.pProtectedSession_.key;
  for (unsigned i = 0; i < c.NumCastableFormats_.value; ++i) {
    state->castableFormats.push_back(c.pCastableFormats_.value[i]);
  }
  state->iid = c.riidResource_.value;
  state->isMappable = isResourceHeapMappable(state->heapProperties);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
}

void StateTrackingLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12PlacedResourceState* state = new D3D12PlacedResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapKey = c.pHeap_.key;
  state->heapOffset = c.HeapOffset_.value;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riid_.value;
  state->isMappable = isResourceHeapMappable(state->heapKey);
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);
  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12PlacedResource1State* state = new D3D12PlacedResource1State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapKey = c.pHeap_.key;
  state->heapOffset = c.HeapOffset_.value;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riid_.value;
  state->isMappable = isResourceHeapMappable(state->heapKey);
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);
  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12PlacedResource2State* state = new D3D12PlacedResource2State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->heapKey = c.pHeap_.key;
  state->heapOffset = c.HeapOffset_.value;
  state->desc = *c.pDesc_.value;
  state->initialLayout = c.InitialLayout_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  for (unsigned i = 0; i < c.NumCastableFormats_.value; ++i) {
    state->castableFormats.push_back(c.pCastableFormats_.value[i]);
  }
  state->iid = c.riid_.value;
  state->isMappable = isResourceHeapMappable(state->heapKey);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  stateService_.storeState(state);
  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
}

void StateTrackingLayer::pre(ID3D12DeviceCreateReservedResourceCommand& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12ReservedResourceState* state = new D3D12ReservedResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riid_.value;
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateReservedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12ReservedResource1State* state = new D3D12ReservedResource1State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->initialResourceState = c.InitialState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->protectedSessionKey = c.pProtectedSession_.key;
  state->iid = c.riid_.value;
  state->isGenericRead = state->initialResourceState == D3D12_RESOURCE_STATE_GENERIC_READ;
  stateService_.storeState(state);

  if (state->initialResourceState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialResourceState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateReservedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12ReservedResource2State* state = new D3D12ReservedResource2State();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->initialLayout = c.InitialLayout_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->protectedSessionKey = c.pProtectedSession_.key;
  for (unsigned i = 0; i < c.NumCastableFormats_.value; ++i) {
    state->castableFormats.push_back(c.pCastableFormats_.value[i]);
  }
  state->iid = c.riid_.value;
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
}

void StateTrackingLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  D3D12ShaderResourceViewState* state = new D3D12ShaderResourceViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (c.pDesc_.value) {
    state->isDesc = true;
    state->desc = *c.pDesc_.value;
    if (c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
      state->resourceKey = c.pDesc_.raytracingLocationKey;
      state->raytracingLocationOffset = c.pDesc_.raytracingLocationOffset;
    }
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12FenceState* state = new D3D12FenceState();
  state->deviceKey = c.object_.key;
  state->key = c.ppFence_.key;
  state->object = static_cast<IUnknown*>(*c.ppFence_.value);
  state->flags = c.Flags_.value;
  state->iid = c.riid_.value;
  stateService_.storeState(state);

  fenceTrackingService_.setFenceValue(c.ppFence_.key, c.InitialValue_.value);
}

void StateTrackingLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.pFence_.key, c.Value_.value);

  gpuExecutionFlusher_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void StateTrackingLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  gpuExecutionFlusher_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void StateTrackingLayer::post(ID3D12FenceSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.object_.key, c.Value_.value);

  gpuExecutionFlusher_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void StateTrackingLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void StateTrackingLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  D3D12SamplerState* state = new D3D12SamplerState();
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  state->counterResourceKey = c.pCounterResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
  state->deviceKey = c.object_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->resourceKey = c.pDesc_.bufferLocationKey;
  state->bufferLocationKey = c.pDesc_.bufferLocationKey;
  state->bufferLocationOffset = c.pDesc_.bufferLocationOffset;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12ResourceMapCommand& c) {
  mapStateService_.mapResource(c.object_.key, c.Subresource_.value, c.ppData_.captureValue);
}

void StateTrackingLayer::post(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  reservedResourcesService_.addUpdateTileMappings(c);
}

void StateTrackingLayer::post(ID3D12CommandQueueCopyTileMappingsCommand& c) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "ID3D12CommandQueue::CopyTileMappings not handled in subcapture!";
    logged = true;
  }
}

void StateTrackingLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  descriptorService_.copyDescriptors(c);
}

void StateTrackingLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  descriptorService_.copyDescriptors(c);
}

void StateTrackingLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  D3D12INTCDeviceExtensionContextState* state = new D3D12INTCDeviceExtensionContextState();
  state->deviceKey = c.pDevice_.key;
  state->key = c.ppExtensionContext_.key;
  state->extensionInfo = *c.pExtensionInfo_.value;
  if (state->isExtensionAppInfo = c.pExtensionAppInfo_.value ? true : false) {
    state->extensionAppInfo = *c.pExtensionAppInfo_.value;
  }
  ObjectState* parentState = stateService_.getState(state->deviceKey);
  parentState->childKey = state->key;
  stateService_.storeState(state);
  deviceByINTCExtensionContext_[state->key] = state->deviceKey;
}

void StateTrackingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  D3D12INTCDeviceExtensionContext1State* state = new D3D12INTCDeviceExtensionContext1State();
  state->deviceKey = c.pDevice_.key;
  state->key = c.ppExtensionContext_.key;
  state->extensionInfo = *c.pExtensionInfo_.value;
  if (state->isExtensionAppInfo = c.pExtensionAppInfo_.value ? true : false) {
    state->extensionAppInfo = *c.pExtensionAppInfo_.value;
  }
  ObjectState* parentState = stateService_.getState(state->deviceKey);
  parentState->childKey = state->key;
  stateService_.storeState(state);

  deviceByINTCExtensionContext_[state->key] = state->deviceKey;
}

void StateTrackingLayer::post(INTC_DestroyDeviceExtensionContextCommand& c) {
  stateService_.releaseObject(c.ppExtensionContext_.key, c.result_.value);
  deviceByINTCExtensionContext_.erase(c.ppExtensionContext_.key);
}

void StateTrackingLayer::post(INTC_D3D12_SetFeatureSupportCommand& c) {
  stateService_.storeINTCFeature(*c.pFeature_.value);
}

void StateTrackingLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12INTCCommittedResourceState* state = new D3D12INTCCommittedResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->extensionContextKey = c.pExtensionContext_.key;
  state->heapProperties = *c.pHeapProperties_.value;
  state->heapFlags = c.HeapFlags_.value;
  state->descIntc = *c.pDesc_.value;
  state->desc = *c.pDesc_.value->pD3D12Desc;
  state->initialResourceState = c.InitialResourceState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riidResource_.value;
  state->isMappable = isResourceHeapMappable(state->heapProperties);
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(
      state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
      state->initialResourceState, !state->isMappable);
}

void StateTrackingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12INTCPlacedResourceState* state = new D3D12INTCPlacedResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->extensionContextKey = c.pExtensionContext_.key;
  state->heapKey = c.pHeap_.key;
  state->heapOffset = c.HeapOffset_.value;
  state->descIntc = *c.pDesc_.value;
  state->desc = *c.pDesc_.value->pD3D12Desc;
  state->initialResourceState = c.InitialState_.value;
  if (state->isClearValue = c.pOptimizedClearValue_.value ? true : false) {
    state->clearValue = *c.pOptimizedClearValue_.value;
  }
  state->iid = c.riid_.value;
  state->isMappable = isResourceHeapMappable(state->heapKey);
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(
      state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
      state->initialResourceState, !state->isMappable);
}

void StateTrackingLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.pDesc_.cs = c.pDesc_.value->CS.pShaderBytecode;
  c.pDesc_.compileOptions = c.pDesc_.value->CompileOptions;
  c.pDesc_.internalOptions = c.pDesc_.value->InternalOptions;
}

void StateTrackingLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12INTCComputePipelineState* state = new D3D12INTCComputePipelineState();
  state->key = c.ppPipelineState_.key;
  state->extensionContextKey = c.pExtensionContext_.key;

  state->descEncoded.reset(new char[getSize(c.pDesc_)]);
  unsigned offset{};
  encode(state->descEncoded.get(), offset, c.pDesc_);

  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device1SetResidencyPriorityCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  for (unsigned i = 0; i < c.NumObjects_.value; ++i) {
    ObjectState* state = stateService_.getState(c.ppObjects_.keys[i]);
    GITS_ASSERT(state);
    if (state->id != ObjectState::D3D12_COMMITTEDRESOURCE &&
        state->id != ObjectState::D3D12_COMMITTEDRESOURCE1 &&
        state->id != ObjectState::D3D12_COMMITTEDRESOURCE2 &&
        state->id != ObjectState::D3D12_COMMITTEDRESOURCE3 &&
        state->id != ObjectState::D3D12_INTC_COMMITTEDRESOURCE &&
        state->id != ObjectState::D3D12_HEAP && state->id != ObjectState::D3D12_HEAP1) {
      Log(WARN) << "SetResidencyPriority not handled for " << state->id;
    }
    state->residencyPriority = c.pPriorities_.value[i];
  }
}

void StateTrackingLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  resourceStateTrackingService_.executeCommandLists(c.ppCommandLists_.keys);
  accelerationStructuresSerializeService_.executeCommandLists(c);
  accelerationStructuresBuildService_.executeCommandLists(c);
}

void StateTrackingLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  ResourceState* state = static_cast<ResourceState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  state->gpuVirtualAddress = c.result_.value;
}

void StateTrackingLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  ResourceState* state = static_cast<ResourceState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  c.result_.value = state->gpuVirtualAddress;
}

void StateTrackingLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  D3D12StateObjectPropertiesState* state =
      static_cast<D3D12StateObjectPropertiesState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  auto it = state->shaderIdentifiers.find(c.pExportName_.value);
  if (it == state->shaderIdentifiers.end()) {
    auto& shaderIdentifier = state->shaderIdentifiers[c.pExportName_.value];
    for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
      shaderIdentifier[i] = static_cast<uint8_t*>(c.result_.value)[i];
    }
  }
}

void StateTrackingLayer::post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  D3D12StateObjectPropertiesState* state =
      static_cast<D3D12StateObjectPropertiesState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  auto& shaderIdentifier = state->shaderIdentifiers[c.pExportName_.value];
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    static_cast<uint8_t*>(c.result_.value)[i] = shaderIdentifier[i];
  }
}

void StateTrackingLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  D3D12DescriptorHeapState* state =
      static_cast<D3D12DescriptorHeapState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  state->gpuDescriptorHandle = c.result_.value;
}

void StateTrackingLayer::post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  D3D12DescriptorHeapState* state =
      static_cast<D3D12DescriptorHeapState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  c.result_.value = state->gpuDescriptorHandle;
}

void StateTrackingLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  accelerationStructuresBuildService_.buildAccelerationStructure(c);
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    lastCallCaptureGpuAddresses_.push_back(c.pDesc_.value->Inputs.InstanceDescs);
  }
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  accelerationStructuresSerializeService_.buildAccelerationStructure(c);
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    c.pDesc_.value->Inputs.InstanceDescs = lastCallCaptureGpuAddresses_[0];
    lastCallCaptureGpuAddresses_.clear();
  }

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  accelerationStructuresBuildService_.copyAccelerationStructure(c);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  accelerationStructuresSerializeService_.copyAccelerationStructure(c);

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  lastCallCaptureGpuAddresses_.push_back(c.pDesc_.value->RayGenerationShaderRecord.StartAddress);
  lastCallCaptureGpuAddresses_.push_back(c.pDesc_.value->MissShaderTable.StartAddress);
  lastCallCaptureGpuAddresses_.push_back(c.pDesc_.value->HitGroupTable.StartAddress);
  lastCallCaptureGpuAddresses_.push_back(c.pDesc_.value->CallableShaderTable.StartAddress);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  c.pDesc_.value->RayGenerationShaderRecord.StartAddress = lastCallCaptureGpuAddresses_[0];
  c.pDesc_.value->MissShaderTable.StartAddress = lastCallCaptureGpuAddresses_[1];
  c.pDesc_.value->HitGroupTable.StartAddress = lastCallCaptureGpuAddresses_[2];
  c.pDesc_.value->CallableShaderTable.StartAddress = lastCallCaptureGpuAddresses_[3];
  lastCallCaptureGpuAddresses_.clear();

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4DispatchRaysWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResetCommand& c) {

  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->clearCommands();

  if (state->id == ObjectState::D3D12_COMMANDLIST) {
    static_cast<D3D12CommandListState*>(state)->allocatorKey = c.pAllocator_.key;
    static_cast<D3D12CommandListState*>(state)->initialStateKey = c.pInitialState_.key;
  }

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResetWriter(c));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCloseCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCloseWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearStateCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListClearStateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDrawInstancedWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDrawIndexedInstancedWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDispatchCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDispatchWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyBufferRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyTextureRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyResourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyTilesWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResolveSubresourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetPrimitiveTopologyWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListRSSetViewportsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListRSSetViewportsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListRSSetScissorRectsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListOMSetBlendFactorWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListOMSetStencilRefWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetPipelineStateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  resourceStateTrackingService_.resourceBarrier(
      c.object_.key, c.pBarriers_.value, c.pBarriers_.resourceKeys, c.pBarriers_.resourceAfterKeys);

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResourceBarrierWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}
void StateTrackingLayer::post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListExecuteBundleWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetDescriptorHeapsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRootSignatureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRootSignatureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRootDescriptorTableWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRoot32BitConstantWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRoot32BitConstantsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootConstantBufferViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootShaderResourceViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetIndexBufferWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetVertexBuffersWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSOSetTargetsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  CommandListOMSetRenderTargets* command = new CommandListOMSetRenderTargets(c.key);
  command->renderTargetViews.resize(c.NumRenderTargetDescriptors_.value);
  for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
    if (c.pRenderTargetDescriptors_.interfaceKeys[i]) {
      DescriptorState* descriptorState = descriptorService_.getDescriptorState(
          c.pRenderTargetDescriptors_.interfaceKeys[i], c.pRenderTargetDescriptors_.indexes[i]);
      GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_RENDERTARGETVIEW);
      command->renderTargetViews[i].reset(new D3D12RenderTargetViewState(
          *static_cast<D3D12RenderTargetViewState*>(descriptorState)));
    }
  }
  if (!c.pDepthStencilDescriptor_.interfaceKeys.empty()) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.pDepthStencilDescriptor_.interfaceKeys[0], c.pDepthStencilDescriptor_.indexes[0]);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_DEPTHSTENCILVIEW);
    command->depthStencilView.reset(
        new D3D12DepthStencilViewState(*static_cast<D3D12DepthStencilViewState*>(descriptorState)));
  }
  command->commandListKey = c.object_.key;
  command->rtsSingleHandleToDescriptorRange = c.RTsSingleHandleToDescriptorRange_.value;

  command->commandWriter.reset(new ID3D12GraphicsCommandListOMSetRenderTargetsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListClearDepthStencilViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  CommandListClearRenderTargetView* command = new CommandListClearRenderTargetView(c.key);
  if (c.RenderTargetView_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_RENDERTARGETVIEW);
    command->renderTargetView.reset(
        new D3D12RenderTargetViewState(*static_cast<D3D12RenderTargetViewState*>(descriptorState)));
  }
  command->commandListKey = c.object_.key;
  for (unsigned i = 0; i < 4; ++i) {
    command->colorRGBA[i] = c.ColorRGBA_.value[i];
  }
  if (c.NumRects_.value) {
    command->rects.resize(c.NumRects_.value);
    for (unsigned i = 0; i < c.NumRects_.value; ++i) {
      command->rects[i] = c.pRects_.value[i];
    }
  }

  command->commandWriter.reset(new ID3D12GraphicsCommandListClearRenderTargetViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListClearUnorderedAccessViewUintWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListClearUnorderedAccessViewFloatWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDiscardResourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListBeginQueryWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListEndQueryWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResolveQueryDataWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetPredicationWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetMarkerCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetMarkerWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListBeginEventCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListBeginEventWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListEndEventCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListEndEventWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListExecuteIndirectWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1AtomicCopyBufferUINTWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Writer(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1OMSetDepthBoundsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1SetSamplePositionsCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1SetSamplePositionsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1ResolveSubresourceRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1SetViewInstanceMaskWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList2WriteBufferImmediateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList3SetProtectedResourceSessionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4BeginRenderPassWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4EndRenderPassCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4EndRenderPassWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "ID3D12GraphicsCommandList4ExecuteMetaCommand is not supported in subcapture.";
    logged = true;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4ExecuteMetaCommandWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4InitializeMetaCommandWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4SetPipelineState1Writer(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList5RSSetShadingRateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList5RSSetShadingRateImageWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList6DispatchMeshWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "ID3D12GraphicsCommandList7::Barrier is not supported in subcapture.";
    logged = true;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList7BarrierWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(xessD3D12CreateContextCommand& c) {
  if (c.result_.value != XESS_RESULT_SUCCESS) {
    return;
  }
  XessStateService::ContextState* state = new XessStateService::ContextState();
  state->key = c.phContext_.key;
  state->deviceKey = c.pDevice_.key;
  state->device = c.pDevice_.value;
  xessStateService_.storeContextState(state);
}

void StateTrackingLayer::post(xessD3D12InitCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->initParamsEncoded.reset(new char[getSize(c.pInitParams_)]);
  unsigned offset{};
  encode(state->initParamsEncoded.get(), offset, c.pInitParams_);
}

void StateTrackingLayer::pre(xessDestroyContextCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->device->AddRef();
}

void StateTrackingLayer::post(xessDestroyContextCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  ULONG ref = state->device->Release();
  if (ref == 0) {
    stateService_.releaseObject(state->deviceKey, 0);
  }
  xessStateService_.destroyContext(c.hContext_.key);
}

void StateTrackingLayer::post(xessSetJitterScaleCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  if (!state->jitterScale) {
    state->jitterScale = std::make_unique<float[]>(2);
  }
  state->jitterScale[0] = c.x_.value;
  state->jitterScale[1] = c.y_.value;
}

void StateTrackingLayer::post(xessSetVelocityScaleCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  if (!state->velocityScale) {
    state->velocityScale = std::make_unique<float[]>(2);
  }
  state->velocityScale[0] = c.x_.value;
  state->velocityScale[1] = c.y_.value;
}

void StateTrackingLayer::post(xessSetExposureMultiplierCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->exposureScale = c.scale_.value;
}

void StateTrackingLayer::post(xessForceLegacyScaleFactorsCommand& c) {
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->forceLegacyScaleFactors = c.force_.value;
}

void StateTrackingLayer::post(DStorageGetFactoryCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DStorageFactoryState* state = new DStorageFactoryState();
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->iid = c.riid_.value;
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDStorageFactoryOpenFileCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DStorageFileState* state = new DStorageFileState();
  state->key = c.ppv_.key;
  state->parentKey = c.object_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->iid = c.riid_.value;
  state->path = c.path_.value;
  stateService_.storeState(state);

  ObjectState* parentState = stateService_.getState(state->parentKey);
  parentState->childKey = state->key;
}

void StateTrackingLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DStorageQueueState* state = new DStorageQueueState();
  state->key = c.ppv_.key;
  state->parentKey = c.object_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->iid = c.riid_.value;
  state->desc = *c.desc_.value;
  state->deviceKey = c.desc_.deviceKey;
  if (c.desc_.name) {
    state->name = c.desc_.name;
  }
  stateService_.storeState(state);

  ObjectState* parentState = stateService_.getState(state->parentKey);
  parentState->childKey = state->key;
}

void StateTrackingLayer::post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  DStorageStatusArrayState* state = new DStorageStatusArrayState();
  state->key = c.ppv_.key;
  state->parentKey = c.object_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->iid = c.riid_.value;
  if (c.name_.value) {
    state->name = c.name_.value;
  }
  state->capacity = c.capacity_.value;
  stateService_.storeState(state);

  ObjectState* parentState = stateService_.getState(state->parentKey);
  parentState->childKey = state->key;
}

} // namespace DirectX
} // namespace gits
