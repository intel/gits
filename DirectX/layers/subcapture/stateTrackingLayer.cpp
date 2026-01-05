// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingLayer.h"
#include "descriptorService.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "configurationLib.h"
#include "keyUtils.h"
#include "nvapi.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

StateTrackingLayer::StateTrackingLayer(SubcaptureRecorder& recorder,
                                       SubcaptureRange& subcaptureRange)
    : Layer("StateTracking"),
      stateService_(recorder,
                    fenceTrackingService_,
                    mapStateService_,
                    resourceStateTrackingService_,
                    reservedResourcesService_,
                    descriptorService_,
                    commandListService_,
                    commandQueueService_,
                    xessStateService_,
                    accelerationStructuresSerializeService_,
                    accelerationStructuresBuildService_,
                    residencyService_,
                    analyzerResults_,
                    resourceUsageTrackingService_,
                    resourceForCBVRestoreService_),
      recorder_(recorder),
      subcaptureRange_(subcaptureRange),
      mapStateService_(stateService_),
      resourceStateTrackingService_(stateService_),
      reservedResourcesService_(stateService_),
      resourceForCBVRestoreService_(stateService_),
      descriptorService_(&stateService_, &resourceForCBVRestoreService_),
      commandListService_(stateService_),
      commandQueueService_(stateService_),
      xessStateService_(stateService_, recorder),
      accelerationStructuresSerializeService_(stateService_, recorder_),
      accelerationStructuresBuildService_(stateService_,
                                          recorder_,
                                          reservedResourcesService_,
                                          resourceStateTracker_,
                                          gpuAddressService_),
      residencyService_(stateService_) {}

void StateTrackingLayer::setAsChildInParent(unsigned parentKey, unsigned childKey) {
  ObjectState* parentState = stateService_.getState(parentKey);
  if (!parentState) {
    return;
  }
  parentState->childrenKeys.insert(childKey);
}

bool StateTrackingLayer::isResourceHeapMappable(unsigned heapKey,
                                                const D3D12_TEXTURE_LAYOUT& textureLayout) {
  ObjectState* state = stateService_.getState(heapKey);
  if (state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE_CREATEHEAP) {
    auto* command = static_cast<ID3D12DeviceCreateHeapCommand*>(state->creationCommand.get());
    return isResourceHeapMappable(command->pDesc_.value->Properties, textureLayout);
  } else if (state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE4_CREATEHEAP1) {
    auto* command = static_cast<ID3D12DeviceCreateHeapCommand*>(state->creationCommand.get());
    return isResourceHeapMappable(command->pDesc_.value->Properties, textureLayout);
  } else if (state->creationCommand->getId() == CommandId::ID_CREATE_HEAP_ALLOCATION) {
    return true;
  } else if (state->creationCommand->getId() == CommandId::INTC_D3D12_CREATEHEAP) {
    auto* command = static_cast<INTC_D3D12_CreateHeapCommand*>(state->creationCommand.get());
    return isResourceHeapMappable(command->pDesc_.value->pD3D12Desc->Properties, textureLayout);
  } else {
    GITS_ASSERT(0 && "Unexpected state type");
  }
  return false;
}

bool StateTrackingLayer::isResourceBarrierRestricted(D3D12_RESOURCE_FLAGS flags) {
  // ResourceBarrier on placed resource can corrupt underlying heap data if resource does not currently own that data
  // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createplacedresource
  constexpr D3D12_RESOURCE_FLAGS resourceTypesRestrictedFromBarrier =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  return (flags & resourceTypesRestrictedFromBarrier) != 0;
}

void StateTrackingLayer::post(IDXGISwapChainPresentCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.Flags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  if (!subcaptureRange_.commandListSubcapture() &&
      subcaptureRange_.isFrameRangeStart(isStateRestoreKey(c.key))) {
    gpuExecutionFlusher_.flushCommandQueues();
    stateService_.restoreState();
    stateRestored_ = true;
  }
}

void StateTrackingLayer::post(IDXGISwapChain1Present1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  if (!subcaptureRange_.commandListSubcapture() &&
      subcaptureRange_.isFrameRangeStart(isStateRestoreKey(c.key))) {
    gpuExecutionFlusher_.flushCommandQueues();
    stateService_.restoreState();
    stateRestored_ = true;
  }
}

void StateTrackingLayer::pre(IUnknownReleaseCommand& c) {
  if (stateRestored_) {
    return;
  }
  releaseSwapChainBuffers(c.object_.key, c.result_.value);
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
    residencyService_.destroyObject(c.object_.key);
    resourceUsageTrackingService_.destroyResource(c.object_.key);
    resourceStateTracker_.destroyResource(c.object_.key);
    gpuAddressService_.destroyInterface(c.object_.key);

    auto it = resourceHeaps_.find(c.object_.key);
    if (it != resourceHeaps_.end()) {
      for (unsigned resourceKey : it->second) {
        stateService_.releaseObject(resourceKey, 0);
        mapStateService_.destroyResource(resourceKey);
        resourceStateTrackingService_.destroyResource(resourceKey);
        descriptorService_.removeState(resourceKey);
        accelerationStructuresSerializeService_.destroyResource(resourceKey);
        residencyService_.destroyObject(resourceKey);
        resourceUsageTrackingService_.destroyResource(resourceKey);
      }
      resourceHeaps_.erase(it);
    }

    gpuExecutionFlusher_.destroyCommandQueue(c.object_.key);
  }
}

void StateTrackingLayer::releaseSwapChainBuffers(unsigned key, unsigned referenceCount) {
  if (referenceCount > 0) {
    return;
  }

  ObjectState* state = stateService_.getState(key);
  if (!state || state->creationCommand->getId() != CommandId::ID_IDXGISWAPCHAIN_GETBUFFER) {
    return;
  }

  // SwapChain buffers share the same reference count
  // Remove all buffers from the same SwapChain if one of them has 0 references
  IDXGISwapChainGetBufferCommand* command =
      static_cast<IDXGISwapChainGetBufferCommand*>(state->creationCommand.get());
  unsigned swapChainKey = command->object_.key;
  for (unsigned bufferKey : swapchainBuffers_[swapChainKey]) {
    if (bufferKey == key) {
      continue;
    }
    resourceStateTrackingService_.destroyResource(bufferKey);
    descriptorService_.removeState(bufferKey);
    stateService_.releaseObject(bufferKey, 0);
  }
  swapchainBuffers_.erase(swapChainKey);
}

void StateTrackingLayer::post(IUnknownAddRefCommand& c) {
  if (stateRestored_) {
    return;
  }
  stateService_.setReferenceCount(c.object_.key, c.result_.value);
}

void StateTrackingLayer::post(IUnknownQueryInterfaceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }

  if (c.object_.value) {
    c.object_.value->AddRef();
    ULONG refCount = c.object_.value->Release();
    stateService_.setReferenceCount(c.object_.key, refCount);
  }

  IID riid = c.riid_.value;
  if (riid == IID_ID3D12StateObjectProperties) {
    D3D12StateObjectPropertiesState* state = new D3D12StateObjectPropertiesState();
    state->parentKey = c.object_.key;
    state->linkedLifetimeKey = c.object_.key;
    state->key = c.ppvObject_.key;

    setAsChildInParent(state->parentKey, state->key);
    state->creationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    stateService_.storeState(state);
  } else if ((riid == __uuidof(IDStorageCustomDecompressionQueue)) ||
             (riid == __uuidof(IDStorageCustomDecompressionQueue1))) {
    ObjectState* state = new ObjectState();
    state->parentKey = c.object_.key;
    state->linkedLifetimeKey = c.object_.key;
    state->key = c.ppvObject_.key;

    setAsChildInParent(state->parentKey, state->key);
    state->creationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(CreateDXGIFactoryCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->creationCommand.reset(new CreateDXGIFactoryCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateDXGIFactory1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->creationCommand.reset(new CreateDXGIFactory1Command(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateDXGIFactory2Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppFactory_.value);
  state->creationCommand.reset(new CreateDXGIFactory2Command(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactoryEnumAdaptersCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppAdapter_.value);
  state->creationCommand.reset(new IDXGIFactoryEnumAdaptersCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory1EnumAdapters1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory1EnumAdapters1Command(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory6EnumAdapterByGpuPreferenceCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory4EnumAdapterByLuidCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIAdapterEnumOutputsCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppOutput_.key;
  state->object = static_cast<IUnknown*>(*c.ppOutput_.value);
  state->creationCommand.reset(new IDXGIAdapterEnumOutputsCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIObjectGetParentCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppParent_.key;
  state->object = static_cast<IUnknown*>(*c.ppParent_.value);
  state->creationCommand.reset(new IDXGIObjectGetParentCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(D3D12CreateDeviceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppDevice_.key;
  state->object = static_cast<IUnknown*>(*c.ppDevice_.value);
  state->creationCommand.reset(new D3D12CreateDeviceCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(D3D12EnableExperimentalFeaturesCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  stateService_.storeD3D12EnableExperimentalFeatures(c);
}

void StateTrackingLayer::post(D3D12GetInterfaceCommand& c) {
  if (stateRestored_) {
    return;
  }

  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppvDebug_.key;
  state->object = static_cast<IUnknown*>(*c.ppvDebug_.value);
  state->creationCommand.reset(new D3D12GetInterfaceCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandQueueCommand(c));
  stateService_.storeState(state);

  accelerationStructuresSerializeService_.setDevice(c.object_.value, c.object_.key);
  accelerationStructuresBuildService_.setDeviceKey(c.object_.key);
  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(ID3D12Device9CreateCommandQueue1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->creationCommand.reset(new ID3D12Device9CreateCommandQueue1Command(c));
  stateService_.storeState(state);

  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppSwapChain_.key;
  state->object = static_cast<IUnknown*>(*c.ppSwapChain_.value);
  state->creationCommand.reset(new IDXGIFactoryCreateSwapChainCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppSwapChain_.key;
  state->object = static_cast<IUnknown*>(*c.ppSwapChain_.value);
  state->creationCommand.reset(new IDXGIFactory2CreateSwapChainForHwndCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainResizeBuffersCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* objectState = stateService_.getState(c.object_.key);
  if (objectState->creationCommand->getId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    IDXGIFactoryCreateSwapChainCommand* command =
        static_cast<IDXGIFactoryCreateSwapChainCommand*>(objectState->creationCommand.get());
    command->pDesc_.value->BufferDesc.Width = c.Width_.value;
    command->pDesc_.value->BufferDesc.Height = c.Height_.value;
    command->pDesc_.value->Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      command->pDesc_.value->BufferCount = c.BufferCount_.value;
    }
    if (c.NewFormat_.value != DXGI_FORMAT_UNKNOWN) {
      command->pDesc_.value->BufferDesc.Format = c.NewFormat_.value;
    }
  } else if (objectState->creationCommand->getId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    IDXGIFactory2CreateSwapChainForHwndCommand* command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(
            objectState->creationCommand.get());
    command->pDesc_.value->Width = c.Width_.value;
    command->pDesc_.value->Height = c.Height_.value;
    command->pDesc_.value->Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      command->pDesc_.value->BufferCount = c.BufferCount_.value;
    }
    if (c.NewFormat_.value != DXGI_FORMAT_UNKNOWN) {
      command->pDesc_.value->Format = c.NewFormat_.value;
    }
  }

  unsigned swapChainKey = c.object_.key;
  for (unsigned bufferKey : swapchainBuffers_[swapChainKey]) {
    resourceStateTrackingService_.destroyResource(bufferKey);
    descriptorService_.removeState(bufferKey);
    stateService_.removeState(bufferKey);
  }
  swapchainBuffers_.erase(swapChainKey);
}

void StateTrackingLayer::post(IDXGISwapChain3ResizeBuffers1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* objectState = stateService_.getState(c.object_.key);
  if (objectState->creationCommand->getId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    IDXGIFactoryCreateSwapChainCommand* command =
        static_cast<IDXGIFactoryCreateSwapChainCommand*>(objectState->creationCommand.get());
    command->pDesc_.value->BufferDesc.Width = c.Width_.value;
    command->pDesc_.value->BufferDesc.Height = c.Height_.value;
    command->pDesc_.value->Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      command->pDesc_.value->BufferCount = c.BufferCount_.value;
    }
    if (c.Format_.value != DXGI_FORMAT_UNKNOWN) {
      command->pDesc_.value->BufferDesc.Format = c.Format_.value;
    }
  } else if (objectState->creationCommand->getId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    IDXGIFactory2CreateSwapChainForHwndCommand* command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(
            objectState->creationCommand.get());
    command->pDesc_.value->Width = c.Width_.value;
    command->pDesc_.value->Height = c.Height_.value;
    command->pDesc_.value->Flags = c.SwapChainFlags_.value;
    if (c.BufferCount_.value) {
      command->pDesc_.value->BufferCount = c.BufferCount_.value;
    }
    if (c.Format_.value != DXGI_FORMAT_UNKNOWN) {
      command->pDesc_.value->Format = c.Format_.value;
    }
  }

  unsigned swapChainKey = c.object_.key;
  for (unsigned bufferKey : swapchainBuffers_[swapChainKey]) {
    resourceStateTrackingService_.destroyResource(bufferKey);
    descriptorService_.removeState(bufferKey);
    stateService_.removeState(bufferKey);
  }
  swapchainBuffers_.erase(swapChainKey);
}

void StateTrackingLayer::post(ID3D12ObjectSetNameCommand& c) {
  if (stateRestored_) {
    return;
  }
  ObjectState* state = stateService_.getState(c.object_.key);
  if (state == nullptr) {
    LOG_ERROR << "StateTrackingLayer: SetName failed. Cannot find object O" << c.object_.key << ".";
    return;
  }
  state->name = c.Name_.value;
}

void StateTrackingLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  D3D12DescriptorHeapState* state = new D3D12DescriptorHeapState();
  state->parentKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateDescriptorHeapCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->parentKey = c.object_.key;
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateHeapCommand(c));
  stateService_.storeState(state);

  if (c.pDesc_.value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, c.object_.key);
  }
}

void StateTrackingLayer::post(ID3D12Device4CreateHeap1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->parentKey = c.object_.key;
  state->deviceKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12Device4CreateHeap1Command(c));
  stateService_.storeState(state);

  if (c.pDesc_.value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, c.object_.key);
  }
}

void StateTrackingLayer::post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateQueryHeapCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateHeapAllocationMetaCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12HeapFromAddressState* state = new D3D12HeapFromAddressState();
  state->key = c.heap_.key;
  state->creationCommand.reset(new CreateHeapAllocationMetaCommand(c));
  heapAllocationStateService_.setHeapState(state);
}

void StateTrackingLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12HeapFromAddressState* state = heapAllocationStateService_.getHeapState(c.ppvHeap_.key);
  state->parentKey = c.object_.key;
  state->openExistingHeapFromAddressCommand.reset(
      new ID3D12Device3OpenExistingHeapFromAddressCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppSurface_.key;
  state->object = static_cast<IUnknown*>(*c.ppSurface_.value);
  state->creationCommand.reset(new IDXGISwapChainGetBufferCommand(c));
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(0, static_cast<ID3D12Resource*>(*c.ppSurface_.value),
                                            state->key, D3D12_RESOURCE_STATE_COMMON, false);
  stateService_.addBackBuffer(c.Buffer_.value, state->key,
                              static_cast<ID3D12Resource*>(*c.ppSurface_.value));

  // Keep track of the buffer key
  swapchainBuffers_[c.object_.key].push_back(state->key);
}

void StateTrackingLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (stateRestored_) {
    return;
  }
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
  if (stateRestored_) {
    return;
  }
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
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppCommandAllocator_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandAllocator_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandAllocatorCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvRootSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvRootSignature_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateRootSignatureCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppPipelineLibrary_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineLibrary_.value);
  state->creationCommand.reset(new ID3D12Device1CreatePipelineLibraryCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->parentKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibrary1LoadPipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->parentKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibraryLoadGraphicsPipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->parentKey = c.object_.key;
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibraryLoadComputePipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvCommandSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvCommandSignature_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandSignatureCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateGraphicsPipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateComputePipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12Device2CreatePipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppStateObject_.value);
  state->creationCommand.reset(new ID3D12Device5CreateStateObjectCommand(c));
  stateService_.storeState(state);

  for (auto& it : c.pDesc_.interfaceKeysBySubobject) {
    stateService_.keepState(it.second);
  }

  accelerationStructuresSerializeService_.setDevice(c.object_.value, c.object_.key);
  accelerationStructuresBuildService_.setDeviceKey(c.object_.key);
}

void StateTrackingLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.pStateObjectToGrowFrom_.key;
  state->key = c.ppNewStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppNewStateObject_.value);
  state->creationCommand.reset(new ID3D12Device7AddToStateObjectCommand(c));
  stateService_.storeState(state);

  for (auto& it : c.pAddition_.interfaceKeysBySubobject) {
    stateService_.keepState(it.second);
  }

  stateService_.keepState(c.pStateObjectToGrowFrom_.key);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->parentKey = c.object_.key;
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandListCommand(c));
  stateService_.storeState(state);

  commandListService_.addCommandList(state);
}

void StateTrackingLayer::post(ID3D12Device4CreateCommandList1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->parentKey = c.object_.key;
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->creationCommand.reset(new ID3D12Device4CreateCommandList1Command(c));
  stateService_.storeState(state);

  commandListService_.addCommandList(state);
}

void StateTrackingLayer::pre(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommittedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateCommittedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device4CreateCommittedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);

  stateService_.storeState(state);
  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device8CreateCommittedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device8CreateCommittedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateCommittedResource3Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreateCommittedResource3Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.pHeap_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreatePlacedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  resourceForCBVRestoreService_.addResourceCreationCommand(
      state->key, state->heapKey, new ID3D12DeviceCreatePlacedResourceCommand(c));

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
  gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
}

void StateTrackingLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.pHeap_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device8CreatePlacedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  resourceForCBVRestoreService_.addResourceCreationCommand(
      state->key, state->heapKey, new ID3D12Device8CreatePlacedResource1Command(c));

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
  gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
}

void StateTrackingLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.pHeap_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreatePlacedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  resourceForCBVRestoreService_.addResourceCreationCommand(
      state->key, state->heapKey, new ID3D12Device10CreatePlacedResource2Command(c));

  resourceStateTrackingService_.addResource(
      state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
      state->initialState, !(state->isMappable || state->isBarrierRestricted));
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
  gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
}

void StateTrackingLayer::pre(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateReservedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateReservedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device4CreateReservedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateReservedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->parentKey = c.object_.key;
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreateReservedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
}

void StateTrackingLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (stateRestored_) {
    return;
  }
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
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppFence_.key;
  state->object = static_cast<IUnknown*>(*c.ppFence_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateFenceCommand(c));
  stateService_.storeState(state);

  fenceTrackingService_.setFenceValue(c.ppFence_.key, c.InitialValue_.value);
  accelerationStructuresBuildService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  gpuExecutionFlusher_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  resourceUsageTrackingService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void StateTrackingLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addCommandQueueSignal(c);
  } else {
    fenceTrackingService_.setFenceValue(c.pFence_.key, c.Value_.value);
  }

  accelerationStructuresBuildService_.commandQueueSignal(c);
  gpuExecutionFlusher_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  resourceUsageTrackingService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key,
                                                   c.Value_.value);
}

void StateTrackingLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addCommandQueueWait(c);
  }

  accelerationStructuresBuildService_.commandQueueWait(c);
  gpuExecutionFlusher_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  resourceUsageTrackingService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key,
                                                 c.Value_.value);
}

void StateTrackingLayer::post(ID3D12FenceSignalCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.object_.key, c.Value_.value);
  accelerationStructuresBuildService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  gpuExecutionFlusher_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  resourceUsageTrackingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void StateTrackingLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  accelerationStructuresBuildService_.fenceSignal(c.key, c.pFenceToSignal_.key,
                                                  c.FenceValueToSignal_.value);
  gpuExecutionFlusher_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);

  residencyService_.makeResident(c.ppObjects_.keys, c.object_.key);
  resourceUsageTrackingService_.fenceSignal(c.key, c.pFenceToSignal_.key,
                                            c.FenceValueToSignal_.value);
}

void StateTrackingLayer::post(ID3D12DeviceMakeResidentCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  residencyService_.makeResident(c.ppObjects_.keys, c.object_.key);
}

void StateTrackingLayer::post(ID3D12DeviceEvictCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  residencyService_.evict(c.ppObjects_.keys, c.object_.key);
}

void StateTrackingLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12SamplerState* state = new D3D12SamplerState();
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  state->auxiliaryResourceKey = c.pCounterResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
  state->deviceKey = c.object_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->resourceKey = c.pDesc_.bufferLocationKey;
  state->bufferLocationOffset = c.pDesc_.bufferLocationOffset;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12ResourceMapCommand& c) {
  if (stateRestored_) {
    return;
  }
  mapStateService_.mapResource(c.object_.key, c.Subresource_.value, c.ppData_.captureValue);
}

void StateTrackingLayer::post(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addUpdateTileMappings(c);
  } else {
    reservedResourcesService_.addUpdateTileMappings(c);
  }
}

void StateTrackingLayer::post(ID3D12CommandQueueCopyTileMappingsCommand& c) {
  if (stateRestored_) {
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "ID3D12CommandQueue::CopyTileMappings not handled in subcapture!";
    logged = true;
  }
}

void StateTrackingLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (stateRestored_) {
    return;
  }
  descriptorService_.copyDescriptors(c);
}

void StateTrackingLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (stateRestored_) {
    return;
  }
  descriptorService_.copyDescriptors(c);
}

void StateTrackingLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (stateRestored_) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppExtensionContext_.key;
  state->creationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContextCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(c.pDevice_.key, state->key);
  deviceByINTCExtensionContext_[state->key] = c.pDevice_.key;
}

void StateTrackingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (stateRestored_) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppExtensionContext_.key;
  state->creationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContext1Command(c));
  stateService_.storeState(state);

  setAsChildInParent(c.pDevice_.key, state->key);
  deviceByINTCExtensionContext_[state->key] = c.pDevice_.key;
}

void StateTrackingLayer::post(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (stateRestored_) {
    return;
  }
  stateService_.storeINTCApplicationInfo(c);
}

void StateTrackingLayer::post(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (stateRestored_) {
    return;
  }
  stateService_.releaseObject(c.ppExtensionContext_.key, c.result_.value);
  deviceByINTCExtensionContext_.erase(c.ppExtensionContext_.key);
}

void StateTrackingLayer::post(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (stateRestored_) {
    return;
  }
  stateService_.storeINTCFeature(*c.pFeature_.value);
}

void StateTrackingLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new INTC_D3D12_CreateCommittedResourceCommand(c));

  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->pD3D12Desc->Dimension;
  state->sampleCount = c.pDesc_.value->pD3D12Desc->SampleDesc.Count;
  state->isMappable =
      isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->pD3D12Desc->Layout);

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new INTC_D3D12_CreatePlacedResourceCommand(c));

  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->pD3D12Desc->Dimension;
  state->sampleCount = c.pDesc_.value->pD3D12Desc->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->pD3D12Desc->Layout);
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->pD3D12Desc->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  resourceForCBVRestoreService_.addResourceCreationCommand(
      state->key, state->heapKey, new INTC_D3D12_CreatePlacedResourceCommand(c));

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new INTC_D3D12_CreateReservedResourceCommand(c));

  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->pD3D12Desc->Dimension;
  state->sampleCount = c.pDesc_.value->pD3D12Desc->SampleDesc.Count;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
    resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->creationCommand.reset(new INTC_D3D12_CreateCommandQueueCommand(c));
  stateService_.storeState(state);

  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(INTC_D3D12_CreateHeapCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->deviceKey = deviceByINTCExtensionContext_[c.pExtensionContext_.key];
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new INTC_D3D12_CreateHeapCommand(c));
  stateService_.storeState(state);

  if (c.pDesc_.value->pD3D12Desc->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new INTC_D3D12_CreateComputePipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device1SetResidencyPriorityCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  for (unsigned i = 0; i < c.NumObjects_.value; ++i) {
    ObjectState* state = stateService_.getState(c.ppObjects_.keys[i]);
    GITS_ASSERT(state);
    if (state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE &&
        state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1 &&
        state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2 &&
        state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3 &&
        state->creationCommand->getId() != CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE &&
        state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE_CREATEHEAP &&
        state->creationCommand->getId() != CommandId::ID_ID3D12DEVICE4_CREATEHEAP1 &&
        state->creationCommand->getId() != CommandId::INTC_D3D12_CREATEHEAP) {
      LOG_WARNING << "SetResidencyPriority not handled for command "
                  << static_cast<unsigned>(state->creationCommand->getId());
    }
    state->residencyPriority = c.pPriorities_.value[i];
  }
}

void StateTrackingLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addExecuteCommandLists(c);
  } else {
    resourceStateTrackingService_.executeCommandLists(c.ppCommandLists_.keys);
    accelerationStructuresSerializeService_.executeCommandLists(c);
    accelerationStructuresBuildService_.executeCommandLists(c);
  }
  resourceUsageTrackingService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  resourceStateTracker_.executeCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(c.ppCommandLists_.value),
      c.NumCommandLists_.value);
}

void StateTrackingLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (stateRestored_) {
    return;
  }
  ResourceState* state = static_cast<ResourceState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  state->gpuVirtualAddress = c.result_.value;
  gpuAddressService_.addGpuCaptureAddress(c.object_.value, c.object_.key,
                                          c.object_.value->GetDesc().Width, c.result_.value);
}

void StateTrackingLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12StateObjectPropertiesState* state =
      static_cast<D3D12StateObjectPropertiesState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  stateService_.keepState(state->key);
  auto it = state->shaderIdentifiers.find(c.pExportName_.value);
  if (it == state->shaderIdentifiers.end()) {
    auto& shaderIdentifier = state->shaderIdentifiers[c.pExportName_.value];
    for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
      shaderIdentifier[i] = static_cast<uint8_t*>(c.result_.value)[i];
    }
  }
}

void StateTrackingLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (stateRestored_) {
    return;
  }
  D3D12DescriptorHeapState* state =
      static_cast<D3D12DescriptorHeapState*>(stateService_.getState(c.object_.key));
  GITS_ASSERT(state);
  state->gpuDescriptorHandle = c.result_.value;
}

void StateTrackingLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresBuildService_.buildAccelerationStructure(c);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresSerializeService_.buildAccelerationStructure(c);

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresBuildService_.copyAccelerationStructure(c);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresSerializeService_.copyAccelerationStructure(c);

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4DispatchRaysWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (stateRestored_) {
    return;
  }

  if (subcaptureRange_.isExecutionRangeStart()) {
    gpuExecutionFlusher_.flushCommandQueues();
    stateService_.restoreState();
  }

  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->clearCommands();

  if (state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST) {
    ID3D12DeviceCreateCommandListCommand* command =
        static_cast<ID3D12DeviceCreateCommandListCommand*>(state->creationCommand.get());
    command->pCommandAllocator_.key = c.pAllocator_.key;
    command->pInitialState_.key = c.pInitialState_.key;
  }

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResetWriter(c));
  state->commands.push_back(command);

  resourceUsageTrackingService_.commandListReset(state->key);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCloseCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCloseWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->closed = true;
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListClearStateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDrawInstancedWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDrawIndexedInstancedWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDispatchWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  if (stateRestored_) {
    return;
  }
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDstBuffer_.key);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyBufferRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  if (stateRestored_) {
    return;
  }
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDst_.resourceKey);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyTextureRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDstResource_.key);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyResourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  if (stateRestored_) {
    return;
  }
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pTiledResource_.key);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyTilesWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResolveSubresourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetPrimitiveTopologyWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListRSSetViewportsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListRSSetViewportsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListRSSetScissorRectsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListOMSetBlendFactorWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListOMSetStencilRefWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetPipelineStateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (stateRestored_) {
    return;
  }
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pBarriers_.resourceKeys);
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key,
                                                         c.pBarriers_.resourceAfterKeys);
  resourceStateTrackingService_.resourceBarrier(
      c.object_.key, c.pBarriers_.value, c.pBarriers_.resourceKeys, c.pBarriers_.resourceAfterKeys);
  resourceStateTracker_.resourceBarrier(c.object_.value, c.pBarriers_.value, c.NumBarriers_.value,
                                        c.pBarriers_.resourceKeys.data());

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResourceBarrierWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}
void StateTrackingLayer::post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListExecuteBundleWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetDescriptorHeapsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->descriptorHeapKeys = c.ppDescriptorHeaps_.keys;
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRootSignatureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRootSignatureWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRootDescriptorTableWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRoot32BitConstantWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetComputeRoot32BitConstantsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootConstantBufferViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootShaderResourceViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetIndexBufferWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListIASetVertexBuffersWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSOSetTargetsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListOMSetRenderTargets* command = new CommandListOMSetRenderTargets(c.key);
  command->renderTargetViews.resize(c.NumRenderTargetDescriptors_.value);
  {
    unsigned heapKey{};
    unsigned heapIndex{};
    for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
      if (i == 0 || !c.RTsSingleHandleToDescriptorRange_.value) {
        heapKey = c.pRenderTargetDescriptors_.interfaceKeys[i];
        heapIndex = c.pRenderTargetDescriptors_.indexes[i];
      } else {
        ++heapIndex;
      }
      if (heapKey) {
        DescriptorState* descriptorState =
            descriptorService_.getDescriptorState(heapKey, heapIndex);
        GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_RENDERTARGETVIEW);
        command->renderTargetViews[i].reset(new D3D12RenderTargetViewState(
            *static_cast<D3D12RenderTargetViewState*>(descriptorState)));
      }
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
  if (stateRestored_) {
    return;
  }
  CommandListClearDepthStencilView* command = new CommandListClearDepthStencilView(c.key);
  if (c.DepthStencilView_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_DEPTHSTENCILVIEW);
    command->depthStencilView.reset(
        new D3D12DepthStencilViewState(*static_cast<D3D12DepthStencilViewState*>(descriptorState)));
  }
  command->commandListKey = c.object_.key;
  command->depth = c.Depth_.value;
  command->stencil = c.Stencil_.value;
  if (c.NumRects_.value) {
    command->rects.resize(c.NumRects_.value);
    for (unsigned i = 0; i < c.NumRects_.value; ++i) {
      command->rects[i] = c.pRects_.value[i];
    }
  }

  command->commandWriter.reset(new ID3D12GraphicsCommandListClearDepthStencilViewWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (stateRestored_) {
    return;
  }
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
  if (stateRestored_) {
    return;
  }
  CommandListClearUnorderedAccessViewUint* command =
      new CommandListClearUnorderedAccessViewUint(c.key);
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    command->viewGPUHandleInCurrentHeap.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    command->viewCPUHandle.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  command->commandListKey = c.object_.key;
  command->resourceKey = c.pResource_.key;
  for (unsigned i = 0; i < 4; ++i) {
    command->values[i] = c.Values_.value[i];
  }
  if (c.NumRects_.value) {
    command->rects.resize(c.NumRects_.value);
    for (unsigned i = 0; i < c.NumRects_.value; ++i) {
      command->rects[i] = c.pRects_.value[i];
    }
  }

  command->commandWriter.reset(new ID3D12GraphicsCommandListClearUnorderedAccessViewUintWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListClearUnorderedAccessViewFloat* command =
      new CommandListClearUnorderedAccessViewFloat(c.key);
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    command->viewGPUHandleInCurrentHeap.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    DescriptorState* descriptorState = descriptorService_.getDescriptorState(
        c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index);
    GITS_ASSERT(descriptorState->id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    command->viewCPUHandle.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  command->commandListKey = c.object_.key;
  command->resourceKey = c.pResource_.key;
  for (unsigned i = 0; i < 4; ++i) {
    command->values[i] = c.Values_.value[i];
  }
  if (c.NumRects_.value) {
    command->rects.resize(c.NumRects_.value);
    for (unsigned i = 0; i < c.NumRects_.value; ++i) {
      command->rects[i] = c.pRects_.value[i];
    }
  }

  command->commandWriter.reset(new ID3D12GraphicsCommandListClearUnorderedAccessViewFloatWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListDiscardResourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListBeginQueryWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListEndQueryWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListResolveQueryDataWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetPredicationWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListSetMarkerCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListSetMarkerWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListBeginEventCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListBeginEventWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListEndEventCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListEndEventWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListExecuteIndirectWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1AtomicCopyBufferUINTWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Writer(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1OMSetDepthBoundsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1SetSamplePositionsCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1SetSamplePositionsWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1ResolveSubresourceRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList1SetViewInstanceMaskWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList2WriteBufferImmediateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList3SetProtectedResourceSessionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4BeginRenderPassWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(
      new ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4EndRenderPassCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4EndRenderPassWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  if (stateRestored_) {
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "ID3D12GraphicsCommandList4ExecuteMetaCommand is not supported in subcapture.";
    logged = true;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4ExecuteMetaCommandWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4InitializeMetaCommandWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList4SetPipelineState1Writer(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList5RSSetShadingRateWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList5RSSetShadingRateImageWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  if (stateRestored_) {
    return;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList6DispatchMeshWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (stateRestored_) {
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "ID3D12GraphicsCommandList7::Barrier is not supported in subcapture.";
    logged = true;
  }
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandList7BarrierWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& c) {
  if (stateRestored_) {
    return;
  }

  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvFactory_.key;
  state->object = static_cast<IUnknown*>(*c.ppvFactory_.value);
  state->creationCommand.reset(new ID3D12SDKConfiguration1CreateDeviceFactoryCommand(c));
  stateService_.storeState(state);
  stateService_.keepState(state->parentKey);
}

void StateTrackingLayer::post(ID3D12DeviceFactoryCreateDeviceCommand& c) {
  if (stateRestored_) {
    return;
  }

  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvDevice_.key;
  state->object = static_cast<IUnknown*>(*c.ppvDevice_.value);
  state->creationCommand.reset(new ID3D12DeviceFactoryCreateDeviceCommand(c));
  stateService_.storeState(state);
  stateService_.keepState(state->parentKey);
}

void StateTrackingLayer::post(xessD3D12CreateContextCommand& c) {
  if (stateRestored_) {
    return;
  }
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
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->initParams.emplace(c.pInitParams_);
}

void StateTrackingLayer::pre(xessDestroyContextCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->device->AddRef();
}

void StateTrackingLayer::post(xessDestroyContextCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  ULONG ref = state->device->Release();
  if (ref == 0) {
    stateService_.releaseObject(state->deviceKey, 0);
  }
  xessStateService_.destroyContext(c.hContext_.key);
}

void StateTrackingLayer::post(xessSetJitterScaleCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  if (!state->jitterScale) {
    state->jitterScale = std::make_unique<float[]>(2);
  }
  state->jitterScale[0] = c.x_.value;
  state->jitterScale[1] = c.y_.value;
}

void StateTrackingLayer::post(xessSetVelocityScaleCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  if (!state->velocityScale) {
    state->velocityScale = std::make_unique<float[]>(2);
  }
  state->velocityScale[0] = c.x_.value;
  state->velocityScale[1] = c.y_.value;
}

void StateTrackingLayer::post(xessSetExposureMultiplierCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->exposureScale = c.scale_.value;
}

void StateTrackingLayer::post(xessForceLegacyScaleFactorsCommand& c) {
  if (stateRestored_) {
    return;
  }
  XessStateService::ContextState* state = xessStateService_.getContextState(c.hContext_.key);
  state->forceLegacyScaleFactors = c.force_.value;
}

void StateTrackingLayer::post(DStorageGetFactoryCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new DStorageGetFactoryCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDStorageFactoryOpenFileCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryOpenFileCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryCreateQueueCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->linkedLifetimeKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryCreateStatusArrayCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(NvAPI_InitializeCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != NVAPI_OK) {
    return;
  }
  stateService_.getNvAPIGlobalStateService().incrementInitialize();
}

void StateTrackingLayer::post(NvAPI_UnloadCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != NVAPI_OK) {
    return;
  }
  stateService_.getNvAPIGlobalStateService().decrementInitialize();
}

void StateTrackingLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != NVAPI_OK) {
    return;
  }
  stateService_.getNvAPIGlobalStateService().addSetCreatePipelineStateOptionsCommand(c);
}

void StateTrackingLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != NVAPI_OK) {
    return;
  }
  stateService_.getNvAPIGlobalStateService().addSetNvShaderExtnSlotSpaceCommand(c);
}

void StateTrackingLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (stateRestored_) {
    return;
  }
  if (c.result_.value != NVAPI_OK) {
    return;
  }
  stateService_.getNvAPIGlobalStateService().addSetNvShaderExtnSlotSpaceLocalThreadCommand(c);
}

void StateTrackingLayer::pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresBuildService_.nvapiBuildAccelerationStructureEx(c);
}

void StateTrackingLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (stateRestored_) {
    return;
  }

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(c));
  CommandListState* state =
      static_cast<CommandListState*>(stateService_.getState(c.pCommandList_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (stateRestored_) {
    return;
  }
  accelerationStructuresBuildService_.nvapiBuildOpacityMicromapArray(c);
}

void StateTrackingLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (stateRestored_) {
    return;
  }

  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(c));
  CommandListState* state =
      static_cast<CommandListState*>(stateService_.getState(c.pCommandList_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(DllContainerMetaCommand& c) {
  if (stateRestored_) {
    return;
  }

  stateService_.storeDllContainer(c);
}

} // namespace DirectX
} // namespace gits
