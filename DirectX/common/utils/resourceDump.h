// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gpuExecutionTracker.h"
#include "configurationLib.h"
#include "tools_lite.h"

#include <d3d12.h>
#include <memory>
#include <thread>
#include <wrl/client.h>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <DirectXTex.h>
#include <atomic>

namespace gits {
namespace DirectX {

class ResourceDump : public gits::noncopyable {
public:
  ResourceDump(ImageFormat format = ImageFormat::JPEG, const std::string& textureRescaleRange = "");
  virtual ~ResourceDump();
  void dumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned subresource,
                    D3D12_RESOURCE_STATES resourceState,
                    const std::wstring& dumpName,
                    unsigned mipLevel = 0,
                    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void commandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);
  void waitUntilDumped();
  std::string formatToString(DXGI_FORMAT value);

protected:
  struct DumpInfo {
    virtual ~DumpInfo() {}
    unsigned subresource{};
    unsigned size{};
    unsigned offset{};
    unsigned mipLevel{};
    std::wstring dumpName{};
    Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> resolvedResource{};
    D3D12_RESOURCE_DESC desc{};
    DXGI_FORMAT subresourceFormat{};
    unsigned rowPitch{};
  };

  std::unordered_map<ID3D12CommandList*, std::vector<DumpInfo*>> stagedResources_;

private:
  GpuExecutionTracker gpuExecutionTracker_;

  struct ThreadInfo : public GpuExecutionTracker::Executable {
    UINT64 fenceValue{};
    std::unique_ptr<std::thread> dumpThread{};
    std::vector<std::unique_ptr<DumpInfo>> dumpInfos;
  };

  ID3D12Fence* fence_{};
  HANDLE fenceEvent_{};
  UINT64 fenceValue_{};

  ImageFormat format_{ImageFormat::JPEG};
  std::optional<std::pair<float, float>> textureRescaleRange_;

protected:
  virtual void dumpBuffer(DumpInfo& dumpInfo, void* data);
  virtual void dumpTexture(DumpInfo& dumpInfo, void* data);
  virtual void dumpStagedResource(DumpInfo& dumpInfo);
  void stageResource(ID3D12GraphicsCommandList* commandList,
                     ID3D12Resource* resource,
                     D3D12_RESOURCE_STATES resourceState,
                     DumpInfo& dumpInfo,
                     bool dependent = false);

private:
  void dumpStagedResources(ThreadInfo* threadInfo);
  void initFence(ID3D12DeviceChild* deviceChild);
  HRESULT rescaleTexture(const ::DirectX::Image& srcImage,
                         ::DirectX::ScratchImage& destScratchImage);
  DXGI_FORMAT getDumpableFormat(DXGI_FORMAT format);
};

} // namespace DirectX
} // namespace gits
