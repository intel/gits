// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"
#include "commandsAuto.h"

namespace gits {
namespace DirectX {

class AccelerationStructuresDump : public ResourceDump {
public:
  AccelerationStructuresDump() : ResourceDump() {}
  ~AccelerationStructuresDump();
  void dumpAccelerationStructure(ID3D12GraphicsCommandList4* commandList,
                                 D3D12_GPU_VIRTUAL_ADDRESS accelerationStructure,
                                 const std::wstring& dumpName);

protected:
  struct AccelerationStructuresDumpInfo : public DumpInfo {
    Microsoft::WRL::ComPtr<ID3D12Resource> decodedBuffer{};
  };

  void dumpStagedResource(DumpInfo& dumpInfo) override;

private:
  void printGeometryFlags(std::ostream& stream, D3D12_RAYTRACING_GEOMETRY_FLAGS flags);
  void printTrianglesDesc(std::ostream& stream,
                          D3D12_GPU_VIRTUAL_ADDRESS baseAddress,
                          uint8_t* data,
                          D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& desc);
  void printVertices(std::ostream& stream, uint8_t* data, unsigned vertexCount);
};

} // namespace DirectX
} // namespace gits
