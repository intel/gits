// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayer.h"
#include "to_string/toStr.h"
#include "log.h"
#include "configurationLib.h"
#include "messageBus.h"
#include "keyUtils.h"
#include <wrl/client.h>

namespace gits {
namespace DirectX {

PortabilityLayer::PortabilityLayer() : Layer("Portability") {
  if (Configurator::IsRecorder() &&
          Configurator::Get().directx.recorder.portability.resourcePlacementStorage ||
      Configurator::IsPlayer() &&
          Configurator::Get().directx.player.portability.resourcePlacement == "store" &&
          Configurator::Get().directx.player.execute) {
    m_StoreResourcePlacementData = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.resourcePlacement == "store" &&
      !Configurator::Get().directx.player.execute) {
    LOG_WARNING << "Portability - storing placement data without execution is experimental";
    m_StoreResourcePlacementDataNoExecute = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.resourcePlacement == "use" &&
      Configurator::Get().directx.player.execute) {
    m_UseResourcePlacementData = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.portabilityChecks &&
      Configurator::Get().directx.player.execute) {
    m_PortabilityChecks = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.portabilityAssertions &&
      Configurator::Get().directx.player.execute && !m_UseResourcePlacementData) {
    m_PortabilityAssertions = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.forcePlacedToCommittedResources &&
      Configurator::Get().directx.player.execute && !m_UseResourcePlacementData) {
    m_ForcePlacedToCommittedResources = true;
  }
  if (Configurator::IsRecorder()) {
    m_AccelerationStructurePadding =
        Configurator::Get().directx.recorder.portability.raytracing.accelerationStructurePadding;
    if (m_AccelerationStructurePadding < 1.0) {
      m_AccelerationStructurePadding = 1.0;
    }
    m_AccelerationStructureScratchPadding =
        Configurator::Get()
            .directx.recorder.portability.raytracing.accelerationStructureScratchPadding;
    if (m_AccelerationStructureScratchPadding < 1.0) {
      m_AccelerationStructureScratchPadding = 1.0;
    }
  }

  if (Configurator::IsRecorder() && m_StoreResourcePlacementData) {
    gits::MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                      [this](Topic t, const MessagePtr& m) {
                                        m_ResourcePlacementCapture.storeResourcePlacement();
                                      });
  }
}

PortabilityLayer::PortabilityLayer(ResourceRegistrationCallback registerResource)
    : PortabilityLayer() {
  m_RegisterResource = std::move(registerResource);
}

PortabilityLayer::~PortabilityLayer() {
  try {
    if (Configurator::IsPlayer()) {
      if (m_StoreResourcePlacementData) {
        m_ResourcePlacementCapture.storeResourcePlacement();
      } else if (m_StoreResourcePlacementDataNoExecute) {
        m_ResourcePlacementCaptureNoExecute.storeResourcePlacement();
      }
    }
  } catch (...) {
    topmost_exception_handler("PortabilityLayer::~PortabilityLayer");
  }
}

void PortabilityLayer::Pre(D3D12CreateDeviceCommand& c) {
  if (!m_PortabilityChecks) {
    return;
  }

  // Check if the minimum feature level is supported and set it to D3D_FEATURE_LEVEL_12_0 if not
  auto hr = D3D12CreateDevice(c.m_pAdapter.Value, c.m_MinimumFeatureLevel.Value, IID_ID3D12Device,
                              nullptr);
  if (hr != S_FALSE) {
    LOG_WARNING << "D3D12CreateDevice - Minimum feature level "
                << toStr(c.m_MinimumFeatureLevel.Value)
                << " is not supported by the adapter. Will set D3D_FEATURE_LEVEL_12_0.";
    c.m_MinimumFeatureLevel.Value = D3D_FEATURE_LEVEL_12_0;
  }
}

void PortabilityLayer::Pre(ID3D12DeviceCreateHeapCommand& c) {
  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.createHeap(c.m_Object.Value, c.m_ppvHeap.Key,
                                           c.m_pDesc.Value->SizeInBytes);
  }
  if (m_PortabilityChecks) {
    ConfigureHeapMemoryPool(c.m_Object.Value, c.m_pDesc.Value);
  }
}

void PortabilityLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (m_PortabilityChecks && c.m_Result.Value != S_OK) {
    CheckHeapCreationFlags(c.m_ppvHeap.Key, c.m_Object.Value, c.m_pDesc.Value);
  }
}

void PortabilityLayer::Pre(ID3D12Device4CreateHeap1Command& c) {
  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.createHeap(c.m_Object.Value, c.m_ppvHeap.Key,
                                           c.m_pDesc.Value->SizeInBytes);
  }
}

void PortabilityLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (m_PortabilityChecks && c.m_Result.Value != S_OK) {
    CheckHeapCreationFlags(c.m_ppvHeap.Key, c.m_Object.Value, c.m_pDesc.Value);
  }
}

void PortabilityLayer::Pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.createPlacedResource(c.m_ppvResource.Key, c.m_HeapOffset.Value);
    return;
  }
  if (m_ForcePlacedToCommittedResources && c.m_pHeap.Value && c.m_pDesc.Value &&
      c.m_ppvResource.Value) {
    const D3D12_HEAP_DESC heapDesc = c.m_pHeap.Value->GetDesc();
    if (!CanReplacePlacedResourceWithCommitted(heapDesc, *c.m_pDesc.Value)) {
      FailPlacedToCommittedIncompatibility("CreatePlacedResource -> CreateCommittedResource", c.Key,
                                           c.m_pHeap.Key, heapDesc, *c.m_pDesc.Value);
      return;
    }

    HRESULT hr = CreateCommittedResource(c.m_pHeap.Value, c.m_pDesc.Value, c.m_InitialState.Value,
                                         c.m_pOptimizedClearValue.Value, c.m_riid.Value,
                                         c.m_ppvResource.Value);
    if (hr == S_OK) {
      c.m_Result.Value = S_OK;
      RegisterForcedPlacedResource(c);
      c.m_pHeap.Value = nullptr;
      c.Skip = true;
    } else {
      FailPlacedToCommittedCreation("CreatePlacedResource -> CreateCommittedResource", c.Key,
                                    c.m_pHeap.Key, c.m_HeapOffset.Value, *c.m_pDesc.Value,
                                    c.m_InitialState.Value, hr);
      return;
    }
  }
  if (m_PortabilityAssertions && !IsStateRestoreKey(c.m_ppvResource.Key)) {
    m_ResourcePlacementAssertions.createPlacedResource(c.m_ppvResource.Key, *c.m_pDesc.Value,
                                                       c.m_Object.Value);
  }
}

void PortabilityLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_ForcePlacedToCommittedResources && c.Skip) {
    m_ForcedCommittedResources.insert(c.m_ppvResource.Key);
  }
  if (m_StoreResourcePlacementData) {
    m_ResourcePlacementCapture.createPlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                                    c.m_HeapOffset.Value, c.m_Object.Value,
                                                    *c.m_pDesc.Value);
  } else if (m_StoreResourcePlacementDataNoExecute) {
    m_ResourcePlacementCaptureNoExecute.createPlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                                             c.m_HeapOffset.Value, c.m_Object.Value,
                                                             *c.m_pDesc.Value);
  }
}

void PortabilityLayer::Pre(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.createPlacedResource(c.m_ppvResource.Key, c.m_HeapOffset.Value);
    return;
  }

  if (m_ForcePlacedToCommittedResources && c.m_pHeap.Value && c.m_pDesc.Value &&
      c.m_ppvResource.Value) {
    const D3D12_HEAP_DESC heapDesc = c.m_pHeap.Value->GetDesc();
    const D3D12_RESOURCE_DESC baseDesc = {
        c.m_pDesc.Value->Dimension, c.m_pDesc.Value->Alignment,        c.m_pDesc.Value->Width,
        c.m_pDesc.Value->Height,    c.m_pDesc.Value->DepthOrArraySize, c.m_pDesc.Value->MipLevels,
        c.m_pDesc.Value->Format,    c.m_pDesc.Value->SampleDesc,       c.m_pDesc.Value->Layout,
        c.m_pDesc.Value->Flags};
    if (!CanReplacePlacedResourceWithCommitted(heapDesc, baseDesc)) {
      FailPlacedToCommittedIncompatibility("CreatePlacedResource1 -> CreateCommittedResource2",
                                           c.Key, c.m_pHeap.Key, heapDesc, baseDesc);
      return;
    }

    HRESULT hr = CreateCommittedResource2(c.m_pHeap.Value, c.m_pDesc.Value, c.m_InitialState.Value,
                                          c.m_pOptimizedClearValue.Value, c.m_riid.Value,
                                          c.m_ppvResource.Value);
    if (hr == S_OK) {
      c.m_Result.Value = S_OK;
      RegisterForcedPlacedResource(c);
      c.m_pHeap.Value = nullptr;
      c.Skip = true;
    } else {
      FailPlacedToCommittedCreation("CreatePlacedResource1 -> CreateCommittedResource2", c.Key,
                                    c.m_pHeap.Key, c.m_HeapOffset.Value, baseDesc,
                                    c.m_InitialState.Value, hr);
      return;
    }
  }
  if (m_PortabilityAssertions && !IsStateRestoreKey(c.m_ppvResource.Key)) {
    m_ResourcePlacementAssertions.createPlacedResource(c.m_ppvResource.Key, *c.m_pDesc.Value,
                                                       c.m_Object.Value);
  }
}

void PortabilityLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_ForcePlacedToCommittedResources && c.Skip) {
    m_ForcedCommittedResources.insert(c.m_ppvResource.Key);
  }
  D3D12_RESOURCE_DESC desc =
      (*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value))->GetDesc();
  if (m_StoreResourcePlacementData) {
    m_ResourcePlacementCapture.createPlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                                    c.m_HeapOffset.Value, c.m_Object.Value, desc);
  } else if (m_StoreResourcePlacementDataNoExecute) {
    m_ResourcePlacementCaptureNoExecute.createPlacedResource(
        c.m_pHeap.Key, c.m_ppvResource.Key, c.m_HeapOffset.Value, c.m_Object.Value, desc);
  }
}

void PortabilityLayer::Pre(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.createPlacedResource(c.m_ppvResource.Key, c.m_HeapOffset.Value);
    return;
  }
  if (m_ForcePlacedToCommittedResources && c.m_pHeap.Value && c.m_pDesc.Value &&
      c.m_ppvResource.Value) {
    const D3D12_HEAP_DESC heapDesc = c.m_pHeap.Value->GetDesc();
    const D3D12_RESOURCE_DESC baseDesc = {
        c.m_pDesc.Value->Dimension, c.m_pDesc.Value->Alignment,        c.m_pDesc.Value->Width,
        c.m_pDesc.Value->Height,    c.m_pDesc.Value->DepthOrArraySize, c.m_pDesc.Value->MipLevels,
        c.m_pDesc.Value->Format,    c.m_pDesc.Value->SampleDesc,       c.m_pDesc.Value->Layout,
        c.m_pDesc.Value->Flags};
    if (!CanReplacePlacedResourceWithCommitted(heapDesc, baseDesc)) {
      FailPlacedToCommittedIncompatibility("CreatePlacedResource2 -> CreateCommittedResource3",
                                           c.Key, c.m_pHeap.Key, heapDesc, baseDesc);
      return;
    }

    HRESULT hr =
        CreateCommittedResource3(c.m_pHeap.Value, c.m_pDesc.Value, c.m_InitialLayout.Value,
                                 c.m_pOptimizedClearValue.Value, c.m_NumCastableFormats.Value,
                                 c.m_pCastableFormats.Value, c.m_riid.Value, c.m_ppvResource.Value);
    if (hr == S_OK) {
      c.m_Result.Value = S_OK;
      RegisterForcedPlacedResource(c);
      c.m_pHeap.Value = nullptr;
      c.Skip = true;
    } else {
      FailPlacedToCommittedCreation("CreatePlacedResource2 -> CreateCommittedResource3", c.Key,
                                    c.m_pHeap.Key, c.m_HeapOffset.Value, baseDesc,
                                    c.m_InitialLayout.Value, hr);
      return;
    }
  }
  if (m_PortabilityAssertions && !IsStateRestoreKey(c.m_ppvResource.Key)) {
    m_ResourcePlacementAssertions.createPlacedResource(c.m_ppvResource.Key, *c.m_pDesc.Value,
                                                       c.m_Object.Value);
  }
}

template <typename CommandType>
void PortabilityLayer::RegisterForcedPlacedResource(CommandType& c) {
  if (m_RegisterResource && c.m_ppvResource.Value && *c.m_ppvResource.Value) {
    m_RegisterResource(c.m_ppvResource.Key, static_cast<ID3D12Resource*>(*c.m_ppvResource.Value));
  }
}

void PortabilityLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_ForcePlacedToCommittedResources && c.Skip) {
    m_ForcedCommittedResources.insert(c.m_ppvResource.Key);
  }
  D3D12_RESOURCE_DESC desc =
      (*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value))->GetDesc();
  if (m_StoreResourcePlacementData) {
    m_ResourcePlacementCapture.createPlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                                    c.m_HeapOffset.Value, c.m_Object.Value, desc);
  } else if (m_StoreResourcePlacementDataNoExecute) {
    m_ResourcePlacementCaptureNoExecute.createPlacedResource(
        c.m_pHeap.Key, c.m_ppvResource.Key, c.m_HeapOffset.Value, c.m_Object.Value, desc);
  }
}

void PortabilityLayer::Post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  c.m_pInfo.Value->ResultDataMaxSizeInBytes *= m_AccelerationStructurePadding;
  c.m_pInfo.Value->ScratchDataSizeInBytes *= m_AccelerationStructureScratchPadding;
  c.m_pInfo.Value->UpdateScratchDataSizeInBytes *= m_AccelerationStructureScratchPadding;
}

void PortabilityLayer::Post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (m_PortabilityChecks) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "Portability - padding in capture post build info in "
                     "EmitRaytracingAccelerationStructurePostbuildInfo not "
                     "supported";
      logged = true;
    }
  }
}

void PortabilityLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_PortabilityChecks && c.m_pPostbuildInfoDescs.Value) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "Portability - padding in capture post build info in "
                     "BuildRaytracingAccelerationStructure not supported";
      logged = true;
    }
  }
}

void PortabilityLayer::Pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  if (m_UseResourcePlacementData) {
    m_ResourcePlacementPlayback.updateTileMappings(c);
  }
}

void PortabilityLayer::Pre(ID3D12DeviceGetResourceAllocationInfoCommand& c) {
  if (c.m_numResourceDescs.Value == 1) {
    if (m_StoreResourcePlacementDataNoExecute) {
      m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
          c.m_pResourceDescs.Value[0], c.m_Result.Value.SizeInBytes, c.m_Result.Value.Alignment);
    }
  }
}

void PortabilityLayer::Pre(ID3D12Device4GetResourceAllocationInfo1Command& c) {
  if (c.m_numResourceDescs.Value == 1) {
    if (m_StoreResourcePlacementDataNoExecute) {
      m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
          c.m_pResourceDescs.Value[0], c.m_Result.Value.SizeInBytes, c.m_Result.Value.Alignment);
    }
  } else if (c.m_pResourceAllocationInfo1.Value) {
    for (unsigned i = 0; i < c.m_numResourceDescs.Value; ++i) {
      if (m_StoreResourcePlacementDataNoExecute) {
        m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
            c.m_pResourceDescs.Value[i], c.m_pResourceAllocationInfo1.Value[i].SizeInBytes,
            c.m_pResourceAllocationInfo1.Value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::Pre(ID3D12Device8GetResourceAllocationInfo2Command& c) {
  if (c.m_numResourceDescs.Value == 1) {
    if (m_StoreResourcePlacementDataNoExecute) {
      m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
          c.m_pResourceDescs.Value[0], c.m_Result.Value.SizeInBytes, c.m_Result.Value.Alignment);
    }
  } else if (c.m_pResourceAllocationInfo1.Value) {
    for (unsigned i = 0; i < c.m_numResourceDescs.Value; ++i) {
      if (m_StoreResourcePlacementDataNoExecute) {
        m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
            c.m_pResourceDescs.Value[i], c.m_pResourceAllocationInfo1.Value[i].SizeInBytes,
            c.m_pResourceAllocationInfo1.Value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::Pre(ID3D12Device12GetResourceAllocationInfo3Command& c) {
  if (c.m_numResourceDescs.Value == 1) {
    if (m_StoreResourcePlacementDataNoExecute) {
      m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
          c.m_pResourceDescs.Value[0], c.m_Result.Value.SizeInBytes, c.m_Result.Value.Alignment);
    }
  } else if (c.m_pResourceAllocationInfo1.Value) {
    for (unsigned i = 0; i < c.m_numResourceDescs.Value; ++i) {
      if (m_StoreResourcePlacementDataNoExecute) {
        m_ResourcePlacementCaptureNoExecute.getResourceAllocation(
            c.m_pResourceDescs.Value[i], c.m_pResourceAllocationInfo1.Value[i].SizeInBytes,
            c.m_pResourceAllocationInfo1.Value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::Pre(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (!m_ForcePlacedToCommittedResources || !c.m_NumBarriers.Value || !c.m_pBarriers.Value) {
    return;
  }

  UINT originalCount = c.m_NumBarriers.Value;
  UINT newCount = 0;

  for (UINT i = 0; i < originalCount; ++i) {
    if (c.m_pBarriers.Value[i].Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
      continue;
    }
    if (newCount != i) {
      c.m_pBarriers.Value[newCount] = c.m_pBarriers.Value[i];
    }
    ++newCount;
  }

  if (newCount == 0) {
    c.Skip = true;
  } else {
    c.m_NumBarriers.Value = newCount;
  }
}

void PortabilityLayer::ConfigureHeapMemoryPool(ID3D12Device* device, D3D12_HEAP_DESC* heapDesc) {
  D3D12_FEATURE_DATA_ARCHITECTURE1 architectureInfo{0};
  HRESULT result = device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &architectureInfo,
                                               sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
  if (result != S_OK || !architectureInfo.UMA) {
    return;
  }

  D3D12_MEMORY_POOL preferredPool = (heapDesc->Properties.Type == D3D12_HEAP_TYPE_CUSTOM)
                                        ? D3D12_MEMORY_POOL_L0
                                        : D3D12_MEMORY_POOL_UNKNOWN;

  if (heapDesc->Properties.MemoryPoolPreference != preferredPool) {
    heapDesc->Properties.MemoryPoolPreference = preferredPool;
  }
}

void PortabilityLayer::CheckHeapCreationFlags(unsigned heapKey,
                                              ID3D12Device* device,
                                              D3D12_HEAP_DESC* desc) {
  D3D12_FEATURE_DATA_D3D12_OPTIONS featureOptions{};
  HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOptions,
                                           sizeof(featureOptions));
  if (FAILED(hr)) {
    LOG_ERROR << "Portability - Failed to CheckFeatureSupport (D3D12_FEATURE_D3D12_OPTIONS)";
    return;
  }

  if (featureOptions.ResourceHeapTier != 1) {
    return;
  }

  // Stream not supported for current GPU/GPU driver
  GITS_ASSERT(desc->Flags != D3D12_HEAP_FLAG_NONE,
              "Resource heap creation flag is D3D12_HEAP_FLAG_NONE");

  const D3D12_HEAP_FLAGS allowFlags[] = {D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
                                         D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES,
                                         D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES};
  if (std::find(std::begin(allowFlags), std::end(allowFlags), desc->Flags) !=
      std::end(allowFlags)) {
    return;
  }

  const D3D12_HEAP_FLAGS denyFlags[] = {
      D3D12_HEAP_FLAG_DENY_BUFFERS,
      D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES,
      D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
  };
  unsigned denyFlagsCount =
      std::count_if(std::begin(denyFlags), std::end(denyFlags),
                    [desc](int element) { return (desc->Flags & element) == element; });
  if (denyFlagsCount >= 2) {
    return;
  }

  // Stream not supported for current GPU/GPU driver
  GITS_ASSERT(0, "Resource heap creation flag inappropriate for available resource heap tiers");
}

HRESULT PortabilityLayer::CreateCommittedResource(ID3D12Heap* heap,
                                                  const D3D12_RESOURCE_DESC* desc,
                                                  D3D12_RESOURCE_STATES initialState,
                                                  const D3D12_CLEAR_VALUE* clearValue,
                                                  REFIID riid,
                                                  void** ppvResource) {
  if (!heap || !desc || !ppvResource) {
    return E_INVALIDARG;
  }

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = heap->GetDevice(IID_PPV_ARGS(&device));
  if (FAILED(hr) || !device) {
    return FAILED(hr) ? hr : E_FAIL;
  }

  const D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC adjustedDesc = *desc;
  AdjustResourceFlagsForCommitted(heapDesc, adjustedDesc, adjustedDesc.Flags);

  D3D12_HEAP_FLAGS committedHeapFlags = GetCompatibleHeapFlags(heapDesc, adjustedDesc);

  D3D12_HEAP_PROPERTIES Properties =
      GetCompatibleHeapProperties(heapDesc, adjustedDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      GetCompatibleInitialState(Properties, adjustedDesc, initialState);
  const D3D12_CLEAR_VALUE* clearValueAdjusted = GetCompatibleClearValue(adjustedDesc, clearValue);

  hr = device->CreateCommittedResource(&Properties, committedHeapFlags, &adjustedDesc,
                                       creationState, clearValueAdjusted, riid, ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "CreateCommittedResource: CreateCommittedResource failed 0x" << std::hex << hr
              << std::dec << " | HeapType=" << Properties.Type
              << " CPUPageProperty=" << Properties.CPUPageProperty
              << " MemoryPoolPreference=" << Properties.MemoryPoolPreference
              << " OriginalHeapFlags=0x" << std::hex << heapDesc.Flags << " CommittedHeapFlags=0x"
              << committedHeapFlags << std::dec << " | Dimension=" << adjustedDesc.Dimension
              << " Format=" << adjustedDesc.Format << " Width=" << adjustedDesc.Width
              << " Height=" << adjustedDesc.Height
              << " DepthOrArraySize=" << adjustedDesc.DepthOrArraySize
              << " MipLevels=" << adjustedDesc.MipLevels << " Alignment=" << adjustedDesc.Alignment
              << " Layout=" << adjustedDesc.Layout << " OriginalResourceFlags=0x" << std::hex
              << desc->Flags << " AdjustedResourceFlags=0x" << adjustedDesc.Flags << std::dec
              << " SampleCount=" << adjustedDesc.SampleDesc.Count
              << " SampleQuality=" << adjustedDesc.SampleDesc.Quality << " | initialState=0x"
              << std::hex << initialState << " creationState=0x" << creationState << std::dec
              << " | clearValue=" << (clearValueAdjusted ? "yes" : "null");
    return hr;
  }

  if (creationState != initialState && *ppvResource) {
    TransitionResource(device.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                       D3D12_RESOURCE_STATE_COMMON, initialState);
  }

  return S_OK;
}

HRESULT PortabilityLayer::CreateCommittedResource2(ID3D12Heap* heap,
                                                   const D3D12_RESOURCE_DESC1* desc,
                                                   D3D12_RESOURCE_STATES initialState,
                                                   const D3D12_CLEAR_VALUE* clearValue,
                                                   REFIID riid,
                                                   void** ppvResource) {
  if (!heap || !desc || !ppvResource) {
    return E_INVALIDARG;
  }

  Microsoft::WRL::ComPtr<ID3D12Device8> device8;
  HRESULT hr = heap->GetDevice(IID_PPV_ARGS(&device8));
  if (FAILED(hr) || !device8) {
    return FAILED(hr) ? hr : E_FAIL;
  }

  const D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC1 adjustedDesc = *desc;
  D3D12_RESOURCE_DESC baseDesc = {
      adjustedDesc.Dimension, adjustedDesc.Alignment,        adjustedDesc.Width,
      adjustedDesc.Height,    adjustedDesc.DepthOrArraySize, adjustedDesc.MipLevels,
      adjustedDesc.Format,    adjustedDesc.SampleDesc,       adjustedDesc.Layout,
      adjustedDesc.Flags};
  AdjustResourceFlagsForCommitted(heapDesc, baseDesc, adjustedDesc.Flags);
  baseDesc.Flags = adjustedDesc.Flags;

  D3D12_HEAP_FLAGS committedHeapFlags = GetCompatibleHeapFlags(heapDesc, baseDesc);
  D3D12_HEAP_PROPERTIES properties = GetCompatibleHeapProperties(heapDesc, baseDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      GetCompatibleInitialState(properties, baseDesc, initialState);
  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
      properties.Type == D3D12_HEAP_TYPE_DEFAULT && creationState != D3D12_RESOURCE_STATE_COMMON) {
    creationState = D3D12_RESOURCE_STATE_COMMON;
  }
  const D3D12_CLEAR_VALUE* clearValueAdjusted = GetCompatibleClearValue(baseDesc, clearValue);

  hr = device8->CreateCommittedResource2(&properties, committedHeapFlags, &adjustedDesc,
                                         creationState, clearValueAdjusted, nullptr, riid,
                                         ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "CreateCommittedResource2: CreateCommittedResource2 failed 0x" << std::hex << hr
              << std::dec << " | HeapType=" << properties.Type
              << " CPUPageProperty=" << properties.CPUPageProperty
              << " MemoryPoolPreference=" << properties.MemoryPoolPreference
              << " OriginalHeapFlags=0x" << std::hex << heapDesc.Flags << " CommittedHeapFlags=0x"
              << committedHeapFlags << std::dec << " | Dimension=" << adjustedDesc.Dimension
              << " Format=" << adjustedDesc.Format << " Width=" << adjustedDesc.Width
              << " Height=" << adjustedDesc.Height
              << " DepthOrArraySize=" << adjustedDesc.DepthOrArraySize
              << " MipLevels=" << adjustedDesc.MipLevels << " Alignment=" << adjustedDesc.Alignment
              << " Layout=" << adjustedDesc.Layout << " OriginalResourceFlags=0x" << std::hex
              << desc->Flags << " AdjustedResourceFlags=0x" << adjustedDesc.Flags << std::dec
              << " SampleCount=" << adjustedDesc.SampleDesc.Count
              << " SampleQuality=" << adjustedDesc.SampleDesc.Quality << " | initialState=0x"
              << std::hex << initialState << " creationState=0x" << creationState << std::dec
              << " | clearValue=" << (clearValueAdjusted ? "yes" : "null");
    return hr;
  }

  if (creationState != initialState && *ppvResource) {
    TransitionResource(device8.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                       D3D12_RESOURCE_STATE_COMMON, initialState);
  }

  return S_OK;
}

HRESULT PortabilityLayer::CreateCommittedResource3(ID3D12Heap* heap,
                                                   const D3D12_RESOURCE_DESC1* desc,
                                                   D3D12_BARRIER_LAYOUT initialLayout,
                                                   const D3D12_CLEAR_VALUE* clearValue,
                                                   UINT32 numCastableFormats,
                                                   const DXGI_FORMAT* pCastableFormats,
                                                   REFIID riid,
                                                   void** ppvResource) {
  if (!heap || !desc || !ppvResource) {
    return E_INVALIDARG;
  }

  Microsoft::WRL::ComPtr<ID3D12Device10> device10;
  HRESULT hr = heap->GetDevice(IID_PPV_ARGS(&device10));
  if (FAILED(hr) || !device10) {
    return FAILED(hr) ? hr : E_FAIL;
  }

  const D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC1 adjustedDesc = *desc;
  D3D12_RESOURCE_DESC baseDesc = {
      adjustedDesc.Dimension, adjustedDesc.Alignment,        adjustedDesc.Width,
      adjustedDesc.Height,    adjustedDesc.DepthOrArraySize, adjustedDesc.MipLevels,
      adjustedDesc.Format,    adjustedDesc.SampleDesc,       adjustedDesc.Layout,
      adjustedDesc.Flags};
  AdjustResourceFlagsForCommitted(heapDesc, baseDesc, adjustedDesc.Flags);
  baseDesc.Flags = adjustedDesc.Flags;

  D3D12_RESOURCE_STATES initialState = BarrierLayoutToResourceState(initialLayout);
  D3D12_HEAP_FLAGS committedHeapFlags = GetCompatibleHeapFlags(heapDesc, baseDesc);
  D3D12_HEAP_PROPERTIES properties = GetCompatibleHeapProperties(heapDesc, baseDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      GetCompatibleInitialState(properties, baseDesc, initialState);

  D3D12_BARRIER_LAYOUT creationLayout = initialLayout;
  if (creationState != initialState) {
    creationLayout = D3D12_BARRIER_LAYOUT_COMMON;
  }
  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
      properties.Type == D3D12_HEAP_TYPE_DEFAULT && creationLayout != D3D12_BARRIER_LAYOUT_COMMON) {
    creationLayout = D3D12_BARRIER_LAYOUT_COMMON;
  }

  const D3D12_CLEAR_VALUE* clearValueAdjusted = GetCompatibleClearValue(baseDesc, clearValue);

  hr = device10->CreateCommittedResource3(&properties, committedHeapFlags, &adjustedDesc,
                                          creationLayout, clearValueAdjusted, nullptr,
                                          numCastableFormats, pCastableFormats, riid, ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "CreateCommittedResource3: CreateCommittedResource3 failed 0x" << std::hex << hr
              << std::dec << " | HeapType=" << properties.Type
              << " CPUPageProperty=" << properties.CPUPageProperty
              << " MemoryPoolPreference=" << properties.MemoryPoolPreference
              << " OriginalHeapFlags=0x" << std::hex << heapDesc.Flags << " CommittedHeapFlags=0x"
              << committedHeapFlags << std::dec << " | Dimension=" << adjustedDesc.Dimension
              << " Format=" << adjustedDesc.Format << " Width=" << adjustedDesc.Width
              << " Height=" << adjustedDesc.Height
              << " DepthOrArraySize=" << adjustedDesc.DepthOrArraySize
              << " MipLevels=" << adjustedDesc.MipLevels << " Alignment=" << adjustedDesc.Alignment
              << " Layout=" << adjustedDesc.Layout << " OriginalResourceFlags=0x" << std::hex
              << desc->Flags << " AdjustedResourceFlags=0x" << adjustedDesc.Flags << std::dec
              << " SampleCount=" << adjustedDesc.SampleDesc.Count
              << " SampleQuality=" << adjustedDesc.SampleDesc.Quality
              << " | initialLayout=" << static_cast<int>(initialLayout)
              << " creationLayout=" << static_cast<int>(creationLayout)
              << " numCastableFormats=" << numCastableFormats
              << " | clearValue=" << (clearValueAdjusted ? "yes" : "null");
    return hr;
  }

  if (creationLayout != initialLayout && *ppvResource) {
    D3D12_RESOURCE_STATES targetState = BarrierLayoutToResourceState(initialLayout);
    if (targetState != D3D12_RESOURCE_STATE_COMMON) {
      TransitionResource(device10.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                         D3D12_RESOURCE_STATE_COMMON, targetState);
    }
  }

  return S_OK;
}

D3D12_RESOURCE_STATES PortabilityLayer::BarrierLayoutToResourceState(D3D12_BARRIER_LAYOUT layout) {
  switch (layout) {
  case D3D12_BARRIER_LAYOUT_COMMON:
    return D3D12_RESOURCE_STATE_COMMON;
  case D3D12_BARRIER_LAYOUT_GENERIC_READ:
    return D3D12_RESOURCE_STATE_GENERIC_READ;
  case D3D12_BARRIER_LAYOUT_RENDER_TARGET:
    return D3D12_RESOURCE_STATE_RENDER_TARGET;
  case D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS:
    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE:
    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
  case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ:
    return D3D12_RESOURCE_STATE_DEPTH_READ;
  case D3D12_BARRIER_LAYOUT_SHADER_RESOURCE:
    return static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  case D3D12_BARRIER_LAYOUT_COPY_SOURCE:
    return D3D12_RESOURCE_STATE_COPY_SOURCE;
  case D3D12_BARRIER_LAYOUT_COPY_DEST:
    return D3D12_RESOURCE_STATE_COPY_DEST;
  case D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE:
    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
  case D3D12_BARRIER_LAYOUT_RESOLVE_DEST:
    return D3D12_RESOURCE_STATE_RESOLVE_DEST;
  case D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE:
    return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
  default:
    LOG_WARNING << "BarrierLayoutToResourceState: unhandled layout " << static_cast<int>(layout);
    return D3D12_RESOURCE_STATE_COMMON;
  }
}

static inline bool IsBuffer(const D3D12_RESOURCE_DESC& d) {
  return d.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
}

static inline bool IsTexture(const D3D12_RESOURCE_DESC& d) {
  return d.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER;
}

static inline bool IsRTorDS(const D3D12_RESOURCE_DESC& d) {
  return (d.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
         (d.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
}

static inline bool IsCrossAdapterCandidate(const D3D12_RESOURCE_DESC& d) {
  return d.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
         d.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
}

static inline bool IsDisplayableCandidate(const D3D12_RESOURCE_DESC& d) {
  return d.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && d.MipLevels == 1 &&
         d.SampleDesc.Count == 1 && d.SampleDesc.Quality == 0 &&
         d.Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN &&
         !(d.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) &&
         !(d.Flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);
}

D3D12_HEAP_FLAGS PortabilityLayer::GetCompatibleHeapFlags(const D3D12_HEAP_DESC& heapDesc,
                                                          const D3D12_RESOURCE_DESC& adjustedDesc) {
  D3D12_HEAP_FLAGS flags = heapDesc.Flags;

  flags = static_cast<D3D12_HEAP_FLAGS>(
      flags &
      ~(D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS |
        D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT |
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED | D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS |
        D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH));

  if (flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER) {
    if (!IsCrossAdapterCandidate(adjustedDesc)) {
      flags = static_cast<D3D12_HEAP_FLAGS>(
          flags & ~(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED));
    } else {
      flags = static_cast<D3D12_HEAP_FLAGS>((flags | D3D12_HEAP_FLAG_SHARED) &
                                            ~D3D12_HEAP_FLAG_ALLOW_DISPLAY);
    }
  }

  if (flags & D3D12_HEAP_FLAG_ALLOW_DISPLAY) {
    if (!IsDisplayableCandidate(adjustedDesc)) {
      flags = static_cast<D3D12_HEAP_FLAGS>(flags & ~D3D12_HEAP_FLAG_ALLOW_DISPLAY);
    }
  }

  return flags;
}

void PortabilityLayer::AdjustResourceFlagsForCommitted(const D3D12_HEAP_DESC& heapDesc,
                                                       const D3D12_RESOURCE_DESC& originalDesc,
                                                       D3D12_RESOURCE_FLAGS& resourceFlags) {
  const auto heapFlags = heapDesc.Flags;

  if (!(heapFlags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER) ||
      !IsCrossAdapterCandidate(originalDesc)) {
    resourceFlags =
        static_cast<D3D12_RESOURCE_FLAGS>(resourceFlags & ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);
  }

  if (heapFlags & D3D12_HEAP_FLAG_ALLOW_DISPLAY) {
    if (!IsDisplayableCandidate(originalDesc)) {
      resourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(resourceFlags &
                                                        ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER &
                                                        ~D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    }
  }

  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD ||
      heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK) {
    resourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(
        resourceFlags & ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS &
        ~D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & ~D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
  }

  if (IsBuffer(originalDesc)) {
    resourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(
        resourceFlags & ~D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
  }

  if ((resourceFlags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) &&
      !(resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
    resourceFlags = static_cast<D3D12_RESOURCE_FLAGS>(resourceFlags &
                                                      ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
  }
}

D3D12_HEAP_PROPERTIES PortabilityLayer::GetCompatibleHeapProperties(
    const D3D12_HEAP_DESC& heapDesc,
    const D3D12_RESOURCE_DESC& adjustedDesc,
    D3D12_RESOURCE_STATES creationState) {
  D3D12_HEAP_PROPERTIES hp = heapDesc.Properties;

  if (hp.CreationNodeMask == 0) {
    hp.CreationNodeMask = 1;
  }
  if (hp.VisibleNodeMask == 0) {
    hp.VisibleNodeMask = hp.CreationNodeMask;
  }

  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    return hp;
  }

  if ((creationState & D3D12_RESOURCE_STATE_COPY_DEST) != 0) {
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    return hp;
  }

  if (hp.Type == D3D12_HEAP_TYPE_CUSTOM) {
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  }

  return hp;
}

D3D12_RESOURCE_STATES PortabilityLayer::GetCompatibleInitialState(
    const D3D12_HEAP_PROPERTIES& heapProps,
    const D3D12_RESOURCE_DESC& adjustedDesc,
    D3D12_RESOURCE_STATES originalInitialState) {
  switch (heapProps.Type) {
  case D3D12_HEAP_TYPE_UPLOAD:
    return D3D12_RESOURCE_STATE_GENERIC_READ;

  case D3D12_HEAP_TYPE_READBACK:
    return D3D12_RESOURCE_STATE_COPY_DEST;

  case D3D12_HEAP_TYPE_DEFAULT:
  default:
    break;
  }

  if (originalInitialState == 0) {
    return D3D12_RESOURCE_STATE_COMMON;
  }

  return originalInitialState;
}

const D3D12_CLEAR_VALUE* PortabilityLayer::GetCompatibleClearValue(
    const D3D12_RESOURCE_DESC& adjustedDesc, const D3D12_CLEAR_VALUE* originalClearValue) {
  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return nullptr;
  }

  const bool isRT = (adjustedDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0;
  const bool isDS = (adjustedDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;

  if (!isRT && !isDS) {
    return nullptr;
  }

  if (!originalClearValue) {
    return nullptr;
  }

  if (originalClearValue->Format != adjustedDesc.Format) {
    return nullptr;
  }

  return originalClearValue;
}

void PortabilityLayer::TransitionResource(ID3D12Device* device,
                                          ID3D12Resource* resource,
                                          D3D12_RESOURCE_STATES stateBefore,
                                          D3D12_RESOURCE_STATES stateAfter) {
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
  if (FAILED(hr)) {
    LOG_WARNING << "TransitionResource: CreateCommandQueue failed 0x" << std::hex << hr;
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
  if (FAILED(hr)) {
    LOG_WARNING << "TransitionResource: CreateCommandAllocator failed 0x" << std::hex << hr;
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                 IID_PPV_ARGS(&cmdList));
  if (FAILED(hr)) {
    LOG_WARNING << "TransitionResource: CreateCommandList failed 0x" << std::hex << hr;
    return;
  }

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = stateBefore;
  barrier.Transition.StateAfter = stateAfter;
  cmdList->ResourceBarrier(1, &barrier);
  cmdList->Close();

  ID3D12CommandList* ppCommandLists[] = {cmdList.Get()};
  queue->ExecuteCommandLists(1, ppCommandLists);

  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  if (FAILED(hr)) {
    LOG_WARNING << "TransitionResource: CreateFence failed 0x" << std::hex << hr;
    return;
  }

  queue->Signal(fence.Get(), 1);
  if (fence->GetCompletedValue() < 1) {
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }
}

std::string PortabilityLayer::GetPlacedToCommittedIncompatibilityReasons(
    const D3D12_HEAP_DESC& heapDesc, const D3D12_RESOURCE_DESC& resourceDesc) const {
  std::string reasons;

  if ((resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER) &&
      resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    reasons += "cross-adapter buffers cannot be recreated with committed resources";
  }

  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM &&
      (heapDesc.Flags & D3D12_HEAP_FLAG_SHARED) &&
      heapDesc.Properties.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE) {
    if (!reasons.empty()) {
      reasons += "; ";
    }
    reasons +=
        "shared CUSTOM heap with CPU-visible pages cannot be recreated with committed resources";
  }

  if (reasons.empty()) {
    reasons = "unknown incompatibility";
  }

  return reasons;
}

std::string PortabilityLayer::GetPlacedToCommittedFailureContext(
    const char* apiName,
    unsigned commandKey,
    unsigned heapKey,
    const D3D12_HEAP_DESC& heapDesc,
    const D3D12_RESOURCE_DESC& resourceDesc) const {
  std::ostringstream stream;
  stream << apiName << " | Command " << commandKey << " heapKey=" << heapKey
         << " HeapType=" << heapDesc.Properties.Type
         << " CPUPageProperty=" << heapDesc.Properties.CPUPageProperty
         << " MemoryPoolPreference=" << heapDesc.Properties.MemoryPoolPreference << " HeapFlags=0x"
         << std::hex << heapDesc.Flags << " ResourceFlags=0x" << resourceDesc.Flags << std::dec
         << " Dimension=" << resourceDesc.Dimension;
  return stream.str();
}

void PortabilityLayer::FailPlacedToCommittedIncompatibility(
    const char* apiName,
    unsigned commandKey,
    unsigned heapKey,
    const D3D12_HEAP_DESC& heapDesc,
    const D3D12_RESOURCE_DESC& resourceDesc) const {
  std::ostringstream stream;
  stream << "Forced placed->committed replacement failed. Reasons: "
         << GetPlacedToCommittedIncompatibilityReasons(heapDesc, resourceDesc) << " | "
         << GetPlacedToCommittedFailureContext(apiName, commandKey, heapKey, heapDesc,
                                               resourceDesc);
  const std::string message = stream.str();
  LOG_ERROR << message;
  GITS_ASSERT(false, message.c_str());
}

void PortabilityLayer::FailPlacedToCommittedCreation(const char* apiName,
                                                     unsigned commandKey,
                                                     unsigned heapKey,
                                                     UINT64 heapOffset,
                                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                                     D3D12_RESOURCE_STATES initialState,
                                                     HRESULT hr) const {
  std::ostringstream stream;
  stream << "Forced placed->committed replacement failed. CreateCommittedResource* returned 0x"
         << std::hex << hr << std::dec << " | " << apiName << " | Command " << commandKey
         << " heapKey=" << heapKey << " heapOffset=" << heapOffset << " ResourceFlags=0x"
         << std::hex << resourceDesc.Flags << std::dec << " Dimension=" << resourceDesc.Dimension
         << " initialState=0x" << std::hex << initialState << std::dec;
  const std::string message = stream.str();
  LOG_ERROR << message;
  GITS_ASSERT(false, message.c_str());
}

void PortabilityLayer::FailPlacedToCommittedCreation(const char* apiName,
                                                     unsigned commandKey,
                                                     unsigned heapKey,
                                                     UINT64 heapOffset,
                                                     const D3D12_RESOURCE_DESC& resourceDesc,
                                                     D3D12_BARRIER_LAYOUT initialLayout,
                                                     HRESULT hr) const {
  std::ostringstream stream;
  stream << "Forced placed->committed replacement failed. CreateCommittedResource* returned 0x"
         << std::hex << hr << std::dec << " | " << apiName << " | Command " << commandKey
         << " heapKey=" << heapKey << " heapOffset=" << heapOffset << " ResourceFlags=0x"
         << std::hex << resourceDesc.Flags << std::dec << " Dimension=" << resourceDesc.Dimension
         << " initialLayout=" << static_cast<int>(initialLayout);
  const std::string message = stream.str();
  LOG_ERROR << message;
  GITS_ASSERT(false, message.c_str());
}

bool PortabilityLayer::CanReplacePlacedResourceWithCommitted(
    const D3D12_HEAP_DESC& heapDesc, const D3D12_RESOURCE_DESC& resourceDesc) const {

  if ((resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER) &&
      resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return false;
  }

  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM &&
      (heapDesc.Flags & D3D12_HEAP_FLAG_SHARED) &&
      heapDesc.Properties.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE) {
    return false;
  }

  return true;
}

} // namespace DirectX
} // namespace gits
