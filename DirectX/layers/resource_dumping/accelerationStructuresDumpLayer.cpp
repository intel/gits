// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresDumpLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

AccelerationStructuresDumpLayer::AccelerationStructuresDumpLayer()
    : Layer("AccelerationStructuresDump") {
  auto& dumpPath = Config::Get().common.player.outputDir.empty()
                       ? Config::Get().common.player.streamDir / "acceleration_structures"
                       : Config::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  dumpPath_ = dumpPath;

  auto& raytracingDump = Config::Get().directx.features.raytracingDump;
  if (!raytracingDump.commandKeys.empty()) {
    extractKeys(raytracingDump.commandKeys, callKeys_);
  }
}

void AccelerationStructuresDumpLayer::pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.pDesc_.value->Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.pDesc_.value->Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.type_.value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::pre(ID3D12Device4CreateCommandList1Command& c) {
  if (c.type_.value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::pre(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.type_.value == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
    c.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

void AccelerationStructuresDumpLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!callKeys_.empty() && callKeys_.find(c.key) == callKeys_.end()) {
    return;
  }
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  c.pDesc_.value->Inputs.Flags &=
      ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

  if (c.pDesc_.value->Inputs.Flags &
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
    c.pDesc_.value->Inputs.Flags &=
        ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    D3D12_GPU_VIRTUAL_ADDRESS sourceAccelerationStructureData =
        c.pDesc_.value->SourceAccelerationStructureData;
    c.pDesc_.value->SourceAccelerationStructureData = 0;

    c.object_.value->BuildRaytracingAccelerationStructure(
        c.pDesc_.value, c.NumPostbuildInfoDescs_.value, c.pPostbuildInfoDescs_.value);

    std::wstring dumpName = dumpPath_ + L"/build_blas_" + std::to_wstring(c.key) + L".txt";
    accelerationStructuresDump_.dumpAccelerationStructure(
        c.object_.value, c.pDesc_.value->DestAccelerationStructureData, dumpName);

    c.pDesc_.value->Inputs.Flags |=
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    c.pDesc_.value->SourceAccelerationStructureData = sourceAccelerationStructureData;
  }
}

void AccelerationStructuresDumpLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {

  if (!callKeys_.empty() && callKeys_.find(c.key) == callKeys_.end()) {
    return;
  }

  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  if (c.pDesc_.value->Inputs.Flags &
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
    return;
  }
  std::wstring dumpName = dumpPath_ + L"/build_blas_" + std::to_wstring(c.key) + L".txt";
  accelerationStructuresDump_.dumpAccelerationStructure(
      c.object_.value, c.pDesc_.value->DestAccelerationStructureData, dumpName);
}

void AccelerationStructuresDumpLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  accelerationStructuresDump_.executeCommandLists(
      c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value, c.NumCommandLists_.value);
}

void AccelerationStructuresDumpLayer::post(ID3D12CommandQueueWaitCommand& c) {
  accelerationStructuresDump_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AccelerationStructuresDumpLayer::post(ID3D12CommandQueueSignalCommand& c) {
  accelerationStructuresDump_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key,
                                                 c.Value_.value);
}

void AccelerationStructuresDumpLayer::post(ID3D12FenceSignalCommand& c) {
  accelerationStructuresDump_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void AccelerationStructuresDumpLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  accelerationStructuresDump_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void AccelerationStructuresDumpLayer::extractKeys(const std::string& keyString,
                                                  std::unordered_set<unsigned>& keySet) {
  const char* p = keyString.data();
  do {
    const char* begin = p;
    while (*p != ',' && *p) {
      ++p;
    }
    std::string key(begin, p);
    keySet.insert(std::stoi(key));
  } while (*p++);
}

} // namespace DirectX
} // namespace gits
