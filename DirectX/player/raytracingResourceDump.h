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
#include "tools_lite.h"

namespace gits {
namespace DirectX {

class RaytracingResourceDump : public ResourceDump {
public:
  enum ResourceType {
    Instances,
    InstancesArrayOfPointers,
    BindingTable
  };

  RaytracingResourceDump(CapturePlayerGpuAddressService& addressService,
                         CapturePlayerShaderIdentifierService& shaderIdentifierService,
                         CapturePlayerDescriptorHandleService& descriptorHandleService)
      : addressService_(addressService),
        shaderIdentifierService_(shaderIdentifierService),
        descriptorHandleService_(descriptorHandleService) {}
  ~RaytracingResourceDump();
  void dumpResource(ID3D12GraphicsCommandList* commandList,
                    ID3D12Resource* resource,
                    unsigned offset,
                    unsigned size,
                    unsigned stride,
                    D3D12_RESOURCE_STATES resourceState,
                    const std::wstring& dumpName,
                    ResourceType resourceType,
                    bool fromCapture);

protected:
  struct RaytracingDumpInfo : public DumpInfo {
    ResourceType resourceType;
    unsigned stride;
    bool fromCapture;
  };

  void dumpBuffer(DumpInfo& dumpInfo, void* data) override;

private:
  CapturePlayerGpuAddressService& addressService_;
  CapturePlayerShaderIdentifierService& shaderIdentifierService_;
  CapturePlayerDescriptorHandleService& descriptorHandleService_;

  void dumpInstancesBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void dumpInstancesArrayOfPointersBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void dumpBindingTableBuffer(RaytracingDumpInfo& dumpInfo, void* data);
  void printGpuAddress(std::ostream& stream, D3D12_GPU_VIRTUAL_ADDRESS address, bool fromCapture);
};

} // namespace DirectX
} // namespace gits
