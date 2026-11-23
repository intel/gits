// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"

#include <cstdint>
#include <memory>

namespace gits {
namespace vulkan {

class StateTrackingService;
class IGpuReadbackHelper;
class CommandBufferLifecycleService;
class AnalyzerResults;
class SubcaptureRange;
class AnalyzerService;
class RaytracingOptimizationService;

// Record-time opacity micromap (VK_EXT_opacity_micromap) tracking: stages the GPU readback of
// every build's input buffers, snapshots the build and copy command bytes for the chain replay,
// feeds the analysis pass, and refuses what subcapture cannot restore. The restore-time
// counterpart is MicromapRestoreService, reached only through StateTrackingService. Must not be
// referenced from raytracingOptimizationService - the host-only Vulkan_subcapture_test builds
// that without the layer's dependency graph.
class MicromapStateService {
public:
  // analyzerResults is SubcaptureLayer's by-value member, never null - unlike the pointer of the
  // same type on StateTrackingService.
  MicromapStateService(StateTrackingService& sts,
                       IGpuReadbackHelper& readbackHelper,
                       CommandBufferLifecycleService& cbLifecycle,
                       const AnalyzerResults& analyzerResults,
                       const SubcaptureRange& range,
                       bool analysisMode);

  // Records the input-buffer readback copies before the build executes. Recording pass only.
  void OnCmdBuildPre(vkCmdBuildMicromapsEXTCommand& command);

  // Tracks dependencies, validates the build mode, feeds the chain graph and the analyzer, and
  // snapshots the command bytes for the chain replay.
  void OnCmdBuild(vkCmdBuildMicromapsEXTCommand& command);

  // The CLONE/COMPACT copy, and the reason micromaps need a chain at all.
  void OnCmdCopy(vkCmdCopyMicromapEXTCommand& command);

  void OnCreate(vkCreateMicromapEXTCommand& command);
  void OnDestroy(vkDestroyMicromapEXTCommand& command);

  // Analysis pass: remember which micromaps an acceleration structure build reads, so a retained
  // build can pull them into the restore closure. No-op outside analysis mode.
  void NoteAsBuildMicromapReads(const vkCmdBuildAccelerationStructuresKHRCommand& command);

  // Analysis-pass-only collaborators, null in the recording pass.
  void SetAnalyzerService(AnalyzerService* service) {
    m_AnalyzerService = service;
  }
  void SetRaytracingOptimizationService(RaytracingOptimizationService* service) {
    m_RaytracingOptimizationService = service;
  }

private:
  void RefuseUnsupportedBuildMode(uint64_t micromapKey, VkBuildMicromapModeEXT mode);
  void RefuseUnsupportedCopyMode(uint64_t micromapKey, VkCopyMicromapModeEXT mode);
  // Recording pass: abort when a pre-range micromap op is seen but no reduced chain was loaded.
  // A micromap has no serialized restore to fall back on, unlike an acceleration structure.
  void RequireChainRestore(const char* commandName);
  // Recording pass: abort when a pre-range micromap op is seen but the loaded analysis file has
  // no MicromapChain section, i.e. it was written before micromaps were supported.
  void RequireMicromapChain(const char* commandName);

  // Duplicates SubcaptureLayer::StoreState - sharing it would pull commandCodersAuto.h into
  // stateTrackingService.h for every dependent TU.
  template <typename TCommand>
  void StoreState(std::unique_ptr<ObjectState> state, const TCommand& command);

  StateTrackingService& m_StateTracking;
  IGpuReadbackHelper& m_GpuReadbackHelper;
  CommandBufferLifecycleService& m_CommandBufferLifecycle;
  const AnalyzerResults& m_AnalyzerResults;
  const SubcaptureRange& m_SubcaptureRange;
  bool m_AnalysisMode{false};
  AnalyzerService* m_AnalyzerService{nullptr};
  RaytracingOptimizationService* m_RaytracingOptimizationService{nullptr};
};

} // namespace vulkan
} // namespace gits
