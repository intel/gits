// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  void DumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned subresource,
                    BarrierState resourceState,
                    const std::wstring& dumpName,
                    unsigned mipLevel,
                    DXGI_FORMAT format,
                    unsigned commandListDrawCount);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

  std::wstring m_DumpNameExecutionMarker{L"#e#"};

protected:
  struct RenderTargetDumpInfo : public DumpInfo {
    unsigned CommandListDrawCount;
    std::wstring ExecutionCount;
  };

  void DumpTexture(DumpInfo& dumpInfo, void* data) override;
};

} // namespace DirectX
} // namespace gits
