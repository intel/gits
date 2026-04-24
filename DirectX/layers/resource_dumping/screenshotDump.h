// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "configurationLib.h"

#include <string>
#include <thread>
#include <wrl/client.h>
#include <memory>

namespace gits {
namespace DirectX {

class ScreenshotDump {
public:
  ScreenshotDump(ID3D12CommandQueue* commandQueue);
  ~ScreenshotDump();
  ScreenshotDump(const ScreenshotDump&) = delete;
  ScreenshotDump& operator=(const ScreenshotDump&) = delete;

  void dump(ID3D12Resource* backBuffer, const std::wstring& dumpName);

private:
  void createStagingBuffer();
  void dumpStagedResource(std::wstring dumpName);

  Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
  UINT64 m_FenceValue{};
  HANDLE m_FenceEvent{};
  D3D12_RESOURCE_DESC m_BackBufferDesc{};
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_BackBufferFootprint{};
  Microsoft::WRL::ComPtr<ID3D12Resource> m_StagingBuffer;
  std::unique_ptr<std::thread> m_DumpThread{};
  ImageFormat m_Format{ImageFormat::PNG};
};

} // namespace DirectX
} // namespace gits
