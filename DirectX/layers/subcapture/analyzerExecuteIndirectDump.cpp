// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerExecuteIndirectDump.h"
#include "analyzerExecuteIndirectService.h"
#include "capturePlayerGpuAddressService.h"

namespace gits {
namespace DirectX {

void AnalyzerExecuteIndirectDump::dumpArgumentBuffer(
    ID3D12GraphicsCommandList* commandList,
    const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
    unsigned maxCommandCount,
    ID3D12Resource* argumentBuffer,
    unsigned argumentBufferOffset,
    ID3D12Resource* countBuffer,
    unsigned countBufferOffset) {

  ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
  dumpInfo->commandSignature = commandSignature;
  dumpInfo->offset = argumentBufferOffset;
  dumpInfo->size = commandSignature->ByteStride * maxCommandCount;
  if (countBuffer) {
    dumpInfo->countDumpInfo.offset = countBufferOffset;
    dumpInfo->countDumpInfo.size = sizeof(unsigned);
  }

  stageResource(commandList, argumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, *dumpInfo);
  if (countBuffer) {
    stageResource(commandList, countBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                  dumpInfo->countDumpInfo, true);
  }
}

void AnalyzerExecuteIndirectDump::dumpStagedResource(DumpInfo& dumpInfo) {
  ExecuteIndirectDumpInfo& info = static_cast<ExecuteIndirectDumpInfo&>(dumpInfo);
  unsigned count = info.size / info.commandSignature->ByteStride;
  if (info.countDumpInfo.stagingBuffer) {
    void* data{};
    HRESULT hr = info.countDumpInfo.stagingBuffer->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    count = *static_cast<unsigned*>(data);
    info.countDumpInfo.stagingBuffer->Unmap(0, nullptr);
  }
  void* data{};
  HRESULT hr = info.stagingBuffer->Map(0, nullptr, &data);
  GITS_ASSERT(hr == S_OK);

  dumpArgumentBuffer(info, count, data);

  info.stagingBuffer->Unmap(0, nullptr);
}

void AnalyzerExecuteIndirectDump::dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo,
                                                     unsigned argumentCount,
                                                     void* data) {
  std::lock_guard<std::mutex> lock(mutex_);

  unsigned offset = 0;
  for (unsigned i = 0; i < argumentCount; ++i) {
    offset = i * dumpInfo.commandSignature->ByteStride;

    for (unsigned j = 0; j < dumpInfo.commandSignature->NumArgumentDescs; ++j) {
      const D3D12_INDIRECT_ARGUMENT_DESC& desc = dumpInfo.commandSignature->pArgumentDescs[j];
      switch (desc.Type) {
      case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW: {
        D3D12_VERTEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            executeIndirectService_.getGpuAddressService().getResourceInfoByCaptureAddress(
                args.BufferLocation);
        if (resourceInfo) {
          argumentBuffersResources_.insert(resourceInfo->key);
        }
        offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: {
        D3D12_INDEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            executeIndirectService_.getGpuAddressService().getResourceInfoByCaptureAddress(
                args.BufferLocation);
        if (resourceInfo) {
          argumentBuffersResources_.insert(resourceInfo->key);
        }
        offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            executeIndirectService_.getGpuAddressService().getResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          argumentBuffersResources_.insert(resourceInfo->key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            executeIndirectService_.getGpuAddressService().getResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          argumentBuffersResources_.insert(resourceInfo->key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
            executeIndirectService_.getGpuAddressService().getResourceInfoByCaptureAddress(args);
        if (resourceInfo) {
          argumentBuffersResources_.insert(resourceInfo->key);
        }
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
