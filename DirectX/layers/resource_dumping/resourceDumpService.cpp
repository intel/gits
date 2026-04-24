// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpService.h"
#include "to_string/toStr.h"
#include "configurationLib.h"

#include <d3dx12.h>

namespace gits {
namespace DirectX {

ResourceDumpService::ResourceDumpService()
    : m_ResourceDump(Configurator::Get().directx.features.resourcesDump.format,
                     Configurator::Get().directx.features.resourcesDump.textureRescaleRange),
      m_ResourceKeys(Configurator::Get().directx.features.resourcesDump.resourceKeys),
      m_CallKeys(Configurator::Get().directx.features.resourcesDump.commandKeys) {

  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "resources"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  m_DumpPath = dumpPath;
}

void ResourceDumpService::CreateResource(unsigned ResourceKey,
                                         ID3D12Resource* resource,
                                         D3D12_RESOURCE_STATES initialState) {
  if (!m_ResourceKeys.Contains(ResourceKey)) {
    return;
  }
  m_ResourceStateTracker.AddResource(ResourceKey, initialState);
  m_Resources[ResourceKey] = resource;
}

void ResourceDumpService::DestroyResource(unsigned ResourceKey) {
  if (!m_ResourceKeys.Contains(ResourceKey)) {
    return;
  }
  m_Resources.erase(ResourceKey);
}

void ResourceDumpService::CommandListCall(unsigned callKey,
                                          ID3D12GraphicsCommandList* commandList) {
  if (!m_CallKeys.Contains(callKey)) {
    return;
  }
  for (unsigned ResourceKey : m_ResourceKeys) {
    auto it = m_Resources.find(ResourceKey);
    if (it != m_Resources.end()) {

      ID3D12Resource* resource = it->second;
      D3D12_RESOURCE_DESC desc = resource->GetDesc();

      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        std::wstring dumpName =
            m_DumpPath + L"/command_" + keyToWStr(callKey) + L"_buffer_O" + keyToWStr(ResourceKey);
        D3D12_RESOURCE_STATES resourceState =
            m_ResourceStateTracker.GetResourceState(commandList, ResourceKey, 0);
        m_ResourceDump.dumpResource(commandList, resource, 0, resourceState, dumpName);
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

              std::wstring dumpName = m_DumpPath;
              dumpName += L"/command_" + keyToWStr(callKey);
              dumpName += L"_texture_O" + keyToWStr(ResourceKey);
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
                  m_ResourceStateTracker.GetResourceState(commandList, ResourceKey, subresource);
              m_ResourceDump.dumpResource(commandList, resource, subresource, resourceState,
                                          dumpName, mipLevel);
            }
          }
        }
      }
    }
  }
}

void ResourceDumpService::ResourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                         c.m_NumBarriers.Value, c.m_pBarriers.ResourceKeys.data());
}

void ResourceDumpService::ExecuteCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum) {
  m_ResourceStateTracker.ExecuteCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(commandLists), commandListNum);
  m_ResourceDump.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                     commandListNum);
}

void ResourceDumpService::CommandQueueWait(unsigned key,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  m_ResourceDump.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDumpService::CommandQueueSignal(unsigned key,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  m_ResourceDump.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDumpService::FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  m_ResourceDump.fenceSignal(key, fenceKey, fenceValue);
}

} // namespace DirectX
} // namespace gits
