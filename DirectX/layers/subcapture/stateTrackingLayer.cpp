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
#include "configurationLib.h"

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
                    commandQueueService_,
                    xessStateService_,
                    accelerationStructuresSerializeService_,
                    accelerationStructuresBuildService_,
                    residencyService_,
                    analyzerResults_,
                    resourceUsageTrackingService_),
      recorder_(recorder),
      mapStateService_(stateService_),
      resourceStateTrackingService_(stateService_),
      reservedResourcesService_(stateService_, resourceStateTrackingService_),
      descriptorService_(stateService_),
      commandListService_(stateService_),
      commandQueueService_(stateService_),
      xessStateService_(stateService_, recorder),
      accelerationStructuresSerializeService_(stateService_, recorder_),
      accelerationStructuresBuildService_(stateService_, recorder_, reservedResourcesService_),
      residencyService_(stateService_) {

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      startFrame_ = std::stoi(frames.substr(0, pos));
    } else {
      startFrame_ = std::stoi(frames);
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Configurator::Get().directx.features.subcapture.frames + "'");
  }
  const std::string& commandListExecutions =
      Configurator::Get().directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    commandListSubcapture_ = true;
  }
}

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
  if (c.Flags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  if (!commandListSubcapture_) {
    unsigned currentFrame = CGits::Instance().CurrentFrame();
    if (currentFrame == startFrame_ - 1) {
      Log(INFOV) << "Start subcapture frame " << currentFrame + 1 << " call " << c.key;
      gpuExecutionFlusher_.flushCommandQueues();
      stateService_.restoreState();
    }
  }
}

void StateTrackingLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  if (!commandListSubcapture_) {
    unsigned currentFrame = CGits::Instance().CurrentFrame();
    if (currentFrame == startFrame_ - 1) {
      Log(INFOV) << "Start subcapture frame " << currentFrame + 1 << " call " << c.key;
      gpuExecutionFlusher_.flushCommandQueues();
      stateService_.restoreState();
    }
  }
}

void StateTrackingLayer::pre(IUnknownReleaseCommand& c) {
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
  stateService_.setReferenceCount(c.object_.key, c.result_.value);
}

void StateTrackingLayer::post(IUnknownQueryInterfaceCommand& c) {
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
    state->key = c.ppvObject_.key;

    setAsChildInParent(state->parentKey, state->key);
    state->creationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    stateService_.storeState(state);
  } else if ((riid == __uuidof(IDStorageCustomDecompressionQueue)) ||
             (riid == __uuidof(IDStorageCustomDecompressionQueue1))) {
    ObjectState* state = new ObjectState();
    state->parentKey = c.object_.key;
    state->key = c.ppvObject_.key;

    setAsChildInParent(state->parentKey, state->key);
    state->creationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(CreateDXGIFactoryCommand& c) {
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
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppAdapter_.value);
  state->creationCommand.reset(new IDXGIFactoryEnumAdaptersCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory1EnumAdapters1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory1EnumAdapters1Command(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory6EnumAdapterByGpuPreferenceCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppvAdapter_.key;
  state->object = static_cast<IUnknown*>(*c.ppvAdapter_.value);
  state->creationCommand.reset(new IDXGIFactory4EnumAdapterByLuidCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDXGIObjectGetParentCommand& c) {
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
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppDevice_.key;
  state->object = static_cast<IUnknown*>(*c.ppDevice_.value);
  state->creationCommand.reset(new D3D12CreateDeviceCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
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
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppCommandQueue_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandQueue_.value);
  state->creationCommand.reset(new ID3D12Device9CreateCommandQueue1Command(c));
  stateService_.storeState(state);

  gpuExecutionFlusher_.createCommandQueue(
      c.ppCommandQueue_.key, *reinterpret_cast<ID3D12CommandQueue**>(c.ppCommandQueue_.value));
}

void StateTrackingLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppSwapChain_.key;
  state->object = static_cast<IUnknown*>(*c.ppSwapChain_.value);
  state->creationCommand.reset(new IDXGIFactoryCreateSwapChainCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppSwapChain_.key;
  state->object = static_cast<IUnknown*>(*c.ppSwapChain_.value);
  state->creationCommand.reset(new IDXGIFactory2CreateSwapChainForHwndCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainResizeBuffersCommand& c) {
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
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateDescriptorHeapCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
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
  if (c.result_.value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
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
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppvHeap_.key;
  state->object = static_cast<IUnknown*>(*c.ppvHeap_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateQueryHeapCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(CreateHeapAllocationMetaCommand& c) {
  D3D12HeapFromAddressState* state = new D3D12HeapFromAddressState();
  state->key = c.heap_.key;
  state->creationCommand.reset(new CreateHeapAllocationMetaCommand(c));
  heapAllocationStateService_.setHeapState(state);
}

void StateTrackingLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  D3D12HeapFromAddressState* state = heapAllocationStateService_.getHeapState(c.ppvHeap_.key);
  state->openExistingHeapFromAddressCommand.reset(
      new ID3D12Device3OpenExistingHeapFromAddressCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppSurface_.key;
  state->object = static_cast<IUnknown*>(*c.ppSurface_.value);
  state->creationCommand.reset(new IDXGISwapChainGetBufferCommand(c));
  stateService_.storeState(state);

  resourceStateTrackingService_.addResource(0, static_cast<ID3D12Resource*>(*c.ppSurface_.value),
                                            state->key, D3D12_RESOURCE_STATE_COMMON, false);

  // Keep track of the buffer key
  swapchainBuffers_[c.object_.key].push_back(state->key);
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
  ObjectState* state = new ObjectState();
  state->key = c.ppCommandAllocator_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandAllocator_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandAllocatorCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppvRootSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvRootSignature_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateRootSignatureCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppPipelineLibrary_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineLibrary_.value);
  state->creationCommand.reset(new ID3D12Device1CreatePipelineLibraryCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibrary1LoadPipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibraryLoadGraphicsPipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* exisitingState = stateService_.getState(c.ppPipelineState_.key);
  if (exisitingState) {
    ++exisitingState->refCount;
  } else {
    ObjectState* state = new ObjectState();
    state->key = c.ppPipelineState_.key;
    state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
    state->creationCommand.reset(new ID3D12PipelineLibraryLoadComputePipelineCommand(c));
    stateService_.storeState(state);
  }
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppvCommandSignature_.key;
  state->object = static_cast<IUnknown*>(*c.ppvCommandSignature_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandSignatureCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateGraphicsPipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateComputePipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppPipelineState_.key;
  state->object = static_cast<IUnknown*>(*c.ppPipelineState_.value);
  state->creationCommand.reset(new ID3D12Device2CreatePipelineStateCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppStateObject_.value);
  state->creationCommand.reset(new ID3D12Device5CreateStateObjectCommand(c));
  stateService_.storeState(state);

  accelerationStructuresSerializeService_.setDevice(c.object_.value, c.object_.key);
  accelerationStructuresBuildService_.setDeviceKey(c.object_.key);
}

void StateTrackingLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->key = c.ppNewStateObject_.key;
  state->object = static_cast<IUnknown*>(*c.ppNewStateObject_.value);
  state->creationCommand.reset(new ID3D12Device7AddToStateObjectCommand(c));
  stateService_.storeState(state);

  stateService_.keepState(c.pStateObjectToGrowFrom_.key);
}

void StateTrackingLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommandListCommand(c));
  stateService_.storeState(state);

  commandListService_.addCommandList(state);
}

void StateTrackingLayer::post(ID3D12Device4CreateCommandList1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->key = c.ppCommandList_.key;
  state->object = static_cast<IUnknown*>(*c.ppCommandList_.value);
  state->creationCommand.reset(new ID3D12Device4CreateCommandList1Command(c));
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
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateCommittedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateCommittedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device4CreateCommittedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);
  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device8CreateCommittedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device8CreateCommittedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialResourceState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateCommittedResource3Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreateCommittedResource3Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(*c.pHeapProperties_.value, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  resourceStateTrackingService_.addResource(state->deviceKey,
                                            static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                            state->key, state->initialLayout, !state->isMappable);
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreatePlacedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
  }
}

void StateTrackingLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device8CreatePlacedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreatePlacedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isMappable = isResourceHeapMappable(c.pHeap_.key, c.pDesc_.value->Layout);
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  resourceStateTrackingService_.addResource(
      state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
      state->initialState, !(state->isMappable || state->isBarrierRestricted));
}

void StateTrackingLayer::pre(ID3D12DeviceCreateReservedResourceCommand& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12DeviceCreateReservedResourceCommand(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device4CreateReservedResource1Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device4CreateReservedResource1Command(c));

  state->deviceKey = c.object_.key;
  state->initialState = c.InitialState_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
}

void StateTrackingLayer::pre(ID3D12Device10CreateReservedResource2Command& c) {
  c.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

void StateTrackingLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->key = c.ppvResource_.key;
  state->object = static_cast<IUnknown*>(*c.ppvResource_.value);
  state->creationCommand.reset(new ID3D12Device10CreateReservedResource2Command(c));

  state->deviceKey = c.object_.key;
  state->initialLayout = c.InitialLayout_.value;
  state->dimension = c.pDesc_.value->Dimension;
  state->sampleCount = c.pDesc_.value->SampleDesc.Count;
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

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
  ObjectState* state = new ObjectState();
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
  if (c.result_.value != S_OK) {
    return;
  }
  fenceTrackingService_.setFenceValue(c.object_.key, c.Value_.value);
  accelerationStructuresBuildService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  gpuExecutionFlusher_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  resourceUsageTrackingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void StateTrackingLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
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
  if (c.result_.value != S_OK) {
    return;
  }
  residencyService_.makeResident(c.ppObjects_.keys, c.object_.key);
}

void StateTrackingLayer::post(ID3D12DeviceEvictCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  residencyService_.evict(c.ppObjects_.keys, c.object_.key);
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
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addUpdateTileMappings(c);
  } else {
    reservedResourcesService_.addUpdateTileMappings(c);
  }
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
  ObjectState* state = new ObjectState();
  state->key = c.ppExtensionContext_.key;
  state->creationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContextCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(c.pDevice_.key, state->key);
  deviceByINTCExtensionContext_[state->key] = c.pDevice_.key;
}

void StateTrackingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  ObjectState* state = new ObjectState();
  state->key = c.ppExtensionContext_.key;
  state->creationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContext1Command(c));
  stateService_.storeState(state);

  setAsChildInParent(c.pDevice_.key, state->key);
  deviceByINTCExtensionContext_[state->key] = c.pDevice_.key;
}

void StateTrackingLayer::post(INTC_D3D12_SetApplicationInfoCommand& c) {
  stateService_.storeINTCApplicationInfo(c);
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
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
  if (c.HeapFlags_.value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    residencyService_.createNotResident(state->key, state->deviceKey);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
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
  state->isGenericRead = state->initialLayout == D3D12_BARRIER_LAYOUT_GENERIC_READ;
  state->isBarrierRestricted = isResourceBarrierRestricted(c.pDesc_.value->pD3D12Desc->Flags);
  state->heapKey = c.pHeap_.key;

  stateService_.storeState(state);

  resourceHeaps_[c.pHeap_.key].insert(c.ppvResource_.key);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(
        state->deviceKey, static_cast<ID3D12Resource*>(*c.ppvResource_.value), state->key,
        state->initialState, !(state->isMappable || state->isBarrierRestricted));
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
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
  state->isGenericRead = state->initialState == D3D12_RESOURCE_STATE_GENERIC_READ;

  stateService_.storeState(state);

  resourceUsageTrackingService_.addResource(state->key);

  if (state->initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    resourceStateTrackingService_.addResource(state->deviceKey,
                                              static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                              state->key, state->initialState, !state->isMappable);
  }
}

void StateTrackingLayer::post(INTC_D3D12_CreateCommandQueueCommand& c) {
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

void StateTrackingLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.pDesc_.cs = c.pDesc_.value->CS.pShaderBytecode;
  c.pDesc_.compileOptions = c.pDesc_.value->CompileOptions;
  c.pDesc_.internalOptions = c.pDesc_.value->InternalOptions;
}

void StateTrackingLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
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
      Log(WARN) << "SetResidencyPriority not handled for command "
                << static_cast<unsigned>(state->creationCommand->getId());
    }
    state->residencyPriority = c.pPriorities_.value[i];
  }
}

void StateTrackingLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (analyzerResults_.restoreCommandQueueCommand(c.key)) {
    commandQueueService_.addExecuteCommandLists(c);
  } else {
    resourceStateTrackingService_.executeCommandLists(c.ppCommandLists_.keys);
    accelerationStructuresSerializeService_.executeCommandLists(c);
    accelerationStructuresBuildService_.executeCommandLists(c);
  }
  resourceUsageTrackingService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
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
  stateService_.keepState(state->key);
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

  if (commandListSubcapture_ && recorder_.isExecutionRangeStart()) {
    Log(INFOV) << "Start command lists subcapture call " << c.key;
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
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDstBuffer_.key);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyBufferRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDst_.resourceKey);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyTextureRegionWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pDstResource_.key);
  CommandListCommand* command = new CommandListCommand(c.getId(), c.key);
  command->commandWriter.reset(new ID3D12GraphicsCommandListCopyResourceWriter(c));
  CommandListState* state = static_cast<CommandListState*>(stateService_.getState(c.object_.key));
  state->commands.push_back(command);
}

void StateTrackingLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pTiledResource_.key);
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
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key, c.pBarriers_.resourceKeys);
  resourceUsageTrackingService_.commandListResourceUsage(c.object_.key,
                                                         c.pBarriers_.resourceAfterKeys);
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
  state->descriptorHeapKeys = c.ppDescriptorHeaps_.keys;
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
  state->initParams.emplace(c.pInitParams_);
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
  ObjectState* state = new ObjectState();
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new DStorageGetFactoryCommand(c));
  stateService_.storeState(state);
}

void StateTrackingLayer::post(IDStorageFactoryOpenFileCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryOpenFileCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryCreateQueueCommand(c));
  stateService_.storeState(state);

  setAsChildInParent(state->parentKey, state->key);
}

void StateTrackingLayer::post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->parentKey = c.object_.key;
  state->key = c.ppv_.key;
  state->object = static_cast<IUnknown*>(*c.ppv_.value);
  state->creationCommand.reset(new IDStorageFactoryCreateStatusArrayCommand(c));

  setAsChildInParent(state->parentKey, state->key);
}

} // namespace DirectX
} // namespace gits
