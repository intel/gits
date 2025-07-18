// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "pluginUtils.h"

namespace gits {
namespace DirectX {

RtasCacheLayer::RtasCacheLayer(CGits& gits, const RtasCacheConfig& cfg)
    : Layer("RtasCache"),
      gits_(gits),
      cfg_(cfg),
      serializer_(gits, cfg_.cacheFile, cfg_.dumpCacheInfoFile),
      deserializer_(gits, cfg_.cacheFile),
      stateRestore_(false) {}

RtasCacheLayer::~RtasCacheLayer() {
  try {
    log(gits_, "RtasCache: ", cfg_.record ? "Serialized " : "Deserialized ", blasCount_, " BLASes");
  } catch (...) {
    fprintf(stderr, "Exception in RtasCacheLayer::~RtasCacheLayer");
  }
}

void RtasCacheLayer::pre(StateRestoreBeginCommand& c) {
  stateRestore_ = true;
}

void RtasCacheLayer::pre(StateRestoreEndCommand& c) {
  stateRestore_ = false;
}

void RtasCacheLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  static bool firstBuildCall = true;
  if (replay()) {
    if (firstBuildCall) {
      Microsoft::WRL::ComPtr<ID3D12Device5> device;
      HRESULT hr = c.object_.value->GetDevice(IID_PPV_ARGS(&device));
      assert(hr == S_OK);

      isCompatible_ = deserializer_.isCompatible(device.Get());
    }

    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      return;
    }

    deserializer_.deserialize(c.key, c.object_.value, *c.pDesc_.value);
    c.skip = true;
    ++blasCount_;
  }

  firstBuildCall = false;
}

void RtasCacheLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (record()) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      return;
    }

    serializer_.serialize(c.key, c.object_.value, *c.pDesc_.value);
    ++blasCount_;
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (record()) {
    serializer_.executeCommandLists(c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value,
                                    c.NumCommandLists_.value);
  } else if (replay()) {
    deserializer_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                      c.ppCommandLists_.value, c.NumCommandLists_.value);
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (record()) {
    serializer_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (record()) {
    serializer_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12FenceSignalCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void RtasCacheLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  }
}

} // namespace DirectX
} // namespace gits
