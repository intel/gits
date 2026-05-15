// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "resourceStateTracker.h"
#include "dispatchOutputsDump.h"
#include "dispatchOutputsAnalyzer.h"
#include "configurationLib.h"
#include "bit_range.h"

namespace gits {
namespace DirectX {

class DispatchOutputsDumpService {
public:
  DispatchOutputsDumpService(DispatchOutputsAnalyzer& dispatchOutputsAnalyzer);
  void CreateResource(ID3D12Resource* resource,
                      unsigned resourceKey,
                      D3D12_RESOURCE_STATES initialState);
  void CreateResource(ID3D12Resource* resource,
                      unsigned resourceKey,
                      D3D12_BARRIER_LAYOUT initialLayout);
  void DestroyInterface(unsigned interfaceKey);
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c,
                           unsigned frame,
                           unsigned execution);
  void CommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void CommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void FenceSignal(ID3D12FenceSignalCommand& c);
  void CreateFence(ID3D12DeviceCreateFenceCommand& c);
  void EnqueueMakeResident(ID3D12Device3EnqueueMakeResidentCommand& c);
  void ResourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void Barrier(ID3D12GraphicsCommandList7BarrierCommand& c);
  void Dispatch(ID3D12GraphicsCommandListDispatchCommand& c,
                unsigned frame,
                unsigned frameDispatch,
                unsigned commandListDispatch);

  void DumpDryRun();

private:
  bool DumpComputeOutput(ID3D12GraphicsCommandList* commandList,
                         unsigned resourceKey,
                         ID3D12Resource* resource,
                         unsigned slot,
                         unsigned frame,
                         unsigned frameDispatch,
                         unsigned commandListDispatch);
  bool IsUav(BarrierState resourceState) const;

private:
  ResourceStateTracker m_ResourceStateTracker;
  std::unordered_map<unsigned, ID3D12Resource*> m_ResourceByKey;
  DispatchOutputsDump m_ResourceDump;
  DispatchOutputsAnalyzer& m_DispatchOutputsAnalyzer;
  bool m_SkipUnboundedHeaps{};
  BitRange m_SlotResourcesRange;
  std::wstring m_DumpPath;
  bool m_DryRun{};

private:
  class DispatchOutputsDryRun {
  public:
    void DumpFile(std::filesystem::path& dumpDir);
    void AddDispatch(unsigned frame, unsigned dispatch) {
      m_DispatchesWithTextureByFrame[frame].insert(dispatch);
    }

  private:
    std::map<unsigned, std::set<unsigned>> m_DispatchesWithTextureByFrame;
  };
  DispatchOutputsDryRun m_DispatchOutputsDryRun;
};

} // namespace DirectX
} // namespace gits
