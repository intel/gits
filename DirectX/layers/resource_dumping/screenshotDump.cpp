// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotDump.h"
#include "resourceSizeUtils.h"
#include "gits.h"
#include "log.h"
#include "configurationLib.h"
#include "imageWriter.h"

#include <set>

namespace gits {
namespace DirectX {

ScreenshotDump::ScreenshotDump(ID3D12CommandQueue* commandQueue) {
  m_Format = Configurator::Get().directx.features.screenshots.format;

  HRESULT hr = commandQueue->QueryInterface(IID_PPV_ARGS(&m_CommandQueue));
  GITS_ASSERT(hr == S_OK);
  hr = commandQueue->GetDevice(IID_PPV_ARGS(&m_Device));
  GITS_ASSERT(hr == S_OK);
  D3D12_COMMAND_QUEUE_DESC desc = commandQueue->GetDesc();
  hr = m_Device->CreateCommandAllocator(desc.Type, IID_PPV_ARGS(&m_CommandAllocator));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateCommandList(0, desc.Type, m_CommandAllocator.Get(), nullptr,
                                   IID_PPV_ARGS(&m_CommandList));
  GITS_ASSERT(hr == S_OK);
  hr = m_CommandList->Close();
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
  GITS_ASSERT(hr == S_OK);
  m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  GITS_ASSERT(m_FenceEvent);
}

ScreenshotDump::~ScreenshotDump() {
  if (m_DumpThread && m_DumpThread->joinable()) {
    m_DumpThread->join();
  }
  if (m_FenceEvent) {
    CloseHandle(m_FenceEvent);
  }
}

void ScreenshotDump::dump(ID3D12Resource* backBuffer, const std::wstring& dumpName) {
  if (m_DumpThread && m_DumpThread->joinable()) {
    m_DumpThread->join();
  }

  HRESULT hr = m_CommandAllocator->Reset();
  GITS_ASSERT(hr == S_OK);

  hr = m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
  GITS_ASSERT(hr == S_OK);

  D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
  if (desc.Width != m_BackBufferDesc.Width || desc.Height != m_BackBufferDesc.Height ||
      desc.Format != m_BackBufferDesc.Format) {
    m_BackBufferDesc = desc;
    createStagingBuffer();
  }

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = backBuffer;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

  m_CommandList->ResourceBarrier(1, &barrier);

  D3D12_TEXTURE_COPY_LOCATION dest{};
  dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dest.pResource = m_StagingBuffer.Get();
  dest.PlacedFootprint.Footprint = m_BackBufferFootprint.Footprint;
  dest.PlacedFootprint.Offset = 0;
  D3D12_TEXTURE_COPY_LOCATION src{};
  src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  src.pResource = backBuffer;
  src.SubresourceIndex = 0;
  m_CommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

  barrier.Transition.StateAfter = barrier.Transition.StateBefore;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;

  m_CommandList->ResourceBarrier(1, &barrier);

  m_CommandList->Close();

  ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
  m_CommandQueue->ExecuteCommandLists(1, commandLists);

  ++m_FenceValue;
  hr = m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
  GITS_ASSERT(hr == S_OK);
  hr = m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue);
  GITS_ASSERT(hr == S_OK);

  m_DumpThread = std::make_unique<std::thread>(&ScreenshotDump::dumpStagedResource, this, dumpName);
}

void ScreenshotDump::createStagingBuffer() {
  if (m_StagingBuffer) {
    m_StagingBuffer->Release();
  }

  GetCopyableFootprintsSafe(m_Device.Get(), &m_BackBufferDesc, 0, 1, 0, &m_BackBufferFootprint,
                            nullptr, nullptr, nullptr);
  UINT size = m_BackBufferFootprint.Footprint.RowPitch * m_BackBufferFootprint.Footprint.Height;

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = size;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  HRESULT hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                                 &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                 nullptr, IID_PPV_ARGS(&m_StagingBuffer));
  GITS_ASSERT(hr == S_OK);
}

void ScreenshotDump::dumpStagedResource(std::wstring dumpName) {
  WaitForSingleObject(m_FenceEvent, INFINITE);
  GITS_ASSERT(m_StagingBuffer.Get());

  void* data{};
  HRESULT hr = m_StagingBuffer->Map(0, nullptr, &data);
  GITS_ASSERT(hr == S_OK);

  uint8_t* pixels = reinterpret_cast<uint8_t*>(data) + m_BackBufferFootprint.Offset;
  writeImage(dumpName, m_Format, pixels, m_BackBufferDesc.Format, m_BackBufferDesc.Width,
             m_BackBufferDesc.Height, m_BackBufferFootprint.Footprint.RowPitch);

  m_StagingBuffer->Unmap(0, nullptr);
}

} // namespace DirectX
} // namespace gits
