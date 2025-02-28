// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDump.h"
#include "gits.h"

#include <fstream>
#include <wincodec.h>

namespace gits {
namespace DirectX {

ResourceDump::ResourceDump(bool dumpJpg, const std::string& textureRescaleRange) {
  dumpJpg_ = dumpJpg;
  if (!textureRescaleRange.empty()) {
    try {
      float min = std::stof(textureRescaleRange);
      size_t pos = textureRescaleRange.find('-');
      if (pos == std::string::npos) {
        throw;
      }
      float max = std::stof(textureRescaleRange.substr(pos + 1));
      if (min < 0 || min > 1 || min > max || max < 0 || max > 1) {
        throw;
      }
      textureRescaleRange_ = std::make_pair(min, max);
    } catch (...) {
      Log(ERR) << "Improper TextureRescaleRange.";
    }
  }
}

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
  GITS_ASSERT(hr == S_OK);

  dumpInfo.desc = resource->GetDesc();
  if (dumpInfo.format != DXGI_FORMAT_UNKNOWN) {
    dumpInfo.desc.Format = dumpInfo.format;
  }
  dumpInfo.desc.Format = getDumpableFormat(dumpInfo.desc.Format);

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
    if (hr != S_OK) {
      if (hr == E_OUTOFMEMORY) {
        Log(ERR) << "Resource dumping - create staging buffer failed - E_OUTOFMEMORY - try with "
                    "less resources.";
      } else {
        Log(ERR) << "Resource dumping - create staging buffer failed - 0x" << std::hex << hr
                 << std::dec << " - try with less resources.";
      }
      exit(EXIT_FAILURE);
    }
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
      if (hr != S_OK) {
        if (hr == E_OUTOFMEMORY) {
          Log(ERR)
              << "Resource dumping - create resolved resource failed - E_OUTOFMEMORY - try with "
                 "less resources.";
        } else {
          Log(ERR) << "Resource dumping - create resolved resource failed - 0x" << std::hex << hr
                   << std::dec << " - try with less resources.";
        }
        exit(EXIT_FAILURE);
      }
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
  GITS_ASSERT(hr == S_OK);
  hr = commandQueue->Signal(fence_, fenceValue_);
  GITS_ASSERT(hr == S_OK);

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

  struct Cleanup : public gits::noncopyable {
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

void ResourceDump::dumpStagedResource(DumpInfo& dumpInfo) {
  void* data{};
  HRESULT hr = dumpInfo.stagingBuffer->Map(0, nullptr, &data);
  if (hr != S_OK) {
    auto printHr = [](HRESULT hr) {
      if (hr == E_OUTOFMEMORY) {
        return std::string("E_OUTOFMEMORY");
      } else if (hr == DXGI_ERROR_DEVICE_REMOVED) {
        return std::string("DXGI_ERROR_DEVICE_REMOVED");
      } else {
        std::stringstream s;
        s << "0x" << std::hex << hr;
        return s.str();
      }
    };
    std::string dumpName(dumpInfo.dumpName.begin(), dumpInfo.dumpName.end());
    Log(ERR) << "ResourceDump - Map failed " << printHr(hr) << " " << dumpName;
    return;
  }

  if (dumpInfo.desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    dumpBuffer(dumpInfo, data);
  } else {
    dumpTexture(dumpInfo, data);
  }

  dumpInfo.stagingBuffer->Unmap(0, nullptr);
}

void ResourceDump::initFence(ID3D12DeviceChild* deviceChild) {
  if (!fence_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = deviceChild->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    GITS_ASSERT(hr == S_OK);
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    GITS_ASSERT(fenceEvent_);
  }
}

void ResourceDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::ofstream stream((dumpInfo.dumpName + L".txt").c_str());
  uint8_t* byteData = static_cast<uint8_t*>(data);
  for (unsigned i = 0; i < dumpInfo.desc.Width; ++i) {
    if (i % 8 == 0 && i > 0) {
      stream << "\n";
    }
    stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byteData[i]);
  }
}

void ResourceDump::dumpTexture(DumpInfo& dumpInfo, void* data) {
  DXGI_FORMAT format = dumpInfo.desc.Format;
  if (format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS) {
    format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  } else if (format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS) {
    format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  }

  DXGI_FORMAT destFormat =
      ::DirectX::IsSRGB(format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

  unsigned depth = dumpInfo.desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D
                       ? dumpInfo.desc.DepthOrArraySize
                       : 1;

  for (unsigned slice = 0; slice < depth; ++slice) {

    std::wstring dumpNameW = dumpInfo.dumpName;
    if (depth > 1) {
      dumpNameW += L"_slice_" + std::to_wstring(slice);
    }
    std::string dumpNameA(dumpNameW.begin(), dumpNameW.end());

    ::DirectX::Image image{};
    image.width = dumpInfo.desc.Width / pow(2, dumpInfo.mipLevel);
    image.height = dumpInfo.desc.Height / pow(2, dumpInfo.mipLevel);
    image.format = format;
    image.rowPitch = dumpInfo.rowPitch;
    image.slicePitch = dumpInfo.size / depth;
    image.pixels = reinterpret_cast<uint8_t*>(data) + slice * image.slicePitch;

    static bool initialized = false;
    const ::DirectX::Image* imageConverted{};
    ::DirectX::ScratchImage scratchImage;
    if (::DirectX::IsCompressed(image.format)) {
      HRESULT hr = ::DirectX::Decompress(image, destFormat, scratchImage);
      if (hr != S_OK) {
        Log(ERR) << "Dumping " + dumpNameA + " format " << formatToString(dumpInfo.desc.Format)
                 << " failed in Decompress 0x" << std::hex << hr << std::dec;
        return;
      }
      imageConverted = scratchImage.GetImage(0, 0, 0);
    } else if (image.format != DXGI_FORMAT_R8G8B8A8_UNORM &&
               image.format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
      HRESULT hr{};
      if (textureRescaleRange_.has_value()) {
        hr = rescaleTexture(image, scratchImage);
      } else {
        auto convert = [&]() {
          hr = ::DirectX::Convert(image, destFormat, ::DirectX::TEX_FILTER_DEFAULT,
                                  ::DirectX::TEX_THRESHOLD_DEFAULT, scratchImage);
        };
        convert();
        if (!initialized && hr != S_OK) {
          CoInitializeEx(nullptr, COINIT_MULTITHREADED);
          convert();
          initialized = true;
        }
      }
      if (hr != S_OK) {
        Log(ERR) << "Dumping " + dumpNameA + " format " << formatToString(dumpInfo.desc.Format)
                 << " failed 0x" << std::hex << hr << std::dec;
        return;
      }

      imageConverted = scratchImage.GetImage(0, 0, 0);
    } else {
      imageConverted = &image;
    }

    HRESULT hr{};
    auto saveToWICFile = [&]() {
      hr = SaveToWICFile(
          image, ::DirectX::WIC_FLAGS_FORCE_SRGB,
          GetWICCodec(dumpJpg_ ? ::DirectX::WIC_CODEC_JPEG : ::DirectX::WIC_CODEC_PNG),
          (dumpNameW + (dumpJpg_ ? L".jpg" : L".png")).c_str(),
          dumpJpg_ ? nullptr : &GUID_WICPixelFormat48bppRGB);
    };
    saveToWICFile();
    if (!initialized && hr != S_OK) {
      CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      saveToWICFile();
      initialized = true;
    }
    if (hr != S_OK) {
      Log(ERR) << "Dumping " + dumpNameA + " format " << formatToString(dumpInfo.desc.Format)
               << " failed in SaveToWICFile 0x" << std::hex << hr << std::dec;
    }
  }
}

HRESULT ResourceDump::rescaleTexture(const ::DirectX::Image& srcImage,
                                     ::DirectX::ScratchImage& destScratchImage) {
  HRESULT hr = destScratchImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, srcImage.width,
                                             srcImage.height, 1, 1);
  if (hr != S_OK) {
    return hr;
  }
  ::DirectX::ScratchImage floatScratchImage;
  hr = floatScratchImage.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, srcImage.width,
                                      srcImage.height, 1, 1);
  if (hr != S_OK) {
    return hr;
  }

  hr = ::DirectX::Convert(srcImage, DXGI_FORMAT_R32G32B32A32_FLOAT, ::DirectX::TEX_FILTER_DEFAULT,
                          0, floatScratchImage);
  if (hr != S_OK) {
    return hr;
  }

  const ::DirectX::Image* floatImage = floatScratchImage.GetImage(0, 0, 0);

  float minValue = std::numeric_limits<float>::max();
  float maxValue = std::numeric_limits<float>::min();
  for (unsigned y = 0; y < floatImage->height; ++y) {
    for (unsigned x = 0; x < floatImage->width; ++x) {
      for (unsigned i = 0; i < 4; ++i) {
        if (i == 3) {
          continue;
        }
        float val = *reinterpret_cast<float*>(
            &floatImage
                 ->pixels[y * floatImage->rowPitch + x * 4 * sizeof(float) + i * sizeof(float)]);
        if (minValue > val) {
          minValue = val;
        }
        if (maxValue < val) {
          maxValue = val;
        }
      }
    }
  }

  minValue += textureRescaleRange_->first * (maxValue - minValue);
  maxValue -= (1 - textureRescaleRange_->second) * (maxValue - minValue);

  const ::DirectX::Image* destImage = destScratchImage.GetImage(0, 0, 0);

  for (unsigned y = 0; y < floatImage->height; ++y) {
    for (unsigned x = 0; x < floatImage->width; ++x) {
      for (unsigned i = 0; i < 4; ++i) {
        if (i == 3) {
          continue;
        }
        float src = *reinterpret_cast<float*>(
            &floatImage->pixels[y * floatImage->rowPitch + x * 4 * sizeof(float) + i * 4]);

        uint8_t& dest = destImage->pixels[y * destImage->rowPitch + x * 4 + i];
        if (src < minValue) {
          src = minValue;
        } else if (src > maxValue) {
          src = maxValue;
        }
        dest = (src - minValue) / (maxValue - minValue) * std::numeric_limits<uint8_t>::max();
      }
    }
  }

  return S_OK;
}

DXGI_FORMAT ResourceDump::getDumpableFormat(DXGI_FORMAT format) {
  switch (format) {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case DXGI_FORMAT_R32G32B32_TYPELESS:
    return DXGI_FORMAT_R32G32B32_FLOAT;
  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  case DXGI_FORMAT_R32G32_TYPELESS:
    return DXGI_FORMAT_R32G32_FLOAT;
  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case DXGI_FORMAT_R16G16_TYPELESS:
    return DXGI_FORMAT_R16G16_UNORM;
  case DXGI_FORMAT_R32_TYPELESS:
    return DXGI_FORMAT_R32_FLOAT;
  case DXGI_FORMAT_R8G8_TYPELESS:
    return DXGI_FORMAT_R8G8_UNORM;
  case DXGI_FORMAT_R16_TYPELESS:
    return DXGI_FORMAT_R16_UNORM;
  case DXGI_FORMAT_R8_TYPELESS:
    return DXGI_FORMAT_R8_UNORM;
  case DXGI_FORMAT_BC1_TYPELESS:
    return DXGI_FORMAT_BC1_UNORM;
  case DXGI_FORMAT_BC2_TYPELESS:
    return DXGI_FORMAT_BC2_UNORM;
  case DXGI_FORMAT_BC3_TYPELESS:
    return DXGI_FORMAT_BC3_UNORM;
  case DXGI_FORMAT_BC4_TYPELESS:
    return DXGI_FORMAT_BC4_UNORM;
  case DXGI_FORMAT_BC5_TYPELESS:
    return DXGI_FORMAT_BC5_UNORM;
  case DXGI_FORMAT_BC7_TYPELESS:
    return DXGI_FORMAT_BC7_UNORM;
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    return DXGI_FORMAT_B8G8R8X8_UNORM;
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  case DXGI_FORMAT_D32_FLOAT:
    return DXGI_FORMAT_R32_FLOAT;
  default:
    return format;
  }
}

std::string ResourceDump::formatToString(DXGI_FORMAT value) {
  switch (value) {
  case DXGI_FORMAT_UNKNOWN:
    return "UNKNOWN";
    break;
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    return "R32G32B32A32_TYPELESS";
    break;
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
    return "R32G32B32A32_FLOAT";
    break;
  case DXGI_FORMAT_R32G32B32A32_UINT:
    return "R32G32B32A32_UINT";
    break;
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return "R32G32B32A32_SINT";
    break;
  case DXGI_FORMAT_R32G32B32_TYPELESS:
    return "R32G32B32_TYPELESS";
    break;
  case DXGI_FORMAT_R32G32B32_FLOAT:
    return "R32G32B32_FLOAT";
    break;
  case DXGI_FORMAT_R32G32B32_UINT:
    return "R32G32B32_UINT";
    break;
  case DXGI_FORMAT_R32G32B32_SINT:
    return "R32G32B32_SINT";
    break;
  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    return "R16G16B16A16_TYPELESS";
    break;
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
    return "R16G16B16A16_FLOAT";
    break;
  case DXGI_FORMAT_R16G16B16A16_UNORM:
    return "R16G16B16A16_UNORM";
    break;
  case DXGI_FORMAT_R16G16B16A16_UINT:
    return "R16G16B16A16_UINT";
    break;
  case DXGI_FORMAT_R16G16B16A16_SNORM:
    return "R16G16B16A16_SNORM";
    break;
  case DXGI_FORMAT_R16G16B16A16_SINT:
    return "R16G16B16A16_SINT";
    break;
  case DXGI_FORMAT_R32G32_TYPELESS:
    return "R32G32_TYPELESS";
    break;
  case DXGI_FORMAT_R32G32_FLOAT:
    return "R32G32_FLOAT";
    break;
  case DXGI_FORMAT_R32G32_UINT:
    return "R32G32_UINT";
    break;
  case DXGI_FORMAT_R32G32_SINT:
    return "R32G32_SINT";
    break;
  case DXGI_FORMAT_R32G8X24_TYPELESS:
    return "R32G8X24_TYPELESS";
    break;
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    return "D32_FLOAT_S8X24_UINT";
    break;
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    return "R32_FLOAT_X8X24_TYPELESS";
    break;
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return "X32_TYPELESS_G8X24_UINT";
    break;
  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    return "R10G10B10A2_TYPELESS";
    break;
  case DXGI_FORMAT_R10G10B10A2_UNORM:
    return "R10G10B10A2_UNORM";
    break;
  case DXGI_FORMAT_R10G10B10A2_UINT:
    return "R10G10B10A2_UINT";
    break;
  case DXGI_FORMAT_R11G11B10_FLOAT:
    return "R11G11B10_FLOAT";
    break;
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    return "R8G8B8A8_TYPELESS";
    break;
  case DXGI_FORMAT_R8G8B8A8_UNORM:
    return "R8G8B8A8_UNORM";
    break;
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    return "R8G8B8A8_UNORM_SRGB";
    break;
  case DXGI_FORMAT_R8G8B8A8_UINT:
    return "R8G8B8A8_UINT";
    break;
  case DXGI_FORMAT_R8G8B8A8_SNORM:
    return "R8G8B8A8_SNORM";
    break;
  case DXGI_FORMAT_R8G8B8A8_SINT:
    return "R8G8B8A8_SINT";
    break;
  case DXGI_FORMAT_R16G16_TYPELESS:
    return "R16G16_TYPELESS";
    break;
  case DXGI_FORMAT_R16G16_FLOAT:
    return "R16G16_FLOAT";
    break;
  case DXGI_FORMAT_R16G16_UNORM:
    return "R16G16_UNORM";
    break;
  case DXGI_FORMAT_R16G16_UINT:
    return "R16G16_UINT";
    break;
  case DXGI_FORMAT_R16G16_SNORM:
    return "R16G16_SNORM";
    break;
  case DXGI_FORMAT_R16G16_SINT:
    return "R16G16_SINT";
    break;
  case DXGI_FORMAT_R32_TYPELESS:
    return "R32_TYPELESS";
    break;
  case DXGI_FORMAT_D32_FLOAT:
    return "D32_FLOAT";
    break;
  case DXGI_FORMAT_R32_FLOAT:
    return "R32_FLOAT";
    break;
  case DXGI_FORMAT_R32_UINT:
    return "R32_UINT";
    break;
  case DXGI_FORMAT_R32_SINT:
    return "R32_SINT";
    break;
  case DXGI_FORMAT_R24G8_TYPELESS:
    return "R24G8_TYPELESS";
    break;
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
    return "D24_UNORM_S8_UINT";
    break;
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    return "R24_UNORM_X8_TYPELESS";
    break;
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    return "X24_TYPELESS_G8_UINT";
    break;
  case DXGI_FORMAT_R8G8_TYPELESS:
    return "R8G8_TYPELESS";
    break;
  case DXGI_FORMAT_R8G8_UNORM:
    return "R8G8_UNORM";
    break;
  case DXGI_FORMAT_R8G8_UINT:
    return "R8G8_UINT";
    break;
  case DXGI_FORMAT_R8G8_SNORM:
    return "R8G8_SNORM";
    break;
  case DXGI_FORMAT_R8G8_SINT:
    return "R8G8_SINT";
    break;
  case DXGI_FORMAT_R16_TYPELESS:
    return "R16_TYPELESS";
    break;
  case DXGI_FORMAT_R16_FLOAT:
    return "R16_FLOAT";
    break;
  case DXGI_FORMAT_D16_UNORM:
    return "D16_UNORM";
    break;
  case DXGI_FORMAT_R16_UNORM:
    return "R16_UNORM";
    break;
  case DXGI_FORMAT_R16_UINT:
    return "R16_UINT";
    break;
  case DXGI_FORMAT_R16_SNORM:
    return "R16_SNORM";
    break;
  case DXGI_FORMAT_R16_SINT:
    return "R16_SINT";
    break;
  case DXGI_FORMAT_R8_TYPELESS:
    return "R8_TYPELESS";
    break;
  case DXGI_FORMAT_R8_UNORM:
    return "R8_UNORM";
    break;
  case DXGI_FORMAT_R8_UINT:
    return "R8_UINT";
    break;
  case DXGI_FORMAT_R8_SNORM:
    return "R8_SNORM";
    break;
  case DXGI_FORMAT_R8_SINT:
    return "R8_SINT";
    break;
  case DXGI_FORMAT_A8_UNORM:
    return "A8_UNORM";
    break;
  case DXGI_FORMAT_R1_UNORM:
    return "R1_UNORM";
    break;
  case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    return "R9G9B9E5_SHAREDEXP";
    break;
  case DXGI_FORMAT_R8G8_B8G8_UNORM:
    return "R8G8_B8G8_UNORM";
    break;
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
    return "G8R8_G8B8_UNORM";
    break;
  case DXGI_FORMAT_BC1_TYPELESS:
    return "BC1_TYPELESS";
    break;
  case DXGI_FORMAT_BC1_UNORM:
    return "BC1_UNORM";
    break;
  case DXGI_FORMAT_BC1_UNORM_SRGB:
    return "BC1_UNORM_SRGB";
    break;
  case DXGI_FORMAT_BC2_TYPELESS:
    return "BC2_TYPELESS";
    break;
  case DXGI_FORMAT_BC2_UNORM:
    return "BC2_UNORM";
    break;
  case DXGI_FORMAT_BC2_UNORM_SRGB:
    return "BC2_UNORM_SRGB";
    break;
  case DXGI_FORMAT_BC3_TYPELESS:
    return "BC3_TYPELESS";
    break;
  case DXGI_FORMAT_BC3_UNORM:
    return "BC3_UNORM";
    break;
  case DXGI_FORMAT_BC3_UNORM_SRGB:
    return "BC3_UNORM_SRGB";
    break;
  case DXGI_FORMAT_BC4_TYPELESS:
    return "BC4_TYPELESS";
    break;
  case DXGI_FORMAT_BC4_UNORM:
    return "BC4_UNORM";
    break;
  case DXGI_FORMAT_BC4_SNORM:
    return "BC4_SNORM";
    break;
  case DXGI_FORMAT_BC5_TYPELESS:
    return "BC5_TYPELESS";
    break;
  case DXGI_FORMAT_BC5_UNORM:
    return "BC5_UNORM";
    break;
  case DXGI_FORMAT_BC5_SNORM:
    return "BC5_SNORM";
    break;
  case DXGI_FORMAT_B5G6R5_UNORM:
    return "B5G6R5_UNORM";
    break;
  case DXGI_FORMAT_B5G5R5A1_UNORM:
    return "B5G5R5A1_UNORM";
    break;
  case DXGI_FORMAT_B8G8R8A8_UNORM:
    return "B8G8R8A8_UNORM";
    break;
  case DXGI_FORMAT_B8G8R8X8_UNORM:
    return "B8G8R8X8_UNORM";
    break;
  case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    return "R10G10B10_XR_BIAS_A2_UNORM";
    break;
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    return "B8G8R8A8_TYPELESS";
    break;
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    return "B8G8R8A8_UNORM_SRGB";
    break;
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    return "B8G8R8X8_TYPELESS";
    break;
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return "B8G8R8X8_UNORM_SRGB";
    break;
  case DXGI_FORMAT_BC6H_TYPELESS:
    return "BC6H_TYPELESS";
    break;
  case DXGI_FORMAT_BC6H_UF16:
    return "BC6H_UF16";
    break;
  case DXGI_FORMAT_BC6H_SF16:
    return "BC6H_SF16";
    break;
  case DXGI_FORMAT_BC7_TYPELESS:
    return "BC7_TYPELESS";
    break;
  case DXGI_FORMAT_BC7_UNORM:
    return "BC7_UNORM";
    break;
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return "BC7_UNORM_SRGB";
    break;
  case DXGI_FORMAT_AYUV:
    return "AYUV";
    break;
  case DXGI_FORMAT_Y410:
    return "Y410";
    break;
  case DXGI_FORMAT_Y416:
    return "Y416";
    break;
  case DXGI_FORMAT_NV12:
    return "NV12";
    break;
  case DXGI_FORMAT_P010:
    return "P010";
    break;
  case DXGI_FORMAT_P016:
    return "P016";
    break;
  case DXGI_FORMAT_420_OPAQUE:
    return "420_OPAQUE";
    break;
  case DXGI_FORMAT_YUY2:
    return "YUY2";
    break;
  case DXGI_FORMAT_Y210:
    return "Y210";
    break;
  case DXGI_FORMAT_Y216:
    return "Y216";
    break;
  case DXGI_FORMAT_NV11:
    return "NV11";
    break;
  case DXGI_FORMAT_AI44:
    return "AI44";
    break;
  case DXGI_FORMAT_IA44:
    return "IA44";
    break;
  case DXGI_FORMAT_P8:
    return "P8";
    break;
  case DXGI_FORMAT_A8P8:
    return "A8P8";
    break;
  case DXGI_FORMAT_B4G4R4A4_UNORM:
    return "B4G4R4A4_UNORM";
    break;
  case DXGI_FORMAT_P208:
    return "P208";
    break;
  case DXGI_FORMAT_V208:
    return "V208";
    break;
  case DXGI_FORMAT_V408:
    return "V408";
    break;
  case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:
    return "SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
    break;
  case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:
    return "SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";
    break;
  case DXGI_FORMAT_A4B4G4R4_UNORM:
    return "A4B4G4R4_UNORM";
    break;
  case DXGI_FORMAT_FORCE_UINT:
    return "FORCE_UINT";
    break;
  default:
    return "UNKNOWN";
  }
}

} // namespace DirectX
} // namespace gits
