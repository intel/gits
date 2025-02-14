// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayer.h"

namespace gits {
namespace DirectX {

PortabilityLayer::PortabilityLayer() : Layer("Portability"), isPlayer_(Config::IsPlayer()) {}

PortabilityLayer::~PortabilityLayer() {}

void PortabilityLayer::post(D3D12CreateDeviceCommand& c) {
  heapPlacementOrganizer_.onPostCreateDevice(c);
}

void PortabilityLayer::pre(ID3D12DeviceCreateHeapCommand& c) {
  heapPlacementOrganizer_.onPreCreateHeap(c);
}

void PortabilityLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  heapPlacementOrganizer_.onPostCreateHeap(c);
  checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value, c.result_.value);
}

void PortabilityLayer::pre(ID3D12Device4CreateHeap1Command& c) {
  heapPlacementOrganizer_.onPreCreateHeap(c);
}

void PortabilityLayer::post(ID3D12Device4CreateHeap1Command& c) {
  heapPlacementOrganizer_.onPostCreateHeap(c);
  checkHeapCreationFlags(c.ppvHeap_.key, c.object_.value, c.pDesc_.value, c.result_.value);
}

void PortabilityLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& c) {
  heapPlacementOrganizer_.onPreCreatePlacedRes(c);
}

void PortabilityLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  heapPlacementOrganizer_.onPostCreatePlacedRes(c);
}

void PortabilityLayer::pre(ID3D12Device8CreatePlacedResource1Command& c) {
  heapPlacementOrganizer_.onPreCreatePlacedRes(c);
}

void PortabilityLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  heapPlacementOrganizer_.onPostCreatePlacedRes(c);
}

void PortabilityLayer::pre(ID3D12Device10CreatePlacedResource2Command& c) {
  heapPlacementOrganizer_.onPreCreatePlacedRes(c);
}

void PortabilityLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  heapPlacementOrganizer_.onPostCreatePlacedRes(c);
}

void PortabilityLayer::pre(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  heapPlacementOrganizer_.onPreUpdateTileMappings(c);
}

void PortabilityLayer::checkHeapCreationFlags(unsigned heapKey,
                                              ID3D12Device* device,
                                              D3D12_HEAP_DESC* desc,
                                              HRESULT result) {

  if (!isPlayer_ || result == S_OK) {
    return;
  }

  D3D12_FEATURE_DATA_D3D12_OPTIONS featureOptions{};
  HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOptions,
                                           sizeof(featureOptions));
  if (FAILED(hr)) {
    Log(ERR) << "checkHeapCreationFlags: Failed to query feature support.";
    return;
  }

  if (featureOptions.ResourceHeapTier != 1) {
    return;
  }

  if (desc->Flags == D3D12_HEAP_FLAG_NONE) {
    Log(ERR) << "checkHeapCreationFlags: Resource heap creation flag is D3D12_HEAP_FLAG_NONE. "
                "Stream non-playable "
                "for current GPU/GPU driver.";
    return;
  }

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

  Log(ERR) << "checkHeapCreationFlags: Resource heap creation flag inappropriate for available "
              "resource heap tiers."
           << " Stream non-playable for current GPU/GPU driver.";
}

} // namespace DirectX
} // namespace gits
