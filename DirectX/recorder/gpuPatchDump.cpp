// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchDump.h"
#include "log.h"
#include "exception.h"

#include <algorithm>
#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

GpuPatchDump::GpuPatchDump() {}

GpuPatchDump::~GpuPatchDump() {
  try {
    flush();
  } catch (...) {
    topmost_exception_handler("GpuPatchDump::~GpuPatchDump");
  }
}

void GpuPatchDump::dumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                                      D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                                      unsigned maxCommandCount,
                                      ID3D12Resource* argumentBuffer,
                                      unsigned argumentBufferOffset,
                                      D3D12_RESOURCE_STATES argumentBufferState,
                                      ID3D12Resource* countBuffer,
                                      unsigned countBufferOffset,
                                      D3D12_RESOURCE_STATES countBufferState,
                                      unsigned callKey) {
  ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
  dumpInfo->CommandSignature = &commandSignature;
  dumpInfo->offset = argumentBufferOffset;
  dumpInfo->size = commandSignature.ByteStride * maxCommandCount;
  dumpInfo->CallKey = callKey;
  if (countBuffer) {
    dumpInfo->CountDumpInfo.offset = countBufferOffset;
    dumpInfo->CountDumpInfo.size = sizeof(unsigned);
  }

  stageResource(commandList, argumentBuffer, argumentBufferState, *dumpInfo);
  if (countBuffer) {
    stageResource(commandList, countBuffer, countBufferState, dumpInfo->CountDumpInfo, true);
  }
}

void GpuPatchDump::dumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                                ID3D12Resource* instancesBuffer,
                                                unsigned offset,
                                                unsigned pointersCount,
                                                D3D12_RESOURCE_STATES bufferState,
                                                unsigned callKey) {
  InstancesArrayOfPointersDumpInfo* dumpInfo = new InstancesArrayOfPointersDumpInfo();
  dumpInfo->offset = offset;
  dumpInfo->size = sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * pointersCount;
  dumpInfo->CallKey = callKey;

  stageResource(commandList, instancesBuffer, bufferState, *dumpInfo);
}

void GpuPatchDump::dumpStagedResource(DumpInfo& dumpInfo) {
  ExecuteIndirectDumpInfo* executeIndirectInfo = dynamic_cast<ExecuteIndirectDumpInfo*>(&dumpInfo);
  if (executeIndirectInfo) {
    unsigned count = executeIndirectInfo->size / executeIndirectInfo->CommandSignature->ByteStride;
    if (executeIndirectInfo->CountDumpInfo.stagingBuffer) {
      void* data{};
      HRESULT hr = executeIndirectInfo->CountDumpInfo.stagingBuffer->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      count = std::min(count, *static_cast<unsigned*>(data));
      executeIndirectInfo->CountDumpInfo.stagingBuffer->Unmap(0, nullptr);
    }
    void* data{};
    HRESULT hr = executeIndirectInfo->stagingBuffer->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    dumpArgumentBuffer(*executeIndirectInfo, count, data);
    executeIndirectInfo->stagingBuffer->Unmap(0, nullptr);
    return;
  }

  InstancesArrayOfPointersDumpInfo* instancesInfo =
      dynamic_cast<InstancesArrayOfPointersDumpInfo*>(&dumpInfo);
  if (instancesInfo) {
    void* data{};
    HRESULT hr = instancesInfo->stagingBuffer->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    dumpInstancesBuffer(*instancesInfo, data);
    instancesInfo->stagingBuffer->Unmap(0, nullptr);
    return;
  }
}

void GpuPatchDump::flush() {
  waitUntilDumped();
  {
    std::lock_guard<std::mutex> lock(m_ExecuteIndirectMutex);
    m_ExecuteIndirectStream.flush();
  }
  {
    std::lock_guard<std::mutex> lock(m_InstancesMutex);
    m_InstancesStream.flush();
  }
}

void GpuPatchDump::dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo,
                                      unsigned argumentCount,
                                      void* data) {
  unsigned offset = 0;
  for (unsigned i = 0; i < argumentCount; ++i) {
    offset = i * dumpInfo.CommandSignature->ByteStride;

    for (unsigned j = 0; j < dumpInfo.CommandSignature->NumArgumentDescs; ++j) {
      const D3D12_INDIRECT_ARGUMENT_DESC& desc = dumpInfo.CommandSignature->pArgumentDescs[j];
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
        offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: {
        offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT: {
        offset += desc.Constant.Num32BitValuesToSet * sizeof(UINT);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW: {
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW: {
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: {
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS: {
        std::lock_guard<std::mutex> lock(m_ExecuteIndirectMutex);
        D3D12_DISPATCH_RAYS_DESC& desc =
            *reinterpret_cast<D3D12_DISPATCH_RAYS_DESC*>(static_cast<uint8_t*>(data) + offset);

        if (!m_ExecuteIndirectStream.is_open()) {
          std::filesystem::path dumpPath = Configurator::Get().common.recorder.dumpPath;
          m_ExecuteIndirectStream.open(dumpPath / "executeIndirectRaytracing.txt");
        }
        m_ExecuteIndirectStream << dumpInfo.CallKey << " ";
        m_ExecuteIndirectStream << desc.RayGenerationShaderRecord.StartAddress << " "
                                << desc.RayGenerationShaderRecord.SizeInBytes << " ";
        m_ExecuteIndirectStream << desc.MissShaderTable.StartAddress << " "
                                << desc.MissShaderTable.SizeInBytes << " "
                                << desc.MissShaderTable.StrideInBytes << " ";
        m_ExecuteIndirectStream << desc.HitGroupTable.StartAddress << " "
                                << desc.HitGroupTable.SizeInBytes << " "
                                << desc.HitGroupTable.StrideInBytes << " ";
        m_ExecuteIndirectStream << desc.CallableShaderTable.StartAddress << " "
                                << desc.CallableShaderTable.SizeInBytes << " "
                                << desc.CallableShaderTable.StrideInBytes << " ";
        m_ExecuteIndirectStream << desc.Width << " " << desc.Height << " " << desc.Depth;
        m_ExecuteIndirectStream << "\n";

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

void GpuPatchDump::dumpInstancesBuffer(InstancesArrayOfPointersDumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(m_InstancesMutex);
  if (!m_InstancesStream.is_open()) {
    std::filesystem::path dumpPath = Configurator::Get().common.recorder.dumpPath;
    m_InstancesStream.open(dumpPath / "raytracingArraysOfPointers.dat", std::ios::binary);
  }
  m_InstancesStream.write(reinterpret_cast<char*>(&dumpInfo.CallKey), sizeof(unsigned));
  unsigned count = dumpInfo.size / sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
  m_InstancesStream.write(reinterpret_cast<char*>(&count), sizeof(unsigned));
  D3D12_GPU_VIRTUAL_ADDRESS* address = static_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(data);
  for (unsigned i = 0; i < count; ++i) {
    m_InstancesStream.write(reinterpret_cast<char*>(&address[i]),
                            sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
  }
}

} // namespace DirectX
} // namespace gits
