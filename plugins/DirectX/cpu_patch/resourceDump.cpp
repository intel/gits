// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDump.h"
#include "resourceSizeUtils.h"

#include <fstream>
#include <wincodec.h>
#include <sstream>
#include <iomanip>
#include "log.h"

namespace gits {
namespace DirectX {

ResourceDump::~ResourceDump() {
  WaitUntilDumped();
}

void ResourceDump::WaitUntilDumped() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_GpuExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ThreadInfo* threadInfo = static_cast<ThreadInfo*>(executable);
    if (threadInfo->DumpThread->joinable()) {
      threadInfo->DumpThread->join();
    }
    delete threadInfo;
  }
  executables.clear();
}

void ResourceDump::DumpResource(ID3D12GraphicsCommandList* commandList,
                                ID3D12Resource* resource,
                                unsigned subresource,
                                D3D12_RESOURCE_STATES resourceState,
                                const std::wstring& dumpName,
                                unsigned mipLevel,
                                DXGI_FORMAT format) {

  DumpInfo* dumpInfo = new DumpInfo();
  dumpInfo->Subresource = subresource;
  dumpInfo->DumpName = dumpName;
  dumpInfo->MipLevel = mipLevel;
  dumpInfo->SubresourceFormat = format;

  StageResource(commandList, resource, resourceState, *dumpInfo);
}

void ResourceDump::StageResource(ID3D12GraphicsCommandList* commandList,
                                 ID3D12Resource* resource,
                                 D3D12_RESOURCE_STATES resourceState,
                                 DumpInfo& dumpInfo,
                                 bool dependent) {

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  dumpInfo.Desc = resource->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
  if (dumpInfo.Desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    D3D12_RESOURCE_DESC desc = dumpInfo.Desc;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    UINT64 size{};
    GetCopyableFootprintsSafe(device.Get(), &desc, dumpInfo.Subresource, 1, 0, &footprint, nullptr,
                              nullptr, &size);
    dumpInfo.Size = size;
    dumpInfo.RowPitch = footprint.Footprint.RowPitch;
    dumpInfo.SubresourceFormat = footprint.Footprint.Format;
  } else if (!dumpInfo.Size) {
    dumpInfo.Size = dumpInfo.Desc.Width - dumpInfo.Offset;
  }
  {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = dumpInfo.Size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                 D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                 IID_PPV_ARGS(&dumpInfo.StagingBuffer));
    GITS_ASSERT(hr == S_OK);
  }
  if (dumpInfo.Desc.SampleDesc.Count > 1) {
    {
      D3D12_HEAP_PROPERTIES heapProperties{};
      heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      heapProperties.CreationNodeMask = 1;
      heapProperties.VisibleNodeMask = 1;

      D3D12_RESOURCE_DESC desc = dumpInfo.Desc;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

      HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                   D3D12_RESOURCE_STATE_RESOLVE_DEST, nullptr,
                                                   IID_PPV_ARGS(&dumpInfo.ResolvedResource));
      GITS_ASSERT(hr == S_OK);
    }

    D3D12_RESOURCE_BARRIER barrier{};
    if (resourceState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE) {
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.pResource = resource;
      barrier.Transition.Subresource = dumpInfo.Subresource;
      barrier.Transition.StateBefore = resourceState;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

      commandList->ResourceBarrier(1, &barrier);
    }

    commandList->ResolveSubresource(dumpInfo.ResolvedResource.Get(), dumpInfo.Subresource, resource,
                                    dumpInfo.Subresource, dumpInfo.SubresourceFormat);

    if (resourceState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
      barrier.Transition.StateAfter = barrier.Transition.StateBefore;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

      commandList->ResourceBarrier(1, &barrier);
    }
  }

  D3D12_RESOURCE_BARRIER barrier{};
  if (resourceState != D3D12_RESOURCE_STATE_COPY_SOURCE &&
      resourceState != D3D12_RESOURCE_STATE_GENERIC_READ) {
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource =
        dumpInfo.ResolvedResource.Get() ? dumpInfo.ResolvedResource.Get() : resource;
    barrier.Transition.Subresource = dumpInfo.Subresource;
    barrier.Transition.StateBefore =
        dumpInfo.ResolvedResource.Get() ? D3D12_RESOURCE_STATE_RESOLVE_DEST : resourceState;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    commandList->ResourceBarrier(1, &barrier);
  }

  if (dumpInfo.Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    commandList->CopyBufferRegion(dumpInfo.StagingBuffer.Get(), dumpInfo.Subresource, resource,
                                  dumpInfo.Offset, dumpInfo.Size);
  } else {
    D3D12_TEXTURE_COPY_LOCATION dest{};
    dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dest.pResource = dumpInfo.StagingBuffer.Get();
    dest.PlacedFootprint.Footprint = footprint.Footprint;
    dest.PlacedFootprint.Offset = 0;
    D3D12_TEXTURE_COPY_LOCATION src{};
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.pResource = dumpInfo.ResolvedResource.Get() ? dumpInfo.ResolvedResource.Get() : resource;
    src.SubresourceIndex = dumpInfo.Subresource;
    commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
  }

  if (!dumpInfo.ResolvedResource.Get() && resourceState != D3D12_RESOURCE_STATE_COPY_SOURCE &&
      resourceState != D3D12_RESOURCE_STATE_GENERIC_READ) {
    barrier.Transition.StateAfter = barrier.Transition.StateBefore;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;

    commandList->ResourceBarrier(1, &barrier);
  }

  if (!dependent) {
    m_StagedResources[commandList].push_back(&dumpInfo);
  }
}

void ResourceDump::ExecuteCommandLists(unsigned key,
                                       unsigned commandQueueKey,
                                       ID3D12CommandQueue* commandQueue,
                                       ID3D12CommandList** commandLists,
                                       unsigned commandListNum) {

  bool found = false;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = m_StagedResources.find(commandLists[i]);
    if (it != m_StagedResources.end()) {
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }

  WaitUntilDumped();
  InitFence(commandQueue);

  ++m_FenceValue;
  HRESULT hr = m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
  hr = commandQueue->Signal(m_Fence, m_FenceValue);
  GITS_ASSERT(hr == S_OK);

  ThreadInfo* threadInfo = new ThreadInfo();
  threadInfo->FenceValue = m_FenceValue;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = m_StagedResources.find(commandLists[i]);
    if (it != m_StagedResources.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        threadInfo->DumpInfos.emplace_back(dumpInfo);
      }
      m_StagedResources.erase(it);
    }
  }

  threadInfo->DumpThread =
      std::make_unique<std::thread>(&ResourceDump::DumpStagedResources, this, threadInfo);

  m_GpuExecutionTracker.Execute(key, commandQueueKey, threadInfo);
}

void ResourceDump::CommandQueueWait(unsigned key,
                                    unsigned commandQueueKey,
                                    unsigned fenceKey,
                                    UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDump::CommandQueueSignal(unsigned key,
                                      unsigned commandQueueKey,
                                      unsigned fenceKey,
                                      UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDump::FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  m_GpuExecutionTracker.FenceSignal(key, fenceKey, fenceValue);
}

void ResourceDump::DumpStagedResources(ThreadInfo* threadInfo) {

  struct Cleanup {
    Cleanup(ThreadInfo* threadInfo_) : threadInfo(threadInfo_) {}
    ~Cleanup() {
      for (auto& it : threadInfo->DumpInfos) {
        it.reset();
      }
    }
    ThreadInfo* threadInfo;
  } cleanup(threadInfo);

  do {
    WaitForSingleObject(m_FenceEvent, 10000);
  } while (m_Fence->GetCompletedValue() < threadInfo->FenceValue);

  for (auto& dumpInfo : threadInfo->DumpInfos) {
    DumpStagedResource(*dumpInfo);
  }
}

void ResourceDump::InitFence(ID3D12DeviceChild* deviceChild) {
  if (!m_Fence) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = deviceChild->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
    GITS_ASSERT(hr == S_OK);
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  }
}

} // namespace DirectX
} // namespace gits
