// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

#include <filesystem>
#include <map>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

class RtasSerializer : public ResourceDump {
public:
  RtasSerializer(const std::string& cacheFile, bool dumpCacheInfo);
  ~RtasSerializer();

  // Disallow copying (gits::noncopyable is not available here).
  RtasSerializer(const RtasSerializer&) = delete;
  RtasSerializer& operator=(const RtasSerializer&) = delete;

  void Serialize(unsigned buildKey,
                 ID3D12GraphicsCommandList4* commandList,
                 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc);
  void WriteCache();

protected:
  struct RtasDumpInfo : public DumpInfo {
    Microsoft::WRL::ComPtr<ID3D12Resource> SerializedBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> PostbuildInfoBuffer{};
    DumpInfo PostbuildInfo;
  };
  void DumpStagedResource(DumpInfo& dumpInfo) override;

private:
  struct CacheInfo {
    D3D12_GPU_VIRTUAL_ADDRESS DestVA;
  };

  void Initialize();
  void DumpCacheInfo();

  bool m_Initialized{false};
  std::wstring m_TmpCacheDir;
  std::filesystem::path m_CacheFile;
  bool m_DumpCacheInfo{false};
  std::vector<unsigned> m_BuildKeys;
  std::map<unsigned, CacheInfo> m_CacheInfoByBuildKey;
};

} // namespace DirectX
} // namespace gits
