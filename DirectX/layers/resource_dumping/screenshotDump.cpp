// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotDump.h"
#include "gits.h"
#include "configurationLib.h"

#include <DirectXTex.h>
#include <wincodec.h>
#include <set>

namespace gits {
namespace DirectX {

ScreenshotDump::ScreenshotDump(ID3D12CommandQueue* commandQueue) {
  std::string format = Configurator::Get().directx.features.screenshots.format;
  if (format == "jpg") {
    dumpJpg = true;
  }

  HRESULT hr = commandQueue->QueryInterface(IID_PPV_ARGS(&commandQueue_));
  GITS_ASSERT(hr == S_OK);
  hr = commandQueue->GetDevice(IID_PPV_ARGS(&device_));
  GITS_ASSERT(hr == S_OK);
  D3D12_COMMAND_QUEUE_DESC desc = commandQueue->GetDesc();
  hr = device_->CreateCommandAllocator(desc.Type, IID_PPV_ARGS(&commandAllocator_));
  GITS_ASSERT(hr == S_OK);
  hr = device_->CreateCommandList(0, desc.Type, commandAllocator_.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList_));
  GITS_ASSERT(hr == S_OK);
  hr = commandList_->Close();
  GITS_ASSERT(hr == S_OK);
  hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  GITS_ASSERT(hr == S_OK);
  fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  GITS_ASSERT(fenceEvent_);
}

ScreenshotDump::~ScreenshotDump() {
  if (dumpThread_ && dumpThread_->joinable()) {
    dumpThread_->join();
  }
  if (fenceEvent_) {
    CloseHandle(fenceEvent_);
  }
}

void ScreenshotDump::dump(ID3D12Resource* backBuffer, const std::wstring& dumpName) {
  if (dumpThread_ && dumpThread_->joinable()) {
    dumpThread_->join();
  }

  HRESULT hr = commandAllocator_->Reset();
  GITS_ASSERT(hr == S_OK);

  hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
  GITS_ASSERT(hr == S_OK);

  D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
  if (desc.Width != backBufferDesc_.Width || desc.Height != backBufferDesc_.Height ||
      desc.Format != backBufferDesc_.Format) {
    backBufferDesc_ = desc;
    createStagingBuffer();
  }

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = backBuffer;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

  commandList_->ResourceBarrier(1, &barrier);

  D3D12_TEXTURE_COPY_LOCATION dest{};
  dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dest.pResource = stagingBuffer_.Get();
  dest.PlacedFootprint.Footprint = backBufferFootprint_.Footprint;
  dest.PlacedFootprint.Offset = 0;
  D3D12_TEXTURE_COPY_LOCATION src{};
  src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  src.pResource = backBuffer;
  src.SubresourceIndex = 0;
  commandList_->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

  barrier.Transition.StateAfter = barrier.Transition.StateBefore;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;

  commandList_->ResourceBarrier(1, &barrier);

  commandList_->Close();

  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  commandQueue_->ExecuteCommandLists(1, commandLists);

  ++fenceValue_;
  hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
  GITS_ASSERT(hr == S_OK);
  hr = commandQueue_->Signal(fence_.Get(), fenceValue_);
  GITS_ASSERT(hr == S_OK);

  dumpThread_ = std::make_unique<std::thread>(&ScreenshotDump::dumpStagedResource, this, dumpName);
}

void ScreenshotDump::createStagingBuffer() {
  if (stagingBuffer_) {
    stagingBuffer_->Release();
  }

  device_->GetCopyableFootprints(&backBufferDesc_, 0, 1, 0, &backBufferFootprint_, nullptr, nullptr,
                                 nullptr);
  UINT size = backBufferFootprint_.Footprint.RowPitch * backBufferFootprint_.Footprint.Height;

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

  HRESULT hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                                &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr, IID_PPV_ARGS(&stagingBuffer_));
  GITS_ASSERT(hr == S_OK);
}

void ScreenshotDump::dumpStagedResource(std::wstring dumpName) {
  WaitForSingleObject(fenceEvent_, INFINITE);
  GITS_ASSERT(stagingBuffer_.Get());

  void* data{};
  HRESULT hr = stagingBuffer_->Map(0, nullptr, &data);
  GITS_ASSERT(hr == S_OK);

  ::DirectX::Image image{};
  image.width = backBufferDesc_.Width;
  image.height = backBufferDesc_.Height;
  image.format = backBufferDesc_.Format;
  image.rowPitch = backBufferFootprint_.Footprint.RowPitch;
  image.slicePitch = backBufferFootprint_.Footprint.RowPitch * backBufferDesc_.Height;
  image.pixels = reinterpret_cast<uint8_t*>(data) + backBufferFootprint_.Offset;

  auto saveToWICFile = [&]() {
    hr = SaveToWICFile(image, ::DirectX::WIC_FLAGS_FORCE_SRGB,
                       GetWICCodec(dumpJpg ? ::DirectX::WIC_CODEC_JPEG : ::DirectX::WIC_CODEC_PNG),
                       (dumpName + (dumpJpg ? L".jpg" : L".png")).c_str(),
                       dumpJpg ? nullptr : &GUID_WICPixelFormat48bppRGB);
  };
  saveToWICFile();
  static bool initialized = false;
  if (!initialized && hr != S_OK) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    saveToWICFile();
    initialized = true;
  }
  if (hr != S_OK) {
    std::string name(dumpName.begin(), dumpName.end());
    if (hr == 0x80070032) {
      Log(ERR) << "Dumping " + name + " failed: format not supported";
    } else {
      Log(ERR) << "Dumping " + name + " failed: 0x" << std::hex << hr << std::dec;
    }
  }

  stagingBuffer_->Unmap(0, nullptr);
}

} // namespace DirectX
} // namespace gits
