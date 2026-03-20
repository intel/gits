// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx/directx.h"
#include "imageUtils.h"

#include <filesystem>
#include <unordered_map>

namespace directx {

class ScreenshotService {
public:
  static ScreenshotService& Get();

  void Initialize(const std::filesystem::path& outputDir);
  void RegisterSwapChain(unsigned key, IDXGISwapChain* swapChain, ID3D12CommandQueue* queue);
  void CaptureFrame(unsigned key);

private:
  ScreenshotService() = default;
  ~ScreenshotService();

  ScreenshotService(const ScreenshotService&) = delete;
  ScreenshotService& operator=(const ScreenshotService&) = delete;

  struct Entry {
    ComPtr<IDXGISwapChain3> SwapChain;
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> Queue;
    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue = 0;
    ComPtr<ID3D12Resource> ReadbackBuffer;
    size_t ReadbackBufferSize = 0;
    size_t FrameIndex = 0;
  };

  void SaveBackBuffer(unsigned key, Entry& entry);
  bool CreateReadbackBuffer(Entry& entry, D3D12_SUBRESOURCE_FOOTPRINT& backBufferFootprint);
  bool CopyBackBuffer(Entry& entry,
                      ID3D12Resource* backBuffer,
                      D3D12_PLACED_SUBRESOURCE_FOOTPRINT& backBufferFootprint);
  bool SaveToFile(unsigned key, Entry& entry, D3D12_SUBRESOURCE_FOOTPRINT& backBufferFootprint);

  bool m_Enabled = false;
  std::filesystem::path m_OutputDir;
  std::unordered_map<unsigned, Entry> m_Entries;
};

} // namespace directx
