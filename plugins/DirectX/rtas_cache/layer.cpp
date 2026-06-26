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

static std::string GetFileSize(const std::string& filePath) {
  auto fileSize = std::filesystem::file_size(filePath);
  auto fileSizeMB = static_cast<double>(fileSize) / (1024 * 1024);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << fileSizeMB << "MB";
  return oss.str();
}

RtasCacheLayer::RtasCacheLayer(const RtasCacheConfig& cfg)
    : Layer("RtasCache"),
      m_Cfg(cfg),
      m_Serializer(m_Cfg.CacheFile, m_Cfg.DumpCacheInfoFile),
      m_Deserializer(m_Cfg.CacheFile),
      m_StateRestore(false) {
  LOG_INFO << "RtasCache - State restore only: " << (cfg.StateRestoreOnly ? "true" : "false");
  LOG_INFO << "RtasCache - Cache file: " << cfg.CacheFile;
  if (!cfg.Record) {
    if (std::filesystem::exists(cfg.CacheFile)) {
      LOG_INFO << "RtasCache - Cache file size is " << GetFileSize(m_Cfg.CacheFile);
    } else {
      LOG_ERROR << "RtasCache - Cache file does not exist!";
      m_IsValid = false;
    }
  }
}

RtasCacheLayer::~RtasCacheLayer() {
  try {
    if (m_Cfg.Record) {
      LOG_INFO << "RtasCache - Serialized " << m_BlasCount << " BLASes";
      m_Serializer.WriteCache();
      LOG_INFO << "RtasCache - Cache file size is " << GetFileSize(m_Cfg.CacheFile);
    } else {
      LOG_INFO << "RtasCache - Deserialized " << m_CachedBlasCount << "/" << m_BlasCount
               << " BLASes";
    }
  } catch (...) {
    fprintf(stderr, "Exception in RtasCacheLayer::~RtasCacheLayer");
  }
}

void RtasCacheLayer::Pre(StateRestoreBeginCommand& command) {
  m_StateRestore = true;
}

void RtasCacheLayer::Pre(StateRestoreEndCommand& command) {
  m_StateRestore = false;
}

void RtasCacheLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  static bool firstBuildCall = true;
  if (command.m_pDesc.Value->Inputs.Type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (Replay()) {
    if (firstBuildCall) {
      firstBuildCall = false;

      Microsoft::WRL::ComPtr<ID3D12Device5> device;
      HRESULT hr = command.m_Object.Value->GetDevice(IID_PPV_ARGS(&device));
      assert(hr == S_OK);

      m_IsValid = m_Deserializer.PreloadCache(device.Get());
      if (!m_IsValid) {
        LOG_WARNING << "RtasCache - Failed to preload RTAS cache. Will not deserialize. Fallback "
                       "to performing standard builds.";
        return;
      }
    }

    if (m_Deserializer.Deserialize(command.Key, command.m_Object.Value, *command.m_pDesc.Value)) {
      command.Skip = true;
      ++m_CachedBlasCount;
    }
  }
}

void RtasCacheLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  if (command.m_pDesc.Value->Inputs.Type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (Record()) {
    m_Serializer.Serialize(command.Key, command.m_Object.Value, *command.m_pDesc.Value);
  }

  ++m_BlasCount;
}

void RtasCacheLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  if (Record()) {
    m_Serializer.ExecuteCommandLists(command.Key, command.m_Object.Key, command.m_Object.Value,
                                     command.m_ppCommandLists.Value,
                                     command.m_NumCommandLists.Value);
  } else if (Replay()) {
    m_Deserializer.ExecuteCommandLists(command.Key, command.m_Object.Key, command.m_Object.Value,
                                       command.m_ppCommandLists.Value,
                                       command.m_NumCommandLists.Value);
  }
}

void RtasCacheLayer::Post(ID3D12CommandQueueWaitCommand& command) {
  if (Record()) {
    m_Serializer.CommandQueueWait(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                  command.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12CommandQueueSignalCommand& command) {
  if (Record()) {
    m_Serializer.CommandQueueSignal(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                    command.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12FenceSignalCommand& command) {
  if (Record()) {
    m_Serializer.FenceSignal(command.Key, command.m_Object.Key, command.m_Value.Value);
  }
}

void RtasCacheLayer::Post(ID3D12DeviceCreateFenceCommand& command) {
  if (Record()) {
    m_Serializer.FenceSignal(command.Key, command.m_ppFence.Key, command.m_InitialValue.Value);
  }
}

void RtasCacheLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& command) {
  if (Record()) {
    m_Serializer.FenceSignal(command.Key, command.m_pFenceToSignal.Key,
                             command.m_FenceValueToSignal.Value);
  }
}

} // namespace DirectX
} // namespace gits
