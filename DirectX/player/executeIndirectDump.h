// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
      : m_AddressService(addressService) {}
  ~ExecuteIndirectDump();
  void DumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          BarrierState argumentBufferState,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset,
                          BarrierState countBufferState,
                          const std::wstring& dumpName,
                          bool fromCapture);

private:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    const D3D12_COMMAND_SIGNATURE_DESC* CommandSignature{};
    DumpInfo CountDumpInfo;
    bool FromCapture{};
  };

  void DumpStagedResource(DumpInfo& dumpInfo) override;
  void DumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);
  void PrintGpuAddress(std::ostream& stream, D3D12_GPU_VIRTUAL_ADDRESS address, bool fromCapture);

private:
  CapturePlayerGpuAddressService& m_AddressService;
};

} // namespace DirectX
} // namespace gits
