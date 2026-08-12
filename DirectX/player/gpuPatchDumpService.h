// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "raytracingResourceDump.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "executeIndirectDump.h"
#include "keyUtils.h"

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
  void DumpInstances(ID3D12GraphicsCommandList* commandList,
                     ID3D12Resource* resource,
                     GITSKey resourceKey,
                     unsigned size,
                     BarrierState resourceState,
                     GITSKey callKey,
                     bool prePatch);
  void DumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                    ID3D12Resource* resource,
                                    GITSKey resourceKey,
                                    unsigned offset,
                                    unsigned size,
                                    BarrierState resourceState,
                                    GITSKey callKey,
                                    bool prePatch);
  void DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        unsigned offset,
                        unsigned size,
                        unsigned stride,
                        BarrierState resourceState,
                        GITSKey callKey,
                        BindingTableType bindingTableType,
                        bool prePatch);
  void DumpExecuteIndirectArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                                         const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                                         unsigned maxCommandCount,
                                         ID3D12Resource* argumentBuffer,
                                         unsigned argumentBufferOffset,
                                         BarrierState argumentBufferState,
                                         ID3D12Resource* countBuffer,
                                         unsigned countBufferOffset,
                                         BarrierState countBufferState,
                                         GITSKey callKey,
                                         bool prePatch);
  void ExecuteCommandLists(GITSKey key,
                           GITSKey commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(GITSKey key, GITSKey commandQueueKey, GITSKey fenceKey, UINT64 fenceValue);
  void CommandQueueSignal(GITSKey key,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(GITSKey key, GITSKey fenceKey, UINT64 fenceValue);

private:
  RaytracingResourceDump m_ResourceDump;
  std::wstring m_DumpPath;
  ConfigKeySet m_RaytracingKeys;
  bool m_DumpInstancesPre{};
  bool m_DumpInstancesPost{};
  bool m_DumpBindingTablesPre{};
  bool m_DumpBindingTablesPost{};

  ConfigKeySet m_ExecuteIndirectKeys;
  ExecuteIndirectDump m_ExecuteIndirectDump;
  bool m_DumpArgumentBufferPre{};
  bool m_DumpArgumentBufferPost{};
};

} // namespace DirectX
} // namespace gits
