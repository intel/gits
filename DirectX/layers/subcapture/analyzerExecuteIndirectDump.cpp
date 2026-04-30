// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerExecuteIndirectDump.h"
#include "analyzerExecuteIndirectService.h"
#include "capturePlayerGpuAddressService.h"

#include <algorithm>

namespace gits {
namespace DirectX {

void AnalyzerExecuteIndirectDump::DumpArgumentBuffer(
    ID3D12GraphicsCommandList* commandList,
    const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
    unsigned maxCommandCount,
    ID3D12Resource* argumentBuffer,
    unsigned argumentBufferOffset,
    BarrierState argumentBufferState,
    ID3D12Resource* countBuffer,
    unsigned countBufferOffset,
    BarrierState countBufferState) {

  ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
  dumpInfo->commandSignature = commandSignature;
  dumpInfo->Offset = argumentBufferOffset;
  dumpInfo->Size = commandSignature->ByteStride * maxCommandCount;
  if (countBuffer) {
    dumpInfo->countDumpInfo.Offset = countBufferOffset;
    dumpInfo->countDumpInfo.Size = sizeof(unsigned);
  }

  StageResource(commandList, argumentBuffer, argumentBufferState, *dumpInfo);
  if (countBuffer) {
    StageResource(commandList, countBuffer, countBufferState, dumpInfo->countDumpInfo, true);
  }
}

void AnalyzerExecuteIndirectDump::DumpStagedResource(DumpInfo& dumpInfo) {
  ExecuteIndirectDumpInfo& info = static_cast<ExecuteIndirectDumpInfo&>(dumpInfo);
  unsigned count = info.Size / info.commandSignature->ByteStride;
  if (info.countDumpInfo.StagingBuffer) {
    void* data{};
    HRESULT hr = info.countDumpInfo.StagingBuffer->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    count = std::min(count, *static_cast<unsigned*>(data));
    info.countDumpInfo.StagingBuffer->Unmap(0, nullptr);
  }
  void* data{};
  HRESULT hr = info.StagingBuffer->Map(0, nullptr, &data);
  GITS_ASSERT(hr == S_OK);

  DumpArgumentBuffer(info, count, data);

  info.StagingBuffer->Unmap(0, nullptr);
}

void AnalyzerExecuteIndirectDump::DumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo,
                                                     unsigned argumentCount,
                                                     void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  unsigned offset = 0;
  for (unsigned i = 0; i < argumentCount; ++i) {
    offset = i * dumpInfo.commandSignature->ByteStride;

    for (unsigned j = 0; j < dumpInfo.commandSignature->NumArgumentDescs; ++j) {
      const D3D12_INDIRECT_ARGUMENT_DESC& desc = dumpInfo.commandSignature->pArgumentDescs[j];
      switch (desc.Type) {
      case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW: {
        offset += sizeof(D3D12_DRAW_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED: {
        offset += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH: {
        offset += sizeof(D3D12_DISPATCH_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW: {
        D3D12_VERTEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            m_ExecuteIndirectService.GetGpuAddressService().GetResourceInfoByCaptureAddress(
                args.BufferLocation);
        if (resourceInfo) {
          m_ArgumentBuffersResources.insert(resourceInfo->Key);
        }
        offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: {
        D3D12_INDEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            m_ExecuteIndirectService.GetGpuAddressService().GetResourceInfoByCaptureAddress(
                args.BufferLocation);
        if (resourceInfo) {
          m_ArgumentBuffersResources.insert(resourceInfo->Key);
        }
        offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT: {
        offset += desc.Constant.Num32BitValuesToSet * sizeof(UINT);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            m_ExecuteIndirectService.GetGpuAddressService().GetResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          m_ArgumentBuffersResources.insert(resourceInfo->Key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            m_ExecuteIndirectService.GetGpuAddressService().GetResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          m_ArgumentBuffersResources.insert(resourceInfo->Key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            m_ExecuteIndirectService.GetGpuAddressService().GetResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          m_ArgumentBuffersResources.insert(resourceInfo->Key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS: {
        offset += sizeof(D3D12_DISPATCH_RAYS_DESC);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH: {
        offset += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
        break;
      }
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
