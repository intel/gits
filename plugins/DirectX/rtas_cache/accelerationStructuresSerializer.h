// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

class AccelerationStructuresSerializer : public ResourceDump {
public:
  AccelerationStructuresSerializer() : ResourceDump() {}
  ~AccelerationStructuresSerializer();
  void serializeAccelerationStructure(ID3D12GraphicsCommandList4* commandList,
                                      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc,
                                      const std::wstring& filePath);

protected:
  struct AccelerationStructuresDumpInfo : public DumpInfo {
    Microsoft::WRL::ComPtr<ID3D12Resource> serializedBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> postbuildInfoBuffer{};
    DumpInfo postbuildInfo;
  };
  void dumpStagedResource(DumpInfo& dumpInfo) override;
};

} // namespace DirectX
} // namespace gits
