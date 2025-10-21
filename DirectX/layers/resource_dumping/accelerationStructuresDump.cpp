// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresDump.h"
#include "gits.h"
#include "log.h"

#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

AccelerationStructuresDump::~AccelerationStructuresDump() {
  waitUntilDumped();
}

void AccelerationStructuresDump::dumpAccelerationStructure(
    ID3D12GraphicsCommandList4* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS accelerationStructure,
    const std::wstring& dumpName) {

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  const unsigned decodedSize = 0x1000000;

  AccelerationStructuresDumpInfo* dumpInfo = new AccelerationStructuresDumpInfo();
  dumpInfo->dumpName = dumpName;

  {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = decodedSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc = {1, 0};
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->decodedBuffer));
    if (hr != S_OK) {
      if (hr == E_OUTOFMEMORY) {
        LOG_ERROR << "BLAS dumping - create buffer failed - E_OUTOFMEMORY - try with less buffers.";
      } else {
        LOG_ERROR << "BLAS dumping - create staging buffer failed - 0x" << std::hex << hr
                  << std::dec << " - try with less buffers.";
      }
      exit(EXIT_FAILURE);
    }

    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->stagingBuffer));
    if (hr != S_OK) {
      if (hr == E_OUTOFMEMORY) {
        LOG_ERROR << "BLAS dumping - create buffer failed - E_OUTOFMEMORY - try with less buffers.";
      } else {
        LOG_ERROR << "BLAS dumping - create staging buffer failed - 0x" << std::hex << hr
                  << std::dec << " - try with less buffers.";
      }
      exit(EXIT_FAILURE);
    }
  }

  commandList->CopyRaytracingAccelerationStructure(
      dumpInfo->decodedBuffer->GetGPUVirtualAddress(), accelerationStructure,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_VISUALIZATION_DECODE_FOR_TOOLS);

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barrier.Transition.pResource = dumpInfo->decodedBuffer.Get();
  commandList->ResourceBarrier(1, &barrier);

  commandList->CopyResource(dumpInfo->stagingBuffer.Get(), dumpInfo->decodedBuffer.Get());

  stagedResources_[commandList].push_back(dumpInfo);
}

void AccelerationStructuresDump::dumpStagedResource(DumpInfo& dumpInfo) {
  AccelerationStructuresDumpInfo& info = static_cast<AccelerationStructuresDumpInfo&>(dumpInfo);

  void* data{};
  HRESULT hr = info.stagingBuffer->Map(0, nullptr, &data);
  GITS_ASSERT(hr == S_OK);

  std::ofstream stream(info.dumpName);

  unsigned offset = 0;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE type;
  memcpy(&type, static_cast<uint8_t*>(data) + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  if (type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    LOG_ERROR << "Can't dump bottom level acceleration structure "
              << std::string(dumpInfo.dumpName.begin(), dumpInfo.dumpName.end());
    return;
  }

  unsigned numDescs;
  memcpy(&numDescs, static_cast<uint8_t*>(data) + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL NumDesc " << numDescs
         << "\n\n";

  D3D12_GPU_VIRTUAL_ADDRESS baseAddress = info.decodedBuffer->GetGPUVirtualAddress();

  for (unsigned i = 0; i < numDescs; ++i) {
    D3D12_RAYTRACING_GEOMETRY_DESC desc{};
    memcpy(&desc, static_cast<uint8_t*>(data) + offset, sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
    offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);

    if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
      stream << "D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES";
      printGeometryFlags(stream, desc.Flags);
      stream << "\n";
      printTrianglesDesc(stream, baseAddress, static_cast<uint8_t*>(data), desc.Triangles);
    } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
      stream << "D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS";
      printGeometryFlags(stream, desc.Flags);
      stream << "\n";
    }
  }
  info.stagingBuffer->Unmap(0, nullptr);
}

void AccelerationStructuresDump::printGeometryFlags(std::ostream& stream,
                                                    D3D12_RAYTRACING_GEOMETRY_FLAGS flags) {
  if (flags & D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE) {
    stream << " D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE";
  }
  if (flags & D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION) {
    stream << " D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION";
  }
}

void AccelerationStructuresDump::printTrianglesDesc(
    std::ostream& stream,
    D3D12_GPU_VIRTUAL_ADDRESS baseAddress,
    uint8_t* data,
    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& desc) {

  stream << "Transform3x4 0x" << std::hex << std::setw(16) << std::setfill('0') << desc.Transform3x4
         << std::dec << "\n";
  stream << "IndexFormat " << formatToString(desc.IndexFormat) << "\n";
  stream << "VertexFormat " << formatToString(desc.VertexFormat) << "\n";
  stream << "IndexCount " << desc.IndexCount << "\n";
  stream << "VertexCount " << desc.VertexCount << "\n";
  stream << "IndexBuffer 0x" << std::hex << std::setw(16) << std::setfill('0') << desc.IndexBuffer
         << std::dec << "\n";
  stream << "VertexBuffer stride " << desc.VertexBuffer.StrideInBytes << "\n";

  unsigned offset = static_cast<unsigned>(desc.VertexBuffer.StartAddress - baseAddress);
  if (desc.VertexCount) {
    printVertices(stream, data + offset, desc.VertexCount);
  }
}

void AccelerationStructuresDump::printVertices(std::ostream& stream,
                                               uint8_t* data,
                                               unsigned vertexCount) {
  stream << "VERTICES\n";
  for (unsigned i = 0; i < vertexCount; ++i) {
    unsigned* vertex = reinterpret_cast<unsigned*>(data + i * sizeof(unsigned) * 3);
    stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << vertex[0] << " ";
    stream << "0x" << std::setw(8) << std::setfill('0') << vertex[1] << " ";
    stream << "0x" << std::setw(8) << std::setfill('0') << vertex[2] << std::dec << "\n";
  }
  stream << "\n";
}

} // namespace DirectX
} // namespace gits
