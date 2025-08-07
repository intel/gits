// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
class CGits;
namespace DirectX {

class RtasSerializer : public ResourceDump {
public:
  RtasSerializer(CGits& gits, const std::string& cacheFile, bool dumpCacheInfo);
  ~RtasSerializer();

  // Disallow copying (gits::noncopyable is not available here).
  RtasSerializer(const RtasSerializer&) = delete;
  RtasSerializer& operator=(const RtasSerializer&) = delete;

  void serialize(unsigned buildKey,
                 ID3D12GraphicsCommandList4* commandList,
                 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc);

protected:
  struct RtasDumpInfo : public DumpInfo {
    Microsoft::WRL::ComPtr<ID3D12Resource> serializedBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> postbuildInfoBuffer{};
    DumpInfo postbuildInfo;
  };
  void dumpStagedResource(DumpInfo& dumpInfo) override;

private:
  struct CacheInfo {
    D3D12_GPU_VIRTUAL_ADDRESS destVA;
  };

  void initialize();
  void dumpCacheInfo();

  CGits& gits_;
  bool initialized_{false};
  std::wstring tmpCacheDir_;
  std::filesystem::path cacheFile_;
  bool dumpCacheInfo_{false};
  std::vector<unsigned> buildKeys_;
  std::map<unsigned, CacheInfo> cacheInfoByBuildKey;
};

} // namespace DirectX
} // namespace gits
