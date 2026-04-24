// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "accelerationStructuresDump.h"
#include "keyUtils.h"

#include <unordered_set>
#include <unordered_map>

namespace gits {
namespace DirectX {

class AccelerationStructuresDumpLayer : public Layer {
public:
  AccelerationStructuresDumpLayer();
  void Pre(ID3D12DeviceCreateCommandQueueCommand& c) override;
  void Pre(ID3D12DeviceCreateCommandListCommand& c) override;
  void Pre(ID3D12Device4CreateCommandList1Command& c) override;
  void Pre(ID3D12DeviceCreateCommandAllocatorCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12GraphicsCommandListResetCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;

private:
  std::wstring m_DumpPath;
  AccelerationStructuresDump m_AccelerationStructuresDump;
  ConfigKeySet m_CallKeys;
  bool m_DumpCurrentBuild{};

  class CommandListModuloStep {
  public:
    void Parse(const std::string& range);
    bool CheckNextCommandListCall(unsigned commandListKey);
    void ResetCommandList(unsigned commandListKey);

  private:
    std::unordered_map<unsigned, unsigned> m_CommandListCalls;
    unsigned m_Start{};
    unsigned m_Step{};
  };
  CommandListModuloStep m_CommandListModuloStep;
};

} // namespace DirectX
} // namespace gits
