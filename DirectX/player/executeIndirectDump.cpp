// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executeIndirectDump.h"
#include "gits.h"

#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

ExecuteIndirectDump ::~ExecuteIndirectDump() {
  waitUntilDumped();
}

void ExecuteIndirectDump::dumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                                             const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                                             unsigned maxCommandCount,
                                             ID3D12Resource* argumentBuffer,
                                             unsigned argumentBufferOffset,
                                             D3D12_RESOURCE_STATES argumentBufferState,
                                             ID3D12Resource* countBuffer,
                                             unsigned countBufferOffset,
                                             D3D12_RESOURCE_STATES countBufferState,
                                             const std::wstring& dumpName,
                                             bool fromCapture) {
  ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
  dumpInfo->commandSignature = commandSignature;
  dumpInfo->offset = argumentBufferOffset;
  dumpInfo->size = commandSignature->ByteStride * maxCommandCount;
  dumpInfo->dumpName = dumpName;
  dumpInfo->fromCapture = fromCapture;
  if (countBuffer) {
    dumpInfo->countDumpInfo.offset = countBufferOffset;
    dumpInfo->countDumpInfo.size = sizeof(unsigned);
  }

  stageResource(commandList, argumentBuffer, argumentBufferState, *dumpInfo);
  if (countBuffer) {
    stageResource(commandList, countBuffer, countBufferState, dumpInfo->countDumpInfo, true);
  }
}

void ExecuteIndirectDump::dumpStagedResource(DumpInfo& dumpInfo) {
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

void ExecuteIndirectDump::dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo,
                                             unsigned argumentCount,
                                             void* data) {
  std::ofstream stream(dumpInfo.dumpName);
  unsigned offset = 0;
  for (unsigned i = 0; i < argumentCount; ++i) {
    stream << "ARGUMENT " << i + 1 << "\n";
    offset = i * dumpInfo.commandSignature->ByteStride;

    for (unsigned j = 0; j < dumpInfo.commandSignature->NumArgumentDescs; ++j) {
      const D3D12_INDIRECT_ARGUMENT_DESC& desc = dumpInfo.commandSignature->pArgumentDescs[j];
      switch (desc.Type) {
      case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW: {
        D3D12_DRAW_ARGUMENTS& args =
            *reinterpret_cast<D3D12_DRAW_ARGUMENTS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  DRAW " << args.VertexCountPerInstance << ", " << args.InstanceCount << ", "
               << args.StartVertexLocation << ", " << args.StartInstanceLocation << "\n";
        offset += sizeof(D3D12_DRAW_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED: {
        D3D12_DRAW_INDEXED_ARGUMENTS& args =
            *reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  DRAW INDEXED " << args.IndexCountPerInstance << ", " << args.InstanceCount
               << ", " << args.StartIndexLocation << ", " << args.BaseVertexLocation << ", "
               << args.StartInstanceLocation << "\n";
        offset += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH: {
        D3D12_DISPATCH_ARGUMENTS& args =
            *reinterpret_cast<D3D12_DISPATCH_ARGUMENTS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  DISPATCH " << args.ThreadGroupCountX << ", " << args.ThreadGroupCountY << ", "
               << args.ThreadGroupCountZ << "\n";
        offset += sizeof(D3D12_DISPATCH_ARGUMENTS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW: {
        D3D12_VERTEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        stream << "  VERTEX BUFFER VIEW ";
        printGpuAddress(stream, args.BufferLocation, dumpInfo.fromCapture);
        stream << ", " << args.SizeInBytes << ", " << args.StrideInBytes << "\n";
        offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: {
        D3D12_INDEX_BUFFER_VIEW& args =
            *reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(static_cast<uint8_t*>(data) + offset);
        stream << "  INDEX BUFFER VIEW ";
        printGpuAddress(stream, args.BufferLocation, dumpInfo.fromCapture);
        stream << ", " << args.SizeInBytes << ", " << args.Format << "\n";
        offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT: {
        for (unsigned i = 0; i < desc.Constant.Num32BitValuesToSet; ++i) {
          UINT& args = *reinterpret_cast<UINT*>(static_cast<uint8_t*>(data) + offset);
          stream << "  CONSTANT " << args << "\n";
          offset += sizeof(UINT);
        }
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  CONSTANT BUFFER VIEW ";
        printGpuAddress(stream, args, dumpInfo.fromCapture);
        stream << "\n";
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  SHADER BUFFER VIEW ";
        printGpuAddress(stream, args, dumpInfo.fromCapture);
        stream << "\n";
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: {
        D3D12_GPU_VIRTUAL_ADDRESS& args =
            *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  UNORDERED ACCESS VIEW ";
        printGpuAddress(stream, args, dumpInfo.fromCapture);
        stream << "\n";
        offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS: {
        D3D12_DISPATCH_RAYS_DESC& args =
            *reinterpret_cast<D3D12_DISPATCH_RAYS_DESC*>(static_cast<uint8_t*>(data) + offset);
        stream << "  DISPATCH RAYS\n";
        stream << "    RayGenerationShaderRecord ";
        printGpuAddress(stream, args.RayGenerationShaderRecord.StartAddress, dumpInfo.fromCapture);
        stream << ", " << args.RayGenerationShaderRecord.SizeInBytes << "\n";
        stream << "    MissShaderTable ";
        printGpuAddress(stream, args.MissShaderTable.StartAddress, dumpInfo.fromCapture);
        stream << ", " << args.MissShaderTable.SizeInBytes << ", "
               << args.MissShaderTable.StrideInBytes << "\n";
        stream << "    HitGroupTable ";
        printGpuAddress(stream, args.HitGroupTable.StartAddress, dumpInfo.fromCapture);
        stream << ", " << args.HitGroupTable.SizeInBytes << ", " << args.HitGroupTable.StrideInBytes
               << "\n";
        stream << "    CallableShaderTable ";
        printGpuAddress(stream, args.CallableShaderTable.StartAddress, dumpInfo.fromCapture);
        stream << ", " << args.CallableShaderTable.SizeInBytes << ", "
               << args.CallableShaderTable.StrideInBytes << "\n";
        stream << "    Width " << args.Width << "\n";
        stream << "    Height " << args.Height << "\n";
        stream << "    Depth " << args.Depth << "\n";
        offset += sizeof(D3D12_DISPATCH_RAYS_DESC);
        break;
      }
      case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH: {
        D3D12_DISPATCH_MESH_ARGUMENTS& args =
            *reinterpret_cast<D3D12_DISPATCH_MESH_ARGUMENTS*>(static_cast<uint8_t*>(data) + offset);
        stream << "  DISPATCH MESH " << args.ThreadGroupCountX << ", " << args.ThreadGroupCountY
               << ", " << args.ThreadGroupCountZ << "\n";
        offset += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
        break;
      }
      }
    }
  }
}

void ExecuteIndirectDump::printGpuAddress(std::ostream& stream,
                                          D3D12_GPU_VIRTUAL_ADDRESS address,
                                          bool fromCapture) {
  std::ios state(nullptr);
  state.copyfmt(stream);

  stream << "{0x" << std::hex << std::setw(16) << std::setfill('0') << address << std::dec;
  if (address) {
    CapturePlayerGpuAddressService::ResourceInfo* info{};
    unsigned offset{};
    if (fromCapture) {
      info = addressService_.getResourceInfoByCaptureAddress(address);
    } else {
      info = addressService_.getResourceInfoByPlayerAddress(address);
    }
    if (!info) {
      stream << ", NOT FOUND";
    } else {
      if (fromCapture) {
        offset = address - info->captureStart;
      } else {
        offset = address - info->playerStart;
      }
      stream << ", key O" << info->key << ", offset " << offset;
    }
  }
  stream << "}";

  stream.copyfmt(state);
}

} // namespace DirectX
} // namespace gits
