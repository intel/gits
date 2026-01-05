// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

#include <fstream>
#include <mutex>

namespace gits {
namespace DirectX {

class GpuPatchDump : public ResourceDump {
public:
  GpuPatchDump();
  ~GpuPatchDump();
  void dumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          D3D12_RESOURCE_STATES argumentBufferState,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset,
                          D3D12_RESOURCE_STATES countBufferState,
                          unsigned callKey);
  void dumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                    ID3D12Resource* instancesBuffer,
                                    unsigned offset,
                                    unsigned pointersCount,
                                    D3D12_RESOURCE_STATES bufferState,
                                    unsigned callKey);
  void flush();

protected:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    D3D12_COMMAND_SIGNATURE_DESC* commandSignature{};
    DumpInfo countDumpInfo;
    unsigned callKey;
  };
  struct InstancesArrayOfPointersDumpInfo : public DumpInfo {
    unsigned callKey;
  };

  void dumpStagedResource(DumpInfo& dumpInfo) override;
  void dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);
  void dumpInstancesBuffer(InstancesArrayOfPointersDumpInfo& dumpInfo, void* data);

private:
  std::mutex executeIndirectMutex_;
  std::ofstream executeIndirectStream_;
  std::mutex instancesMutex_;
  std::ofstream instancesStream_;
};

} // namespace DirectX
} // namespace gits
