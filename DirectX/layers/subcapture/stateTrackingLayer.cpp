// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingLayer.h"
#include "descriptorService.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
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
      m_StateService(recorder,
                     m_FenceTrackingService,
                     m_MapStateService,
                     m_ResourceStateTrackingService,
                     m_ReservedResourcesService,
                     m_DescriptorService,
                     m_CommandListService,
                     m_CommandQueueService,
                     m_XessStateService,
                     m_AccelerationStructuresSerializeService,
                     m_AccelerationStructuresBuildService,
                     m_ResidencyService,
                     m_AnalyzerResults,
                     m_ResourceUsageTrackingService,
                     m_ResourceForCBVRestoreService,
                     m_XellStateService,
                     m_XefgStateService),
      m_Recorder(recorder),
      m_SubcaptureRange(subcaptureRange),
      m_MapStateService(m_StateService),
      m_ResourceStateTrackingService(m_StateService),
      m_ReservedResourcesService(m_StateService),
      m_ResourceForCBVRestoreService(m_StateService),
      m_DescriptorService(&m_StateService, &m_ResourceForCBVRestoreService),
      m_CommandListService(m_StateService),
      m_CommandQueueService(m_StateService),
      m_XessStateService(m_StateService, m_Recorder),
      m_XellStateService(m_StateService, m_Recorder),
      m_XefgStateService(m_StateService, m_Recorder),
      m_AccelerationStructuresSerializeService(m_StateService, m_Recorder),
      m_AccelerationStructuresBuildService(m_StateService,
                                           m_Recorder,
                                           m_ReservedResourcesService,
                                           m_ResourceStateTracker,
                                           m_GpuAddressService),
      m_ResidencyService(m_StateService) {}

void StateTrackingLayer::SetAsChildInParent(unsigned parentKey, unsigned childKey) {
  ObjectState* parentState = m_StateService.GetState(parentKey);
  if (!parentState) {
    return;
  }
  parentState->ChildrenKeys.insert(childKey);
}

bool StateTrackingLayer::IsResourceHeapMappable(unsigned heapKey,
                                                const D3D12_TEXTURE_LAYOUT& textureLayout) {
  ObjectState* state = m_StateService.GetState(heapKey);
  if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE_CREATEHEAP) {
    auto* Command = static_cast<ID3D12DeviceCreateHeapCommand*>(state->CreationCommand.get());
    return IsResourceHeapMappable(Command->m_pDesc.Value->Properties, textureLayout);
  } else if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE4_CREATEHEAP1) {
    auto* Command = static_cast<ID3D12Device4CreateHeap1Command*>(state->CreationCommand.get());
    return IsResourceHeapMappable(Command->m_pDesc.Value->Properties, textureLayout);
  } else if (state->CreationCommand->GetId() == CommandId::ID_CREATE_HEAP_ALLOCATION) {
    return true;
  } else if (state->CreationCommand->GetId() == CommandId::INTC_D3D12_CREATEHEAP) {
    auto* Command = static_cast<INTC_D3D12_CreateHeapCommand*>(state->CreationCommand.get());
    return IsResourceHeapMappable(Command->m_pDesc.Value->pD3D12Desc->Properties, textureLayout);
  } else {
    GITS_ASSERT(0 && "Unexpected state type");
  }
  return false;
}

bool StateTrackingLayer::IsResourceBarrierRestricted(D3D12_RESOURCE_FLAGS flags) {
  // ResourceBarrier on placed resource can corrupt underlying heap data if resource does not currently own that data
  // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createplacedresource
  constexpr D3D12_RESOURCE_FLAGS resourceTypesRestrictedFromBarrier =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  return (flags & resourceTypesRestrictedFromBarrier) != 0;
}

void StateTrackingLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Flags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  if (!m_SubcaptureRange.CommandListSubcapture() &&
      m_SubcaptureRange.IsFrameRangeStart(IsStateRestoreKey(c.Key))) {
    m_GpuExecutionFlusher.FlushCommandQueues();
    m_StateService.RestoreState();
    m_StateRestored = true;
  }
}

void StateTrackingLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_PresentFlags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  if (!m_SubcaptureRange.CommandListSubcapture() &&
      m_SubcaptureRange.IsFrameRangeStart(IsStateRestoreKey(c.Key))) {
    m_GpuExecutionFlusher.FlushCommandQueues();
    m_StateService.RestoreState();
    m_StateRestored = true;
  }
}

void StateTrackingLayer::Pre(IUnknownReleaseCommand& c) {
  if (m_StateRestored) {
    return;
  }
  ReleaseSwapChainBuffers(c.m_Object.Key, c.m_Result.Value);
  m_StateService.ReleaseObject(c.m_Object.Key, c.m_Result.Value);
  if (c.m_Result.Value == 0) {
    m_MapStateService.DestroyResource(c.m_Object.Key);
    m_ResourceStateTrackingService.DestroyResource(c.m_Object.Key);
    m_HeapAllocationStateService.DestroyHeap(c.m_Object.Key);
    m_ReservedResourcesService.DestroyObject(c.m_Object.Key);
    m_DescriptorService.RemoveState(c.m_Object.Key);
    m_CommandListService.RemoveCommandList(c.m_Object.Key);
    m_XessStateService.DestroyDevice(c.m_Object.Key);
    m_XefgStateService.DestroyDevice(c.m_Object.Key);
    m_XellStateService.DestroyDevice(c.m_Object.Key);
    m_AccelerationStructuresSerializeService.DestroyResource(c.m_Object.Key);
    m_ResidencyService.DestroyObject(c.m_Object.Key);
    m_ResourceUsageTrackingService.DestroyResource(c.m_Object.Key);
    m_GpuAddressService.DestroyInterface(c.m_Object.Key);

    auto it = m_ResourceHeaps.find(c.m_Object.Key);
    if (it != m_ResourceHeaps.end()) {
      for (unsigned ResourceKey : it->second) {
        m_StateService.ReleaseObject(ResourceKey, 0);
        m_MapStateService.DestroyResource(ResourceKey);
        m_ResourceStateTrackingService.DestroyResource(ResourceKey);
        m_DescriptorService.RemoveState(ResourceKey);
        m_AccelerationStructuresSerializeService.DestroyResource(ResourceKey);
        m_ResidencyService.DestroyObject(ResourceKey);
        m_ResourceUsageTrackingService.DestroyResource(ResourceKey);
      }
      m_ResourceHeaps.erase(it);
    }

    m_GpuExecutionFlusher.DestroyCommandQueue(c.m_Object.Key);

    unsigned commandQueueKey =
        m_CommandQueueSwapChainRefCountTracker.DestroySwapChain(c.m_Object.Key);
    if (commandQueueKey) {
      m_StateService.ReleaseObject(commandQueueKey, 0);
      m_GpuExecutionFlusher.DestroyCommandQueue(commandQueueKey);
    }
  }
}

void StateTrackingLayer::ReleaseSwapChainBuffers(unsigned key, unsigned referenceCount) {
  if (referenceCount > 0) {
    return;
  }

  ObjectState* state = m_StateService.GetState(key);
  if (!state || state->CreationCommand->GetId() != CommandId::ID_IDXGISWAPCHAIN_GETBUFFER) {
    return;
  }

  // SwapChain buffers share the same reference count
  // Remove all buffers from the same SwapChain if one of them has 0 references
  IDXGISwapChainGetBufferCommand* Command =
      static_cast<IDXGISwapChainGetBufferCommand*>(state->CreationCommand.get());
  unsigned swapChainKey = Command->m_Object.Key;
  for (unsigned bufferKey : m_SwapchainBuffers[swapChainKey]) {
    if (bufferKey == key) {
      continue;
    }
    m_ResourceStateTrackingService.DestroyResource(bufferKey);
    m_DescriptorService.RemoveState(bufferKey);
    m_StateService.ReleaseObject(bufferKey, 0);
  }
  m_SwapchainBuffers.erase(swapChainKey);
}

void StateTrackingLayer::Post(IUnknownAddRefCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_StateService.SetReferenceCount(c.m_Object.Key, c.m_Result.Value);
}

void StateTrackingLayer::Post(IUnknownQueryInterfaceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }

  if (c.m_Object.Value) {
    c.m_Object.Value->AddRef();
    ULONG refCount = c.m_Object.Value->Release();
    m_StateService.SetReferenceCount(c.m_Object.Key, refCount);
  }

  IID riid = c.m_riid.Value;
  if (riid == IID_ID3D12StateObjectProperties) {
    D3D12StateObjectPropertiesState* state = new D3D12StateObjectPropertiesState();
    state->ParentKey = c.m_Object.Key;
    state->LinkedLifetimeKey = c.m_Object.Key;
    state->Key = c.m_ppvObject.Key;

    SetAsChildInParent(state->ParentKey, state->Key);
    state->CreationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    m_StateService.StoreState(state);
  } else if ((riid == __uuidof(IDStorageCustomDecompressionQueue)) ||
             (riid == __uuidof(IDStorageCustomDecompressionQueue1))) {
    ObjectState* state = new ObjectState();
    state->ParentKey = c.m_Object.Key;
    state->LinkedLifetimeKey = c.m_Object.Key;
    state->Key = c.m_ppvObject.Key;

    SetAsChildInParent(state->ParentKey, state->Key);
    state->CreationCommand.reset(new IUnknownQueryInterfaceCommand(c));
    m_StateService.StoreState(state);
  }
}

void StateTrackingLayer::Post(CreateDXGIFactoryCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppFactory.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppFactory.Value);
  state->CreationCommand.reset(new CreateDXGIFactoryCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(CreateDXGIFactory1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppFactory.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppFactory.Value);
  state->CreationCommand.reset(new CreateDXGIFactory1Command(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(CreateDXGIFactory2Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppFactory.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppFactory.Value);
  state->CreationCommand.reset(new CreateDXGIFactory2Command(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(IDXGIFactoryEnumAdaptersCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppAdapter.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppAdapter.Value);
  state->CreationCommand.reset(new IDXGIFactoryEnumAdaptersCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDXGIFactory1EnumAdapters1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppAdapter.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppAdapter.Value);
  state->CreationCommand.reset(new IDXGIFactory1EnumAdapters1Command(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppvAdapter.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvAdapter.Value);
  state->CreationCommand.reset(new IDXGIFactory6EnumAdapterByGpuPreferenceCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppvAdapter.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvAdapter.Value);
  state->CreationCommand.reset(new IDXGIFactory4EnumAdapterByLuidCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDXGIAdapterEnumOutputsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppOutput.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppOutput.Value);
  state->CreationCommand.reset(new IDXGIAdapterEnumOutputsCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDXGIObjectGetParentCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppParent.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppParent.Value);
  state->CreationCommand.reset(new IDXGIObjectGetParentCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(D3D12CreateDeviceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppDevice.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppDevice.Value);
  state->CreationCommand.reset(new D3D12CreateDeviceCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(D3D12EnableExperimentalFeaturesCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_StateService.StoreD3D12EnableExperimentalFeatures(c);
}

void StateTrackingLayer::Post(D3D12GetInterfaceCommand& c) {
  if (m_StateRestored) {
    return;
  }

  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppvDebug.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvDebug.Value);
  state->CreationCommand.reset(new D3D12GetInterfaceCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppCommandQueue.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandQueue.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateCommandQueueCommand(c));
  m_StateService.StoreState(state);

  m_AccelerationStructuresSerializeService.SetDevice(c.m_Object.Value, c.m_Object.Key);
  m_AccelerationStructuresBuildService.SetDeviceKey(c.m_Object.Key);
  m_GpuExecutionFlusher.CreateCommandQueue(
      c.m_ppCommandQueue.Key, *reinterpret_cast<ID3D12CommandQueue**>(c.m_ppCommandQueue.Value));
}

void StateTrackingLayer::Post(ID3D12Device9CreateCommandQueue1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppCommandQueue.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandQueue.Value);
  state->CreationCommand.reset(new ID3D12Device9CreateCommandQueue1Command(c));
  m_StateService.StoreState(state);

  m_GpuExecutionFlusher.CreateCommandQueue(
      c.m_ppCommandQueue.Key, *reinterpret_cast<ID3D12CommandQueue**>(c.m_ppCommandQueue.Value));
}

void StateTrackingLayer::Pre(IDXGIFactoryCreateSwapChainCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_CommandQueueSwapChainRefCountTracker.PreCreateSwapChain(
      c.m_pDevice.Key, static_cast<ID3D12CommandQueue*>(c.m_pDevice.Value), c.m_ppSwapChain.Key);
}

void StateTrackingLayer::Post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppSwapChain.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppSwapChain.Value);
  state->CreationCommand.reset(new IDXGIFactoryCreateSwapChainCommand(c));
  m_StateService.StoreState(state);

  m_CommandQueueSwapChainRefCountTracker.PostCreateSwapChain(
      c.m_pDevice.Key, static_cast<ID3D12CommandQueue*>(c.m_pDevice.Value), c.m_ppSwapChain.Key);
}

void StateTrackingLayer::Pre(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_CommandQueueSwapChainRefCountTracker.PreCreateSwapChain(
      c.m_pDevice.Key, static_cast<ID3D12CommandQueue*>(c.m_pDevice.Value), c.m_ppSwapChain.Key);
}

void StateTrackingLayer::Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppSwapChain.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppSwapChain.Value);
  state->CreationCommand.reset(new IDXGIFactory2CreateSwapChainForHwndCommand(c));
  m_StateService.StoreState(state);

  m_CommandQueueSwapChainRefCountTracker.PostCreateSwapChain(
      c.m_pDevice.Key, static_cast<ID3D12CommandQueue*>(c.m_pDevice.Value), c.m_ppSwapChain.Key);
}

void StateTrackingLayer::Post(IDXGISwapChainResizeBuffersCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* objectState = m_StateService.GetState(c.m_Object.Key);
  if (objectState->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    IDXGIFactoryCreateSwapChainCommand* Command =
        static_cast<IDXGIFactoryCreateSwapChainCommand*>(objectState->CreationCommand.get());
    Command->m_pDesc.Value->BufferDesc.Width = c.m_Width.Value;
    Command->m_pDesc.Value->BufferDesc.Height = c.m_Height.Value;
    Command->m_pDesc.Value->Flags = c.m_SwapChainFlags.Value;
    if (c.m_BufferCount.Value) {
      Command->m_pDesc.Value->BufferCount = c.m_BufferCount.Value;
    }
    if (c.m_NewFormat.Value != DXGI_FORMAT_UNKNOWN) {
      Command->m_pDesc.Value->BufferDesc.Format = c.m_NewFormat.Value;
    }
  } else if (objectState->CreationCommand->GetId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    IDXGIFactory2CreateSwapChainForHwndCommand* Command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(
            objectState->CreationCommand.get());
    Command->m_pDesc.Value->Width = c.m_Width.Value;
    Command->m_pDesc.Value->Height = c.m_Height.Value;
    Command->m_pDesc.Value->Flags = c.m_SwapChainFlags.Value;
    if (c.m_BufferCount.Value) {
      Command->m_pDesc.Value->BufferCount = c.m_BufferCount.Value;
    }
    if (c.m_NewFormat.Value != DXGI_FORMAT_UNKNOWN) {
      Command->m_pDesc.Value->Format = c.m_NewFormat.Value;
    }
  } else if (objectState->CreationCommand->GetId() ==
             CommandId::ID_XEFGSWAPCHAIND3D12GETSWAPCHAINPTR) {
    xefgSwapChainD3D12GetSwapChainPtrCommand* Command =
        static_cast<xefgSwapChainD3D12GetSwapChainPtrCommand*>(objectState->CreationCommand.get());
    auto contextKey = Command->m_hSwapChain.Key;
    auto xefgContextState = m_XefgStateService.GetContextState(contextKey);
    if (xefgContextState->InitFromSwapChainDescParams.has_value()) {
      auto& swapChainDesc = xefgContextState->InitFromSwapChainDescParams.value().SwapChainDesc;
      swapChainDesc.Width = c.m_Width.Value;
      swapChainDesc.Height = c.m_Height.Value;
      swapChainDesc.Flags = c.m_SwapChainFlags.Value;
      if (c.m_BufferCount.Value) {
        swapChainDesc.BufferCount = c.m_BufferCount.Value;
      }
      if (c.m_NewFormat.Value != DXGI_FORMAT_UNKNOWN) {
        swapChainDesc.Format = c.m_NewFormat.Value;
      }
    }
  }

  unsigned swapChainKey = c.m_Object.Key;
  for (unsigned bufferKey : m_SwapchainBuffers[swapChainKey]) {
    m_ResourceStateTrackingService.DestroyResource(bufferKey);
    m_DescriptorService.RemoveState(bufferKey);
    m_StateService.RemoveState(bufferKey);
  }
  m_SwapchainBuffers.erase(swapChainKey);
}

void StateTrackingLayer::Post(IDXGISwapChain3ResizeBuffers1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* objectState = m_StateService.GetState(c.m_Object.Key);
  if (objectState->CreationCommand->GetId() == CommandId::ID_IDXGIFACTORY_CREATESWAPCHAIN) {
    IDXGIFactoryCreateSwapChainCommand* Command =
        static_cast<IDXGIFactoryCreateSwapChainCommand*>(objectState->CreationCommand.get());
    Command->m_pDesc.Value->BufferDesc.Width = c.m_Width.Value;
    Command->m_pDesc.Value->BufferDesc.Height = c.m_Height.Value;
    Command->m_pDesc.Value->Flags = c.m_SwapChainFlags.Value;
    if (c.m_BufferCount.Value) {
      Command->m_pDesc.Value->BufferCount = c.m_BufferCount.Value;
    }
    if (c.m_Format.Value != DXGI_FORMAT_UNKNOWN) {
      Command->m_pDesc.Value->BufferDesc.Format = c.m_Format.Value;
    }
  } else if (objectState->CreationCommand->GetId() ==
             CommandId::ID_IDXGIFACTORY2_CREATESWAPCHAINFORHWND) {
    IDXGIFactory2CreateSwapChainForHwndCommand* Command =
        static_cast<IDXGIFactory2CreateSwapChainForHwndCommand*>(
            objectState->CreationCommand.get());
    Command->m_pDesc.Value->Width = c.m_Width.Value;
    Command->m_pDesc.Value->Height = c.m_Height.Value;
    Command->m_pDesc.Value->Flags = c.m_SwapChainFlags.Value;
    if (c.m_BufferCount.Value) {
      Command->m_pDesc.Value->BufferCount = c.m_BufferCount.Value;
    }
    if (c.m_Format.Value != DXGI_FORMAT_UNKNOWN) {
      Command->m_pDesc.Value->Format = c.m_Format.Value;
    }
  } else if (objectState->CreationCommand->GetId() ==
             CommandId::ID_XEFGSWAPCHAIND3D12GETSWAPCHAINPTR) {
    xefgSwapChainD3D12GetSwapChainPtrCommand* Command =
        static_cast<xefgSwapChainD3D12GetSwapChainPtrCommand*>(objectState->CreationCommand.get());
    auto contextKey = Command->m_hSwapChain.Key;
    auto xefgContextState = m_XefgStateService.GetContextState(contextKey);
    if (xefgContextState->InitFromSwapChainDescParams.has_value()) {
      auto& swapChainDesc = xefgContextState->InitFromSwapChainDescParams.value().SwapChainDesc;
      swapChainDesc.Width = c.m_Width.Value;
      swapChainDesc.Height = c.m_Height.Value;
      swapChainDesc.Flags = c.m_SwapChainFlags.Value;
      if (c.m_BufferCount.Value) {
        swapChainDesc.BufferCount = c.m_BufferCount.Value;
      }
      if (c.m_Format.Value != DXGI_FORMAT_UNKNOWN) {
        swapChainDesc.Format = c.m_Format.Value;
      }
    }
  }

  unsigned swapChainKey = c.m_Object.Key;
  for (unsigned bufferKey : m_SwapchainBuffers[swapChainKey]) {
    m_ResourceStateTrackingService.DestroyResource(bufferKey);
    m_DescriptorService.RemoveState(bufferKey);
    m_StateService.RemoveState(bufferKey);
  }
  m_SwapchainBuffers.erase(swapChainKey);
}

void StateTrackingLayer::Post(ID3D12ObjectSetNameCommand& c) {
  if (m_StateRestored) {
    return;
  }
  ObjectState* state = m_StateService.GetState(c.m_Object.Key);
  if (state == nullptr) {
    LOG_ERROR << "StateTrackingLayer: SetName failed. Cannot find object O" << c.m_Object.Key
              << ".";
    return;
  }
  state->Name = c.m_Name.Value;
}

void StateTrackingLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  D3D12DescriptorHeapState* state = new D3D12DescriptorHeapState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvHeap.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvHeap.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateDescriptorHeapCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->ParentKey = c.m_Object.Key;
  state->DeviceKey = c.m_Object.Key;
  state->Key = c.m_ppvHeap.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvHeap.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateHeapCommand(c));
  m_StateService.StoreState(state);

  if (c.m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, c.m_Object.Key);
  }
}

void StateTrackingLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->ParentKey = c.m_Object.Key;
  state->DeviceKey = c.m_Object.Key;
  state->Key = c.m_ppvHeap.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvHeap.Value);
  state->CreationCommand.reset(new ID3D12Device4CreateHeap1Command(c));
  m_StateService.StoreState(state);

  if (c.m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, c.m_Object.Key);
  }
}

void StateTrackingLayer::Post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvHeap.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvHeap.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateQueryHeapCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(CreateHeapAllocationMetaCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12HeapFromAddressState* state = new D3D12HeapFromAddressState();
  state->Key = c.m_heap.Key;
  state->CreationCommand.reset(new CreateHeapAllocationMetaCommand(c));
  m_HeapAllocationStateService.SetHeapState(state);
}

void StateTrackingLayer::Post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12HeapFromAddressState* state = m_HeapAllocationStateService.GetHeapState(c.m_ppvHeap.Key);
  state->ParentKey = c.m_Object.Key;
  state->OpenExistingHeapFromAddressCommand.reset(
      new ID3D12Device3OpenExistingHeapFromAddressCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppSurface.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppSurface.Value);
  state->CreationCommand.reset(new IDXGISwapChainGetBufferCommand(c));
  m_StateService.StoreState(state);

  m_ResourceStateTrackingService.AddResource(0, static_cast<ID3D12Resource*>(*c.m_ppSurface.Value),
                                             state->Key, D3D12_RESOURCE_STATE_COMMON, false);
  m_StateService.AddBackBuffer(c.m_Buffer.Value, state->Key,
                               static_cast<ID3D12Resource*>(*c.m_ppSurface.Value));

  // Keep track of the buffer key
  m_SwapchainBuffers[c.m_Object.Key].push_back(state->Key);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12RenderTargetViewState* state = new D3D12RenderTargetViewState();
  state->DeviceKey = c.m_Object.Key;
  state->ResourceKey = c.m_pResource.Key;
  if (state->IsDesc = c.m_pDesc.Value ? true : false) {
    state->Desc = *c.m_pDesc.Value;
  }
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12DepthStencilViewState* state = new D3D12DepthStencilViewState();
  state->DeviceKey = c.m_Object.Key;
  state->ResourceKey = c.m_pResource.Key;
  if (state->IsDesc = c.m_pDesc.Value ? true : false) {
    state->Desc = *c.m_pDesc.Value;
  }
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppCommandAllocator.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandAllocator.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateCommandAllocatorCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvRootSignature.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvRootSignature.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateRootSignatureCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppPipelineLibrary.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppPipelineLibrary.Value);
  state->CreationCommand.reset(new ID3D12Device1CreatePipelineLibraryCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* exisitingState = m_StateService.GetState(c.m_ppPipelineState.Key);
  if (exisitingState) {
    ++exisitingState->RefCount;
  } else {
    ObjectState* state = new ObjectState();
    state->ParentKey = c.m_Object.Key;
    state->Key = c.m_ppPipelineState.Key;
    state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
    state->CreationCommand.reset(new ID3D12PipelineLibrary1LoadPipelineCommand(c));
    m_StateService.StoreState(state);
  }
}

void StateTrackingLayer::Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* exisitingState = m_StateService.GetState(c.m_ppPipelineState.Key);
  if (exisitingState) {
    ++exisitingState->RefCount;
  } else {
    ObjectState* state = new ObjectState();
    state->ParentKey = c.m_Object.Key;
    state->Key = c.m_ppPipelineState.Key;
    state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
    state->CreationCommand.reset(new ID3D12PipelineLibraryLoadGraphicsPipelineCommand(c));
    m_StateService.StoreState(state);
  }
}

void StateTrackingLayer::Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* exisitingState = m_StateService.GetState(c.m_ppPipelineState.Key);
  if (exisitingState) {
    ++exisitingState->RefCount;
  } else {
    ObjectState* state = new ObjectState();
    state->ParentKey = c.m_Object.Key;
    state->Key = c.m_ppPipelineState.Key;
    state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
    state->CreationCommand.reset(new ID3D12PipelineLibraryLoadComputePipelineCommand(c));
    m_StateService.StoreState(state);
  }
}

void StateTrackingLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvCommandSignature.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvCommandSignature.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateCommandSignatureCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppPipelineState.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateGraphicsPipelineStateCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppPipelineState.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateComputePipelineStateCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppPipelineState.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
  state->CreationCommand.reset(new ID3D12Device2CreatePipelineStateCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12Device5CreateStateObjectCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppStateObject.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppStateObject.Value);
  state->CreationCommand.reset(new ID3D12Device5CreateStateObjectCommand(c));
  m_StateService.StoreState(state);

  for (auto& it : c.m_pDesc.InterfaceKeysBySubobject) {
    m_StateService.KeepState(it.second);
  }

  m_AccelerationStructuresSerializeService.SetDevice(c.m_Object.Value, c.m_Object.Key);
  m_AccelerationStructuresBuildService.SetDeviceKey(c.m_Object.Key);
}

void StateTrackingLayer::Post(ID3D12Device7AddToStateObjectCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_pStateObjectToGrowFrom.Key;
  state->Key = c.m_ppNewStateObject.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppNewStateObject.Value);
  state->CreationCommand.reset(new ID3D12Device7AddToStateObjectCommand(c));
  m_StateService.StoreState(state);

  for (auto& it : c.m_pAddition.InterfaceKeysBySubobject) {
    m_StateService.KeepState(it.second);
  }

  m_StateService.KeepState(c.m_pStateObjectToGrowFrom.Key);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateCommandListCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppCommandList.Key;
  state->m_AllocatorKey = c.m_pCommandAllocator.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandList.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateCommandListCommand(c));
  m_StateService.StoreState(state);

  m_CommandListService.AddCommandList(state);
}

void StateTrackingLayer::Post(ID3D12Device4CreateCommandList1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  CommandListState* state = new CommandListState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppCommandList.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandList.Value);
  state->CreationCommand.reset(new ID3D12Device4CreateCommandList1Command(c));
  m_StateService.StoreState(state);

  m_CommandListService.AddCommandList(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateCommittedResourceCommand(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialResourceState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(*c.m_pHeapProperties.Value, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
  }
  if (c.m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device4CreateCommittedResource1Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialResourceState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(*c.m_pHeapProperties.Value, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);
  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
  }
  if (c.m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device8CreateCommittedResource2Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialResourceState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(*c.m_pHeapProperties.Value, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
  }
  if (c.m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device10CreateCommittedResource3Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialLayout = c.m_InitialLayout.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(*c.m_pHeapProperties.Value, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                             static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             state->Key, state->InitialLayout, !state->IsMappable);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
  if (c.m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_pHeap.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreatePlacedResourceCommand(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(c.m_pHeap.Key, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  state->BarrierRestricted = IsResourceBarrierRestricted(c.m_pDesc.Value->Flags);
  state->HeapKey = c.m_pHeap.Key;

  m_StateService.StoreState(state);

  m_ResourceHeaps[c.m_pHeap.Key].insert(c.m_ppvResource.Key);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceForCBVRestoreService.AddResourceCreationCommand(
      state->Key, state->HeapKey, new ID3D12DeviceCreatePlacedResourceCommand(c));

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(
        state->DeviceKey, static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), state->Key,
        state->InitialState, !(state->IsMappable || state->BarrierRestricted));
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
  m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                           c.m_pDesc.Value->Flags);
}

void StateTrackingLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_pHeap.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device8CreatePlacedResource1Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(c.m_pHeap.Key, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  state->BarrierRestricted = IsResourceBarrierRestricted(c.m_pDesc.Value->Flags);
  state->HeapKey = c.m_pHeap.Key;

  m_StateService.StoreState(state);

  m_ResourceHeaps[c.m_pHeap.Key].insert(c.m_ppvResource.Key);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceForCBVRestoreService.AddResourceCreationCommand(
      state->Key, state->HeapKey, new ID3D12Device8CreatePlacedResource1Command(c));

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(
        state->DeviceKey, static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), state->Key,
        state->InitialState, !(state->IsMappable || state->BarrierRestricted));
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
  m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                           c.m_pDesc.Value->Flags);
}

void StateTrackingLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_pHeap.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device10CreatePlacedResource2Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialLayout = c.m_InitialLayout.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(c.m_pHeap.Key, c.m_pDesc.Value->Layout);
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  state->BarrierRestricted = IsResourceBarrierRestricted(c.m_pDesc.Value->Flags);
  state->HeapKey = c.m_pHeap.Key;

  m_StateService.StoreState(state);

  m_ResourceHeaps[c.m_pHeap.Key].insert(c.m_ppvResource.Key);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceForCBVRestoreService.AddResourceCreationCommand(
      state->Key, state->HeapKey, new ID3D12Device10CreatePlacedResource2Command(c));

  m_ResourceStateTrackingService.AddResource(
      state->DeviceKey, static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), state->Key,
      state->InitialLayout, !(state->IsMappable || state->BarrierRestricted));
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
  m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                           c.m_pDesc.Value->Flags);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateReservedResourceCommand(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
}

void StateTrackingLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device4CreateReservedResource1Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
}

void StateTrackingLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new ID3D12Device10CreateReservedResource2Command(c));

  state->DeviceKey = c.m_Object.Key;
  state->InitialLayout = c.m_InitialLayout.Value;
  state->Dimension = c.m_pDesc.Value->Dimension;
  state->SampleCount = c.m_pDesc.Value->SampleDesc.Count;
  state->DenyShaderResource = c.m_pDesc.Value->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                             static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             state->Key, state->InitialLayout, !state->IsMappable);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12ShaderResourceViewState* state = new D3D12ShaderResourceViewState();
  state->DeviceKey = c.m_Object.Key;
  state->ResourceKey = c.m_pResource.Key;
  if (c.m_pDesc.Value) {
    state->IsDesc = true;
    state->Desc = *c.m_pDesc.Value;
    if (c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
      state->ResourceKey = c.m_pDesc.RaytracingLocationKey;
      state->RaytracingLocationOffset = c.m_pDesc.RaytracingLocationOffset;
    }
  }
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppFence.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppFence.Value);
  state->CreationCommand.reset(new ID3D12DeviceCreateFenceCommand(c));
  m_StateService.StoreState(state);

  m_FenceTrackingService.SetFenceValue(c.m_ppFence.Key, c.m_InitialValue.Value);
  m_AccelerationStructuresBuildService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  m_GpuExecutionFlusher.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  m_ResourceUsageTrackingService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void StateTrackingLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_AnalyzerResults.RestoreCommandQueueCommand(c.Key)) {
    m_CommandQueueService.AddCommandQueueSignal(c);
  } else {
    m_FenceTrackingService.SetFenceValue(c.m_pFence.Key, c.m_Value.Value);
  }

  m_AccelerationStructuresBuildService.CommandQueueSignal(c);
  m_GpuExecutionFlusher.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  m_ResourceUsageTrackingService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                                    c.m_Value.Value);
}

void StateTrackingLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_AnalyzerResults.RestoreCommandQueueCommand(c.Key)) {
    m_CommandQueueService.AddCommandQueueWait(c);
  }

  m_AccelerationStructuresBuildService.CommandQueueWait(c);
  m_GpuExecutionFlusher.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  m_ResourceUsageTrackingService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                                  c.m_Value.Value);
}

void StateTrackingLayer::Post(ID3D12FenceSignalCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_FenceTrackingService.SetFenceValue(c.m_Object.Key, c.m_Value.Value);
  m_AccelerationStructuresBuildService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  m_GpuExecutionFlusher.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  m_ResourceUsageTrackingService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void StateTrackingLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_FenceTrackingService.SetFenceValue(c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
  m_AccelerationStructuresBuildService.FenceSignal(c.Key, c.m_pFenceToSignal.Key,
                                                   c.m_FenceValueToSignal.Value);
  m_GpuExecutionFlusher.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);

  m_ResidencyService.MakeResident(c.m_ppObjects.Keys, c.m_Object.Key);
  m_ResourceUsageTrackingService.FenceSignal(c.Key, c.m_pFenceToSignal.Key,
                                             c.m_FenceValueToSignal.Value);
}

void StateTrackingLayer::Post(ID3D12DeviceMakeResidentCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_ResidencyService.MakeResident(c.m_ppObjects.Keys, c.m_Object.Key);
}

void StateTrackingLayer::Post(ID3D12DeviceEvictCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_ResidencyService.Evict(c.m_ppObjects.Keys, c.m_Object.Key);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateSamplerCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12SamplerState* state = new D3D12SamplerState();
  state->DeviceKey = c.m_Object.Key;
  state->Desc = *c.m_pDesc.Value;
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
  state->DeviceKey = c.m_Object.Key;
  state->ResourceKey = c.m_pResource.Key;
  state->AuxiliaryResourceKey = c.m_pCounterResource.Key;
  if (state->IsDesc = c.m_pDesc.Value ? true : false) {
    state->Desc = *c.m_pDesc.Value;
  }
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
  state->DeviceKey = c.m_Object.Key;
  if (state->IsDesc = c.m_pDesc.Value ? true : false) {
    state->Desc = *c.m_pDesc.Value;
  }
  state->ResourceKey = c.m_pDesc.BufferLocationKey;
  state->BufferLocationOffset = c.m_pDesc.BufferLocationOffset;
  state->DestDescriptor = c.m_DestDescriptor.Value;
  state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
  state->DestDescriptorIndex = c.m_DestDescriptor.Index;
  m_DescriptorService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12ResourceMapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_MapStateService.MapResource(c.m_Object.Key, c.m_Subresource.Value, c.m_ppData.CaptureValue);
}

void StateTrackingLayer::Post(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (m_AnalyzerResults.RestoreCommandQueueCommand(c.Key)) {
    m_CommandQueueService.AddUpdateTileMappings(c);
  } else {
    m_ReservedResourcesService.AddUpdateTileMappings(c);
  }
}

void StateTrackingLayer::Post(ID3D12CommandQueueCopyTileMappingsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "ID3D12CommandQueue::CopyTileMappings not handled in subcapture!";
    logged = true;
  }
}

void StateTrackingLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_DescriptorService.CopyDescriptors(c);
}

void StateTrackingLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_DescriptorService.CopyDescriptors(c);
}

void StateTrackingLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppExtensionContext.Key;
  state->CreationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContextCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(c.m_pDevice.Key, state->Key);
  m_DeviceByINTCExtensionContext[state->Key] = c.m_pDevice.Key;
}

void StateTrackingLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (m_StateRestored) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppExtensionContext.Key;
  state->CreationCommand.reset(new INTC_D3D12_CreateDeviceExtensionContext1Command(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(c.m_pDevice.Key, state->Key);
  m_DeviceByINTCExtensionContext[state->Key] = c.m_pDevice.Key;
}

void StateTrackingLayer::Post(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_StateService.StoreINTCApplicationInfo(c);
}

void StateTrackingLayer::Post(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_StateService.ReleaseObject(c.m_ppExtensionContext.Key, c.m_Result.Value);
  m_DeviceByINTCExtensionContext.erase(c.m_ppExtensionContext.Key);
}

void StateTrackingLayer::Post(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_StateService.StoreINTCFeature(*c.m_pFeature.Value);
}

void StateTrackingLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreateCommittedResourceCommand(c));

  state->DeviceKey = m_DeviceByINTCExtensionContext[c.m_pExtensionContext.Key];
  state->InitialState = c.m_InitialResourceState.Value;
  state->Dimension = c.m_pDesc.Value->pD3D12Desc->Dimension;
  state->SampleCount = c.m_pDesc.Value->pD3D12Desc->SampleDesc.Count;
  state->IsMappable =
      IsResourceHeapMappable(*c.m_pHeapProperties.Value, c.m_pDesc.Value->pD3D12Desc->Layout);

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
  }
  if (c.m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreatePlacedResourceCommand(c));

  state->DeviceKey = m_DeviceByINTCExtensionContext[c.m_pExtensionContext.Key];
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->pD3D12Desc->Dimension;
  state->SampleCount = c.m_pDesc.Value->pD3D12Desc->SampleDesc.Count;
  state->IsMappable = IsResourceHeapMappable(c.m_pHeap.Key, c.m_pDesc.Value->pD3D12Desc->Layout);
  state->BarrierRestricted = IsResourceBarrierRestricted(c.m_pDesc.Value->pD3D12Desc->Flags);
  state->HeapKey = c.m_pHeap.Key;

  m_StateService.StoreState(state);

  m_ResourceHeaps[c.m_pHeap.Key].insert(c.m_ppvResource.Key);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  m_ResourceForCBVRestoreService.AddResourceCreationCommand(
      state->Key, state->HeapKey, new INTC_D3D12_CreatePlacedResourceCommand(c));

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(
        state->DeviceKey, static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), state->Key,
        state->InitialState, !(state->IsMappable || state->BarrierRestricted));
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
}

void StateTrackingLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ResourceState* state = new ResourceState();
  state->Key = c.m_ppvResource.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvResource.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreateReservedResourceCommand(c));

  state->DeviceKey = m_DeviceByINTCExtensionContext[c.m_pExtensionContext.Key];
  state->InitialState = c.m_InitialState.Value;
  state->Dimension = c.m_pDesc.Value->pD3D12Desc->Dimension;
  state->SampleCount = c.m_pDesc.Value->pD3D12Desc->SampleDesc.Count;

  m_StateService.StoreState(state);

  m_ResourceUsageTrackingService.AddResource(state->Key);

  if (state->InitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_ResourceStateTrackingService.AddResource(state->DeviceKey,
                                               static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                               state->Key, state->InitialState, !state->IsMappable);
    m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
  }
}

void StateTrackingLayer::Post(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppCommandQueue.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppCommandQueue.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreateCommandQueueCommand(c));
  m_StateService.StoreState(state);

  m_GpuExecutionFlusher.CreateCommandQueue(
      c.m_ppCommandQueue.Key, *reinterpret_cast<ID3D12CommandQueue**>(c.m_ppCommandQueue.Value));
}

void StateTrackingLayer::Post(INTC_D3D12_CreateHeapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  HeapState* state = new HeapState();
  state->DeviceKey = m_DeviceByINTCExtensionContext[c.m_pExtensionContext.Key];
  state->Key = c.m_ppvHeap.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvHeap.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreateHeapCommand(c));
  m_StateService.StoreState(state);

  if (c.m_pDesc.Value->pD3D12Desc->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
    m_ResidencyService.CreateNotResident(state->Key, state->DeviceKey);
  }
}

void StateTrackingLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppPipelineState.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppPipelineState.Value);
  state->CreationCommand.reset(new INTC_D3D12_CreateComputePipelineStateCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(ID3D12Device1SetResidencyPriorityCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  for (unsigned i = 0; i < c.m_NumObjects.Value; ++i) {
    ObjectState* state = m_StateService.GetState(c.m_ppObjects.Keys[i]);
    GITS_ASSERT(state);
    if (state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE &&
        state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1 &&
        state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2 &&
        state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3 &&
        state->CreationCommand->GetId() != CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE &&
        state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE_CREATEHEAP &&
        state->CreationCommand->GetId() != CommandId::ID_ID3D12DEVICE4_CREATEHEAP1 &&
        state->CreationCommand->GetId() != CommandId::INTC_D3D12_CREATEHEAP) {
      LOG_WARNING << "SetResidencyPriority not handled for Command "
                  << static_cast<unsigned>(state->CreationCommand->GetId());
    }
    state->ResidencyPriority = c.m_pPriorities.Value[i];
  }
}

void StateTrackingLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (m_AnalyzerResults.RestoreCommandQueueCommand(c.Key)) {
    m_CommandQueueService.AddExecuteCommandLists(c);
  } else {
    m_ResourceStateTrackingService.ExecuteCommandLists(c.m_ppCommandLists.Keys);
    m_AccelerationStructuresSerializeService.ExecuteCommandLists(c);
    m_AccelerationStructuresBuildService.ExecuteCommandLists(c);
  }
  m_ResourceUsageTrackingService.ExecuteCommandLists(c.Key, c.m_Object.Key,
                                                     c.m_ppCommandLists.Keys);
  m_ResourceStateTracker.ExecuteCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
      c.m_NumCommandLists.Value);
}

void StateTrackingLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (m_StateRestored) {
    return;
  }
  ResourceState* state = static_cast<ResourceState*>(m_StateService.GetState(c.m_Object.Key));
  GITS_ASSERT(state);
  state->GpuVirtualAddress = c.m_Result.Value;
  m_GpuAddressService.AddGpuCaptureAddress(c.m_Object.Value, c.m_Object.Key,
                                           c.m_Object.Value->GetDesc().Width, c.m_Result.Value);
}

void StateTrackingLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12StateObjectPropertiesState* state =
      static_cast<D3D12StateObjectPropertiesState*>(m_StateService.GetState(c.m_Object.Key));
  GITS_ASSERT(state);
  m_StateService.KeepState(state->Key);
  auto it = state->ShaderIdentifiers.find(c.m_pExportName.Value);
  if (it == state->ShaderIdentifiers.end()) {
    auto& shaderIdentifier = state->ShaderIdentifiers[c.m_pExportName.Value];
    for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
      shaderIdentifier[i] = static_cast<uint8_t*>(c.m_Result.Value)[i];
    }
  }
}

void StateTrackingLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (m_StateRestored) {
    return;
  }
  D3D12DescriptorHeapState* state =
      static_cast<D3D12DescriptorHeapState*>(m_StateService.GetState(c.m_Object.Key));
  GITS_ASSERT(state);
  state->GpuDescriptorHandle = c.m_Result.Value;
}

void StateTrackingLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresBuildService.BuildAccelerationStructure(c);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresSerializeService.BuildAccelerationStructure(c);

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresBuildService.CopyAccelerationStructure(c);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresSerializeService.CopyAccelerationStructure(c);

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList4DispatchRaysSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (m_StateRestored) {
    return;
  }

  if (m_SubcaptureRange.IsExecutionRangeStart()) {
    m_GpuExecutionFlusher.FlushCommandQueues();
    m_StateService.RestoreState();
  }

  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->ClearCommands();
  state->m_AllocatorKey = c.m_pAllocator.Key;

  if (state->CreationCommand->GetId() == CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST) {
    ID3D12DeviceCreateCommandListCommand* Command =
        static_cast<ID3D12DeviceCreateCommandListCommand*>(state->CreationCommand.get());
    Command->m_pCommandAllocator.Key = c.m_pAllocator.Key;
    Command->m_pInitialState.Key = c.m_pInitialState.Key;
  }

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListResetSerializer(c));
  state->m_Commands.push_back(Command);

  m_ResourceUsageTrackingService.CommandListReset(state->Key);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListCloseCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListCloseSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Closed = true;
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListClearStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListClearStateSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListDrawInstancedSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListDrawIndexedInstancedSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListDispatchSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key, c.m_pDstBuffer.Key);
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListCopyBufferRegionSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key, c.m_pDst.ResourceKey);
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListCopyTextureRegionSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key, c.m_pDstResource.Key);
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListCopyResourceSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key, c.m_pTiledResource.Key);
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListCopyTilesSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListResolveSubresourceSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListIASetPrimitiveTopologySerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListRSSetViewportsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListRSSetViewportsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListRSSetScissorRectsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListOMSetBlendFactorSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListOMSetStencilRefSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListSetPipelineStateSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key,
                                                          c.m_pBarriers.ResourceKeys);
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key,
                                                          c.m_pBarriers.ResourceAfterKeys);
  m_ResourceStateTrackingService.ResourceBarrier(c.m_Object.Key, c.m_pBarriers.Value,
                                                 c.m_pBarriers.ResourceKeys,
                                                 c.m_pBarriers.ResourceAfterKeys);
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                         c.m_NumBarriers.Value, c.m_pBarriers.ResourceKeys.data());

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListResourceBarrierSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}
void StateTrackingLayer::Post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListExecuteBundleSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListSetDescriptorHeapsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_DescriptorHeapKeys = c.m_ppDescriptorHeaps.Keys;
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRootSignatureSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootSignatureSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRootDescriptorTableSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRoot32BitConstantSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRoot32BitConstantsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRootConstantBufferViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRootShaderResourceViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListIASetIndexBufferSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListIASetVertexBuffersSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListSOSetTargetsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListOMSetRenderTargets* Command = new CommandListOMSetRenderTargets(c.Key, c.m_Object.Key);
  Command->m_RenderTargetViews.resize(c.m_NumRenderTargetDescriptors.Value);
  {
    unsigned heapKey{};
    unsigned heapIndex{};
    for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
      if (i == 0 || !c.m_RTsSingleHandleToDescriptorRange.Value) {
        heapKey = c.m_pRenderTargetDescriptors.InterfaceKeys[i];
        heapIndex = c.m_pRenderTargetDescriptors.Indexes[i];
      } else {
        ++heapIndex;
      }
      if (heapKey) {
        DescriptorState* descriptorState =
            m_DescriptorService.GetDescriptorState(heapKey, heapIndex);
        GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_RENDERTARGETVIEW);
        Command->m_RenderTargetViews[i].reset(new D3D12RenderTargetViewState(
            *static_cast<D3D12RenderTargetViewState*>(descriptorState)));
      }
    }
  }
  if (!c.m_pDepthStencilDescriptor.InterfaceKeys.empty()) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_pDepthStencilDescriptor.InterfaceKeys[0], c.m_pDepthStencilDescriptor.Indexes[0]);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_DEPTHSTENCILVIEW);
    Command->m_DepthStencilView.reset(
        new D3D12DepthStencilViewState(*static_cast<D3D12DepthStencilViewState*>(descriptorState)));
  }
  Command->m_CommandListKey = c.m_Object.Key;
  Command->m_RtsSingleHandleToDescriptorRange = c.m_RTsSingleHandleToDescriptorRange.Value;

  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListOMSetRenderTargetsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListClearDepthStencilView* Command =
      new CommandListClearDepthStencilView(c.Key, c.m_Object.Key);
  if (c.m_DepthStencilView.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_DepthStencilView.InterfaceKey, c.m_DepthStencilView.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_DEPTHSTENCILVIEW);
    Command->m_DepthStencilView.reset(
        new D3D12DepthStencilViewState(*static_cast<D3D12DepthStencilViewState*>(descriptorState)));
  }
  Command->m_CommandListKey = c.m_Object.Key;
  Command->m_Depth = c.m_Depth.Value;
  Command->m_Stencil = c.m_Stencil.Value;
  if (c.m_NumRects.Value) {
    Command->m_Rects.resize(c.m_NumRects.Value);
    for (unsigned i = 0; i < c.m_NumRects.Value; ++i) {
      Command->m_Rects[i] = c.m_pRects.Value[i];
    }
  }

  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListClearDepthStencilViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListClearRenderTargetView* Command =
      new CommandListClearRenderTargetView(c.Key, c.m_Object.Key);
  if (c.m_RenderTargetView.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_RenderTargetView.InterfaceKey, c.m_RenderTargetView.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_RENDERTARGETVIEW);
    Command->m_RenderTargetView.reset(
        new D3D12RenderTargetViewState(*static_cast<D3D12RenderTargetViewState*>(descriptorState)));
  }
  Command->m_CommandListKey = c.m_Object.Key;
  for (unsigned i = 0; i < 4; ++i) {
    Command->m_ColorRGBA[i] = c.m_ColorRGBA.Value[i];
  }
  if (c.m_NumRects.Value) {
    Command->m_Rects.resize(c.m_NumRects.Value);
    for (unsigned i = 0; i < c.m_NumRects.Value; ++i) {
      Command->m_Rects[i] = c.m_pRects.Value[i];
    }
  }

  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListClearRenderTargetViewSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListClearUnorderedAccessViewUint* Command =
      new CommandListClearUnorderedAccessViewUint(c.Key, c.m_Object.Key);
  if (c.m_ViewGPUHandleInCurrentHeap.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    Command->m_ViewGPUHandleInCurrentHeap.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  if (c.m_ViewCPUHandle.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_ViewCPUHandle.InterfaceKey, c.m_ViewCPUHandle.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    Command->m_ViewCPUHandle.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  Command->m_CommandListKey = c.m_Object.Key;
  Command->m_ResourceKey = c.m_pResource.Key;
  for (unsigned i = 0; i < 4; ++i) {
    Command->m_Values[i] = c.m_Values.Value[i];
  }
  if (c.m_NumRects.Value) {
    Command->m_Rects.resize(c.m_NumRects.Value);
    for (unsigned i = 0; i < c.m_NumRects.Value; ++i) {
      Command->m_Rects[i] = c.m_pRects.Value[i];
    }
  }

  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListClearUnorderedAccessViewUintSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListClearUnorderedAccessViewFloat* Command =
      new CommandListClearUnorderedAccessViewFloat(c.Key, c.m_Object.Key);
  if (c.m_ViewGPUHandleInCurrentHeap.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    Command->m_ViewGPUHandleInCurrentHeap.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  if (c.m_ViewCPUHandle.InterfaceKey) {
    DescriptorState* descriptorState = m_DescriptorService.GetDescriptorState(
        c.m_ViewCPUHandle.InterfaceKey, c.m_ViewCPUHandle.Index);
    GITS_ASSERT(descriptorState->Id == DescriptorState::D3D12_UNORDEREDACCESSVIEW);
    Command->m_ViewCPUHandle.reset(new D3D12UnorderedAccessViewState(
        *static_cast<D3D12UnorderedAccessViewState*>(descriptorState)));
  }
  Command->m_CommandListKey = c.m_Object.Key;
  Command->m_ResourceKey = c.m_pResource.Key;
  for (unsigned i = 0; i < 4; ++i) {
    Command->m_Values[i] = c.m_Values.Value[i];
  }
  if (c.m_NumRects.Value) {
    Command->m_Rects.resize(c.m_NumRects.Value);
    for (unsigned i = 0; i < c.m_NumRects.Value; ++i) {
      Command->m_Rects[i] = c.m_pRects.Value[i];
    }
  }

  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandListClearUnorderedAccessViewFloatSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListDiscardResourceSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListBeginQuerySerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListEndQuerySerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListResolveQueryDataSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListSetPredicationSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListSetMarkerCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListSetMarkerSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListBeginEventCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListBeginEventSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListEndEventCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListEndEventSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandListExecuteIndirectSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList1AtomicCopyBufferUINTSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Serializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList1OMSetDepthBoundsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1SetSamplePositionsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList1SetSamplePositionsSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList1ResolveSubresourceRegionSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList1SetViewInstanceMaskSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList2WriteBufferImmediateSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList3SetProtectedResourceSessionSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList4BeginRenderPassSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4EndRenderPassCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList4EndRenderPassSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  if (m_StateRestored) {
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "ID3D12GraphicsCommandList4ExecuteMetaCommand is not supported in subcapture.";
    logged = true;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList4ExecuteMetaCommandSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList4InitializeMetaCommandSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList4SetPipelineState1Serializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList5RSSetShadingRateCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList5RSSetShadingRateSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(
      new ID3D12GraphicsCommandList5RSSetShadingRateImageSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  if (m_StateRestored) {
    return;
  }
  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList6DispatchMeshSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_ResourceUsageTrackingService.CommandListResourceUsage(c.m_Object.Key,
                                                          c.m_pBarrierGroups.ResourceKeys);

  m_ResourceStateTrackingService.ResourceBarrier(c.m_Object.Key, c.m_pBarrierGroups.Value,
                                                 c.m_NumBarrierGroups.Value,
                                                 c.m_pBarrierGroups.ResourceKeys);
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarrierGroups.Value,
                                         c.m_NumBarrierGroups.Value,
                                         c.m_pBarrierGroups.ResourceKeys.data());

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_Object.Key);
  Command->m_CommandSerializer.reset(new ID3D12GraphicsCommandList7BarrierSerializer(c));
  CommandListState* state = static_cast<CommandListState*>(m_StateService.GetState(c.m_Object.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& c) {
  if (m_StateRestored) {
    return;
  }

  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvFactory.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvFactory.Value);
  state->CreationCommand.reset(new ID3D12SDKConfiguration1CreateDeviceFactoryCommand(c));
  m_StateService.StoreState(state);
  m_StateService.KeepState(state->ParentKey);
}

void StateTrackingLayer::Post(ID3D12DeviceFactoryCreateDeviceCommand& c) {
  if (m_StateRestored) {
    return;
  }

  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->Key = c.m_ppvDevice.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppvDevice.Value);
  state->CreationCommand.reset(new ID3D12DeviceFactoryCreateDeviceCommand(c));
  m_StateService.StoreState(state);
  m_StateService.KeepState(state->ParentKey);
}

void StateTrackingLayer::Post(xessD3D12CreateContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != XESS_RESULT_SUCCESS) {
    return;
  }
  XessStateService::ContextState* state = new XessStateService::ContextState();
  state->Key = c.m_phContext.Key;
  state->DeviceKey = c.m_pDevice.Key;
  state->Device = c.m_pDevice.Value;
  m_XessStateService.StoreContextState(state);
}

void StateTrackingLayer::Post(xessD3D12InitCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  state->InitParams.emplace(c.m_pInitParams);
}

void StateTrackingLayer::Pre(xessDestroyContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  state->Device->AddRef();
}

void StateTrackingLayer::Post(xessDestroyContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  ULONG ref = state->Device->Release();
  if (ref == 0) {
    m_StateService.ReleaseObject(state->DeviceKey, 0);
  }
  m_XessStateService.DestroyContext(c.m_hContext.Key);
}

void StateTrackingLayer::Post(xessSetJitterScaleCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  if (!state->JitterScale) {
    state->JitterScale = std::make_unique<float[]>(2);
  }
  state->JitterScale[0] = c.m_x.Value;
  state->JitterScale[1] = c.m_y.Value;
}

void StateTrackingLayer::Post(xessSetVelocityScaleCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  if (!state->VelocityScale) {
    state->VelocityScale = std::make_unique<float[]>(2);
  }
  state->VelocityScale[0] = c.m_x.Value;
  state->VelocityScale[1] = c.m_y.Value;
}

void StateTrackingLayer::Post(xessSetExposureMultiplierCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  state->ExposureScale = c.m_scale.Value;
}

void StateTrackingLayer::Post(xessForceLegacyScaleFactorsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XessStateService::ContextState* state = m_XessStateService.GetContextState(c.m_hContext.Key);
  state->ForceLegacyScaleFactors = c.m_force.Value;
}

void StateTrackingLayer::Post(DStorageGetFactoryCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppv.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppv.Value);
  state->CreationCommand.reset(new DStorageGetFactoryCommand(c));
  m_StateService.StoreState(state);
}

void StateTrackingLayer::Post(IDStorageFactoryOpenFileCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppv.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppv.Value);
  state->CreationCommand.reset(new IDStorageFactoryOpenFileCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDStorageFactoryCreateQueueCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppv.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppv.Value);
  state->CreationCommand.reset(new IDStorageFactoryCreateQueueCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->ParentKey = c.m_Object.Key;
  state->LinkedLifetimeKey = c.m_Object.Key;
  state->Key = c.m_ppv.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppv.Value);
  state->CreationCommand.reset(new IDStorageFactoryCreateStatusArrayCommand(c));
  m_StateService.StoreState(state);

  SetAsChildInParent(state->ParentKey, state->Key);
}

void StateTrackingLayer::Post(NvAPI_InitializeCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != NVAPI_OK) {
    return;
  }
  m_StateService.GetNvAPIGlobalStateService().IncrementInitialize();
}

void StateTrackingLayer::Post(NvAPI_UnloadCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != NVAPI_OK) {
    return;
  }
  m_StateService.GetNvAPIGlobalStateService().DecrementInitialize();
}

void StateTrackingLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != NVAPI_OK) {
    return;
  }
  m_StateService.GetNvAPIGlobalStateService().AddSetCreatePipelineStateOptionsCommand(c);
}

void StateTrackingLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != NVAPI_OK) {
    return;
  }
  m_StateService.GetNvAPIGlobalStateService().AddSetNvShaderExtnSlotSpaceCommand(c);
}

void StateTrackingLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != NVAPI_OK) {
    return;
  }
  m_StateService.GetNvAPIGlobalStateService().AddSetNvShaderExtnSlotSpaceLocalThreadCommand(c);
}

void StateTrackingLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresBuildService.NvapiBuildAccelerationStructureEx(c);
}

void StateTrackingLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_StateRestored) {
    return;
  }

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_pCommandList.Key);
  Command->m_CommandSerializer.reset(
      new NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(c));
  CommandListState* state =
      static_cast<CommandListState*>(m_StateService.GetState(c.m_pCommandList.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_AccelerationStructuresBuildService.NvapiBuildOpacityMicromapArray(c);
}

void StateTrackingLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_StateRestored) {
    return;
  }

  CommandListCommand* Command = new CommandListCommand(c.GetId(), c.Key, c.m_pCommandList.Key);
  Command->m_CommandSerializer.reset(
      new NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(c));
  CommandListState* state =
      static_cast<CommandListState*>(m_StateService.GetState(c.m_pCommandList.Key));
  state->m_Commands.push_back(Command);
}

void StateTrackingLayer::Post(DllContainerMetaCommand& c) {
  if (m_StateRestored) {
    return;
  }

  m_StateService.StoreDllContainer(c);
}

void StateTrackingLayer::CommandQueueSwapChainRefCountTracker::PreCreateSwapChain(
    unsigned commandQueueKey, ID3D12CommandQueue* commandQueue, unsigned swapChainKey) {
  if (!commandQueue) {
    return;
  }

  commandQueue->AddRef();
  m_RefCountPre = commandQueue->Release();
  m_CommandQueueBySwapChain[swapChainKey] = commandQueueKey;
  m_CommandQueues[commandQueueKey] = commandQueue;
}

void StateTrackingLayer::CommandQueueSwapChainRefCountTracker::PostCreateSwapChain(
    unsigned commandQueueKey, ID3D12CommandQueue* commandQueue, unsigned swapChainKey) {
  if (!commandQueue) {
    return;
  }

  commandQueue->AddRef();
  unsigned refCountPost = commandQueue->Release();
  m_RefCountIncrements[commandQueueKey][swapChainKey] = refCountPost - m_RefCountPre;
}

unsigned StateTrackingLayer::CommandQueueSwapChainRefCountTracker::DestroySwapChain(
    unsigned swapChainKey) {
  const auto it = m_CommandQueueBySwapChain.find(swapChainKey);
  if (it == m_CommandQueueBySwapChain.end()) {
    return 0;
  }

  unsigned commandQueueKey = it->second;
  unsigned refCountIncrement = m_RefCountIncrements[commandQueueKey][swapChainKey];
  ID3D12CommandQueue* commandQueue = m_CommandQueues[commandQueueKey];
  GITS_ASSERT(commandQueue);
  commandQueue->AddRef();
  unsigned refCount = commandQueue->Release();
  if (refCount == refCountIncrement) {
    return commandQueueKey;
  }
  return 0;
}

void StateTrackingLayer::Post(xellD3D12CreateContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != XELL_RESULT_SUCCESS) {
    return;
  }
  XellStateService::ContextState* state = new XellStateService::ContextState();
  state->Key = c.m_out_context.Key;
  state->DeviceKey = c.m_device.Key;
  state->Device = c.m_device.Value;
  m_XellStateService.StoreContextState(state);
}

void StateTrackingLayer::Pre(xellDestroyContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XellStateService::ContextState* state = m_XellStateService.GetContextState(c.m_context.Key);
  state->Device->AddRef();
}

void StateTrackingLayer::Post(xellDestroyContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XellStateService::ContextState* state = m_XellStateService.GetContextState(c.m_context.Key);
  ULONG ref = state->Device->Release();
  if (ref == 0) {
    m_StateService.ReleaseObject(state->DeviceKey, 0);
  }
  m_XellStateService.DestroyContext(c.m_context.Key);
}

void StateTrackingLayer::Post(xellSetSleepModeCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XellStateService::ContextState* state = m_XellStateService.GetContextState(c.m_context.Key);
  state->SleepParams = *c.m_param.Value;
}

void StateTrackingLayer::Post(xellAddMarkerDataCommand& c) {
  if (m_StateRestored) {
    return;
  }
  m_XellStateService.TrackMarker(c.m_context.Key, c.m_frame_id.Value, c.m_marker.Value);
}

void StateTrackingLayer::Post(xefgSwapChainD3D12CreateContextCommand& c) {
  if (m_StateRestored) {
    return;
  }
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  XefgStateService::ContextState* state = new XefgStateService::ContextState();
  state->Key = c.m_phSwapChain.Key;
  state->DeviceKey = c.m_pDevice.Key;
  state->Device = c.m_pDevice.Value;
  m_XefgStateService.StoreContextState(state);
}

void StateTrackingLayer::Pre(xefgSwapChainDestroyCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  state->Device->AddRef();
}

void StateTrackingLayer::Post(xefgSwapChainDestroyCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  ULONG ref = state->Device->Release();
  if (ref == 0) {
    m_StateService.ReleaseObject(state->DeviceKey, 0);
  }
  m_XefgStateService.DestroyContext(c.m_hSwapChain.Key);
}

void StateTrackingLayer::Post(xefgSwapChainSetLatencyReductionCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  state->XellContext = c.m_hXeLLContext;
}

void StateTrackingLayer::Post(xefgSwapChainSetEnabledCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  state->Enabled = c.m_enable.Value;
}

void StateTrackingLayer::Post(xefgSwapChainSetSceneChangeThresholdCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  state->Threshold = c.m_threshold.Value;
}

void StateTrackingLayer::Post(xefgSwapChainD3D12InitFromSwapChainCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  XefgStateService::InitFromSwapChainState initState(c.m_pInitParams);
  initState.CmdQueue = c.m_pCmdQueue.Value;
  initState.CmdQueueKey = c.m_pCmdQueue.Key;
  state->InitFromSwapChainParams.emplace(initState);
}

void StateTrackingLayer::Post(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  XefgStateService::InitFromSwapChainDescState initState(c.m_pInitParams);
  initState.HWnd = c.m_hWnd.Value;
  initState.SwapChainDesc = *c.m_pSwapChainDesc.Value;
  if (c.m_pFullscreenDesc.Value) {
    initState.FullscreenDesc = *c.m_pFullscreenDesc.Value;
  }
  initState.CmdQueue = c.m_pCmdQueue.Value;
  initState.CmdQueueKey = c.m_pCmdQueue.Key;
  initState.DxgiFactoryKey = c.m_pDxgiFactory.Key;
  state->InitFromSwapChainDescParams.emplace(initState);
}

void StateTrackingLayer::Post(xefgSwapChainD3D12GetSwapChainPtrCommand& c) {
  if (m_StateRestored) {
    return;
  }

  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  ObjectState* state = new ObjectState();
  state->Key = c.m_ppSwapChain.Key;
  state->Object = static_cast<IUnknown*>(*c.m_ppSwapChain.Value);
  state->CreationCommand.reset(new xefgSwapChainD3D12GetSwapChainPtrCommand(c));
  m_StateService.StoreState(state);
  m_StateService.SetXefgSwapChainFlag();

  XefgStateService::ContextState* xefgState =
      m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  XefgStateService::SwapChainPtrState swapChainPtrState{};
  swapChainPtrState.Riid = c.m_riid.Value;
  swapChainPtrState.SwapChain = static_cast<IDXGISwapChain*>(*c.m_ppSwapChain.Value);
  swapChainPtrState.SwapChainKey = c.m_ppSwapChain.Key;
  xefgState->SwapChain.emplace(swapChainPtrState);
}

void StateTrackingLayer::Post(xefgSwapChainD3D12SetDescriptorHeapCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  XefgStateService::DescriptorHeapState descriptorHeapState{};
  descriptorHeapState.DescriptorHeapKey = c.m_pDescriptorHeap.Key;
  descriptorHeapState.DescriptorHeapOffsetInBytes = c.m_descriptorHeapOffsetInBytes.Value;
  state->DescriptorHeap.emplace(descriptorHeapState);
}

void StateTrackingLayer::Post(xefgSwapChainEnableDebugFeatureCommand& c) {
  if (m_StateRestored) {
    return;
  }
  XefgStateService::ContextState* state = m_XefgStateService.GetContextState(c.m_hSwapChain.Key);
  XefgStateService::DebugFeatureState debugFeatureState{};
  debugFeatureState.FeatureId = c.m_featureId.Value;
  debugFeatureState.Enable = c.m_enable.Value;
  debugFeatureState.Argument = c.m_pArgument.Value;
  state->DebugFeature.emplace(debugFeatureState);
}

} // namespace DirectX
} // namespace gits
