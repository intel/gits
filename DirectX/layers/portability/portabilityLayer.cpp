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
    storeResourcePlacementData_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.resourcePlacement == "store" &&
      !Configurator::Get().directx.player.execute) {
    LOG_WARNING << "Portability - storing placement data without execution is experimental";
    storeResourcePlacementDataNoExecute_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.resourcePlacement == "use" &&
      Configurator::Get().directx.player.execute) {
    useResourcePlacementData_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.portabilityChecks &&
      Configurator::Get().directx.player.execute) {
    portabilityChecks_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.portabilityAssertions &&
      Configurator::Get().directx.player.execute && !useResourcePlacementData_) {
    portabilityAssertions_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.forcePlacedToCommittedResources &&
      Configurator::Get().directx.player.execute && !useResourcePlacementData_) {
    forcePlacedToCommittedResources_ = true;
  }
  if (Configurator::IsRecorder()) {
    accelerationStructurePadding_ =
        Configurator::Get().directx.recorder.portability.raytracing.accelerationStructurePadding;
    if (accelerationStructurePadding_ < 1.0) {
      accelerationStructurePadding_ = 1.0;
    }
    accelerationStructureScratchPadding_ =
        Configurator::Get()
            .directx.recorder.portability.raytracing.accelerationStructureScratchPadding;
    if (accelerationStructureScratchPadding_ < 1.0) {
      accelerationStructureScratchPadding_ = 1.0;
    }
  }

  if (Configurator::IsRecorder() && storeResourcePlacementData_) {
    gits::MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                      [this](Topic t, const MessagePtr& m) {
                                        resourcePlacementCapture_.storeResourcePlacement();
                                      });
  }
}

PortabilityLayer::PortabilityLayer(ResourceRegistrationCallback registerResource)
    : PortabilityLayer() {
  registerResource_ = std::move(registerResource);
}

PortabilityLayer::~PortabilityLayer() {
  try {
    if (Configurator::IsPlayer()) {
      if (storeResourcePlacementData_) {
        resourcePlacementCapture_.storeResourcePlacement();
      } else if (storeResourcePlacementDataNoExecute_) {
        resourcePlacementCaptureNoExecute_.storeResourcePlacement();
      }
    }
  } catch (...) {
    topmost_exception_handler("PortabilityLayer::~PortabilityLayer");
  }
}

void PortabilityLayer::pre(D3D12CreateDeviceCommand& c) {
  if (!portabilityChecks_) {
    return;
  }

  // Check if the minimum feature level is supported and set it to D3D_FEATURE_LEVEL_12_0 if not
  auto hr =
      D3D12CreateDevice(c.pAdapter_.value, c.MinimumFeatureLevel_.value, IID_ID3D12Device, nullptr);
  if (hr != S_FALSE) {
    LOG_WARNING << "D3D12CreateDevice - Minimum feature level "
                << toStr(c.MinimumFeatureLevel_.value)
                << " is not supported by the adapter. Will set D3D_FEATURE_LEVEL_12_0.";
    c.MinimumFeatureLevel_.value = D3D_FEATURE_LEVEL_12_0;
  }
}

void PortabilityLayer::pre(ID3D12DeviceCreateHeapCommand& c) {
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createHeap(c.object_.value, c.ppvHeap_.key,
                                          c.pDesc_.value->SizeInBytes);
  }
  if (portabilityChecks_) {
    configureHeapMemoryPool(c.object_.value, c.pDesc_.value);
  }
}

void PortabilityLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (portabilityChecks_ && c.result_.value != S_OK) {
    checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value);
  }
}

void PortabilityLayer::pre(ID3D12Device4CreateHeap1Command& c) {
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createHeap(c.object_.value, c.ppvHeap_.key,
                                          c.pDesc_.value->SizeInBytes);
  }
}

void PortabilityLayer::post(ID3D12Device4CreateHeap1Command& c) {
  if (portabilityChecks_ && c.result_.value != S_OK) {
    checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value);
  }
}

void PortabilityLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
    return;
  }
  if (forcePlacedToCommittedResources_ && c.pHeap_.value && c.pDesc_.value &&
      c.ppvResource_.value) {
    const D3D12_HEAP_DESC heapDesc = c.pHeap_.value->GetDesc();
    if (!canReplacePlacedResourceWithCommitted(heapDesc, *c.pDesc_.value)) {
      failPlacedToCommittedIncompatibility("CreatePlacedResource -> CreateCommittedResource", c.key,
                                           c.pHeap_.key, heapDesc, *c.pDesc_.value);
      return;
    }

    HRESULT hr =
        createCommittedResource(c.pHeap_.value, c.pDesc_.value, c.InitialState_.value,
                                c.pOptimizedClearValue_.value, c.riid_.value, c.ppvResource_.value);
    if (hr == S_OK) {
      c.result_.value = S_OK;
      registerForcedPlacedResource(c);
      c.pHeap_.value = nullptr;
      c.skip = true;
    } else {
      failPlacedToCommittedCreation("CreatePlacedResource -> CreateCommittedResource", c.key,
                                    c.pHeap_.key, c.HeapOffset_.value, *c.pDesc_.value,
                                    c.InitialState_.value, hr);
      return;
    }
  }
  if (portabilityAssertions_ && !isStateRestoreKey(c.ppvResource_.key)) {
    resourcePlacementAssertions_.createPlacedResource(c.ppvResource_.key, *c.pDesc_.value,
                                                      c.object_.value);
  }
}

void PortabilityLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (forcePlacedToCommittedResources_ && c.skip) {
    forcedCommittedResources_.insert(c.ppvResource_.key);
  }
  if (storeResourcePlacementData_) {
    resourcePlacementCapture_.createPlacedResource(
        c.pHeap_.key, c.ppvResource_.key, c.HeapOffset_.value, c.object_.value, *c.pDesc_.value);
  } else if (storeResourcePlacementDataNoExecute_) {
    resourcePlacementCaptureNoExecute_.createPlacedResource(
        c.pHeap_.key, c.ppvResource_.key, c.HeapOffset_.value, c.object_.value, *c.pDesc_.value);
  }
}

void PortabilityLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
    return;
  }

  if (forcePlacedToCommittedResources_ && c.pHeap_.value && c.pDesc_.value &&
      c.ppvResource_.value) {
    const D3D12_HEAP_DESC heapDesc = c.pHeap_.value->GetDesc();
    const D3D12_RESOURCE_DESC baseDesc = {
        c.pDesc_.value->Dimension, c.pDesc_.value->Alignment,        c.pDesc_.value->Width,
        c.pDesc_.value->Height,    c.pDesc_.value->DepthOrArraySize, c.pDesc_.value->MipLevels,
        c.pDesc_.value->Format,    c.pDesc_.value->SampleDesc,       c.pDesc_.value->Layout,
        c.pDesc_.value->Flags};
    if (!canReplacePlacedResourceWithCommitted(heapDesc, baseDesc)) {
      failPlacedToCommittedIncompatibility("CreatePlacedResource1 -> CreateCommittedResource2",
                                           c.key, c.pHeap_.key, heapDesc, baseDesc);
      return;
    }

    HRESULT hr = createCommittedResource2(c.pHeap_.value, c.pDesc_.value, c.InitialState_.value,
                                          c.pOptimizedClearValue_.value, c.riid_.value,
                                          c.ppvResource_.value);
    if (hr == S_OK) {
      c.result_.value = S_OK;
      registerForcedPlacedResource(c);
      c.pHeap_.value = nullptr;
      c.skip = true;
    } else {
      failPlacedToCommittedCreation("CreatePlacedResource1 -> CreateCommittedResource2", c.key,
                                    c.pHeap_.key, c.HeapOffset_.value, baseDesc,
                                    c.InitialState_.value, hr);
      return;
    }
  }
  if (portabilityAssertions_ && !isStateRestoreKey(c.ppvResource_.key)) {
    resourcePlacementAssertions_.createPlacedResource(c.ppvResource_.key, *c.pDesc_.value,
                                                      c.object_.value);
  }
}

void PortabilityLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (forcePlacedToCommittedResources_ && c.skip) {
    forcedCommittedResources_.insert(c.ppvResource_.key);
  }
  D3D12_RESOURCE_DESC desc = (*reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value))->GetDesc();
  if (storeResourcePlacementData_) {
    resourcePlacementCapture_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                   c.HeapOffset_.value, c.object_.value, desc);
  } else if (storeResourcePlacementDataNoExecute_) {
    resourcePlacementCaptureNoExecute_.createPlacedResource(
        c.pHeap_.key, c.ppvResource_.key, c.HeapOffset_.value, c.object_.value, desc);
  }
}

void PortabilityLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
    return;
  }
  if (forcePlacedToCommittedResources_ && c.pHeap_.value && c.pDesc_.value &&
      c.ppvResource_.value) {
    const D3D12_HEAP_DESC heapDesc = c.pHeap_.value->GetDesc();
    const D3D12_RESOURCE_DESC baseDesc = {
        c.pDesc_.value->Dimension, c.pDesc_.value->Alignment,        c.pDesc_.value->Width,
        c.pDesc_.value->Height,    c.pDesc_.value->DepthOrArraySize, c.pDesc_.value->MipLevels,
        c.pDesc_.value->Format,    c.pDesc_.value->SampleDesc,       c.pDesc_.value->Layout,
        c.pDesc_.value->Flags};
    if (!canReplacePlacedResourceWithCommitted(heapDesc, baseDesc)) {
      failPlacedToCommittedIncompatibility("CreatePlacedResource2 -> CreateCommittedResource3",
                                           c.key, c.pHeap_.key, heapDesc, baseDesc);
      return;
    }

    HRESULT hr =
        createCommittedResource3(c.pHeap_.value, c.pDesc_.value, c.InitialLayout_.value,
                                 c.pOptimizedClearValue_.value, c.NumCastableFormats_.value,
                                 c.pCastableFormats_.value, c.riid_.value, c.ppvResource_.value);
    if (hr == S_OK) {
      c.result_.value = S_OK;
      registerForcedPlacedResource(c);
      c.pHeap_.value = nullptr;
      c.skip = true;
    } else {
      failPlacedToCommittedCreation("CreatePlacedResource2 -> CreateCommittedResource3", c.key,
                                    c.pHeap_.key, c.HeapOffset_.value, baseDesc,
                                    c.InitialLayout_.value, hr);
      return;
    }
  }
  if (portabilityAssertions_ && !isStateRestoreKey(c.ppvResource_.key)) {
    resourcePlacementAssertions_.createPlacedResource(c.ppvResource_.key, *c.pDesc_.value,
                                                      c.object_.value);
  }
}

template <typename CommandType>
void PortabilityLayer::registerForcedPlacedResource(CommandType& c) {
  if (registerResource_ && c.ppvResource_.value && *c.ppvResource_.value) {
    registerResource_(c.ppvResource_.key, static_cast<ID3D12Resource*>(*c.ppvResource_.value));
  }
}

void PortabilityLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (forcePlacedToCommittedResources_ && c.skip) {
    forcedCommittedResources_.insert(c.ppvResource_.key);
  }
  D3D12_RESOURCE_DESC desc = (*reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value))->GetDesc();
  if (storeResourcePlacementData_) {
    resourcePlacementCapture_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                   c.HeapOffset_.value, c.object_.value, desc);
  } else if (storeResourcePlacementDataNoExecute_) {
    resourcePlacementCaptureNoExecute_.createPlacedResource(
        c.pHeap_.key, c.ppvResource_.key, c.HeapOffset_.value, c.object_.value, desc);
  }
}

void PortabilityLayer::post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  c.pInfo_.value->ResultDataMaxSizeInBytes *= accelerationStructurePadding_;
  c.pInfo_.value->ScratchDataSizeInBytes *= accelerationStructureScratchPadding_;
  c.pInfo_.value->UpdateScratchDataSizeInBytes *= accelerationStructureScratchPadding_;
}

void PortabilityLayer::post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (portabilityChecks_) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "Portability - padding in capture post build info in "
                     "EmitRaytracingAccelerationStructurePostbuildInfo not "
                     "supported";
      logged = true;
    }
  }
}

void PortabilityLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (portabilityChecks_ && c.pPostbuildInfoDescs_.value) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "Portability - padding in capture post build info in "
                     "BuildRaytracingAccelerationStructure not supported";
      logged = true;
    }
  }
}

void PortabilityLayer::pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.updateTileMappings(c);
  }
}

void PortabilityLayer::pre(ID3D12DeviceGetResourceAllocationInfoCommand& c) {
  if (c.numResourceDescs_.value == 1) {
    if (storeResourcePlacementDataNoExecute_) {
      resourcePlacementCaptureNoExecute_.getResourceAllocation(
          c.pResourceDescs_.value[0], c.result_.value.SizeInBytes, c.result_.value.Alignment);
    }
  }
}

void PortabilityLayer::pre(ID3D12Device4GetResourceAllocationInfo1Command& c) {
  if (c.numResourceDescs_.value == 1) {
    if (storeResourcePlacementDataNoExecute_) {
      resourcePlacementCaptureNoExecute_.getResourceAllocation(
          c.pResourceDescs_.value[0], c.result_.value.SizeInBytes, c.result_.value.Alignment);
    }
  } else if (c.pResourceAllocationInfo1_.value) {
    for (unsigned i = 0; i < c.numResourceDescs_.value; ++i) {
      if (storeResourcePlacementDataNoExecute_) {
        resourcePlacementCaptureNoExecute_.getResourceAllocation(
            c.pResourceDescs_.value[i], c.pResourceAllocationInfo1_.value[i].SizeInBytes,
            c.pResourceAllocationInfo1_.value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::pre(ID3D12Device8GetResourceAllocationInfo2Command& c) {
  if (c.numResourceDescs_.value == 1) {
    if (storeResourcePlacementDataNoExecute_) {
      resourcePlacementCaptureNoExecute_.getResourceAllocation(
          c.pResourceDescs_.value[0], c.result_.value.SizeInBytes, c.result_.value.Alignment);
    }
  } else if (c.pResourceAllocationInfo1_.value) {
    for (unsigned i = 0; i < c.numResourceDescs_.value; ++i) {
      if (storeResourcePlacementDataNoExecute_) {
        resourcePlacementCaptureNoExecute_.getResourceAllocation(
            c.pResourceDescs_.value[i], c.pResourceAllocationInfo1_.value[i].SizeInBytes,
            c.pResourceAllocationInfo1_.value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::pre(ID3D12Device12GetResourceAllocationInfo3Command& c) {
  if (c.numResourceDescs_.value == 1) {
    if (storeResourcePlacementDataNoExecute_) {
      resourcePlacementCaptureNoExecute_.getResourceAllocation(
          c.pResourceDescs_.value[0], c.result_.value.SizeInBytes, c.result_.value.Alignment);
    }
  } else if (c.pResourceAllocationInfo1_.value) {
    for (unsigned i = 0; i < c.numResourceDescs_.value; ++i) {
      if (storeResourcePlacementDataNoExecute_) {
        resourcePlacementCaptureNoExecute_.getResourceAllocation(
            c.pResourceDescs_.value[i], c.pResourceAllocationInfo1_.value[i].SizeInBytes,
            c.pResourceAllocationInfo1_.value[i].Alignment);
      }
    }
  }
}

void PortabilityLayer::pre(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (!forcePlacedToCommittedResources_ || !c.NumBarriers_.value || !c.pBarriers_.value) {
    return;
  }

  UINT originalCount = c.NumBarriers_.value;
  UINT newCount = 0;

  for (UINT i = 0; i < originalCount; ++i) {
    if (c.pBarriers_.value[i].Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
      continue;
    }
    if (newCount != i) {
      c.pBarriers_.value[newCount] = c.pBarriers_.value[i];
    }
    ++newCount;
  }

  if (newCount == 0) {
    c.skip = true;
  } else {
    c.NumBarriers_.value = newCount;
  }
}

void PortabilityLayer::configureHeapMemoryPool(ID3D12Device* device, D3D12_HEAP_DESC* heapDesc) {
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

void PortabilityLayer::checkHeapCreationFlags(unsigned heapKey,
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

HRESULT PortabilityLayer::createCommittedResource(ID3D12Heap* heap,
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
  adjustResourceFlagsForCommitted(heapDesc, adjustedDesc, adjustedDesc.Flags);

  D3D12_HEAP_FLAGS committedHeapFlags = getCompatibleHeapFlags(heapDesc, adjustedDesc);

  D3D12_HEAP_PROPERTIES Properties =
      getCompatibleHeapProperties(heapDesc, adjustedDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      getCompatibleInitialState(Properties, adjustedDesc, initialState);
  const D3D12_CLEAR_VALUE* clearValueAdjusted = getCompatibleClearValue(adjustedDesc, clearValue);

  hr = device->CreateCommittedResource(&Properties, committedHeapFlags, &adjustedDesc,
                                       creationState, clearValueAdjusted, riid, ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "createCommittedResource: CreateCommittedResource failed 0x" << std::hex << hr
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
    transitionResource(device.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                       D3D12_RESOURCE_STATE_COMMON, initialState);
  }

  return S_OK;
}

HRESULT PortabilityLayer::createCommittedResource2(ID3D12Heap* heap,
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
  adjustResourceFlagsForCommitted(heapDesc, baseDesc, adjustedDesc.Flags);
  baseDesc.Flags = adjustedDesc.Flags;

  D3D12_HEAP_FLAGS committedHeapFlags = getCompatibleHeapFlags(heapDesc, baseDesc);
  D3D12_HEAP_PROPERTIES properties = getCompatibleHeapProperties(heapDesc, baseDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      getCompatibleInitialState(properties, baseDesc, initialState);
  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
      properties.Type == D3D12_HEAP_TYPE_DEFAULT && creationState != D3D12_RESOURCE_STATE_COMMON) {
    creationState = D3D12_RESOURCE_STATE_COMMON;
  }
  const D3D12_CLEAR_VALUE* clearValueAdjusted = getCompatibleClearValue(baseDesc, clearValue);

  hr = device8->CreateCommittedResource2(&properties, committedHeapFlags, &adjustedDesc,
                                         creationState, clearValueAdjusted, nullptr, riid,
                                         ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "createCommittedResource2: CreateCommittedResource2 failed 0x" << std::hex << hr
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
    transitionResource(device8.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                       D3D12_RESOURCE_STATE_COMMON, initialState);
  }

  return S_OK;
}

HRESULT PortabilityLayer::createCommittedResource3(ID3D12Heap* heap,
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
  adjustResourceFlagsForCommitted(heapDesc, baseDesc, adjustedDesc.Flags);
  baseDesc.Flags = adjustedDesc.Flags;

  D3D12_RESOURCE_STATES initialState = barrierLayoutToResourceState(initialLayout);
  D3D12_HEAP_FLAGS committedHeapFlags = getCompatibleHeapFlags(heapDesc, baseDesc);
  D3D12_HEAP_PROPERTIES properties = getCompatibleHeapProperties(heapDesc, baseDesc, initialState);
  D3D12_RESOURCE_STATES creationState =
      getCompatibleInitialState(properties, baseDesc, initialState);

  D3D12_BARRIER_LAYOUT creationLayout = initialLayout;
  if (creationState != initialState) {
    creationLayout = D3D12_BARRIER_LAYOUT_COMMON;
  }
  if (adjustedDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
      properties.Type == D3D12_HEAP_TYPE_DEFAULT && creationLayout != D3D12_BARRIER_LAYOUT_COMMON) {
    creationLayout = D3D12_BARRIER_LAYOUT_COMMON;
  }

  const D3D12_CLEAR_VALUE* clearValueAdjusted = getCompatibleClearValue(baseDesc, clearValue);

  hr = device10->CreateCommittedResource3(&properties, committedHeapFlags, &adjustedDesc,
                                          creationLayout, clearValueAdjusted, nullptr,
                                          numCastableFormats, pCastableFormats, riid, ppvResource);
  if (FAILED(hr)) {
    LOG_ERROR << "createCommittedResource3: CreateCommittedResource3 failed 0x" << std::hex << hr
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
    D3D12_RESOURCE_STATES targetState = barrierLayoutToResourceState(initialLayout);
    if (targetState != D3D12_RESOURCE_STATE_COMMON) {
      transitionResource(device10.Get(), static_cast<ID3D12Resource*>(*ppvResource),
                         D3D12_RESOURCE_STATE_COMMON, targetState);
    }
  }

  return S_OK;
}

D3D12_RESOURCE_STATES PortabilityLayer::barrierLayoutToResourceState(D3D12_BARRIER_LAYOUT layout) {
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
    LOG_WARNING << "barrierLayoutToResourceState: unhandled layout " << static_cast<int>(layout);
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

D3D12_HEAP_FLAGS PortabilityLayer::getCompatibleHeapFlags(const D3D12_HEAP_DESC& heapDesc,
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

void PortabilityLayer::adjustResourceFlagsForCommitted(const D3D12_HEAP_DESC& heapDesc,
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

D3D12_HEAP_PROPERTIES PortabilityLayer::getCompatibleHeapProperties(
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

D3D12_RESOURCE_STATES PortabilityLayer::getCompatibleInitialState(
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

const D3D12_CLEAR_VALUE* PortabilityLayer::getCompatibleClearValue(
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

void PortabilityLayer::transitionResource(ID3D12Device* device,
                                          ID3D12Resource* resource,
                                          D3D12_RESOURCE_STATES stateBefore,
                                          D3D12_RESOURCE_STATES stateAfter) {
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
  if (FAILED(hr)) {
    LOG_WARNING << "transitionResource: CreateCommandQueue failed 0x" << std::hex << hr;
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
  if (FAILED(hr)) {
    LOG_WARNING << "transitionResource: CreateCommandAllocator failed 0x" << std::hex << hr;
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                 IID_PPV_ARGS(&cmdList));
  if (FAILED(hr)) {
    LOG_WARNING << "transitionResource: CreateCommandList failed 0x" << std::hex << hr;
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
    LOG_WARNING << "transitionResource: CreateFence failed 0x" << std::hex << hr;
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

std::string PortabilityLayer::getPlacedToCommittedIncompatibilityReasons(
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

std::string PortabilityLayer::getPlacedToCommittedFailureContext(
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

void PortabilityLayer::failPlacedToCommittedIncompatibility(
    const char* apiName,
    unsigned commandKey,
    unsigned heapKey,
    const D3D12_HEAP_DESC& heapDesc,
    const D3D12_RESOURCE_DESC& resourceDesc) const {
  std::ostringstream stream;
  stream << "Forced placed->committed replacement failed. Reasons: "
         << getPlacedToCommittedIncompatibilityReasons(heapDesc, resourceDesc) << " | "
         << getPlacedToCommittedFailureContext(apiName, commandKey, heapKey, heapDesc,
                                               resourceDesc);
  const std::string message = stream.str();
  LOG_ERROR << message;
  GITS_ASSERT(false, message.c_str());
}

void PortabilityLayer::failPlacedToCommittedCreation(const char* apiName,
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

void PortabilityLayer::failPlacedToCommittedCreation(const char* apiName,
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

bool PortabilityLayer::canReplacePlacedResourceWithCommitted(
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
