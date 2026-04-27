// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementCaptureNoExecute.h"
#include "configurationLib.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

void ResourcePlacementCaptureNoExecute::createPlacedResource(unsigned heapKey,
                                                             unsigned resourceKey,
                                                             UINT64 offset,
                                                             ID3D12Device* device,
                                                             D3D12_RESOURCE_DESC& desc) {
  ResourcePlacementInfo info{};
  info.heapKey = heapKey;
  info.key = resourceKey;
  info.offset = offset;
  info.desc = desc;

  m_ResourcePlacementInfos[resourceKey] = info;
}

void ResourcePlacementCaptureNoExecute::getResourceAllocation(const D3D12_RESOURCE_DESC& desc,
                                                              uint64_t sizeInBytes,
                                                              uint64_t alignment) {
  m_ResourceDescToAllocation[desc] = {sizeInBytes, alignment};
}

void ResourcePlacementCaptureNoExecute::getResourceAllocation(const D3D12_RESOURCE_DESC1& desc,
                                                              uint64_t sizeInBytes,
                                                              uint64_t alignment) {
  D3D12_RESOURCE_DESC baseDesc{desc.Dimension,        desc.Alignment, desc.Width,  desc.Height,
                               desc.DepthOrArraySize, desc.MipLevels, desc.Format, desc.SampleDesc,
                               desc.Layout,           desc.Flags};
  m_ResourceDescToAllocation[baseDesc] = {sizeInBytes, alignment};
}

void ResourcePlacementCaptureNoExecute::storeResourcePlacement() {
  std::filesystem::path filePath = Configurator::IsPlayer()
                                       ? Configurator::Get().common.player.streamDir
                                       : Configurator::Get().common.recorder.dumpPath;
  filePath /= "resourcePlacementData.dat";

  std::ofstream file(filePath, std::ios::binary);
  for (auto& [resourceKey, info] : m_ResourcePlacementInfos) {
    const auto allocationInfoIt = m_ResourceDescToAllocation.find(info.desc);
    if (allocationInfoIt != m_ResourceDescToAllocation.end()) {
      info.size = allocationInfoIt->second.SizeInBytes;
      info.alignment = allocationInfoIt->second.Alignment;
    } else if (info.desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      auto align = [](uint64_t value, uint64_t alignment) {
        return ((value - 1) / alignment + 1) * alignment;
      };
      info.alignment = std::max(info.desc.Alignment,
                                static_cast<UINT64>(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
      info.size = align(info.desc.Width, info.alignment);
    }

    if (info.size) {
      file.write(reinterpret_cast<char*>(&info), sizeof(info));
    } else {
      LOG_ERROR << "Portability - no placement data for resource O: " << resourceKey;
    }
  }
}

} // namespace DirectX
} // namespace gits
