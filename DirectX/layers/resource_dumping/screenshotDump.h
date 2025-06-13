// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "tools_lite.h"

namespace gits {
namespace DirectX {

class ScreenshotDump : public gits::noncopyable {
public:
  ScreenshotDump(ID3D12CommandQueue* commandQueue);
  ~ScreenshotDump();
  void dump(ID3D12Resource* backBuffer, const std::wstring& dumpName);

private:
  void createStagingBuffer();
  void dumpStagedResource(std::wstring dumpName);

private:
  Microsoft::WRL::ComPtr<ID3D12Device> device_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  UINT64 fenceValue_{};
  HANDLE fenceEvent_{};
  D3D12_RESOURCE_DESC backBufferDesc_{};
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT backBufferFootprint_{};
  Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer_;
  std::unique_ptr<std::thread> dumpThread_{};
  ImageFormat format_{ImageFormat::PNG};
};

} // namespace DirectX
} // namespace gits
