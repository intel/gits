// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"

namespace gits {
namespace vulkan {

// Destructor defined here so this TU is the key function owner and the vtable
// is emitted only once (avoids MSVC LNK1163 COMDAT conflicts).
AnalyzerLayer::~AnalyzerLayer() = default;

// Custom-handled for TLAS instance buffer readback.
void AnalyzerLayer::Post(vkCmdBuildAccelerationStructuresKHRCommand& command) {
  m_AnalyzerService.AddObjectForRestore(command.m_commandBuffer.Key);
  m_AnalyzerService.AddObjectsForRestore(command.m_pInfos.HandleKeys);

  m_RaytracingService.StageTlasInstanceReadbacks(command);
}

namespace {
// Adds the buffer backing a shader binding table region to the restore set.
void AddSBTBufferForRestore(AnalyzerRaytracingService& raytracingService,
                            AnalyzerService& analyzerService,
                            const VkStridedDeviceAddressRegionKHR* region) {
  if (!region) {
    return;
  }
  uint64_t key = raytracingService.ResolveBufferKeyForAddress(region->deviceAddress);
  if (key) {
    analyzerService.AddObjectForRestore(key);
  }
}
} // namespace

void AnalyzerLayer::Post(vkCmdTraceRaysKHRCommand& command) {
  m_AnalyzerService.AddObjectForRestore(command.m_commandBuffer.Key);

  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pRaygenShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pMissShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pHitShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pCallableShaderBindingTable.Value);
}

void AnalyzerLayer::Post(vkCmdTraceRaysIndirectKHRCommand& command) {
  m_AnalyzerService.AddObjectForRestore(command.m_commandBuffer.Key);

  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pRaygenShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pMissShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pHitShaderBindingTable.Value);
  AddSBTBufferForRestore(m_RaytracingService, m_AnalyzerService,
                         command.m_pCallableShaderBindingTable.Value);

  uint64_t indirectKey =
      m_RaytracingService.ResolveBufferKeyForAddress(command.m_indirectDeviceAddress.Value);
  if (indirectKey) {
    m_AnalyzerService.AddObjectForRestore(indirectKey);
  }
}

} // namespace vulkan
} // namespace gits
