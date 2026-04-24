// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gpuExecutionTracker.h"
#include "resourceStateTracker.h"
#include "configurationLib.h"

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

class ResourceDump {
public:
  ResourceDump(ImageFormat format = ImageFormat::JPEG, const std::string& textureRescaleRange = "");
  virtual ~ResourceDump();
  ResourceDump(const ResourceDump&) = delete;
  ResourceDump& operator=(const ResourceDump&) = delete;

  void DumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned subresource,
                    BarrierState resourceState,
                    const std::wstring& dumpName,
                    unsigned mipLevel = 0,
                    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);
  void WaitUntilDumped();
  std::string FormatToString(DXGI_FORMAT value);

protected:
  struct DumpInfo {
    virtual ~DumpInfo() {}
    unsigned Subresource{};
    unsigned Size{};
    unsigned Offset{};
    unsigned MipLevel{};
    std::wstring DumpName{};
    Microsoft::WRL::ComPtr<ID3D12Resource> StagingBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> ResolvedResource{};
    D3D12_RESOURCE_DESC Desc{};
    DXGI_FORMAT SubresourceFormat{};
    unsigned RowPitch{};
  };

  std::unordered_map<ID3D12CommandList*, std::vector<DumpInfo*>> m_StagedResources;

private:
  GpuExecutionTracker m_GpuExecutionTracker;

  struct ThreadInfo : public GpuExecutionTracker::Executable {
    UINT64 FenceValue{};
    std::unique_ptr<std::thread> DumpThread{};
    std::vector<std::unique_ptr<DumpInfo>> DumpInfos;
  };

  ID3D12Fence* m_Fence{};
  HANDLE m_FenceEvent{};
  UINT64 m_FenceValue{};

  ImageFormat m_Format{ImageFormat::JPEG};
  std::optional<std::pair<float, float>> m_TextureRescaleRange;

protected:
  virtual void DumpBuffer(DumpInfo& dumpInfo, void* data);
  virtual void DumpTexture(DumpInfo& dumpInfo, void* data);
  virtual void DumpStagedResource(DumpInfo& dumpInfo);
  void StageResource(ID3D12GraphicsCommandList* commandList,
                     ID3D12Resource* resource,
                     BarrierState resourceState,
                     DumpInfo& dumpInfo,
                     bool dependent = false);

private:
  void DumpStagedResources(ThreadInfo* threadInfo);
  void InitFence(ID3D12DeviceChild* deviceChild);
  HRESULT RescaleTexture(const ::DirectX::Image& srcImage,
                         ::DirectX::ScratchImage& destScratchImage);
  DXGI_FORMAT GetDumpableFormat(DXGI_FORMAT format);
};

} // namespace DirectX
} // namespace gits
