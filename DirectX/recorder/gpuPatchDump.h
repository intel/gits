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
  void DumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          BarrierState argumentBufferState,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset,
                          BarrierState countBufferState,
                          unsigned callKey);
  void DumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                    ID3D12Resource* instancesBuffer,
                                    unsigned offset,
                                    unsigned pointersCount,
                                    BarrierState bufferState,
                                    unsigned callKey);
  void Flush();

protected:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    D3D12_COMMAND_SIGNATURE_DESC* CommandSignature{};
    DumpInfo CountDumpInfo;
    unsigned CallKey;
  };
  struct InstancesArrayOfPointersDumpInfo : public DumpInfo {
    unsigned CallKey;
  };

  void DumpStagedResource(DumpInfo& dumpInfo) override;
  void DumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);
  void DumpInstancesBuffer(InstancesArrayOfPointersDumpInfo& dumpInfo, void* data);

private:
  std::mutex m_ExecuteIndirectMutex;
  std::ofstream m_ExecuteIndirectStream;
  std::mutex m_InstancesMutex;
  std::ofstream m_InstancesStream;
};

} // namespace DirectX
} // namespace gits
