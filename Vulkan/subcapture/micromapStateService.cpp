// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "micromapStateService.h"
#include "asBuildCommandPatching.h"
#include "buildInputCapture.h"
#include "stateTrackingService.h"
#include "commandBufferLifecycleService.h"
#include "analyzerResults.h"
#include "analyzerService.h"
#include "raytracingOptimizationService.h"
#include "subcaptureRange.h"
#include "subcaptureFatal.h"
#include "log.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace gits {
namespace vulkan {

namespace {

// Usage count i of a micromap build, from whichever of the two arrays is populated.
// VUID-VkMicromapBuildInfoEXT-pUsageCounts-07516 makes exactly one of them non-null.
const VkMicromapUsageEXT* MicromapUsageAt(const VkMicromapBuildInfoEXT& info, uint32_t i) {
  if (info.pUsageCounts) {
    return &info.pUsageCounts[i];
  }
  return info.ppUsageCounts ? info.ppUsageCounts[i] : nullptr;
}

uint32_t MicromapTriangleCount(const VkMicromapBuildInfoEXT& info) {
  uint64_t total = 0;
  for (uint32_t i = 0; i < info.usageCountsCount; ++i) {
    if (const VkMicromapUsageEXT* usage = MicromapUsageAt(info, i)) {
      total += usage->count;
    }
  }
  return static_cast<uint32_t>(std::min<uint64_t>(total, UINT32_MAX));
}

// Payload bytes the usage counts account for. A diagnostic only - see
// ComputeMicromapInputRegions for why this must not become the capture bound.
uint64_t MicromapUsageDerivedPayloadSize(const VkMicromapBuildInfoEXT& info) {
  uint64_t derived = 0;
  for (uint32_t i = 0; i < info.usageCountsCount; ++i) {
    const VkMicromapUsageEXT* usage = MicromapUsageAt(info, i);
    if (!usage) {
      continue;
    }
    // 2-state micromaps are 1 bit per microtriangle, 4-state are 2 bits.
    const uint64_t bitsPerMicroTriangle =
        usage->format == VK_OPACITY_MICROMAP_FORMAT_4_STATE_EXT ? 2 : 1;
    const uint64_t microTriangles = 1ull << (2 * usage->subdivisionLevel);
    derived +=
        static_cast<uint64_t>(usage->count) * ((microTriangles * bitsPerMicroTriangle + 7) / 8);
  }
  return derived;
}

// Append the referenced byte ranges of one micromap build info to regionsByBuffer. triangleArray
// is bounded exactly, data as the tail of its buffer: the exact payload extent lives in
// triangleArray's contents, only valid at build execution, and deriving it from the usage counts
// is a heuristic that would silently build the micromap over garbage. scratchData is excluded.
void ComputeMicromapInputRegions(StateTrackingService& stateTracking,
                                 const VkMicromapBuildInfoEXT& info,
                                 std::map<uint64_t, std::vector<RawInputRegion>>& regionsByBuffer) {
  auto addRegion = [&](VkDeviceAddress address, VkDeviceSize size) {
    if (address == 0 || size == 0) {
      return;
    }
    auto found = stateTracking.GetDeviceAddressTracking().FindContaining(address);
    if (!found) {
      return;
    }
    regionsByBuffer[found->first].push_back({found->second, found->second + size});
  };

  VkDeviceSize triangleStride = info.triangleArrayStride;
  if (triangleStride == 0) {
    triangleStride = sizeof(VkMicromapTriangleEXT);
    LOG_WARNING << "Vulkan subcapture: vkCmdBuildMicromapsEXT with triangleArrayStride == 0, "
                   "assuming sizeof(VkMicromapTriangleEXT)";
  }
  addRegion(info.triangleArray.deviceAddress,
            static_cast<VkDeviceSize>(MicromapTriangleCount(info)) * triangleStride);

  auto found =
      info.data.deviceAddress
          ? stateTracking.GetDeviceAddressTracking().FindContaining(info.data.deviceAddress)
          : std::nullopt;
  if (!found) {
    return;
  }
  auto* buf = stateTracking.GetState<BufferState>(found->first);
  if (!buf || buf->BufferSize <= found->second) {
    return;
  }
  const VkDeviceSize tail = buf->BufferSize - found->second;
  addRegion(info.data.deviceAddress, tail);

  // Surface a title that is paying a lot for the over-capture.
  const uint64_t derived = MicromapUsageDerivedPayloadSize(info);
  if (derived > 0 && tail > derived * 8) {
    LOG_WARNING << "Vulkan subcapture: micromap build input buffer key=" << found->first
                << " is captured as a " << tail
                << " byte tail, but the usage counts only account for " << derived
                << " bytes. The exact extent cannot be bounded at record time - see "
                   "ComputeMicromapInputRegions";
  }
}

} // namespace

MicromapStateService::MicromapStateService(StateTrackingService& sts,
                                           IGpuReadbackHelper& readbackHelper,
                                           CommandBufferLifecycleService& cbLifecycle,
                                           const AnalyzerResults& analyzerResults,
                                           const SubcaptureRange& range,
                                           bool analysisMode)
    : m_StateTracking(sts),
      m_GpuReadbackHelper(readbackHelper),
      m_CommandBufferLifecycle(cbLifecycle),
      m_AnalyzerResults(analyzerResults),
      m_SubcaptureRange(range),
      m_AnalysisMode(analysisMode) {}

template <typename TCommand>
void MicromapStateService::StoreState(std::unique_ptr<ObjectState> state, const TCommand& command) {
  state->CreationCommandId = command.GetId();
  uint32_t size = GetSize(command);
  state->CreationCommandBuffer.resize(size);
  Encode(command, state->CreationCommandBuffer.data());
  m_StateTracking.StoreState(std::move(state));
}

// Mirrors SubcaptureLayer::Pre(vkCmdBuildAccelerationStructuresKHRCommand&) - only the region
// math and the IsMicromap flag differ.
void MicromapStateService::OnCmdBuildPre(vkCmdBuildMicromapsEXTCommand& command) {
  if (m_AnalysisMode || !m_AnalyzerResults.UseAsChainRestore()) {
    return;
  }
  const uint32_t infoCount = command.m_infoCount.Value;
  if (infoCount == 0 || !command.m_pInfos.Value || command.m_pInfos.HandleKeys.size() < infoCount) {
    return;
  }
  const uint64_t cbKey = command.m_commandBuffer.Key;
  auto* cbState = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cbState) {
    return;
  }
  auto* poolState = m_StateTracking.GetState<CommandPoolState>(cbState->PoolKey);
  if (!poolState) {
    return;
  }
  const uint64_t deviceKey = poolState->ParentKey;
  auto* devState = m_StateTracking.GetState<DeviceState>(deviceKey);
  if (!devState) {
    return;
  }
  const uint64_t physDevKey = devState->ParentKey;

  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkMicromapBuildInfoEXT& info = command.m_pInfos.Value[i];
    // One key per info: HandleKeys[i] is that info's dstMicromap.
    const uint64_t dstKey = command.m_pInfos.HandleKeys[i];
    if (!dstKey) {
      continue;
    }
    std::map<uint64_t, std::vector<RawInputRegion>> regionsByBuffer;
    ComputeMicromapInputRegions(m_StateTracking, info, regionsByBuffer);
    if (regionsByBuffer.empty()) {
      continue;
    }

    PendingAsInputReadback pending;
    pending.AsKey = dstKey;
    pending.CommandKey = command.m_Key;
    pending.IsMicromap = true;
    StageBuildInputReadbacks(m_StateTracking, m_GpuReadbackHelper, deviceKey, physDevKey, cbKey,
                             regionsByBuffer, "micromap", pending);
    if (!pending.Buffers.empty()) {
      cbState->AsInputReadbacksAfterSubmit.push_back(std::move(pending));
    }
  }
}

// Modelled on Post(vkCmdBuildAccelerationStructuresKHRCommand&), with a simpler key layout - one
// dstMicromap key per info, no payload run.
void MicromapStateService::OnCmdBuild(vkCmdBuildMicromapsEXTCommand& command) {
  const uint32_t infoCount = command.m_infoCount.Value;
  if (infoCount == 0 || !command.m_pInfos.Value || command.m_pInfos.HandleKeys.size() < infoCount) {
    return;
  }

  RequireChainRestore("vkCmdBuildMicromapsEXT");

  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pInfos.HandleKeys);

  uint32_t sz = GetSize(command);
  std::vector<char> encoded(sz);
  Encode(command, encoded.data());

  for (uint32_t i = 0; i < infoCount; ++i) {
    const VkMicromapBuildInfoEXT& info = command.m_pInfos.Value[i];
    const uint64_t dstKey = command.m_pInfos.HandleKeys[i];
    if (!dstKey) {
      continue;
    }
    auto* dstState = m_StateTracking.GetState<MicromapState>(dstKey);
    if (!dstState) {
      continue;
    }
    RefuseUnsupportedBuildMode(dstKey, info.mode);

    // Analysis pass: feed the micromap chain graph with this pre-range build. Always a chain
    // root - VkBuildMicromapModeEXT has no update mode.
    if (m_AnalysisMode && m_RaytracingOptimizationService && m_SubcaptureRange.BeforeRange()) {
      m_RaytracingOptimizationService->RecordMicromapBuild(command.m_commandBuffer.Key,
                                                           command.m_Key, dstKey);
    }

    // Analysis pass: an in-range build fills its destination itself, so the restore does not
    // have to produce it.
    if (m_AnalysisMode && m_AnalyzerService) {
      m_AnalyzerService->NoteInRangeMicromapWrite(dstKey);
    }

    // Lifecycle tracking only, as in the acceleration structure path: the application may
    // destroy these right after a one-time build, and gating the micromap on them would drop it.
    std::vector<uint64_t> cbDepKeys;
    ResolveAndTrackBufferAddress(m_StateTracking, info.data.deviceAddress, cbDepKeys);
    ResolveAndTrackBufferAddress(m_StateTracking, info.scratchData.deviceAddress, cbDepKeys);
    ResolveAndTrackBufferAddress(m_StateTracking, info.triangleArray.deviceAddress, cbDepKeys);
    for (uint64_t dep : cbDepKeys) {
      m_CommandBufferLifecycle.TrackHandleDependency(command.m_commandBuffer.Key, dep);
    }
  }

  // Recording pass: keep the whole command's bytes for chain replay if it is a retained
  // micromap op (no-op in analysis mode / without a loaded MicromapChain).
  m_StateTracking.StoreRetainedMicromapCommandBytes(command.m_Key, encoded, /*isCopy=*/false);
}

// The reason micromaps need a chain at all: a compaction links the uncompacted micromap's chain
// to the compacted one, so "last build wins" no longer describes the content.
void MicromapStateService::OnCmdCopy(vkCmdCopyMicromapEXTCommand& command) {
  RequireChainRestore("vkCmdCopyMicromapEXT");
  RequireChainForCopy("vkCmdCopyMicromapEXT");

  m_CommandBufferLifecycle.TrackHandleDependencies(command.m_commandBuffer.Key,
                                                   command.m_pInfo.HandleKeys);

  // HandleKeys mirrors VkCopyMicromapInfoEXT field order: [src, dst].
  const uint64_t srcKey =
      command.m_pInfo.HandleKeys.size() >= 2 ? command.m_pInfo.HandleKeys[0] : 0;
  const uint64_t dstKey =
      command.m_pInfo.HandleKeys.size() >= 2 ? command.m_pInfo.HandleKeys[1] : 0;
  if (command.m_pInfo.Value) {
    RefuseUnsupportedCopyMode(dstKey, command.m_pInfo.Value->mode);
  }

  // Recording pass: keep this copy's bytes for chain replay if it is a retained micromap op.
  {
    uint32_t sz = GetSize(command);
    std::vector<char> encoded(sz);
    Encode(command, encoded.data());
    m_StateTracking.StoreRetainedMicromapCommandBytes(command.m_Key, encoded, /*isCopy=*/true);
  }

  // Analysis pass: feed the micromap chain graph with this pre-range copy, which the mode
  // check above has narrowed to CLONE/COMPACT.
  if (m_AnalysisMode && m_RaytracingOptimizationService && m_SubcaptureRange.BeforeRange() &&
      command.m_pInfo.Value && dstKey) {
    m_RaytracingOptimizationService->RecordMicromapCopy(
        command.m_commandBuffer.Key, command.m_Key, dstKey, srcKey, command.m_pInfo.Value->mode);
  }

  // Analysis pass: diagnostic for micromaps used in range
  if (m_AnalysisMode && m_AnalyzerService) {
    m_AnalyzerService->NoteInRangeMicromapRead(srcKey);
    m_AnalyzerService->NoteInRangeMicromapWrite(dstKey);
  }
}

void MicromapStateService::OnCreate(vkCreateMicromapEXTCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  auto state = std::make_unique<MicromapState>();
  state->Key = command.m_pMicromap.Key;
  state->ParentKey = command.m_device.Key;
  if (command.m_pCreateInfo.Value) {
    const auto& ci = *command.m_pCreateInfo.Value;
    state->Type = ci.type;
    state->Offset = ci.offset;
    state->Size = ci.size;
    // No address tracking, unlike the acceleration structure hook: nothing in the API
    // consumes a micromap address, so GITS pins none.
  }
  // VkMicromapCreateInfoEXT::buffer is the only handle member.
  if (!command.m_pCreateInfo.HandleKeys.empty()) {
    state->BufferKey = command.m_pCreateInfo.HandleKeys[0];
    state->DependencyKeys.push_back(state->BufferKey);
    // Same retention need as an acceleration structure's storage buffer, so it reuses the same
    // flag rather than adding a parallel one.
    if (auto* bufState = m_StateTracking.GetState<BufferState>(state->BufferKey)) {
      bufState->AsBacking = true;
      if (auto* memState = m_StateTracking.GetState<DeviceMemoryState>(bufState->BoundMemoryKey)) {
        memState->AsBacking = true;
      }
    }
  }
  StoreState(std::move(state), command);
}

void MicromapStateService::OnDestroy(vkDestroyMicromapEXTCommand& command) {
  // Keep the state (flagged Destroyed) so a micromap a retained chain op still names can be
  // resurrected - an application that compacts destroys the uncompacted one right after the copy.
  auto* state = m_StateTracking.GetState<MicromapState>(command.m_micromap.Key);
  if (state) {
    state->Destroyed = true;
  }
}

// Seeding the closure from the build's Dst/Src acceleration structure keys alone would miss a
// micromap that only a pre-range build names.
void MicromapStateService::NoteAsBuildMicromapReads(
    const vkCmdBuildAccelerationStructuresKHRCommand& command) {
  if (!m_AnalysisMode) {
    return;
  }
  std::vector<uint64_t> micromapKeys;
  if (CollectAsBuildMicromapKeys(command, micromapKeys) && !micromapKeys.empty()) {
    if (m_RaytracingOptimizationService) {
      m_RaytracingOptimizationService->RecordAsBuildMicromapReads(command.m_commandBuffer.Key,
                                                                  command.m_Key, micromapKeys);
    }
    // An in-range build reads the micromap's content, so the restore has to have produced
    // it - the main source of reads for the unproduced-micromap diagnostic.
    if (m_AnalyzerService) {
      for (uint64_t micromapKey : micromapKeys) {
        m_AnalyzerService->NoteInRangeMicromapRead(micromapKey);
      }
    }
  }
}

// VkBuildMicromapModeEXT has only VK_BUILD_MICROMAP_MODE_BUILD_EXT today. Refuse loudly if a
// future header adds a mode, rather than mis-replaying it as a build.
void MicromapStateService::RefuseUnsupportedBuildMode(uint64_t micromapKey,
                                                      VkBuildMicromapModeEXT mode) {
  if (mode == VK_BUILD_MICROMAP_MODE_BUILD_EXT) {
    return;
  }
  if (!m_SubcaptureRange.BeforeRange() && !m_SubcaptureRange.InRange()) {
    return; // after the range: cannot affect the subcapture
  }
  FatalSubcaptureError("the stream builds micromap key=" + std::to_string(micromapKey) +
                       " with VkBuildMicromapModeEXT value " + std::to_string(mode) +
                       ", which subcapture does not restore. Only "
                       "VK_BUILD_MICROMAP_MODE_BUILD_EXT is supported");
}

// VUID-VkCopyMicromapInfoEXT-mode-07531 already limits vkCmdCopyMicromapEXT to these two, so
// this guards against a future header rather than an application. Serialize and deserialize
// arrive as their own commands and stay refused.
void MicromapStateService::RefuseUnsupportedCopyMode(uint64_t micromapKey,
                                                     VkCopyMicromapModeEXT mode) {
  if (mode == VK_COPY_MICROMAP_MODE_COMPACT_EXT || mode == VK_COPY_MICROMAP_MODE_CLONE_EXT) {
    return;
  }
  if (!m_SubcaptureRange.BeforeRange() && !m_SubcaptureRange.InRange()) {
    return; // after the range: cannot affect the subcapture
  }
  FatalSubcaptureError("the stream copies into micromap key=" + std::to_string(micromapKey) +
                       " with VkCopyMicromapModeEXT value " + std::to_string(mode) +
                       ", which subcapture does not restore. Only "
                       "VK_COPY_MICROMAP_MODE_COMPACT_EXT and VK_COPY_MICROMAP_MODE_CLONE_EXT "
                       "are supported");
}

void MicromapStateService::RequireChainRestore(const char* commandName) {
  const bool unrestorable =
      !m_AnalysisMode && m_SubcaptureRange.BeforeRange() && !m_AnalyzerResults.UseAsChainRestore();
  if (!unrestorable) {
    return;
  }
  const std::string reason =
      m_AnalyzerResults.CaptureAsBuildInputs()
          ? "no reduced chain was loaded. Run with Common.Player.Subcapture.Optimize=true, and "
            "delete a stale analysis file '" +
                AnalyzerResults::GetAnalysisFileName() + "' so the analysis pass regenerates it"
          : "Common.Player.Subcapture.Vulkan.CaptureASBuildInputs is false, and a micromap has "
            "no serialized restore to fall back on, unlike an acceleration structure. Run with "
            "that option and Common.Player.Subcapture.Optimize both true";
  FatalSubcaptureError(std::string("the stream calls ") + commandName +
                       " before the subcapture range, so its micromap content can only be "
                       "restored by replaying the captured build chain, but " +
                       reason);
}

void MicromapStateService::RequireChainForCopy(const char* commandName) {
  const bool unrestorable = !m_AnalysisMode && m_AnalyzerResults.UseAsChainRestore() &&
                            !m_AnalyzerResults.HasMicromapChain();
  if (!unrestorable) {
    return;
  }
  FatalSubcaptureError(
      std::string("the stream calls ") + commandName + ", but the analysis file '" +
      AnalyzerResults::GetAnalysisFileName() +
      "' carries no MicromapChain section, so it was written before micromap copies were "
      "supported. Delete that file and re-run so the analysis pass regenerates it");
}

} // namespace vulkan
} // namespace gits
