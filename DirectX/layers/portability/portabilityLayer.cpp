// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayer.h"
#include "gits.h"
#include "log.h"
#include "to_string/toStr.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

PortabilityLayer::PortabilityLayer() : Layer("Portability") {
  if (Configurator::IsRecorder() &&
          Configurator::Get().directx.capture.portability.resourcePlacementStorage ||
      Configurator::IsPlayer() &&
          Configurator::Get().directx.player.portability.resourcePlacement == "store") {
    storeResourcePlacementData_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.resourcePlacement == "use") {
    useResourcePlacementData_ = true;
  }
  if (Configurator::IsPlayer() &&
      Configurator::Get().directx.player.portability.portabilityChecks) {
    portabilityChecks_ = true;
  }
  if (Configurator::IsRecorder()) {
    accelerationStructurePadding_ =
        Configurator::Get().directx.capture.portability.raytracing.accelerationStructurePadding;
    if (accelerationStructurePadding_ < 1.0) {
      accelerationStructurePadding_ = 1.0;
    }
    accelerationStructureScratchPadding_ =
        Configurator::Get()
            .directx.capture.portability.raytracing.accelerationStructureScratchPadding;
    if (accelerationStructureScratchPadding_ < 1.0) {
      accelerationStructureScratchPadding_ = 1.0;
    }
  }

  if (Configurator::IsRecorder() && storeResourcePlacementData_) {
    gits::CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_RECORDER, TOPIC_END}, [this](Topic t, const MessagePtr& m) {
          resourcePlacementCapture_.storeResourcePlacement();
        });
  }
}

PortabilityLayer::~PortabilityLayer() {
  try {
    if (Configurator::IsPlayer() && storeResourcePlacementData_) {
      resourcePlacementCapture_.storeResourcePlacement();
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
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
  }
}

void PortabilityLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (storeResourcePlacementData_) {
    resourcePlacementCapture_.createPlacedResource(
        c.pHeap_.key, c.ppvResource_.key, c.HeapOffset_.value, c.object_.value, *c.pDesc_.value);
  }
}

void PortabilityLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
  }
}

void PortabilityLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (storeResourcePlacementData_) {
    D3D12_RESOURCE_DESC desc =
        (*reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value))->GetDesc();
    resourcePlacementCapture_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                   c.HeapOffset_.value, c.object_.value, desc);
  }
}

void PortabilityLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  if (useResourcePlacementData_) {
    resourcePlacementPlayback_.createPlacedResource(c.ppvResource_.key, c.HeapOffset_.value);
  }
}

void PortabilityLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (storeResourcePlacementData_) {
    D3D12_RESOURCE_DESC desc =
        (*reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value))->GetDesc();
    resourcePlacementCapture_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                   c.HeapOffset_.value, c.object_.value, desc);
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

} // namespace DirectX
} // namespace gits
