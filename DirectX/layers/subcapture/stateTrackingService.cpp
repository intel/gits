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
#include "commandWritersFactory.h"
#include "gits.h"
#include "configurationLib.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

StateTrackingService::~StateTrackingService() {
  for (auto& it : statesByKey_) {
    delete it.second;
  }
}

void StateTrackingService::restoreState() {
  auto recordStatus = [this](MarkerUInt64Command::Value state) {
    recorder_.record(new CTokenMarkerUInt64(state));
  };

  recorder_.record(new CTokenMarker(CToken::ID_INIT_START));
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_BEGIN);
  for (auto& it : statesByKey_) {
    if (analyzerResults_.restoreObject(it.first)) {
      restoreState(it.second);
    }
  }
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_END);
  nvapiGlobalStateService_.restureInitializeCount();
  xessStateService_.restoreState();
  descriptorService_.restoreState();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RTAS_BEGIN);
  accelerationStructuresSerializeService_.restoreAccelerationStructures();
  accelerationStructuresBuildService_.restoreAccelerationStructures();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RTAS_END);
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_BEGIN);
  restoreResources();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_END);
  mapStateService_.restoreMapState();
  residencyService_.restoreResidency();
  commandListService_.restoreCommandLists();
  commandQueueService_.restoreCommandQueues();
  restoreReferenceCount();
  swapChainService_.restoreBackBufferSequence();
  recorder_.record(new CTokenMarker(CToken::ID_INIT_END));
  // one Present after ID_INIT_END to enable PIX first frame capture in gits interactive mode
  swapChainService_.recordSwapChainPresent();

  copyAuxiliaryFiles();
}

void StateTrackingService::copyAuxiliaryFiles() {
  std::filesystem::path streamDir = Configurator::Get().common.player.streamDir;
  std::filesystem::path subcapturePath = Configurator::Get().common.player.subcapturePath;
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
  ObjectState* state = it->second;
  state->keepDestroyed = true;
  if (state->creationCommand.get() &&
      (state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE ||
       state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1 ||
       state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2)) {
    ResourceState* resourceState = static_cast<ResourceState*>(state);
    keepState(resourceState->heapKey);
  }
  if (state->parentKey) {
    keepState(state->parentKey);
  }
}

void StateTrackingService::restoreState(ObjectState* state) {
  if (state->restored) {
    return;
  }
  state->restored = true;
  if (state->parentKey) {
    ObjectState* parentState = getState(state->parentKey);
    GITS_ASSERT(parentState);
    restoreState(parentState);
  }
  switch (state->creationCommand->getId()) {
  case CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN:
  case CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND:
    restoreDXGISwapChain(state);
    break;
  case CommandId::ID_IDXGIFACTORY_ENUMADAPTERS:
  case CommandId::ID_IDXGIFACTORY1_ENUMADAPTERS1:
  case CommandId::ID_IDXGIFACTORY6_ENUMADAPTERBYGPUPREFERENCE:
  case CommandId::ID_IDXGIFACTORY4_ENUMADAPTERBYLUID:
    restoreDXGIAdapter(state);
    break;
  case CommandId::ID_D3D12CREATEDEVICE:
    restoreD3D12Device(state);
    break;
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    restoreQueryInterface(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEFENCE:
    restoreD3D12Fence(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEDESCRIPTORHEAP:
    restoreD3D12DescriptorHeap(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST:
    restoreD3D12CommandList(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEHEAP:
  case CommandId::ID_ID3D12DEVICE4_CREATEHEAP1:
  case CommandId::INTC_D3D12_CREATEHEAP:
    restoreD3D12Heap(state);
    break;
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    restoreD3D12HeapFromAddress(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2:
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3:
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    restoreD3D12CommittedResource(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    restoreD3D12PlacedResource(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE:
    restoreD3D12ReservedResource(state);
    break;
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    restoreD3D12INTCDeviceExtensionContext(state);
    break;
  case CommandId::ID_ID3D12DEVICE5_CREATESTATEOBJECT:
    restoreD3D12StateObject(state);
    break;
  default:
    recorder_.record(createCommandWriter(state->creationCommand.get()));
  }

  if (!state->name.empty()) {
    ID3D12ObjectSetNameCommand c;
    c.key = getUniqueCommandKey();
    c.object_.key = state->key;
    c.Name_.value = state->name.data();
    recorder_.record(new ID3D12ObjectSetNameWriter(c));
  }
}

void StateTrackingService::storeState(ObjectState* state) {
  auto it = statesByKey_.find(state->key);
  if (it == statesByKey_.end()) {
    statesByKey_[state->key] = state;
  } else {
    // if the same objects in capture are not the same in replay
    it->second->object = state->object;
    delete state;
    state = it->second;
  }
  if (!state->refCount) {
    state->refCount = 1;
  }
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

  if (itState->second->creationCommand->getId() ==
      CommandId::ID_ID3D12DEVICE1_CREATEPIPELINELIBRARY) {
    return;
  }

  unsigned linkedLifetimeKey = itState->second->linkedLifetimeKey;
  if (linkedLifetimeKey) {
    auto itLinkedLifetimeState = statesByKey_.find(linkedLifetimeKey);
    if (itLinkedLifetimeState != statesByKey_.end()) {
      itLinkedLifetimeState->second->object->AddRef();
      ULONG refCount = itLinkedLifetimeState->second->object->Release();
      if (refCount == 1 && !itLinkedLifetimeState->second->destroyed) {
        releaseObject(itLinkedLifetimeState->first, 0);
      }
    }
  }

  for (unsigned childKey : itState->second->childrenKeys) {
    if (childKey) {
      auto itChildState = statesByKey_.find(childKey);
      if (itChildState != statesByKey_.end() && !itChildState->second->destroyed) {
        releaseObject(itChildState->first, 0);
      }
    }
  }

  if (!itState->second->keepDestroyed) {
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
    if (!state->object || !analyzerResults_.restoreObject(it.first)) {
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
    if (state->refCount <= 0 ||
        state->creationCommand->getId() == CommandId::ID_IDXGIFACTORY_ENUMADAPTERS ||
        state->creationCommand->getId() == CommandId::ID_IDXGIFACTORY1_ENUMADAPTERS1 ||
        state->creationCommand->getId() == CommandId::ID_IDXGIFACTORY6_ENUMADAPTERBYGPUPREFERENCE ||
        state->creationCommand->getId() == CommandId::ID_IDXGIFACTORY4_ENUMADAPTERBYLUID ||
        state->creationCommand->getId() == CommandId::ID_CREATEDXGIFACTORY ||
        state->creationCommand->getId() == CommandId::ID_CREATEDXGIFACTORY1 ||
        state->creationCommand->getId() == CommandId::ID_CREATEDXGIFACTORY2 ||
        state->creationCommand->getId() == CommandId::ID_IDXGISWAPCHAIN_GETBUFFER ||
        state->creationCommand->getId() == CommandId::ID_IDXGIADAPTER_ENUMOUTPUTS ||
        state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE_CREATEROOTSIGNATURE ||
        state->creationCommand->getId() == CommandId::ID_ID3D12DEVICE1_CREATEPIPELINELIBRARY ||
        state->creationCommand->getId() ==
            CommandId::ID_ID3D12PIPELINELIBRARY_LOADGRAPHICSPIPELINE ||
        state->creationCommand->getId() ==
            CommandId::ID_ID3D12PIPELINELIBRARY_LOADCOMPUTEPIPELINE ||
        state->creationCommand->getId() == CommandId::ID_ID3D12PIPELINELIBRARY1_LOADPIPELINE) {
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

void StateTrackingService::restoreResources() {
  std::vector<unsigned> orderedResources = resourceUsageTrackingService_.getOrderedResources();

  enum ResourceBatchType {
    ReservedResourceBuffer,
    ReservedResourceTexture,
    CommittedOrPlacedResource,
  };

  std::vector<unsigned> reservedResourceBuffers; // handled separately
  std::vector<std::pair<ResourceBatchType, std::vector<unsigned>>> batches;

  // prepare batches
  {
    ResourceBatchType prevType{};
    for (unsigned resourceKey : orderedResources) {
      ResourceState* state = static_cast<ResourceState*>(getState(resourceKey));
      if (!state) {
        continue;
      }

      ResourceBatchType type{};
      CommandId commandId = state->creationCommand->getId();
      bool isReservedResource = commandId == CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE ||
                                commandId == CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1 ||
                                commandId == CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2 ||
                                commandId == CommandId::INTC_D3D12_CREATERESERVEDRESOURCE;

      if (isReservedResource) {
        type = ReservedResourceTexture;
        if (state->dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
          type = ReservedResourceBuffer;
        }
      } else {
        type = CommittedOrPlacedResource;
      }

      if (type == ReservedResourceBuffer) {
        reservedResourceBuffers.push_back(resourceKey);
      } else if (batches.empty() || type != prevType) {
        batches.push_back({type, {resourceKey}});
      } else {
        batches.back().second.push_back(resourceKey);
      }

      prevType = type;
    }
  }

  // restore batches
  {
    // restore reserved resource buffers first
    reservedResourcesService_.restoreContent(reservedResourceBuffers);

    // restore batches in the order of usage
    for (const auto& [type, resourceKeys] : batches) {
      if (type == CommittedOrPlacedResource) {
        resourceContentRestore_.restoreContent(resourceKeys);
      } else if (type == ReservedResourceTexture) {
        reservedResourcesService_.restoreContent(resourceKeys);
      }
    }
  }

  // restore states
  resourceStateTrackingService_.restoreResourceStates(orderedResources);
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

void StateTrackingService::restoreDXGISwapChain(ObjectState* state) {
  if (state->creationCommand->getId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    auto* command = static_cast<IDXGIFactoryCreateSwapChainCommand*>(state->creationCommand.get());
    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.key = getUniqueCommandKey();
    createWindowCommand.hWnd_.value = command->pDesc_.value->OutputWindow;
    createWindowCommand.width_.value = command->pDesc_.value->BufferDesc.Width;
    createWindowCommand.height_.value = command->pDesc_.value->BufferDesc.Height;
    recorder_.record(new CreateWindowMetaWriter(createWindowCommand));

    swapChainService_.setSwapChain(state->key, *command->ppSwapChain_.value,
                                   command->pDesc_.value->BufferCount);
  } else if (state->creationCommand->getId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    auto* command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(state->creationCommand.get());
    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.key = getUniqueCommandKey();
    createWindowCommand.hWnd_.value = command->hWnd_.value;
    createWindowCommand.width_.value = command->pDesc_.value->Width;
    createWindowCommand.height_.value = command->pDesc_.value->Height;
    recorder_.record(new CreateWindowMetaWriter(createWindowCommand));

    swapChainService_.setSwapChain(state->key, *command->ppSwapChain_.value,
                                   command->pDesc_.value->BufferCount);
  }
  recorder_.record(createCommandWriter(state->creationCommand.get()));
}

void StateTrackingService::restoreDXGIAdapter(ObjectState* state) {
  recorder_.record(createCommandWriter(state->creationCommand.get()));
  restoreINTCApplicationInfo();
}

void StateTrackingService::restoreD3D12DescriptorHeap(ObjectState* state) {
  recorder_.record(createCommandWriter(state->creationCommand.get()));

  D3D12DescriptorHeapState* descriptorHeapState = static_cast<D3D12DescriptorHeapState*>(state);
  if (descriptorHeapState->gpuDescriptorHandle.ptr) {
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand getCommand;
    getCommand.key = getUniqueCommandKey();
    getCommand.object_.key = state->key;
    getCommand.result_.value = descriptorHeapState->gpuDescriptorHandle;
    recorder_.record(new ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartWriter(getCommand));
  }
}

void StateTrackingService::restoreD3D12Device(ObjectState* state) {
  deviceKey_ = state->key;
  recorder_.record(createCommandWriter(state->creationCommand.get()));
}

void StateTrackingService::restoreQueryInterface(ObjectState* state) {
  auto* command = static_cast<IUnknownQueryInterfaceCommand*>(state->creationCommand.get());
  if (command->riid_.value == IID_ID3D12StateObjectProperties) {
    ObjectState* stateObjectState = getState(state->parentKey);
    if (!stateObjectState || !stateObjectState->restored) {
      return;
    }

    recorder_.record(createCommandWriter(state->creationCommand.get()));

    auto* propertiesState = static_cast<D3D12StateObjectPropertiesState*>(state);
    for (auto& shaderIdentifier : propertiesState->shaderIdentifiers) {
      ID3D12StateObjectPropertiesGetShaderIdentifierCommand c;
      c.key = getUniqueCommandKey();
      c.object_.key = state->key;
      c.pExportName_.value = const_cast<wchar_t*>(shaderIdentifier.first.c_str());
      c.result_.value = shaderIdentifier.second.data();
      recorder_.record(new ID3D12StateObjectPropertiesGetShaderIdentifierWriter(c));
    }
  } else if (command->riid_.value == __uuidof(IDStorageCustomDecompressionQueue) ||
             command->riid_.value == __uuidof(IDStorageCustomDecompressionQueue1)) {
    recorder_.record(createCommandWriter(state->creationCommand.get()));
  }
}

void StateTrackingService::restoreD3D12Fence(ObjectState* state) {
  ID3D12DeviceCreateFenceCommand* command =
      static_cast<ID3D12DeviceCreateFenceCommand*>(state->creationCommand.get());
  command->InitialValue_.value = fenceTrackingService_.getFenceValue(state->key);
  recorder_.record(createCommandWriter(state->creationCommand.get()));
}

void StateTrackingService::restoreD3D12CommandList(ObjectState* state) {
  auto* command = static_cast<ID3D12DeviceCreateCommandListCommand*>(state->creationCommand.get());
  unsigned allocatorKey = command->pCommandAllocator_.key;
  unsigned initialStateKey = command->pInitialState_.key;

  ObjectState* allocatorState = getState(allocatorKey);
  if (!allocatorState) {
    return;
  }
  restoreState(allocatorState);
  if (initialStateKey) {
    ObjectState* initialState = getState(initialStateKey);
    restoreState(initialState);
  }

  ID3D12CommandAllocatorResetCommand reset;
  reset.key = getUniqueCommandKey();
  reset.object_.key = allocatorKey;
  recorder_.record(new ID3D12CommandAllocatorResetWriter(reset));

  recorder_.record(createCommandWriter(state->creationCommand.get()));

  ID3D12GraphicsCommandListCloseCommand close;
  close.key = getUniqueCommandKey();
  close.object_.key = state->key;
  recorder_.record(new ID3D12GraphicsCommandListCloseWriter(close));
}

void StateTrackingService::restoreD3D12Heap(ObjectState* state) {
  recorder_.record(createCommandWriter(state->creationCommand.get()));
  HeapState* heapState = static_cast<HeapState*>(state);
  restoreResidencyPriority(heapState->deviceKey, state->key, state->residencyPriority);
}

void StateTrackingService::restoreD3D12HeapFromAddress(ObjectState* state) {
  auto* createHeap = static_cast<CreateHeapAllocationMetaCommand*>(state->creationCommand.get());
  auto* heapFromAddressState = static_cast<D3D12HeapFromAddressState*>(state);
  auto* openHeap = static_cast<ID3D12Device3OpenExistingHeapFromAddressCommand*>(
      heapFromAddressState->openExistingHeapFromAddressCommand.get());

  openHeap->pAddress_.value = createHeap->address_.value;

  recorder_.record(createCommandWriter(createHeap));
  recorder_.record(createCommandWriter(openHeap));
}

void StateTrackingService::restoreD3D12CommittedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);
  D3D12_RESOURCE_STATES initialState = resourceState->initialState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*resourceState, resourceState->dimension);
    if (state->refCount > 0) {
      resourceContentRestore_.addCommittedResourceState(resourceState);
    }
  }

  switch (state->creationCommand->getId()) {
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateCommittedResourceCommand*>(state->creationCommand.get());
    command->InitialResourceState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateCommittedResource1Command*>(state->creationCommand.get());
    command->InitialResourceState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device8CreateCommittedResource2Command*>(state->creationCommand.get());
    command->InitialResourceState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3: {
    D3D12_BARRIER_LAYOUT initialLayout =
        getResourceInitialLayout(*resourceState, resourceState->dimension);
    auto* command =
        static_cast<ID3D12Device10CreateCommittedResource3Command*>(state->creationCommand.get());
    command->InitialLayout_.value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(state->creationCommand.get());
    command->InitialResourceState_.value = initialState;
  } break;
  }

  recorder_.record(createCommandWriter(state->creationCommand.get()));

  restoreResidencyPriority(resourceState->deviceKey, state->key, state->residencyPriority);
  restoreGpuVirtualAddress(resourceState);
}

void StateTrackingService::restoreD3D12PlacedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);
  D3D12_RESOURCE_STATES initialState = resourceState->initialState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*resourceState, resourceState->dimension);
    if (state->refCount > 0) {
      resourceContentRestore_.addPlacedResourceState(resourceState);
    }
  }

  switch (state->creationCommand->getId()) {
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreatePlacedResourceCommand*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device8CreatePlacedResource1Command*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2: {
    D3D12_BARRIER_LAYOUT initialLayout =
        getResourceInitialLayout(*resourceState, resourceState->dimension);
    auto* command =
        static_cast<ID3D12Device10CreatePlacedResource2Command*>(state->creationCommand.get());
    command->InitialLayout_.value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  }

  recorder_.record(createCommandWriter(state->creationCommand.get()));

  restoreGpuVirtualAddress(resourceState);
}

void StateTrackingService::restoreD3D12ReservedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);
  D3D12_RESOURCE_STATES initialState = resourceState->initialState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    initialState = getResourceInitialState(*resourceState, resourceState->dimension);
  }

  switch (state->creationCommand->getId()) {
  case CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateReservedResourceCommand*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateReservedResource1Command*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2: {
    D3D12_BARRIER_LAYOUT initialLayout =
        getResourceInitialLayout(*resourceState, resourceState->dimension);
    auto* command =
        static_cast<ID3D12Device10CreateReservedResource2Command*>(state->creationCommand.get());
    command->InitialLayout_.value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateReservedResourceCommand*>(state->creationCommand.get());
    command->InitialState_.value = initialState;
  } break;
  }

  recorder_.record(createCommandWriter(state->creationCommand.get()));

  restoreGpuVirtualAddress(resourceState);
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

void StateTrackingService::restoreD3D12INTCDeviceExtensionContext(ObjectState* state) {
  recorder_.record(createCommandWriter(state->creationCommand.get()));
  if (intcFeature_.EmulatedTyped64bitAtomics) {
    INTC_D3D12_SetFeatureSupportCommand c;
    c.key = getUniqueCommandKey();
    c.pExtensionContext_.key = state->key;
    c.pFeature_.value = &intcFeature_;
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(c));
  }
}

void StateTrackingService::restoreD3D12StateObject(ObjectState* state) {
  nvapiGlobalStateService_.restoreShaderExtnSlotSpaceBeforeCommand(state->creationCommand->key);
  recorder_.record(createCommandWriter(state->creationCommand.get()));
  for (unsigned key : state->childrenKeys) {
    auto it = statesByKey_.find(key);
    GITS_ASSERT(it != statesByKey_.end());
    restoreState(it->second);
  }
}

void StateTrackingService::storeINTCFeature(INTC_D3D12_FEATURE feature) {
  intcFeature_ = feature;
}

void StateTrackingService::storeINTCApplicationInfo(INTC_D3D12_SetApplicationInfoCommand& c) {
  setApplicationInfoCommand_.reset(new INTC_D3D12_SetApplicationInfoCommand(c));
}

void StateTrackingService::restoreINTCApplicationInfo() {
  if (setApplicationInfoCommand_) {
    recorder_.record(createCommandWriter(setApplicationInfoCommand_.get()));
  }
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

void StateTrackingService::NvAPIGlobalStateService::incrementInitialize() {
  ++nvapiInitializeCount_;
}

void StateTrackingService::NvAPIGlobalStateService::decrementInitialize() {
  if (nvapiInitializeCount_ > 0) {
    --nvapiInitializeCount_;
  }
}

void StateTrackingService::NvAPIGlobalStateService::addSetNvShaderExtnSlotSpaceCommand(
    const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  setNvShaderExtnSlotSpaceCommands_.push(OrderedCommand{
      command.key, std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand>(command)});
}

void StateTrackingService::NvAPIGlobalStateService::addSetNvShaderExtnSlotSpaceLocalThreadCommand(
    const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  setNvShaderExtnSlotSpaceCommands_.push(OrderedCommand{
      command.key,
      std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand>(command)});
}

void StateTrackingService::NvAPIGlobalStateService::restureInitializeCount() {
  for (unsigned i = 0; i < nvapiInitializeCount_; ++i) {
    NvAPI_InitializeCommand initializeCommand;
    initializeCommand.key = stateService_.getUniqueCommandKey();
    stateService_.recorder_.record(new NvAPI_InitializeWriter(initializeCommand));
  }
}

void StateTrackingService::NvAPIGlobalStateService::restoreShaderExtnSlotSpaceBeforeCommand(
    unsigned commandKey) {
  while (!setNvShaderExtnSlotSpaceCommands_.empty()) {
    const auto& command = setNvShaderExtnSlotSpaceCommands_.top();
    if (command.key >= commandKey) {
      break;
    }
    stateService_.recorder_.record(createCommandWriter(command.command.get()));
    setNvShaderExtnSlotSpaceCommands_.pop();
  }
}

} // namespace DirectX
} // namespace gits
