// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "raytracingInstancesDump.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"

#include <cstdint>
#include <map>
#include <vector>

namespace gits {
namespace vulkan {

// Discover BLASes used by TLAS.
class AnalyzerRaytracingService {
public:
  AnalyzerRaytracingService(StateTrackingService& stateTracking, IGpuReadbackHelper& gpuReadback)
      : m_StateTracking(stateTracking), m_InstancesDump(stateTracking, gpuReadback) {}

  // Instance data is only valid once the build has executed, so the readback itself
  // runs when the command buffer is submitted.
  void StageTlasInstanceReadbacks(const vkCmdBuildAccelerationStructuresKHRCommand& command);

  // Executes the readbacks staged for cbKey (GPU-ordered after the staged builds)
  // and records the resulting TLAS->BLAS mapping.
  void ReadStagedTlasInstances(uint64_t cbKey, uint64_t submitQueueKey);

  // Moves staged readbacks into the primary so they run when it is submitted.
  void MergeSecondary(uint64_t primaryCbKey, uint64_t secondaryCbKey);

  // Empty if tlasKey is not a TLAS, was never built, or referenced no known BLAS.
  const std::vector<uint64_t>& GetReferencedBlases(uint64_t tlasKey) const;

  // The address may point partway into a larger buffer. Returns 0 if unresolved.
  uint64_t ResolveBufferKeyForAddress(VkDeviceAddress address) const;

private:
  struct PendingTlasReadback {
    uint64_t TlasKey{};
    VkDeviceAddress InstanceArrayAddress{}; // base + primitiveOffset
    uint32_t InstanceCount{};
    // If false, InstanceArrayAddress points to contiguous
    // VkAccelerationStructureInstanceKHR structs. If true, to an array of
    // VkDeviceAddress, each pointing to a struct elsewhere.
    bool IsArrayOfPointers{false};
  };

  StateTrackingService& m_StateTracking;
  RaytracingInstancesDump m_InstancesDump;
  std::map<uint64_t, std::vector<uint64_t>> m_TlasToBlases;
  // Command buffer key to staged PendingTlasReadback
  std::map<uint64_t, std::vector<PendingTlasReadback>> m_PendingByCb;
};

} // namespace vulkan
} // namespace gits
