// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "capturePlayerDescriptorHandleService.h"

#include <d3d12.h>
#include <string>
#include <thread>
#include <wrl/client.h>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

namespace gits {
namespace DirectX {

class RaytracingResourceDump : public ResourceDump {
public:
  enum class DumpContentKind {
    Instances,
    InstancesArrayOfPointers,
    BindingTable
  };

  RaytracingResourceDump(CapturePlayerGpuAddressService& addressService,
                         CapturePlayerShaderIdentifierService& shaderIdentifierService,
                         CapturePlayerDescriptorHandleService& descriptorHandleService)
      : m_AddressService(addressService),
        m_ShaderIdentifierService(shaderIdentifierService),
        m_DescriptorHandleService(descriptorHandleService) {}
  ~RaytracingResourceDump();
  void DumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned offset,
                    unsigned size,
                    unsigned stride,
                    D3D12_RESOURCE_STATES resourceState,
                    const std::wstring& dumpName,
                    DumpContentKind contentKind,
                    bool fromCapture);

protected:
  struct RaytracingDumpInfo : public DumpInfo {
    DumpContentKind ContentKind{};
    unsigned Stride{};
    bool FromCapture{};
  };

  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  CapturePlayerGpuAddressService& m_AddressService;
  CapturePlayerShaderIdentifierService& m_ShaderIdentifierService;
  CapturePlayerDescriptorHandleService& m_DescriptorHandleService;

  void DumpInstancesBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void DumpInstancesArrayOfPointersBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void DumpBindingTableBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void PrintGpuAddress(std::ostream& stream, D3D12_GPU_VIRTUAL_ADDRESS address, bool fromCapture);
};

} // namespace DirectX
} // namespace gits
