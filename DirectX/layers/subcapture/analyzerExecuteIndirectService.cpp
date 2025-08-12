// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerExecuteIndirectService.h"
#include "bindingService.h"
#include "gits.h"

#include <fstream>

namespace gits {
namespace DirectX {

AnalyzerExecuteIndirectService::AnalyzerExecuteIndirectService(
    CapturePlayerGpuAddressService& gpuAddressService,
    AnalyzerRaytracingService& raytracingService,
    BindingService& bindingService)
    : gpuAddressService_(gpuAddressService),
      raytracingService_(raytracingService),
      bindingService_(bindingService) {
  loadExecuteIndirectDispatchRays();
}

AnalyzerExecuteIndirectService::~AnalyzerExecuteIndirectService() {
  for (auto& it : commandSignatures_) {
    delete[] it.second.pArgumentDescs;
  }
}

void AnalyzerExecuteIndirectService::createCommandSignature(
    ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC& desc = commandSignatures_[c.ppvCommandSignature_.key] =
      *c.pDesc_.value;
  desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[desc.NumArgumentDescs];
  std::copy(c.pDesc_.value->pArgumentDescs,
            c.pDesc_.value->pArgumentDescs + c.pDesc_.value->NumArgumentDescs,
            const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(desc.pArgumentDescs));
}

void AnalyzerExecuteIndirectService::executeIndirect(
    ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  bindingService_.addObjectForRestore(c.pCommandSignature_.key);

  auto itCommandSignature = commandSignatures_.find(c.pCommandSignature_.key);
  GITS_ASSERT(itCommandSignature != commandSignatures_.end());
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

  if (!view && !raytracing) {
    return;
  }

  auto dumpBindingTable = [&](UINT64 address, UINT64 size, UINT64 stride) {
    if (!address) {
      return;
    }
    CapturePlayerGpuAddressService::ResourceInfo* info =
        gpuAddressService_.getResourceInfoByCaptureAddress(address);
    GITS_ASSERT(info);
    unsigned offset = address - info->captureStart;
    bindingService_.addObjectForRestore(info->key);
    raytracingService_.dumpBindingTable(c.object_.value, info->resource, info->key, offset, size,
                                        stride);
  };

  auto it = executeIndirectDispatchRays_.find(c.key);
  if (it != executeIndirectDispatchRays_.end()) {
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
}

void AnalyzerExecuteIndirectService::loadExecuteIndirectDispatchRays() {
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
    executeIndirectDispatchRays_[callKey] = desc;
  }
}

} // namespace DirectX
} // namespace gits
