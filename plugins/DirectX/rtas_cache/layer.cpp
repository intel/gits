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
      accelerationStructuresSerializer_(gits, cfg_.record),
      accelerationStructuresDeserializer_(gits) {}

RtasCacheLayer::~RtasCacheLayer() {
  if (replay()) {
    log(gits_, "RtasCache: deserialized ", blasCount_, " BLASes");
  }
  if (record()) {
    log(gits_, "RtasCache: serialized ", blasCount_, " BLASes");
  }
}

void RtasCacheLayer::pre(D3D12CreateDeviceCommand& c) {
  if (c.key & Command::stateRestoreKeyMask) {
    stateRestore_ = true;
  }
}

void RtasCacheLayer::pre(IDXGISwapChainPresentCommand& c) {
  if (replay()) {
    log(gits_, "RtasCache: deserialized ", blasCount_, " BLASes");
  }
  if (record()) {
    log(gits_, "RtasCache: serialized ", blasCount_, " BLASes");
  }
  stateRestore_ = false;
}

void RtasCacheLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (replay()) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      return;
    }

    accelerationStructuresDeserializer_.deserializeAccelerationStructure(c.key, c.object_.value,
                                                                         *c.pDesc_.value);
    c.skip = true;
    ++blasCount_;
  }
}

void RtasCacheLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (record()) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      return;
    }

    accelerationStructuresSerializer_.serializeAccelerationStructure(c.key, c.object_.value,
                                                                     *c.pDesc_.value);
    ++blasCount_;
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (record()) {
    accelerationStructuresSerializer_.executeCommandLists(
        c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value, c.NumCommandLists_.value);
  } else {
    accelerationStructuresDeserializer_.executeCommandLists(
        c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value, c.NumCommandLists_.value);
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (record()) {
    accelerationStructuresSerializer_.commandQueueWait(c.key, c.object_.key, c.pFence_.key,
                                                       c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (record()) {
    accelerationStructuresSerializer_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key,
                                                         c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12FenceSignalCommand& c) {
  if (record()) {
    accelerationStructuresSerializer_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void RtasCacheLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (record()) {
    accelerationStructuresSerializer_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

} // namespace DirectX
} // namespace gits
