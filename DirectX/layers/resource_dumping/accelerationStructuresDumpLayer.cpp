// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresDumpLayer.h"
#include "log.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

AccelerationStructuresDumpLayer::AccelerationStructuresDumpLayer()
    : Layer("AccelerationStructuresDump"),
      m_CallKeys(Configurator::Get().directx.features.raytracingDump.commandKeys) {
  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "acceleration_structures"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  m_DumpPath = dumpPath;

  m_CommandListModuloStep.Parse(
      Configurator::Get().directx.features.raytracingDump.commandListModuloStep);
}

void AccelerationStructuresDumpLayer::Pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.m_pDesc.Value->Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.m_pDesc.Value->Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.m_type.Value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::Pre(ID3D12Device4CreateCommandList1Command& c) {
  if (c.m_type.Value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::Pre(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.m_type.Value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!m_CallKeys.Empty() && !m_CallKeys.Contains(c.Key) ||
      !m_CommandListModuloStep.CheckNextCommandListCall(c.m_Object.Key)) {
    return;
  }
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  m_DumpCurrentBuild = true;

  c.m_pDesc.Value->Inputs.Flags &=
      ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

  if (c.m_pDesc.Value->Inputs.Flags &
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
    c.m_pDesc.Value->Inputs.Flags &=
        ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    D3D12_GPU_VIRTUAL_ADDRESS sourceAccelerationStructureData =
        c.m_pDesc.Value->SourceAccelerationStructureData;
    c.m_pDesc.Value->SourceAccelerationStructureData = 0;

    c.m_Object.Value->BuildRaytracingAccelerationStructure(
        c.m_pDesc.Value, c.m_NumPostbuildInfoDescs.Value, c.m_pPostbuildInfoDescs.Value);

    std::wstring dumpName = m_DumpPath + L"/build_blas_" + std::to_wstring(c.Key) + L".txt";
    m_AccelerationStructuresDump.dumpAccelerationStructure(
        c.m_Object.Value, c.m_pDesc.Value->DestAccelerationStructureData, dumpName);

    c.m_pDesc.Value->Inputs.Flags |=
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    c.m_pDesc.Value->SourceAccelerationStructureData = sourceAccelerationStructureData;
  }
}

void AccelerationStructuresDumpLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {

  if (!m_DumpCurrentBuild) {
    return;
  }
  m_DumpCurrentBuild = false;

  if (c.m_pDesc.Value->Inputs.Flags &
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
    return;
  }
  std::wstring dumpName = m_DumpPath + L"/build_blas_" + std::to_wstring(c.Key) + L".txt";
  m_AccelerationStructuresDump.dumpAccelerationStructure(
      c.m_Object.Value, c.m_pDesc.Value->DestAccelerationStructureData, dumpName);
}

void AccelerationStructuresDumpLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  m_AccelerationStructuresDump.ExecuteCommandLists(
      c.Key, c.m_Object.Key, c.m_Object.Value, c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void AccelerationStructuresDumpLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  m_CommandListModuloStep.ResetCommandList(c.m_Object.Key);
}

void AccelerationStructuresDumpLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  m_AccelerationStructuresDump.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                                c.m_Value.Value);
}

void AccelerationStructuresDumpLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  m_AccelerationStructuresDump.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                                  c.m_Value.Value);
}

void AccelerationStructuresDumpLayer::Post(ID3D12FenceSignalCommand& c) {
  m_AccelerationStructuresDump.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void AccelerationStructuresDumpLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  m_AccelerationStructuresDump.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void AccelerationStructuresDumpLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  m_AccelerationStructuresDump.FenceSignal(c.Key, c.m_pFenceToSignal.Key,
                                           c.m_FenceValueToSignal.Value);
}

void AccelerationStructuresDumpLayer::CommandListModuloStep::Parse(const std::string& range) {
  if (range.empty()) {
    return;
  }
  m_Start = std::stoi(range);
  size_t pos = range.find(':');
  GITS_ASSERT(pos != std::string::npos);
  m_Step = std::stoi(range.substr(pos + 1));
}

bool AccelerationStructuresDumpLayer::CommandListModuloStep::CheckNextCommandListCall(
    unsigned commandListKey) {
  if (!m_Step) {
    return true;
  }
  unsigned& index = m_CommandListCalls[commandListKey];
  ++index;
  if (index % m_Step == m_Start) {
    return true;
  }
  return false;
}

void AccelerationStructuresDumpLayer::CommandListModuloStep::ResetCommandList(
    unsigned commandListKey) {
  m_CommandListCalls.erase(commandListKey);
}

} // namespace DirectX
} // namespace gits
