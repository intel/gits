// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementCapture.h"
#include "gits.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

void ResourcePlacementCapture::createPlacedResource(unsigned heapKey,
                                                    unsigned resourceKey,
                                                    UINT64 offset,
                                                    ID3D12Device* device,
                                                    D3D12_RESOURCE_DESC& desc) {
  ResourcePlacementInfo info{};
  info.heapKey = heapKey;
  info.key = resourceKey;
  info.offset = offset;
  info.desc = desc;

  D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);
  info.size = allocInfo.SizeInBytes;
  info.alignment = allocInfo.Alignment;

  std::lock_guard<std::mutex> lock(mutex_);
  resourcePlacementInfos_.push_back(info);
}

void ResourcePlacementCapture::storeResourcePlacement() {
  std::filesystem::path filePath = Config::IsPlayer() ? Config::Get().common.player.streamDir
                                                      : Config::Get().common.recorder.dumpPath;
  filePath /= "resourcePlacementData.dat";

  std::lock_guard<std::mutex> lock(mutex_);
  std::ofstream file(filePath, std::ios::binary);
  for (ResourcePlacementInfo& info : resourcePlacementInfos_) {
    file.write(reinterpret_cast<char*>(&info), sizeof(info));
  }
}

} // namespace DirectX
} // namespace gits
