// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "commandSerializersFactory.h"
#include "log.h"
#include "configurationLib.h"

#include <array>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

StateTrackingService::~StateTrackingService() {
  for (auto& it : m_StatesByKey) {
    delete it.second;
  }
}

void StateTrackingService::RestoreState() {
  auto recordStatus = [this](MarkerUInt64Command::Value state) {
    m_Recorder.Record(MarkerUInt64Serializer(MarkerUInt64Command(state)));
  };

  m_Recorder.Record(StateRestoreBeginSerializer(StateRestoreBeginCommand()));
  RestoreDllContainers();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_BEGIN);
  for (auto& it : m_StatesByKey) {
    if (m_AnalyzerResults.RestoreObject(it.first)) {
      RestoreState(it.second);
    }
  }
  RestoreStateObjectProperties();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_END);
  m_NvapiGlobalStateService.RestoreInitializeCount();
  m_XessStateService.RestoreState();
  m_XellStateService.RestoreState();
  m_XefgStateService.RestoreState();
  m_DescriptorService.RestoreState();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RTAS_BEGIN);
  m_AccelerationStructuresSerializeService.RestoreAccelerationStructures();
  m_AccelerationStructuresBuildService.RestoreAccelerationStructures();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RTAS_END);
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_BEGIN);
  RestoreResources();
  recordStatus(MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_END);
  m_MapStateService.RestoreMapState();
  m_ResidencyService.RestoreResidency();
  m_CommandListService.RestoreCommandLists();
  m_CommandQueueService.RestoreCommandQueues();
  RestoreReferenceCount();
  m_NvapiGlobalStateService.FinalizeRestore();
  m_SwapChainService.RestoreBackBufferSequence(m_Recorder.CommandListSubcapture());
  m_Recorder.Record(StateRestoreEndSerializer(StateRestoreEndCommand()));
  if (!m_Recorder.CommandListSubcapture()) {
    // one Present after ID_INIT_END to enable PIX first frame capture in gits interactive mode
    m_SwapChainService.RecordSwapChainPresent();
  }
}

void StateTrackingService::KeepState(unsigned objectKey) {
  auto it = m_StatesByKey.find(objectKey);
  GITS_ASSERT(it != m_StatesByKey.end());
  ObjectState* state = it->second;
  state->KeepDestroyed = true;
  if (state->CreationCommand.get() &&
      (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE ||
       state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1 ||
       state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2)) {
    ResourceState* resourceState = static_cast<ResourceState*>(state);
    KeepState(resourceState->HeapKey);
  }
  if (state->ParentKey) {
    KeepState(state->ParentKey);
  }
}

void StateTrackingService::RestoreState(unsigned key) {
  auto it = m_StatesByKey.find(key);
  GITS_ASSERT(it != m_StatesByKey.end());
  RestoreState(it->second);
}

bool StateTrackingService::StateRestored(unsigned key) {
  auto it = m_StatesByKey.find(key);
  if (it != m_StatesByKey.end()) {
    return it->second->Restored;
  } else {
    return m_ResourceForCBVRestoreService.ResourceRestored(key);
  }
}

void StateTrackingService::AddBackBuffer(unsigned buffer,
                                         unsigned ResourceKey,
                                         ID3D12Resource* resource) {
  m_SwapChainService.AddBackBuffer(buffer, ResourceKey, resource);
}

void StateTrackingService::SetXefgSwapChainFlag() {
  m_IsXefgSwapChain = true;
}

void StateTrackingService::RestoreState(ObjectState* state) {
  if (state->Restored) {
    return;
  }
  state->Restored = true;
  if (state->ParentKey) {
    ObjectState* parentState = GetState(state->ParentKey);
    GITS_ASSERT(parentState);
    RestoreState(parentState);
  }
  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN:
  case CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND:
  case CommandId::ID_XEFGSWAPCHAIND3D12GETSWAPCHAINPTR:
    RestoreDXGISwapChain(state);
    break;
  case CommandId::ID_IDXGIFACTORY_ENUMADAPTERS:
  case CommandId::ID_IDXGIFACTORY1_ENUMADAPTERS1:
  case CommandId::ID_IDXGIFACTORY6_ENUMADAPTERBYGPUPREFERENCE:
  case CommandId::ID_IDXGIFACTORY4_ENUMADAPTERBYLUID:
    RestoreDXGIAdapter(state);
    break;
  case CommandId::ID_D3D12CREATEDEVICE:
  case CommandId::ID_ID3D12DEVICEFACTORY_CREATEDEVICE:
    RestoreD3D12Device(state);
    break;
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    RestoreQueryInterface(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEFENCE:
    RestoreD3D12Fence(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEDESCRIPTORHEAP:
    RestoreD3D12DescriptorHeap(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST:
    RestoreD3D12CommandList(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEHEAP:
  case CommandId::ID_ID3D12DEVICE4_CREATEHEAP1:
  case CommandId::INTC_D3D12_CREATEHEAP:
    RestoreD3D12Heap(state);
    break;
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    RestoreD3D12HeapFromAddress(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2:
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3:
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    RestoreD3D12CommittedResource(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    RestoreD3D12PlacedResource(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE:
    RestoreD3D12ReservedResource(state);
    break;
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    RestoreD3D12INTCDeviceExtensionContext(state);
    break;
  case CommandId::ID_ID3D12DEVICE5_CREATESTATEOBJECT:
  case CommandId::ID_ID3D12DEVICE7_ADDTOSTATEOBJECT:
    RestoreD3D12StateObject(state);
    break;
  case CommandId::ID_ID3D12DEVICE_CREATEGRAPHICSPIPELINESTATE:
  case CommandId::ID_ID3D12DEVICE_CREATECOMPUTEPIPELINESTATE:
  case CommandId::ID_ID3D12DEVICE2_CREATEPIPELINESTATE:
    RestoreD3D12PipelineStateObject(state);
    break;
  default:
    m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
  }

  if (!state->Name.empty()) {
    ID3D12ObjectSetNameCommand c;
    c.Key = GetUniqueCommandKey();
    c.m_Object.Key = state->Key;
    c.m_Name.Value = state->Name.data();
    m_Recorder.Record(ID3D12ObjectSetNameSerializer(c));
  }
}

void StateTrackingService::StoreState(ObjectState* state) {
  auto it = m_StatesByKey.find(state->Key);
  if (it == m_StatesByKey.end()) {
    m_StatesByKey[state->Key] = state;
  } else {
    // if the same objects in capture are not the same in replay
    it->second->Object = state->Object;
    delete state;
    state = it->second;
  }
  if (!state->RefCount) {
    state->RefCount = 1;
  }
}

void StateTrackingService::RemoveState(unsigned key) {
  auto it = m_StatesByKey.find(key);
  if (it != m_StatesByKey.end() && !m_AnalyzerResults.RestoreObject(key)) {
    delete it->second;
    m_StatesByKey.erase(it);
  }
}

void StateTrackingService::ReleaseObject(unsigned key, ULONG result) {
  auto itState = m_StatesByKey.find(key);
  if (itState == m_StatesByKey.end()) {
    return;
  }

  itState->second->RefCount = result;

  if (result != 0) {
    return;
  }

  itState->second->Destroyed = true;

  if (itState->second->CreationCommand->GetId() ==
      CommandId::ID_ID3D12DEVICE1_CREATEPIPELINELIBRARY) {
    return;
  }

  unsigned linkedLifetimeKey = itState->second->LinkedLifetimeKey;
  if (linkedLifetimeKey) {
    auto itLinkedLifetimeState = m_StatesByKey.find(linkedLifetimeKey);
    if (itLinkedLifetimeState != m_StatesByKey.end()) {
      itLinkedLifetimeState->second->Object->AddRef();
      ULONG refCount = itLinkedLifetimeState->second->Object->Release();
      if (refCount == 1 && !itLinkedLifetimeState->second->Destroyed) {
        ReleaseObject(itLinkedLifetimeState->first, 0);
      }
    }
  }

  for (unsigned childKey : itState->second->ChildrenKeys) {
    if (childKey) {
      auto itChildState = m_StatesByKey.find(childKey);
      if (itChildState != m_StatesByKey.end() && !itChildState->second->Destroyed) {
        ReleaseObject(itChildState->first, 0);
      }
    }
  }

  if (!itState->second->KeepDestroyed && !m_AnalyzerResults.RestoreObject(key)) {
    delete itState->second;
    m_StatesByKey.erase(itState);
  }
}

void StateTrackingService::SetReferenceCount(unsigned objectKey, ULONG referenceCount) {
  auto itState = m_StatesByKey.find(objectKey);
  if (itState == m_StatesByKey.end()) {
    return;
  }
  itState->second->RefCount = referenceCount;
}

ObjectState* StateTrackingService::GetState(unsigned key) {
  auto it = m_StatesByKey.find(key);
  if (it == m_StatesByKey.end()) {
    return nullptr;
  }
  return it->second;
}

void StateTrackingService::RestoreReferenceCount() {
  for (auto& it : m_StatesByKey) {
    ObjectState* state = it.second;
    if (!state->Object || !state->Restored) {
      continue;
    }
    if (it.second->Destroyed) {
      IUnknownReleaseCommand c;
      c.Key = GetUniqueCommandKey();
      c.m_Object.Key = it.second->Key;
      m_Recorder.Record(IUnknownReleaseSerializer(c));
      continue;
    }
    if (state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND &&
        m_IsXefgSwapChain) {
      continue;
    }
    if (state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND &&
        m_IsXefgSwapChain) {
      continue;
    }

    int refCount = 0;
    if (state->RefCount <= 0 ||
        state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY_ENUMADAPTERS ||
        state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY1_ENUMADAPTERS1 ||
        state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY6_ENUMADAPTERBYGPUPREFERENCE ||
        state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY4_ENUMADAPTERBYLUID ||
        state->CreationCommand->GetId() == CommandId::ID_CREATEDXGIFACTORY ||
        state->CreationCommand->GetId() == CommandId::ID_CREATEDXGIFACTORY1 ||
        state->CreationCommand->GetId() == CommandId::ID_CREATEDXGIFACTORY2 ||
        state->CreationCommand->GetId() == CommandId::ID_IDXGIADAPTER_ENUMOUTPUTS ||
        state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE_CREATEROOTSIGNATURE ||
        state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE1_CREATEPIPELINELIBRARY ||
        state->CreationCommand->GetId() ==
            CommandId::ID_ID3D12PIPELINELIBRARY_LOADGRAPHICSPIPELINE ||
        state->CreationCommand->GetId() ==
            CommandId::ID_ID3D12PIPELINELIBRARY_LOADCOMPUTEPIPELINE ||
        state->CreationCommand->GetId() == CommandId::ID_ID3D12PIPELINELIBRARY1_LOADPIPELINE) {
      refCount = state->RefCount;
    } else if (state->CreationCommand->GetId() == CommandId::ID_IDXGISWAPCHAIN_GETBUFFER) {
      refCount = std::max(
          state->RefCount - static_cast<int>(m_SwapChainService.GetBackBuffersCount()) + 1, 1);
    } else {
      state->Object->AddRef();
      refCount = state->Object->Release();
    }

    for (int i = 1; i < refCount; ++i) {
      IUnknownAddRefCommand c;
      c.Key = GetUniqueCommandKey();
      c.m_Object.Key = state->Key;
      m_Recorder.Record(IUnknownAddRefSerializer(c));
    }
  }
}

void StateTrackingService::RestoreResources() {
  std::vector<unsigned> orderedResources = m_ResourceUsageTrackingService.GetOrderedResources();

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
      ResourceState* state = static_cast<ResourceState*>(GetState(resourceKey));
      if (!state) {
        continue;
      }

      ResourceBatchType type{};
      CommandId commandId = state->CreationCommand->GetId();
      bool isReservedResource = commandId == CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE ||
                                commandId == CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1 ||
                                commandId == CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2 ||
                                commandId == CommandId::INTC_D3D12_CREATERESERVEDRESOURCE;

      if (isReservedResource) {
        type = ReservedResourceTexture;
        if (state->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
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
    m_ReservedResourcesService.RestoreContent(reservedResourceBuffers);

    // restore batches in the order of usage
    for (const auto& [type, resourceKeys] : batches) {
      if (type == CommittedOrPlacedResource) {
        m_ResourceContentRestore.RestoreContent(resourceKeys);
      } else if (type == ReservedResourceTexture) {
        m_ReservedResourcesService.RestoreContent(resourceKeys);
      }
    }
  }

  m_ReservedResourcesService.CleanupRestore();
  m_ResourceContentRestore.CleanupRestoreUnmappableResources();

  // restore states
  m_ResourceStateTrackingService.RestoreResourceStates(orderedResources);
}

D3D12_RESOURCE_STATES StateTrackingService::GetResourceInitialState(
    ResourceState& state, D3D12_RESOURCE_DIMENSION dimension) {
  if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return D3D12_RESOURCE_STATE_COMMON;
  }
  if (state.IsMappable || state.BarrierRestricted) {
    return m_ResourceStateTrackingService.GetResourceState(state.Key);
  }
  return D3D12_RESOURCE_STATE_COPY_DEST;
}

D3D12_BARRIER_LAYOUT StateTrackingService::GetResourceInitialLayout(
    ResourceState& state, D3D12_RESOURCE_DIMENSION dimension, D3D12_RESOURCE_FLAGS flags) {
  if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return D3D12_BARRIER_LAYOUT_UNDEFINED;
  } else if (flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) {
    return D3D12_BARRIER_LAYOUT_COMMON;
  }
  if (state.IsMappable) {
    return m_ResourceStateTrackingService.GetResourceLayout(state.Key);
  }
  return D3D12_BARRIER_LAYOUT_COPY_DEST;
}

void StateTrackingService::RestoreResidencyPriority(unsigned DeviceKey,
                                                    unsigned objectKey,
                                                    D3D12_RESIDENCY_PRIORITY residencyPriority) {
  if (!residencyPriority) {
    return;
  }
  ID3D12Device1SetResidencyPriorityCommand c;
  c.Key = GetUniqueCommandKey();
  c.m_Object.Key = DeviceKey;
  c.m_NumObjects.Value = 1;
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  c.m_ppObjects.Value = &fakePtr;
  c.m_ppObjects.Size = 1;
  c.m_ppObjects.Keys.push_back(objectKey);
  c.m_pPriorities.Size = 1;
  c.m_pPriorities.Value = &residencyPriority;
  m_Recorder.Record(ID3D12Device1SetResidencyPrioritySerializer(c));
}

void StateTrackingService::RestoreDXGISwapChain(ObjectState* state) {
  if (state->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    auto* command = static_cast<IDXGIFactoryCreateSwapChainCommand*>(state->CreationCommand.get());
    RestoreState(command->m_pDevice.Key);
    int width = command->m_pDesc.Value->BufferDesc.Width;
    int height = command->m_pDesc.Value->BufferDesc.Height;
    if (!width || !height) {
      DXGI_SWAP_CHAIN_DESC desc{};
      (*command->m_ppSwapChain.Value)->GetDesc(&desc);
      width = desc.BufferDesc.Width;
      height = desc.BufferDesc.Height;
    }
    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.Key = GetUniqueCommandKey();
    createWindowCommand.m_hWnd.Value = command->m_pDesc.Value->OutputWindow;
    createWindowCommand.m_width.Value = width;
    createWindowCommand.m_height.Value = height;
    m_Recorder.Record(CreateWindowMetaSerializer(createWindowCommand));

    m_SwapChainService.SetSwapChain(
        command->m_pDevice.Key, reinterpret_cast<ID3D12CommandQueue*>(command->m_pDevice.Value),
        state->Key, *command->m_ppSwapChain.Value, command->m_pDesc.Value->BufferCount);
  } else if (state->CreationCommand->GetId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    auto* command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(state->CreationCommand.get());
    RestoreState(command->m_pDevice.Key);
    int width = command->m_pDesc.Value->Width;
    int height = command->m_pDesc.Value->Height;
    if (!width || !height) {
      DXGI_SWAP_CHAIN_DESC1 desc{};
      (*command->m_ppSwapChain.Value)->GetDesc1(&desc);
      width = desc.Width;
      height = desc.Height;
    }
    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.Key = GetUniqueCommandKey();
    createWindowCommand.m_hWnd.Value = command->m_hWnd.Value;
    createWindowCommand.m_width.Value = width;
    createWindowCommand.m_height.Value = height;
    m_Recorder.Record(CreateWindowMetaSerializer(createWindowCommand));

    if (!m_IsXefgSwapChain) {
      m_SwapChainService.SetSwapChain(
          command->m_pDevice.Key, reinterpret_cast<ID3D12CommandQueue*>(command->m_pDevice.Value),
          state->Key, *command->m_ppSwapChain.Value, command->m_pDesc.Value->BufferCount);
    }
  } else if (state->CreationCommand->GetId() == CommandId::ID_XEFGSWAPCHAIND3D12GETSWAPCHAINPTR) {
    auto* command =
        static_cast<xefgSwapChainD3D12GetSwapChainPtrCommand*>(state->CreationCommand.get());
    auto contextKey = command->m_hSwapChain.Key;
    m_XellStateService.RestoreState();
    m_XefgStateService.RestoreState();
    auto xefgContextState = m_XefgStateService.GetContextState(contextKey);
    ID3D12CommandQueue* cmdQueue{};
    if (xefgContextState->InitFromSwapChainParams.has_value()) {
      auto& initParams = xefgContextState->InitFromSwapChainParams.value();
      cmdQueue = initParams.CmdQueue;
    } else if (xefgContextState->InitFromSwapChainDescParams.has_value()) {
      auto& initParams = xefgContextState->InitFromSwapChainDescParams.value();
      cmdQueue = initParams.CmdQueue;
    }
    auto& swapChainInfo = xefgContextState->SwapChain.value();
    DXGI_SWAP_CHAIN_DESC desc{};
    swapChainInfo.SwapChain->GetDesc(&desc);
    unsigned bufferCount = desc.BufferCount;
    m_SwapChainService.SetSwapChain(xefgContextState->DeviceKey, cmdQueue,
                                    swapChainInfo.SwapChainKey, swapChainInfo.SwapChain,
                                    bufferCount);
    return;
  }
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
}

void StateTrackingService::RestoreDXGIAdapter(ObjectState* state) {
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
}

void StateTrackingService::RestoreD3D12DescriptorHeap(ObjectState* state) {
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));

  D3D12DescriptorHeapState* descriptorHeapState = static_cast<D3D12DescriptorHeapState*>(state);
  if (descriptorHeapState->GpuDescriptorHandle.ptr) {
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand getCommand;
    getCommand.Key = GetUniqueCommandKey();
    getCommand.m_Object.Key = state->Key;
    getCommand.m_Result.Value = descriptorHeapState->GpuDescriptorHandle;
    m_Recorder.Record(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartSerializer(getCommand));
  }
}

void StateTrackingService::RestoreD3D12Device(ObjectState* state) {
  RestoreINTCApplicationInfo();
  RestoreD3D12EnableExperimentalFeatures();
  m_DeviceKey = state->Key;
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
}

void StateTrackingService::RestoreQueryInterface(ObjectState* state) {
  auto* command = static_cast<IUnknownQueryInterfaceCommand*>(state->CreationCommand.get());
  if (command->m_riid.Value == IID_ID3D12StateObjectProperties) {
    ObjectState* stateObjectState = GetState(state->ParentKey);
    if (!stateObjectState || !stateObjectState->Restored) {
      return;
    }

    m_StateObjectPropertiesCommands.emplace(
        std::make_unique<IUnknownQueryInterfaceCommand>(*command));

    auto* propertiesState = static_cast<D3D12StateObjectPropertiesState*>(state);
    for (auto& shaderIdentifier : propertiesState->ShaderIdentifiers) {
      ID3D12StateObjectPropertiesGetShaderIdentifierCommand c;
      c.Key = GetUniqueCommandKey();
      c.m_Object.Key = state->Key;
      c.m_pExportName.Value = const_cast<wchar_t*>(shaderIdentifier.first.c_str());
      c.m_Result.Value = shaderIdentifier.second.data();
      m_StateObjectPropertiesCommands.emplace(
          std::make_unique<ID3D12StateObjectPropertiesGetShaderIdentifierCommand>(c));
    }
  } else if (command->m_riid.Value == __uuidof(IDStorageCustomDecompressionQueue) ||
             command->m_riid.Value == __uuidof(IDStorageCustomDecompressionQueue1)) {
    m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
  }
}

void StateTrackingService::RestoreD3D12Fence(ObjectState* state) {
  ID3D12DeviceCreateFenceCommand* command =
      static_cast<ID3D12DeviceCreateFenceCommand*>(state->CreationCommand.get());
  command->m_InitialValue.Value = m_FenceTrackingService.GetFenceValue(state->Key);
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
}

void StateTrackingService::RestoreD3D12CommandList(ObjectState* state) {
  GITS_ASSERT(state);
  auto* command = static_cast<ID3D12DeviceCreateCommandListCommand*>(state->CreationCommand.get());
  unsigned allocatorKey = command->m_pCommandAllocator.Key;
  unsigned initialStateKey = command->m_pInitialState.Key;

  ObjectState* allocatorState = GetState(allocatorKey);
  if (!allocatorState) {
    return;
  }
  RestoreState(allocatorState);
  if (initialStateKey) {
    ObjectState* initialState = GetState(initialStateKey);
    RestoreState(initialState);
  }

  ID3D12CommandAllocatorResetCommand reset;
  reset.Key = GetUniqueCommandKey();
  reset.m_Object.Key = allocatorKey;
  m_Recorder.Record(ID3D12CommandAllocatorResetSerializer(reset));

  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));

  CommandListState* commandListState = static_cast<CommandListState*>(state);
  bool closed = commandListState->Closed;
  if (m_AnalyzerResults.RestoreCommandList(state->Key)) {
    if (!commandListState->Commands.empty()) {
      closed = true;
    }
  }
  if (closed) {
    ID3D12GraphicsCommandListCloseCommand close;
    close.Key = GetUniqueCommandKey();
    close.m_Object.Key = state->Key;
    m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(close));
  }
}

void StateTrackingService::RestoreD3D12Heap(ObjectState* state) {
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
  HeapState* heapState = static_cast<HeapState*>(state);
  RestoreResidencyPriority(heapState->DeviceKey, state->Key, state->ResidencyPriority);
}

void StateTrackingService::RestoreD3D12HeapFromAddress(ObjectState* state) {
  auto* createHeap = static_cast<CreateHeapAllocationMetaCommand*>(state->CreationCommand.get());
  auto* heapFromAddressState = static_cast<D3D12HeapFromAddressState*>(state);
  auto* openHeap = static_cast<ID3D12Device3OpenExistingHeapFromAddressCommand*>(
      heapFromAddressState->OpenExistingHeapFromAddressCommand.get());

  openHeap->m_pAddress.Value = createHeap->m_address.Value;

  m_Recorder.Record(*createCommandSerializer(createHeap));
  m_Recorder.Record(*createCommandSerializer(openHeap));
}

void StateTrackingService::RestoreD3D12CommittedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);
  D3D12_RESOURCE_STATES initialState = resourceState->InitialState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    if (state->RefCount > 0) {
      m_ResourceContentRestore.AddCommittedResourceState(resourceState);
    }
  }

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateCommittedResourceCommand*>(state->CreationCommand.get());
    command->m_InitialResourceState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateCommittedResource1Command*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialResourceState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device8CreateCommittedResource2Command*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialResourceState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3: {
    auto* command =
        static_cast<ID3D12Device10CreateCommittedResource3Command*>(state->CreationCommand.get());
    D3D12_BARRIER_LAYOUT initialLayout = GetResourceInitialLayout(
        *resourceState, resourceState->Dimension, command->m_pDesc.Value->Flags);
    command->m_InitialLayout.Value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialResourceState.Value = initialState;
  } break;
  }

  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));

  RestoreResidencyPriority(resourceState->DeviceKey, state->Key, state->ResidencyPriority);
  RestoreGpuVirtualAddress(resourceState);
}

void StateTrackingService::RestoreD3D12PlacedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);
  D3D12_RESOURCE_STATES initialState = resourceState->InitialState;
  if (initialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    if (state->RefCount > 0) {
      m_ResourceContentRestore.AddPlacedResourceState(resourceState);
    }
  }

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreatePlacedResourceCommand*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device8CreatePlacedResource1Command*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device10CreatePlacedResource2Command*>(state->CreationCommand.get());
    D3D12_BARRIER_LAYOUT initialLayout = GetResourceInitialLayout(
        *resourceState, resourceState->Dimension, command->m_pDesc.Value->Flags);
    command->m_InitialLayout.Value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(state->CreationCommand.get());
    initialState = GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  }

  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));

  RestoreGpuVirtualAddress(resourceState);
}

void StateTrackingService::RestoreD3D12ReservedResource(ObjectState* state) {
  ResourceState* resourceState = static_cast<ResourceState*>(state);

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATERESERVEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateReservedResourceCommand*>(state->CreationCommand.get());
    D3D12_RESOURCE_STATES initialState =
        GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATERESERVEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateReservedResource1Command*>(state->CreationCommand.get());
    D3D12_RESOURCE_STATES initialState =
        GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATERESERVEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device10CreateReservedResource2Command*>(state->CreationCommand.get());
    D3D12_BARRIER_LAYOUT initialLayout = GetResourceInitialLayout(
        *resourceState, resourceState->Dimension, command->m_pDesc.Value->Flags);
    command->m_InitialLayout.Value = initialLayout;
  } break;
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateReservedResourceCommand*>(state->CreationCommand.get());
    D3D12_RESOURCE_STATES initialState =
        GetResourceInitialState(*resourceState, resourceState->Dimension);
    command->m_InitialState.Value = initialState;
  } break;
  }

  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));

  RestoreGpuVirtualAddress(resourceState);
}

void StateTrackingService::RestoreGpuVirtualAddress(ResourceState* state) {
  if (state->GpuVirtualAddress) {
    ID3D12ResourceGetGPUVirtualAddressCommand c;
    c.Key = GetUniqueCommandKey();
    c.m_Object.Key = state->Key;
    c.m_Result.Value = state->GpuVirtualAddress;
    m_Recorder.Record(ID3D12ResourceGetGPUVirtualAddressSerializer(c));
  }
}

void StateTrackingService::RestoreD3D12INTCDeviceExtensionContext(ObjectState* state) {
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
  if (m_IntcFeature.EmulatedTyped64bitAtomics) {
    INTC_D3D12_SetFeatureSupportCommand c;
    c.Key = GetUniqueCommandKey();
    c.m_pExtensionContext.Key = state->Key;
    c.m_pFeature.Value = &m_IntcFeature;
    m_Recorder.Record(INTC_D3D12_SetFeatureSupportSerializer(c));
  }
}

void StateTrackingService::RestoreD3D12StateObject(ObjectState* state) {
  if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE5_CREATESTATEOBJECT) {
    auto* command =
        static_cast<ID3D12Device5CreateStateObjectCommand*>(state->CreationCommand.get());
    for (auto& it : command->m_pDesc.InterfaceKeysBySubobject) {
      auto itState = m_StatesByKey.find(it.second);
      GITS_ASSERT(itState != m_StatesByKey.end());
      RestoreState(itState->second);
    }
  } else if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE7_ADDTOSTATEOBJECT) {
    auto* command =
        static_cast<ID3D12Device7AddToStateObjectCommand*>(state->CreationCommand.get());
    for (auto& it : command->m_pAddition.InterfaceKeysBySubobject) {
      auto itState = m_StatesByKey.find(it.second);
      GITS_ASSERT(itState != m_StatesByKey.end());
      RestoreState(itState->second);
    }
  }
  m_NvapiGlobalStateService.RestoreCreatePipelineStateOptionsBeforeCommand(
      state->CreationCommand->Key);
  m_NvapiGlobalStateService.RestoreShaderExtnSlotSpaceBeforeCommand(state->CreationCommand->Key);
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
  for (unsigned key : state->ChildrenKeys) {
    auto it = m_StatesByKey.find(key);
    GITS_ASSERT(it != m_StatesByKey.end());
    RestoreState(it->second);
  }
}

void StateTrackingService::RestoreD3D12PipelineStateObject(ObjectState* state) {
  if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE_CREATEGRAPHICSPIPELINESTATE) {
    auto* command =
        static_cast<ID3D12DeviceCreateGraphicsPipelineStateCommand*>(state->CreationCommand.get());
    RestoreState(command->m_pDesc.RootSignatureKey);
  } else if (state->CreationCommand->GetId() ==
             CommandId::ID_ID3D12DEVICE_CREATECOMPUTEPIPELINESTATE) {
    auto* command =
        static_cast<ID3D12DeviceCreateComputePipelineStateCommand*>(state->CreationCommand.get());
    RestoreState(command->m_pDesc.RootSignatureKey);
  } else if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE2_CREATEPIPELINESTATE) {
    auto* command =
        static_cast<ID3D12Device2CreatePipelineStateCommand*>(state->CreationCommand.get());
    RestoreState(command->m_pDesc.RootSignatureKey);
  }
  m_Recorder.Record(*createCommandSerializer(state->CreationCommand.get()));
}

void StateTrackingService::StoreINTCFeature(INTC_D3D12_FEATURE feature) {
  m_IntcFeature = feature;
}

void StateTrackingService::StoreINTCApplicationInfo(INTC_D3D12_SetApplicationInfoCommand& c) {
  m_SetApplicationInfoCommand.reset(new INTC_D3D12_SetApplicationInfoCommand(c));
}

void StateTrackingService::StoreD3D12EnableExperimentalFeatures(
    const D3D12EnableExperimentalFeaturesCommand& c) {
  m_EnableExperimentalFeaturesCommand.reset(new D3D12EnableExperimentalFeaturesCommand(c));
}

void StateTrackingService::StoreDllContainer(const DllContainerMetaCommand& c) {
  m_DllContainerCommands.emplace_back(new DllContainerMetaCommand(c));
}

void StateTrackingService::RestoreINTCApplicationInfo() {
  if (m_SetApplicationInfoCommand) {
    m_Recorder.Record(*createCommandSerializer(m_SetApplicationInfoCommand.get()));
  }
}

void StateTrackingService::RestoreD3D12EnableExperimentalFeatures() {
  if (m_EnableExperimentalFeaturesCommand) {
    m_Recorder.Record(*createCommandSerializer(m_EnableExperimentalFeaturesCommand.get()));
    m_EnableExperimentalFeaturesCommand.reset();
  }
}

void StateTrackingService::RestoreDllContainers() {
  for (const auto& command : m_DllContainerCommands) {
    m_Recorder.Record(*createCommandSerializer(command.get()));
  }
}

void StateTrackingService::RestoreStateObjectProperties() {
  while (!m_StateObjectPropertiesCommands.empty()) {
    const auto& command = m_StateObjectPropertiesCommands.front();
    m_Recorder.Record(*createCommandSerializer(command.get()));
    m_StateObjectPropertiesCommands.pop();
  }
}

void StateTrackingService::SwapChainService::SetSwapChain(unsigned commandQueueKey,
                                                          ID3D12CommandQueue* commandQueue,
                                                          unsigned swapChainKey,
                                                          IDXGISwapChain* swapChain,
                                                          unsigned backBuffersCount) {
  m_CommandQueueKey = commandQueueKey;
  m_CommandQueue = commandQueue;
  m_SwapChainKey = swapChainKey;
  m_SwapChain = swapChain;
  m_BackBuffersCount = backBuffersCount;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
  HRESULT hr = m_SwapChain->QueryInterface(IID_PPV_ARGS(&swapChain3));
  GITS_ASSERT(hr == S_OK);
  m_BackBufferShift = swapChain3->GetCurrentBackBufferIndex();
}

void StateTrackingService::SwapChainService::RestoreBackBufferSequence(bool CommandListSubcapture) {

  int backBufferShift = m_BackBufferShift;
  if (!CommandListSubcapture) {
    // taking into account one Present that will be always recorded later
    backBufferShift -= 1;
  }
  if (backBufferShift < 0) {
    backBufferShift += m_BackBuffersCount;
  }

  if (CommandListSubcapture) {
    auto& resource = m_BackBuffers[backBufferShift];
    if (!m_StateService.m_AnalyzerResults.RestoreObject(resource.first)) {
      return;
    }
  }

  for (int i = 0; i < backBufferShift; ++i) {
    RecordSwapChainPresent();
  }

  if (CommandListSubcapture) {
    auto& resource = m_BackBuffers[backBufferShift];
    m_StateService.m_ResourceContentRestore.RestoreBackBuffer(m_CommandQueue, m_CommandQueueKey,
                                                              resource.first, resource.second);
    m_StateService.m_ResourceStateTrackingService.RestoreBackBufferState(
        m_CommandQueueKey, resource.first, D3D12_RESOURCE_STATE_COPY_DEST);
  }
}

void StateTrackingService::SwapChainService::RecordSwapChainPresent() {
  IDXGISwapChainPresentCommand presentCommand;
  presentCommand.Key = m_StateService.GetUniqueCommandKey();
  presentCommand.m_Object.Key = m_SwapChainKey;
  presentCommand.m_SyncInterval.Value = 0;
  presentCommand.m_Flags.Value = 0;
  m_StateService.m_Recorder.Record(IDXGISwapChainPresentSerializer(presentCommand));
}

void StateTrackingService::SwapChainService::AddBackBuffer(unsigned buffer,
                                                           unsigned ResourceKey,
                                                           ID3D12Resource* resource) {
  m_BackBuffers[buffer] = {ResourceKey, resource};
}

void StateTrackingService::NvAPIGlobalStateService::IncrementInitialize() {
  ++m_NvapiInitializeCount;
}

void StateTrackingService::NvAPIGlobalStateService::DecrementInitialize() {
  if (m_NvapiInitializeCount > 0) {
    --m_NvapiInitializeCount;
  }
}

void StateTrackingService::NvAPIGlobalStateService::AddSetCreatePipelineStateOptionsCommand(
    const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  m_SetCreatePipelineStateOptionsCommands.push(OrderedCommand{
      command.Key, std::make_unique<NvAPI_D3D12_SetCreatePipelineStateOptionsCommand>(command)});
}

void StateTrackingService::NvAPIGlobalStateService::AddSetNvShaderExtnSlotSpaceCommand(
    const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  m_SetNvShaderExtnSlotSpaceCommands.push(OrderedCommand{
      command.Key, std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand>(command)});
}

void StateTrackingService::NvAPIGlobalStateService::AddSetNvShaderExtnSlotSpaceLocalThreadCommand(
    const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  m_SetNvShaderExtnSlotSpaceCommands.push(OrderedCommand{
      command.Key,
      std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand>(command)});
}

void StateTrackingService::NvAPIGlobalStateService::RestoreInitializeCount() {
  for (unsigned i = 0; i < m_NvapiInitializeCount; ++i) {
    NvAPI_InitializeCommand initializeCommand;
    initializeCommand.Key = m_StateService.GetUniqueCommandKey();
    m_StateService.m_Recorder.Record(NvAPI_InitializeSerializer(initializeCommand));
  }
}

void StateTrackingService::NvAPIGlobalStateService::RestoreCreatePipelineStateOptionsBeforeCommand(
    unsigned commandKey) {
  while (!m_SetCreatePipelineStateOptionsCommands.empty()) {
    const auto& command = m_SetCreatePipelineStateOptionsCommands.top();
    if (command.Key >= commandKey) {
      break;
    }
    m_StateService.m_Recorder.Record(*createCommandSerializer(command.SerializedCommand.get()));
    m_SetCreatePipelineStateOptionsCommands.pop();
  }
}

void StateTrackingService::NvAPIGlobalStateService::RestoreShaderExtnSlotSpaceBeforeCommand(
    unsigned commandKey) {
  while (!m_SetNvShaderExtnSlotSpaceCommands.empty()) {
    const auto& command = m_SetNvShaderExtnSlotSpaceCommands.top();
    if (command.Key >= commandKey) {
      break;
    }
    m_StateService.m_Recorder.Record(*createCommandSerializer(command.SerializedCommand.get()));
    m_SetNvShaderExtnSlotSpaceCommands.pop();
  }
}

void StateTrackingService::NvAPIGlobalStateService::FinalizeRestore() {
  while (!m_SetNvShaderExtnSlotSpaceCommands.empty()) {
    const auto& command = m_SetNvShaderExtnSlotSpaceCommands.top();
    m_StateService.m_Recorder.Record(*createCommandSerializer(command.SerializedCommand.get()));
    m_SetNvShaderExtnSlotSpaceCommands.pop();
  }

  while (!m_SetCreatePipelineStateOptionsCommands.empty()) {
    const auto& command = m_SetCreatePipelineStateOptionsCommands.top();
    m_StateService.m_Recorder.Record(*createCommandSerializer(command.SerializedCommand.get()));
    m_SetCreatePipelineStateOptionsCommands.pop();
  }
}

} // namespace DirectX
} // namespace gits
