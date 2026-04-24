// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"

namespace gits {
namespace DirectX {

static std::string getFileSize(const std::string& filePath) {
  auto fileSize = std::filesystem::file_size(filePath);
  auto fileSizeMB = static_cast<double>(fileSize) / (1024 * 1024);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << fileSizeMB << "MB";
  return oss.str();
}

RtasCacheLayer::RtasCacheLayer(const RtasCacheConfig& cfg)
    : Layer("RtasCache"),
      cfg_(cfg),
      serializer_(cfg_.cacheFile, cfg_.dumpCacheInfoFile),
      deserializer_(cfg_.cacheFile),
      stateRestore_(false) {
  LOG_INFO << "RtasCache - State restore only: " << (cfg.stateRestoreOnly ? "true" : "false");
  LOG_INFO << "RtasCache - Cache file: " << cfg.cacheFile;
  if (!cfg.record) {
    if (std::filesystem::exists(cfg.cacheFile)) {
      LOG_INFO << "RtasCache - Cache file size is " << getFileSize(cfg_.cacheFile);
    } else {
      LOG_ERROR << "RtasCache - Cache file does not exist!";
      isValid_ = false;
    }
  }
}

RtasCacheLayer::~RtasCacheLayer() {
  try {
    if (cfg_.record) {
      LOG_INFO << "RtasCache - Serialized " << blasCount_ << " BLASes";
      serializer_.writeCache();
      LOG_INFO << "RtasCache - Cache file size is " << getFileSize(cfg_.cacheFile);
    } else {
      LOG_INFO << "RtasCache - Deserialized " << cachedBlasCount_ << "/" << blasCount_ << " BLASes";
    }
  } catch (...) {
    fprintf(stderr, "Exception in RtasCacheLayer::~RtasCacheLayer");
  }
}

void RtasCacheLayer::Pre(StateRestoreBeginCommand& c) {
  stateRestore_ = true;
}

void RtasCacheLayer::Pre(StateRestoreEndCommand& c) {
  stateRestore_ = false;
}

void RtasCacheLayer::Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  static bool firstBuildCall = true;
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (replay()) {
    if (firstBuildCall) {
      firstBuildCall = false;

      Microsoft::WRL::ComPtr<ID3D12Device5> device;
      HRESULT hr = c.m_Object.Value->GetDevice(IID_PPV_ARGS(&device));
      assert(hr == S_OK);

      isValid_ = deserializer_.preloadCache(device.Get());
      if (!isValid_) {
        LOG_WARNING << "RtasCache - Failed to preload RTAS cache. Will not deserialize. Fallback "
                       "to performing standard builds.";
        return;
      }
    }

    if (deserializer_.deserialize(c.Key, c.m_Object.Value, *c.m_pDesc.Value)) {
      c.Skip = true;
      ++cachedBlasCount_;
    }
  }
}

void RtasCacheLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (record()) {
    serializer_.serialize(c.Key, c.m_Object.Value, *c.m_pDesc.Value);
  }

  ++blasCount_;
}

void RtasCacheLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (record()) {
    serializer_.executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                    c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
  } else if (replay()) {
    deserializer_.executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                      c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
  }
}

void RtasCacheLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (record()) {
    serializer_.commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (record()) {
    serializer_.commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12FenceSignalCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  }
}

void RtasCacheLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (record()) {
    serializer_.fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
  }
}

} // namespace DirectX
} // namespace gits
