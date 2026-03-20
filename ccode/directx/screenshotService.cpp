// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotService.h"

#include <plog/Log.h>

#include <d3d12.h>
#include <dxgi1_4.h>

namespace directx {

static Format ToFormat(DXGI_FORMAT dxgiFormat) {
  switch (dxgiFormat) {
  case DXGI_FORMAT_R8G8B8A8_UNORM:
    return Format::R8G8B8A8_UNORM;
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    return Format::R8G8B8A8_UNORM_SRGB;
  case DXGI_FORMAT_B8G8R8X8_UNORM:
    return Format::B8G8R8X8_UNORM;
  case DXGI_FORMAT_B8G8R8A8_UNORM:
    return Format::B8G8R8A8_UNORM;
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    return Format::B8G8R8A8_UNORM_SRGB;
  case DXGI_FORMAT_R10G10B10A2_UNORM:
    return Format::R10G10B10A2_UNORM;
  default:
    return Format::Unknown;
  }
}

ScreenshotService& ScreenshotService::Get() {
  static ScreenshotService s_Instance;
  return s_Instance;
}

void ScreenshotService::Initialize(const std::filesystem::path& outputDir) {
  m_OutputDir = outputDir;
  m_Enabled = true;
  LOG_INFO << "CCode - ScreenshotService enabled";
  LOG_INFO << "CCode - Screenshots will be saved in " << outputDir;
}

void ScreenshotService::RegisterSwapChain(unsigned key,
                                          IDXGISwapChain* swapChain,
                                          ID3D12CommandQueue* queue) {
  Entry& e = m_Entries[key];
  swapChain->QueryInterface(IID_PPV_ARGS(&e.SwapChain));
  queue->GetDevice(IID_PPV_ARGS(&e.Device));
  e.Queue = queue;
  e.FenceValue = 0;
  e.ReadbackBuffer.Reset();
  e.FrameIndex = 0;

  // Create D3D12 resources for copying back buffer and synchronization
  e.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&e.Fence));
  e.Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&e.CmdAllocator));
  e.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, e.CmdAllocator.Get(), nullptr,
                              IID_PPV_ARGS(&e.CmdList));
  e.CmdList->Close();
}

void ScreenshotService::CaptureFrame(unsigned key) {
  if (!m_Enabled) {
    return;
  }

  // Find the entry for the given swap chain key
  auto it = m_Entries.find(key);
  if (it == m_Entries.end()) {
    return;
  }

  Entry& entry = it->second;
  if (!entry.SwapChain || !entry.Device || !entry.Queue || !entry.CmdAllocator) {
    return;
  }

  SaveBackBuffer(key, entry);

  ++entry.FrameIndex;
}

bool ScreenshotService::CreateReadbackBuffer(Entry& entry,
                                             D3D12_SUBRESOURCE_FOOTPRINT& backBufferFootprint) {
  UINT64 backBufferSize = static_cast<UINT64>(backBufferFootprint.RowPitch) *
                          static_cast<UINT64>(backBufferFootprint.Height) *
                          static_cast<UINT64>(backBufferFootprint.Depth);
  // Recreate the readback buffer if needed dimensions exceed the current buffer size
  if (backBufferSize > entry.ReadbackBufferSize) {
    entry.ReadbackBuffer.Reset();
    entry.ReadbackBufferSize = backBufferSize;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = backBufferSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    entry.Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                          D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                          IID_PPV_ARGS(&entry.ReadbackBuffer));
  }

  return entry.ReadbackBuffer != nullptr;
}

void ScreenshotService::SaveBackBuffer(unsigned key, Entry& entry) {
  // Get the current back buffer
  ComPtr<ID3D12Resource> backBuffer;
  UINT backBufferIndex = entry.SwapChain->GetCurrentBackBufferIndex();
  HRESULT hr = entry.SwapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&backBuffer));
  if (FAILED(hr)) {
    LOG_ERROR << "CCode - Failed to get back buffer for ScreenshotService";
    return;
  }

  // Get the footprint of the back buffer
  D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  entry.Device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);

  if (!CreateReadbackBuffer(entry, footprint.Footprint)) {
    LOG_ERROR << "CCode - Failed to create readback buffer for ScreenshotService";
    return;
  }

  if (!CopyBackBuffer(entry, backBuffer.Get(), footprint)) {
    LOG_ERROR << "CCode - Failed to copy texture for ScreenshotService";
    return;
  }

  if (!SaveToFile(key, entry, footprint.Footprint)) {
    LOG_ERROR << "CCode - Failed to write screenshot";
    return;
  }
}

bool ScreenshotService::CopyBackBuffer(Entry& entry,
                                       ID3D12Resource* backBuffer,
                                       D3D12_PLACED_SUBRESOURCE_FOOTPRINT& backBufferFootprint) {
  entry.CmdAllocator->Reset();
  entry.CmdList->Reset(entry.CmdAllocator.Get(), nullptr);

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = backBuffer;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  entry.CmdList->ResourceBarrier(1, &barrier);

  D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
  srcLoc.pResource = backBuffer;
  srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  srcLoc.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
  dstLoc.pResource = entry.ReadbackBuffer.Get();
  dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dstLoc.PlacedFootprint = backBufferFootprint;

  entry.CmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  entry.CmdList->ResourceBarrier(1, &barrier);

  entry.CmdList->Close();

  ID3D12CommandList* lists[] = {entry.CmdList.Get()};
  entry.Queue->ExecuteCommandLists(1, lists);

  ++entry.FenceValue;
  entry.Queue->Signal(entry.Fence.Get(), entry.FenceValue);
  HRESULT hr = entry.Fence->SetEventOnCompletion(entry.FenceValue, nullptr);
  return SUCCEEDED(hr);
}

bool ScreenshotService::SaveToFile(unsigned key,
                                   Entry& entry,
                                   D3D12_SUBRESOURCE_FOOTPRINT& backBufferFootprint) {
  void* mapped = nullptr;
  D3D12_RANGE readRange = {0, entry.ReadbackBufferSize};
  HRESULT hr = entry.ReadbackBuffer->Map(0, &readRange, &mapped);
  if (FAILED(hr)) {
    LOG_ERROR << "CCode - Failed to map readback buffer for ScreenshotService";
    return false;
  }

  std::stringstream ss;
  ss << "SwapChain_O" << key << "_Frame_" << entry.FrameIndex << ".png";
  std::filesystem::path outputFile = m_OutputDir / ss.str();
  bool result = WriteImage(outputFile.string(), static_cast<const uint8_t*>(mapped),
                           ToFormat(backBufferFootprint.Format), backBufferFootprint.Width,
                           backBufferFootprint.Height, backBufferFootprint.RowPitch);

  entry.ReadbackBuffer->Unmap(0, nullptr);
  return result;
}

ScreenshotService::~ScreenshotService() {
  m_Enabled = false;
  m_Entries.clear();
}

} // namespace directx
