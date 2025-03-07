// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpService.h"
#include "gits.h"

#include <d3dx12.h>

namespace gits {
namespace DirectX {

ResourceDumpService::ResourceDumpService()
    : resourceDump_(Config::Get().directx.features.resourcesDump.format == "jpg" ? true : false,
                    Config::Get().directx.features.resourcesDump.textureRescaleRange),
      resourceKeys_(Config::Get().directx.features.resourcesDump.resourceKeys),
      callKeys_(Config::Get().directx.features.resourcesDump.commandKeys) {

  auto& dumpPath = Config::Get().common.player.outputDir.empty()
                       ? Config::Get().common.player.streamDir / "resources"
                       : Config::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  dumpPath_ = dumpPath;
}

void ResourceDumpService::createResource(unsigned resourceKey,
                                         ID3D12Resource* resource,
                                         D3D12_RESOURCE_STATES initialState) {
  if (!resourceKeys_.contains(resourceKey)) {
    return;
  }
  resourceStateTracker_.addResource(resourceKey, initialState);
  resources_[resourceKey] = resource;
}

void ResourceDumpService::destroyResource(unsigned resourceKey) {
  if (!resourceKeys_.contains(resourceKey)) {
    return;
  }
  resourceStateTracker_.destroyResource(resourceKey);
  resources_.erase(resourceKey);
}

void ResourceDumpService::commandListCall(unsigned callKey,
                                          ID3D12GraphicsCommandList* commandList) {
  if (!callKeys_.contains(callKey)) {
    return;
  }
  for (unsigned resourceKey : resourceKeys_) {
    auto it = resources_.find(resourceKey);
    if (it != resources_.end()) {

      ID3D12Resource* resource = it->second;
      D3D12_RESOURCE_DESC desc = resource->GetDesc();

      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        std::wstring dumpName = dumpPath_ + L"/command_" + ConfigKeySet::keyToWString(callKey) +
                                L"_buffer_O" + ConfigKeySet::keyToWString(resourceKey);
        D3D12_RESOURCE_STATES resourceState =
            resourceStateTracker_.getResourceState(commandList, resourceKey, 0);
        resourceDump_.dumpResource(commandList, resource, 0, resourceState, dumpName);
      } else {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
        GITS_ASSERT(hr == S_OK);
        unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), desc.Format);
        unsigned arraySize =
            desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? desc.DepthOrArraySize : 1;

        for (unsigned arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex) {
          for (unsigned mipLevel = 0; mipLevel < desc.MipLevels; ++mipLevel) {
            for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

              std::wstring dumpName = dumpPath_;
              dumpName += L"/command_" + ConfigKeySet::keyToWString(callKey);
              dumpName += L"_texture_O" + ConfigKeySet::keyToWString(resourceKey);
              if (planeCount > 1) {
                dumpName += L"_plane_" + std::to_wstring(planeSlice);
              }
              if (arraySize > 1) {
                dumpName += L"_array_" + std::to_wstring(arrayIndex);
              }
              if (desc.MipLevels > 1) {
                dumpName += L"_mip_" + std::to_wstring(mipLevel);
              }

              unsigned subresource =
                  D3D12CalcSubresource(mipLevel, arrayIndex, planeSlice, desc.MipLevels, arraySize);
              D3D12_RESOURCE_STATES resourceState =
                  resourceStateTracker_.getResourceState(commandList, resourceKey, subresource);
              resourceDump_.dumpResource(commandList, resource, subresource, resourceState,
                                         dumpName, mipLevel);
            }
          }
        }
      }
    }
  }
}

void ResourceDumpService::resourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  resourceStateTracker_.resourceBarrier(c.object_.value, c.pBarriers_.value, c.NumBarriers_.value,
                                        c.pBarriers_.resourceKeys.data());
}

void ResourceDumpService::executeCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum) {
  resourceStateTracker_.executeCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(commandLists), commandListNum);
  resourceDump_.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void ResourceDumpService::commandQueueWait(unsigned key,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  resourceDump_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDumpService::commandQueueSignal(unsigned key,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  resourceDump_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDumpService::fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  resourceDump_.fenceSignal(key, fenceKey, fenceValue);
}

} // namespace DirectX
} // namespace gits
