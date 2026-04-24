// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerExecuteIndirectService.h"
#include "analyzerCommandListService.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

AnalyzerExecuteIndirectService::AnalyzerExecuteIndirectService(
    ResourceStateTracker& resourceStateTracker,
    CapturePlayerGpuAddressService& gpuAddressService,
    AnalyzerRaytracingService& raytracingService,
    AnalyzerCommandListService& commandListService)
    : m_ResourceStateTracker(resourceStateTracker),
      m_GpuAddressService(gpuAddressService),
      m_RaytracingService(raytracingService),
      m_CommandListService(commandListService),
      m_ExecuteIndirectDump(*this) {
  LoadExecuteIndirectDispatchRays();
}

AnalyzerExecuteIndirectService::~AnalyzerExecuteIndirectService() {
  for (auto& it : m_CommandSignatures) {
    delete[] it.second.pArgumentDescs;
  }
}

void AnalyzerExecuteIndirectService::CreateCommandSignature(
    ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC& desc = m_CommandSignatures[c.m_ppvCommandSignature.Key] =
      *c.m_pDesc.Value;
  desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[desc.NumArgumentDescs];
  std::copy(c.m_pDesc.Value->pArgumentDescs,
            c.m_pDesc.Value->pArgumentDescs + c.m_pDesc.Value->NumArgumentDescs,
            const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(desc.pArgumentDescs));
}

void AnalyzerExecuteIndirectService::ExecuteIndirect(
    ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  m_CommandListService.AddObjectForRestore(c.m_pCommandSignature.Key);

  auto itCommandSignature = m_CommandSignatures.find(c.m_pCommandSignature.Key);
  GITS_ASSERT(itCommandSignature != m_CommandSignatures.end());
  D3D12_COMMAND_SIGNATURE_DESC& commandSignature = itCommandSignature->second;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW) {
      view = true;
    } else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }

  if (raytracing) {
    auto dumpBindingTable = [&](UINT64 address, UINT64 size, UINT64 stride) {
      if (!address) {
        return;
      }
      CapturePlayerGpuAddressService::ResourceInfo* info =
          m_GpuAddressService.GetResourceInfoByCaptureAddress(address);
      GITS_ASSERT(info);
      unsigned offset = address - info->CaptureStart;
      m_CommandListService.AddObjectForRestore(info->Key);
      m_RaytracingService.DumpBindingTable(c.m_Object.Value, c.m_Object.Key, info->Resource,
                                           info->Key, offset, size, stride, address);
    };

    auto it = m_ExecuteIndirectDispatchRays.find(c.Key);
    if (it != m_ExecuteIndirectDispatchRays.end()) {
      D3D12_DISPATCH_RAYS_DESC& desc = it->second;
      dumpBindingTable(desc.RayGenerationShaderRecord.StartAddress,
                       desc.RayGenerationShaderRecord.SizeInBytes,
                       desc.RayGenerationShaderRecord.SizeInBytes);
      dumpBindingTable(desc.MissShaderTable.StartAddress, desc.MissShaderTable.SizeInBytes,
                       desc.MissShaderTable.StrideInBytes);
      dumpBindingTable(desc.HitGroupTable.StartAddress, desc.HitGroupTable.SizeInBytes,
                       desc.HitGroupTable.StrideInBytes);
      dumpBindingTable(desc.CallableShaderTable.StartAddress, desc.CallableShaderTable.SizeInBytes,
                       desc.CallableShaderTable.StrideInBytes);
    }
  } else if (view) {
    BarrierState argumentBufferState =
        GetAdjustedCurrentState(m_ResourceStateTracker, m_GpuAddressService, c.m_Object.Value,
                                c.m_pArgumentBuffer.Value, c.m_ArgumentBufferOffset.Value,
                                c.m_pArgumentBuffer.Key, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    BarrierState countBufferState{};
    if (c.m_pCountBuffer.Value) {
      BarrierState countBufferState =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_GpuAddressService, c.m_Object.Value,
                                  c.m_pCountBuffer.Value, c.m_CountBufferOffset.Value,
                                  c.m_pCountBuffer.Key, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }
    m_ExecuteIndirectDump.DumpArgumentBuffer(
        c.m_Object.Value, &commandSignature, c.m_MaxCommandCount.Value, c.m_pArgumentBuffer.Value,
        c.m_ArgumentBufferOffset.Value, argumentBufferState, c.m_pCountBuffer.Value,
        c.m_CountBufferOffset.Value, countBufferState);
  }
}

void AnalyzerExecuteIndirectService::Flush() {
  m_ExecuteIndirectDump.WaitUntilDumped();
}

void AnalyzerExecuteIndirectService::ExecuteCommandLists(unsigned key,
                                                         unsigned commandQueueKey,
                                                         ID3D12CommandQueue* commandQueue,
                                                         ID3D12CommandList** commandLists,
                                                         unsigned commandListNum) {
  m_ExecuteIndirectDump.ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                            commandListNum);
}

void AnalyzerExecuteIndirectService::CommandQueueWait(unsigned key,
                                                      unsigned commandQueueKey,
                                                      unsigned fenceKey,
                                                      UINT64 fenceValue) {
  m_ExecuteIndirectDump.CommandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerExecuteIndirectService::CommandQueueSignal(unsigned key,
                                                        unsigned commandQueueKey,
                                                        unsigned fenceKey,
                                                        UINT64 fenceValue) {
  m_ExecuteIndirectDump.CommandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerExecuteIndirectService::FenceSignal(unsigned key,
                                                 unsigned fenceKey,
                                                 UINT64 fenceValue) {
  m_ExecuteIndirectDump.FenceSignal(key, fenceKey, fenceValue);
}

void AnalyzerExecuteIndirectService::LoadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = Configurator::Get().common.player.streamDir;
  std::ifstream stream(dumpPath / "executeIndirectRaytracing.txt");
  while (true) {
    unsigned callKey{};
    D3D12_DISPATCH_RAYS_DESC desc{};
    stream >> callKey;
    if (!stream) {
      break;
    }
    stream >> desc.RayGenerationShaderRecord.StartAddress >>
        desc.RayGenerationShaderRecord.SizeInBytes;
    stream >> desc.MissShaderTable.StartAddress >> desc.MissShaderTable.SizeInBytes >>
        desc.MissShaderTable.StrideInBytes;
    stream >> desc.HitGroupTable.StartAddress >> desc.HitGroupTable.SizeInBytes >>
        desc.HitGroupTable.StrideInBytes;
    stream >> desc.CallableShaderTable.StartAddress >> desc.CallableShaderTable.SizeInBytes >>
        desc.CallableShaderTable.StrideInBytes;
    stream >> desc.Width >> desc.Height >> desc.Depth;
    m_ExecuteIndirectDispatchRays[callKey] = desc;
  }
}

} // namespace DirectX
} // namespace gits
