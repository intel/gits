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

static std::string getFileSize(const std::string& filePath) {
  auto fileSize = std::filesystem::file_size(filePath);
  auto fileSizeMB = static_cast<double>(fileSize) / (1024 * 1024);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << fileSizeMB << "MB";
  return oss.str();
}

RtasCacheLayer::RtasCacheLayer(CGits& gits, const RtasCacheConfig& cfg)
    : Layer("RtasCache"),
      gits_(gits),
      cfg_(cfg),
      serializer_(gits, cfg_.cacheFile, cfg_.dumpCacheInfoFile),
      deserializer_(gits, cfg_.cacheFile),
      stateRestore_(false) {
  log(gits_, "RtasCache - State restore only: ", cfg.stateRestoreOnly ? "true" : "false");
  log(gits_, "RtasCache - Cache file: ", cfg.cacheFile);
  if (!cfg.record) {
    if (std::filesystem::exists(cfg.cacheFile)) {
      log(gits_, "RtasCache - Cache file size is ", getFileSize(cfg_.cacheFile));
    } else {
      logE(gits_, "RtasCache - Cache file does not exist!");
      isValid_ = false;
    }
  }
}

RtasCacheLayer::~RtasCacheLayer() {
  try {
    if (cfg_.record) {
      log(gits_, "RtasCache - Serialized ", blasCount_, " BLASes");
      serializer_.writeCache();
      log(gits_, "RtasCache - Cache file size is ", getFileSize(cfg_.cacheFile));
    } else {
      log(gits_, "RtasCache - Deserialized ", cachedBlasCount_, "/", blasCount_, " BLASes");
    }
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
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (replay()) {
    if (firstBuildCall) {
      firstBuildCall = false;

      Microsoft::WRL::ComPtr<ID3D12Device5> device;
      HRESULT hr = c.object_.value->GetDevice(IID_PPV_ARGS(&device));
      assert(hr == S_OK);

      isValid_ = deserializer_.preloadCache(device.Get());
      if (!isValid_) {
        logE(gits_, "RtasCache - Failed to preload RTAS cache. Will not deserialize.");
        return;
      }
    }

    if (deserializer_.deserialize(c.key, c.object_.value, *c.pDesc_.value)) {
      c.skip = true;
      ++cachedBlasCount_;
    }
  }
}

void RtasCacheLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (record()) {
    serializer_.serialize(c.key, c.object_.value, *c.pDesc_.value);
  }

  ++blasCount_;
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
