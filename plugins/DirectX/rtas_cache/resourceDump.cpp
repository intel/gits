// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDump.h"

#include <fstream>
#include <wincodec.h>
#include <sstream>
#include <iomanip>
#include <cassert>

namespace gits {
namespace DirectX {

ResourceDump::~ResourceDump() {
  waitUntilDumped();
}

void ResourceDump::waitUntilDumped() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      gpuExecutionTracker_.getReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ThreadInfo* threadInfo = static_cast<ThreadInfo*>(executable);
    if (threadInfo->dumpThread->joinable()) {
      threadInfo->dumpThread->join();
    }
    delete threadInfo;
  }
  executables.clear();
}

void ResourceDump::dumpResource(ID3D12GraphicsCommandList* commandList,
                                ID3D12Resource* resource,
                                unsigned subresource,
                                D3D12_RESOURCE_STATES resourceState,
                                const std::wstring& dumpName,
                                unsigned mipLevel,
                                DXGI_FORMAT format) {

  DumpInfo* dumpInfo = new DumpInfo();
  dumpInfo->subresource = subresource;
  dumpInfo->dumpName = dumpName;
  dumpInfo->mipLevel = mipLevel;
  dumpInfo->format = format;

  stageResource(commandList, resource, resourceState, *dumpInfo);
}

void ResourceDump::stageResource(ID3D12GraphicsCommandList* commandList,
                                 ID3D12Resource* resource,
                                 D3D12_RESOURCE_STATES resourceState,
                                 DumpInfo& dumpInfo,
                                 bool dependent) {

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
  assert(hr == S_OK);

  dumpInfo.desc = resource->GetDesc();
  if (dumpInfo.format != DXGI_FORMAT_UNKNOWN) {
    dumpInfo.desc.Format = dumpInfo.format;
  }

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
  if (dumpInfo.desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    D3D12_RESOURCE_DESC desc = dumpInfo.desc;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    UINT64 size{};
    device->GetCopyableFootprints(&desc, dumpInfo.subresource, 1, 0, &footprint, nullptr, nullptr,
                                  &size);
    dumpInfo.size = size;
    dumpInfo.rowPitch = footprint.Footprint.RowPitch;
  } else if (!dumpInfo.size) {
    dumpInfo.size = dumpInfo.desc.Width;
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
    desc.Width = dumpInfo.size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                 D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                 IID_PPV_ARGS(&dumpInfo.stagingBuffer));
    assert(hr == S_OK);
  }
  if (dumpInfo.desc.SampleDesc.Count > 1) {
    {
      D3D12_HEAP_PROPERTIES heapProperties{};
      heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      heapProperties.CreationNodeMask = 1;
      heapProperties.VisibleNodeMask = 1;

      D3D12_RESOURCE_DESC desc = dumpInfo.desc;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

      HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                                                   D3D12_RESOURCE_STATE_RESOLVE_DEST, nullptr,
                                                   IID_PPV_ARGS(&dumpInfo.resolvedResource));
      assert(hr == S_OK);
    }

    D3D12_RESOURCE_BARRIER barrier{};
    if (resourceState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE) {
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.pResource = resource;
      barrier.Transition.Subresource = dumpInfo.subresource;
      barrier.Transition.StateBefore = resourceState;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

      commandList->ResourceBarrier(1, &barrier);
    }

    commandList->ResolveSubresource(dumpInfo.resolvedResource.Get(), dumpInfo.subresource, resource,
                                    dumpInfo.subresource, dumpInfo.desc.Format);

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
        dumpInfo.resolvedResource.Get() ? dumpInfo.resolvedResource.Get() : resource;
    barrier.Transition.Subresource = dumpInfo.subresource;
    barrier.Transition.StateBefore =
        dumpInfo.resolvedResource.Get() ? D3D12_RESOURCE_STATE_RESOLVE_DEST : resourceState;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    commandList->ResourceBarrier(1, &barrier);
  }

  if (dumpInfo.desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    commandList->CopyBufferRegion(dumpInfo.stagingBuffer.Get(), dumpInfo.subresource, resource,
                                  dumpInfo.offset, dumpInfo.size);
  } else {
    D3D12_TEXTURE_COPY_LOCATION dest{};
    dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dest.pResource = dumpInfo.stagingBuffer.Get();
    dest.PlacedFootprint.Footprint = footprint.Footprint;
    dest.PlacedFootprint.Offset = 0;
    D3D12_TEXTURE_COPY_LOCATION src{};
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.pResource = dumpInfo.resolvedResource.Get() ? dumpInfo.resolvedResource.Get() : resource;
    src.SubresourceIndex = dumpInfo.subresource;
    commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
  }

  if (!dumpInfo.resolvedResource.Get() && resourceState != D3D12_RESOURCE_STATE_COPY_SOURCE &&
      resourceState != D3D12_RESOURCE_STATE_GENERIC_READ) {
    barrier.Transition.StateAfter = barrier.Transition.StateBefore;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;

    commandList->ResourceBarrier(1, &barrier);
  }

  if (!dependent) {
    stagedResources_[commandList].push_back(&dumpInfo);
  }
}

void ResourceDump::executeCommandLists(unsigned key,
                                       unsigned commandQueueKey,
                                       ID3D12CommandQueue* commandQueue,
                                       ID3D12CommandList** commandLists,
                                       unsigned commandListNum) {

  bool found = false;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = stagedResources_.find(commandLists[i]);
    if (it != stagedResources_.end()) {
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }

  waitUntilDumped();
  initFence(commandQueue);

  ++fenceValue_;
  HRESULT hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
  hr = commandQueue->Signal(fence_, fenceValue_);
  assert(hr == S_OK);

  ThreadInfo* threadInfo = new ThreadInfo();
  threadInfo->fenceValue = fenceValue_;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = stagedResources_.find(commandLists[i]);
    if (it != stagedResources_.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        threadInfo->dumpInfos.emplace_back(dumpInfo);
      }
      stagedResources_.erase(it);
    }
  }

  threadInfo->dumpThread =
      std::make_unique<std::thread>(&ResourceDump::dumpStagedResources, this, threadInfo);

  gpuExecutionTracker_.execute(key, commandQueueKey, threadInfo);
}

void ResourceDump::commandQueueWait(unsigned key,
                                    unsigned commandQueueKey,
                                    unsigned fenceKey,
                                    UINT64 fenceValue) {
  gpuExecutionTracker_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDump::commandQueueSignal(unsigned key,
                                      unsigned commandQueueKey,
                                      unsigned fenceKey,
                                      UINT64 fenceValue) {
  gpuExecutionTracker_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void ResourceDump::fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  gpuExecutionTracker_.fenceSignal(key, fenceKey, fenceValue);
}

void ResourceDump::dumpStagedResources(ThreadInfo* threadInfo) {

  struct Cleanup {
    Cleanup(ThreadInfo* threadInfo_) : threadInfo(threadInfo_) {}
    ~Cleanup() {
      for (auto& it : threadInfo->dumpInfos) {
        it.reset();
      }
    }
    ThreadInfo* threadInfo;
  } cleanup(threadInfo);

  do {
    WaitForSingleObject(fenceEvent_, 10000);
  } while (fence_->GetCompletedValue() < threadInfo->fenceValue);

  for (auto& dumpInfo : threadInfo->dumpInfos) {
    dumpStagedResource(*dumpInfo);
  }
}

void ResourceDump::initFence(ID3D12DeviceChild* deviceChild) {
  if (!fence_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = deviceChild->GetDevice(IID_PPV_ARGS(&device));
    assert(hr == S_OK);
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(hr == S_OK);
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  }
}

} // namespace DirectX
} // namespace gits
