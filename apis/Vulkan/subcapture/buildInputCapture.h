// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"

#include <cstdint>
#include <map>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;
class IGpuReadbackHelper;

// Record-time build-input capture: resolve the live device addresses a build reads back to their
// buffers, turn them into exact byte ranges and stage the readback copies. Shared by the
// acceleration structure, micromap and trace-rays SBT paths. The restore-time counterpart is
// asBuildCommandPatching.{h,cpp}.

// Append the buffer holding 'address' to depKeys, if any buffer does.
void ResolveAndTrackBufferAddress(StateTrackingService& stateTracking,
                                  VkDeviceAddress address,
                                  std::vector<uint64_t>& depKeys);

// Append the keys of every input buffer one geometry reads to depKeys, for the dependency walk
// in RestoreOne and AnalyzerService::AddClosure. Keep in step with ComputeGeometryInputRegions.
void CollectGeometryInputBufferKeys(StateTrackingService& stateTracking,
                                    const VkAccelerationStructureGeometryKHR& geometry,
                                    std::vector<uint64_t>& depKeys);

// A raw [Start, End) byte interval within an input buffer, before merging.
struct RawInputRegion {
  VkDeviceSize Start{};
  VkDeviceSize End{};
};

// Append the exact referenced byte range(s) of one geometry to regionsByBuffer, keyed by the
// owning buffer. Vertex spans round the last vertex up to a full stride, so they may run one
// trailing stride past a tightly-packed buffer - MergeInputRegions clamps that.
void ComputeGeometryInputRegions(StateTrackingService& stateTracking,
                                 const VkAccelerationStructureGeometryKHR& geometry,
                                 const VkAccelerationStructureBuildRangeInfoKHR& range,
                                 std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer);

// Merge overlapping/adjacent raw regions into minimal sorted intervals, clamped to the
// buffer size. Hash is left 0, filled at submit-time readback.
std::vector<CapturedBuildInputRegion> MergeInputRegions(std::vector<RawInputRegion> regions,
                                                        VkDeviceSize bufferSize);

// Turn per-buffer raw regions into CapturedBuildInputBuffer entries and stage the readback
// copies for each. Shared by the acceleration structure and micromap build Pre hooks.
void StageBuildInputReadbacks(StateTrackingService& stateTracking,
                              IGpuReadbackHelper& readbackHelper,
                              uint64_t deviceKey,
                              uint64_t physDevKey,
                              uint64_t cbKey,
                              std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer,
                              const char* what,
                              PendingAsInputReadback& pending);

} // namespace vulkan
} // namespace gits
