// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

namespace gits {
namespace DirectX {

class RenderTargetsDump : public ResourceDump {
public:
  RenderTargetsDump(ImageFormat format) : ResourceDump(format) {}
  ~RenderTargetsDump();
  void dumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned subresource,
                    D3D12_RESOURCE_STATES resourceState,
                    const std::wstring& dumpName,
                    unsigned mipLevel,
                    DXGI_FORMAT format,
                    unsigned commandListDrawCount);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

  std::wstring dumpNameExecutionMarker{L"#e#"};

protected:
  struct RenderTargetDumpInfo : public DumpInfo {
    unsigned commandListDrawCount;
    std::wstring executionCount;
  };

  void dumpTexture(DumpInfo& dumpInfo, void* data) override;
};

} // namespace DirectX
} // namespace gits
