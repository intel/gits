// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"
#include "capturePlayerGpuAddressService.h"

namespace gits {
namespace DirectX {

class ExecuteIndirectDump : public ResourceDump {
public:
  ExecuteIndirectDump(CapturePlayerGpuAddressService& addressService)
      : addressService_(addressService) {}
  ~ExecuteIndirectDump();
  void dumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          D3D12_RESOURCE_STATES argumentBufferState,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset,
                          D3D12_RESOURCE_STATES countBufferState,
                          const std::wstring& dumpName,
                          bool fromCapture);

protected:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    D3D12_COMMAND_SIGNATURE_DESC* commandSignature{};
    DumpInfo countDumpInfo;
    bool fromCapture{};
  };

  void dumpStagedResource(DumpInfo& dumpInfo) override;
  void dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);
  void printGpuAddress(std::ostream& stream, D3D12_GPU_VIRTUAL_ADDRESS address, bool fromCapture);

private:
  CapturePlayerGpuAddressService& addressService_;
};

} // namespace DirectX
} // namespace gits
