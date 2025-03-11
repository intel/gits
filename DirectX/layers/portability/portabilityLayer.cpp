// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

PortabilityLayer::PortabilityLayer() : Layer("Portability") {
  if (Config::IsRecorder() &&
          Config::Get().directx.features.portability.storePlacedResourceDataOnCapture ||
      Config::IsPlayer() &&
          Config::Get().directx.features.portability.storePlacedResourceDataOnPlayback) {
    storeResourcePlacementData_ = true;
  }

  if (Config::IsRecorder() && storeResourcePlacementData_) {
    gits::CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_RECORDER, TOPIC_END}, [this](Topic t, const MessagePtr& m) {
          resourcePlacementCapture_.storeResourcePlacement();
        });
  }
}

PortabilityLayer::~PortabilityLayer() {
  try {
    if (Config::IsPlayer() && storeResourcePlacementData_) {
      resourcePlacementCapture_.storeResourcePlacement();
    }
  } catch (...) {
    topmost_exception_handler("PortabilityLayer::~PortabilityLayer");
  }
}

void PortabilityLayer::pre(ID3D12DeviceCreateHeapCommand& c) {
  if (Config::IsPlayer()) {
    resourcePlacementPlayback_.createHeap(c.object_.value, c.ppvHeap_.key,
                                          c.pDesc_.value->SizeInBytes);
  }
}

void PortabilityLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value, c.result_.value);
}

void PortabilityLayer::pre(ID3D12Device4CreateHeap1Command& c) {
  if (Config::IsPlayer()) {
    resourcePlacementPlayback_.createHeap(c.object_.value, c.ppvHeap_.key,
                                          c.pDesc_.value->SizeInBytes);
  }
}

void PortabilityLayer::post(ID3D12Device4CreateHeap1Command& c) {
  checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value, c.result_.value);
}

void PortabilityLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (Config::IsPlayer()) {
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
  if (Config::IsPlayer()) {
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
  if (Config::IsPlayer()) {
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

void PortabilityLayer::checkHeapCreationFlags(unsigned heapKey,
                                              ID3D12Device* device,
                                              D3D12_HEAP_DESC* desc,
                                              HRESULT result) {
  if (Config::IsRecorder() || result == S_OK) {
    return;
  }

  D3D12_FEATURE_DATA_D3D12_OPTIONS featureOptions{};
  HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOptions,
                                           sizeof(featureOptions));
  if (FAILED(hr)) {
    Log(ERR) << "Portability - Failed to CheckFeatureSupport (D3D12_FEATURE_D3D12_OPTIONS)";
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
