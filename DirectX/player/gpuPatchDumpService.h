// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "raytracingResourceDump.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "executeIndirectDump.h"
#include "configKeySet.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

class GpuPatchDumpService {
public:
  enum BindingTableType {
    RayGeneration,
    Miss,
    HitGroup,
    Callable
  };

public:
  GpuPatchDumpService(CapturePlayerGpuAddressService& addressService,
                      CapturePlayerShaderIdentifierService& shaderIdentifierService,
                      CapturePlayerDescriptorHandleService& descriptorHandleService);
  void dumpInstances(ID3D12GraphicsCommandList* commandList,
                     ID3D12Resource* resource,
                     unsigned resourceKey,
                     unsigned size,
                     unsigned callKey,
                     bool prePatch);
  void dumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                    ID3D12Resource* resource,
                                    unsigned resourceKey,
                                    unsigned offset,
                                    unsigned size,
                                    D3D12_RESOURCE_STATES resourceState,
                                    unsigned callKey,
                                    bool prePatch);
  void dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned offset,
                        unsigned size,
                        unsigned stride,
                        unsigned callKey,
                        BindingTableType bindingTableType,
                        bool prePatch);
  void dumpExecuteIndirectArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                                         const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                                         unsigned maxCommandCount,
                                         ID3D12Resource* argumentBuffer,
                                         unsigned argumentBufferOffset,
                                         D3D12_RESOURCE_STATES argumentBufferState,
                                         ID3D12Resource* countBuffer,
                                         unsigned countBufferOffset,
                                         D3D12_RESOURCE_STATES countBufferState,
                                         unsigned callKey,
                                         bool prePatch);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void commandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  RaytracingResourceDump resourceDump_;
  std::wstring dumpPath_;
  ConfigKeySet raytracingKeys_;
  bool dumpInstancesPre_{};
  bool dumpInstancesPost_{};
  bool dumpBindingTablesPre_{};
  bool dumpBindingTablesPost_{};

  ConfigKeySet executeIndirectKeys_;
  ExecuteIndirectDump executeIndirectDump_;
  bool dumpArgumentBufferPre_{};
  bool dumpArgumentBufferPost_{};
};

} // namespace DirectX
} // namespace gits
