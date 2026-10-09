// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "micromapRestoreService.h"
#include "stateTrackingService.h"
#include "analyzerResults.h"
#include "asBuildCommandPatching.h"
#include "subcaptureFatal.h"
#include "commandSerializersAuto.h"
#include "commandCodersAuto.h"
#include "commandsAuto.h"
#include "log.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits {
namespace vulkan {

MicromapRestoreService::MicromapRestoreService(StateTrackingService& sts) : m_Sts(sts) {}

void MicromapRestoreService::MarkBackingContentRestored(uint64_t micromapKey) {
  auto* state = m_Sts.GetState<MicromapState>(micromapKey);
  if (!state) {
    return;
  }
  if (auto* backing = m_Sts.GetState<BufferState>(state->BufferKey)) {
    backing->ContentRestored = true;
  }
}

// The micromap twin of StateTrackingService::EmitAccelerationStructureCopyReplay - no scratch,
// no size query and no address patching, since both endpoints travel by handle.
void MicromapRestoreService::EmitCopyReplay(uint64_t deviceKey,
                                            uint64_t queueKey,
                                            uint64_t poolKey,
                                            const std::vector<char>& commandBytes) {
  if (commandBytes.empty()) {
    return;
  }
  const uint64_t kContentCBKey = m_Sts.AllocateSyntheticKey();

  std::vector<char> scratch = commandBytes;
  vkCmdCopyMicromapEXTCommand cmd;
  Decode(scratch.data(), cmd);
  cmd.m_commandBuffer.Key = kContentCBKey;
  cmd.m_Key = m_Sts.m_Recorder.CreateStateRestoreKey();

  m_Sts.EmitOneShotCommandBuffer(deviceKey, queueKey, poolKey, kContentCBKey, [this, &cmd]() {
    m_Sts.m_Recorder.Record(vkCmdCopyMicromapEXTSerializer(cmd));
  });
}

bool MicromapRestoreService::EmitRelocatedCreate(uint64_t deviceKey,
                                                 uint64_t physDevKey,
                                                 const MicromapState& state,
                                                 uint64_t bufKey,
                                                 uint64_t memKey) {
  GITS_ASSERT(m_Sts.m_GpuReadbackHelper);

  if (state.CreationCommandBuffer.empty() || state.Size == 0 ||
      state.CreationCommandId != CommandId::ID_VKCREATEMICROMAPEXT) {
    return false;
  }

  const VkBufferUsageFlags usage =
      VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  VkDeviceAddress freshDeviceAddress = 0;
  uint64_t bufOpaque = 0;
  uint64_t memOpaque = 0;
  VkMemoryRequirements req{};
  if (!m_Sts.m_GpuReadbackHelper->ReserveFreshBufferAddress(
          deviceKey, physDevKey, state.Size, freshDeviceAddress, bufOpaque, memOpaque) ||
      !m_Sts.QueryCaptureReplayBufferRequirements(deviceKey, state.Size, usage, req)) {
    return false;
  }
  const uint32_t memType =
      m_Sts.m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, req.memoryTypeBits);
  if (memType == UINT32_MAX) {
    return false;
  }

  std::vector<char> scratch = state.CreationCommandBuffer;
  vkCreateMicromapEXTCommand cmd;
  Decode(scratch.data(), cmd);
  if (!cmd.m_pCreateInfo.Value || cmd.m_pCreateInfo.HandleKeys.empty()) {
    return false;
  }

  m_Sts.EmitCaptureReplayBufferCreate(deviceKey, bufKey, memKey, state.Size, req.size, memType,
                                      usage, bufOpaque, memOpaque, freshDeviceAddress);

  // Handle members travel exclusively via HandleKeys (see generator_coders.py).
  cmd.m_pCreateInfo.HandleKeys[0] = bufKey;
  cmd.m_pCreateInfo.Value->offset = 0;
  // Defensive: GITS never requests a micromap address, but an application might have, and that
  // address is not reproducible here.
  cmd.m_pCreateInfo.Value->deviceAddress = 0;
  cmd.m_pCreateInfo.Value->createFlags &= ~static_cast<VkMicromapCreateFlagsEXT>(
      VK_MICROMAP_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_EXT);
  cmd.m_Return.Value = VK_SUCCESS;
  cmd.m_Key = m_Sts.m_Recorder.CreateStateRestoreKey();
  m_Sts.m_Recorder.Record(vkCreateMicromapEXTSerializer(cmd));
  return true;
}

// The micromap counterpart of EmitAccelerationStructureRebuildBytes, sharing its two big helpers.
// No update mode, no ppBuildRangeInfos, only two baked addresses to relocate.
void MicromapRestoreService::EmitRebuildBytes(
    uint64_t deviceKey,
    uint64_t physDevKey,
    uint64_t queueKey,
    uint64_t poolKey,
    const std::vector<char>& commandBytes,
    const std::vector<CapturedBuildInputBuffer>& capturedInputs,
    uint64_t logMicromapKey,
    const std::unordered_set<uint64_t>& keepDstMicromapKeys) {
  if (commandBytes.empty()) {
    return;
  }
  // As in the acceleration structure path: only reservations made below may be released at
  // the end, anything already held belongs to the caller.
  const size_t reservationMark = m_Sts.m_GpuReadbackHelper->MarkReservedAddresses();

  const uint64_t kContentCBKey = m_Sts.AllocateSyntheticKey();

  // Decode mutates the source buffer (AddPtrs), so work on a copy. The stored bytes carry the
  // original app CB's key, dead by restore time, so it is patched to the one-shot CB below.
  std::vector<char> scratch = commandBytes;
  vkCmdBuildMicromapsEXTCommand cmd;
  Decode(scratch.data(), cmd);
  cmd.m_commandBuffer.Key = kContentCBKey;
  cmd.m_Key = m_Sts.m_Recorder.CreateStateRestoreKey();

  if (!RemoveUnreferencedMicromapBuildInfos(cmd, keepDstMicromapKeys)) {
    FatalSubcaptureError(
        "micromap build command key=" + std::to_string(cmd.m_Key) +
        " carries an unexpected handle key layout, so the destinations this restore does not "
        "need cannot be dropped from it");
  }
  if (cmd.m_infoCount.Value == 0) {
    LOG_TRACE << "Vulkan subcapture: micromap build command key=" << cmd.m_Key
              << " writes no needed destination, so it is not replayed";
    return;
  }

  std::vector<std::pair<uint64_t, uint64_t>> transientBufs; // (bufKey, memKey)
  std::vector<BuildInputRemap> inputRemaps;
  m_Sts.EmitCapturedBuildInputs(deviceKey, physDevKey, queueKey, poolKey, capturedInputs,
                                m_Sts.BuildInputBufferUsage(deviceKey, /*forMicromap=*/true),
                                logMicromapKey, "micromap", transientBufs, inputRemaps);

  // Relocate the build's two baked input addresses for any relocated input. scratchData is
  // not among them - it is replaced wholesale below.
  if (!inputRemaps.empty()) {
    auto relocate = [&inputRemaps](VkDeviceAddress& addr) {
      if (addr == 0) {
        return;
      }
      for (const auto& r : inputRemaps) {
        if (addr >= r.OldBase && addr < r.OldBase + r.Size) {
          addr = r.NewBase + (addr - r.OldBase);
          return;
        }
      }
    };
    for (uint32_t i = 0; i < cmd.m_infoCount.Value && cmd.m_pInfos.Value; ++i) {
      VkMicromapBuildInfoEXT& info = cmd.m_pInfos.Value[i];
      relocate(info.data.deviceAddress);
      relocate(info.triangleArray.deviceAddress);
    }
  }

  // Per info: reserve fresh scratch. The captured scratch address belongs to a buffer the
  // application has long freed, and its size cannot be trusted either.
  for (uint32_t i = 0; i < cmd.m_infoCount.Value && cmd.m_pInfos.Value; ++i) {
    VkMicromapBuildInfoEXT& info = cmd.m_pInfos.Value[i];
    const uint64_t dstKey = i < cmd.m_pInfos.HandleKeys.size() ? cmd.m_pInfos.HandleKeys[i] : 0;

    VkMicromapBuildSizesInfoEXT sizes{};
    if (!m_Sts.m_GpuReadbackHelper->QueryMicromapBuildSizes(deviceKey, info, sizes) ||
        sizes.buildScratchSize == 0) {
      // Without the sizes there is no scratch to give the build. Emitting it against the
      // application's long-freed scratch address would fault on replay.
      FatalSubcaptureError(
          "failed to query build sizes for micromap key=" + std::to_string(dstKey) +
          " (command key=" + std::to_string(cmd.m_Key) + "), so its build cannot be replayed");
    }

    // Sanity check, not a policy decision - same reasoning as the acceleration structure path.
    auto* dstState = m_Sts.GetState<MicromapState>(dstKey);
    if (dstState && dstState->Size && sizes.micromapSize > dstState->Size) {
      FatalSubcaptureError("micromap key=" + std::to_string(dstKey) +
                           " was created with size=" + std::to_string(dstState->Size) +
                           " but replaying its build (command key=" + std::to_string(cmd.m_Key) +
                           ") needs " + std::to_string(sizes.micromapSize));
    }

    VkDeviceAddress scratchAddress = 0;
    uint64_t scratchOpaqueAddress = 0;
    uint64_t scratchMemOpaqueAddress = 0;
    VkMemoryRequirements scratchReq{};
    uint32_t scratchMemType = UINT32_MAX;
    if (m_Sts.m_GpuReadbackHelper->ReserveFreshBufferAddress(
            deviceKey, physDevKey, sizes.buildScratchSize, scratchAddress, scratchOpaqueAddress,
            scratchMemOpaqueAddress) &&
        m_Sts.QueryCaptureReplayBufferRequirements(
            deviceKey, sizes.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, scratchReq)) {
      scratchMemType =
          m_Sts.m_GpuReadbackHelper->FindStagingMemoryType(physDevKey, scratchReq.memoryTypeBits);
    }
    if (scratchMemType == UINT32_MAX) {
      FatalSubcaptureError(
          "could not reserve a scratch buffer for micromap key=" + std::to_string(dstKey) +
          " (command key=" + std::to_string(cmd.m_Key) + "), so its build cannot be replayed");
    }
    const uint64_t scratchBufKey = m_Sts.AllocateSyntheticKey();
    const uint64_t scratchMemKey = m_Sts.AllocateSyntheticKey();
    m_Sts.EmitCaptureReplayBufferCreate(deviceKey, scratchBufKey, scratchMemKey,
                                        sizes.buildScratchSize, scratchReq.size, scratchMemType,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, scratchOpaqueAddress,
                                        scratchMemOpaqueAddress, scratchAddress);
    transientBufs.emplace_back(scratchBufKey, scratchMemKey);
    info.scratchData.deviceAddress = scratchAddress;
  }

  m_Sts.EmitOneShotCommandBuffer(deviceKey, queueKey, poolKey, kContentCBKey, [this, &cmd]() {
    m_Sts.m_Recorder.Record(vkCmdBuildMicromapsEXTSerializer(cmd));
  });

  for (const auto& [bufKey, memKey] : transientBufs) {
    m_Sts.EmitCaptureReplayBufferDestroy(deviceKey, bufKey, memKey);
  }

  m_Sts.m_GpuReadbackHelper->ReleaseReservedAddressesSince(reservationMark);
}

// Mirrors StateTrackingService::RestoreAccelerationStructureContents, and must run before it -
// see the declaration.
void MicromapRestoreService::RestoreContents() {
  // The micromap restore is a rebuild from captured inputs, which only exist on the chain path.
  // MicromapStateService::RequireChainRestore already refuses any other mode that writes a
  // micromap before the range, so reaching here means none holds pre-range content.
  if (!m_Sts.m_AnalyzerResults || !m_Sts.m_AnalyzerResults->UseAsChainRestore()) {
    return;
  }

  // Claim every live restored micromap's backing buffer before RestoreBufferContents runs, even
  // where the chain below produces no content - a raw byte copy over it is never a valid fallback.
  for (const auto& [key, sp] : m_Sts.m_States) {
    if (sp->CreationCommandId == CommandId::ID_VKCREATEMICROMAPEXT && !sp->Destroyed &&
        m_Sts.m_RestoredThisPass.count(key)) {
      MarkBackingContentRestored(key);
    }
  }

  RestoreChain();
}

// Three cases, in increasing cost: still live (just not restored yet), destroyed with its storage
// buffer still live, and destroyed along with it. No micromap address is pinned, so there is no
// VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS recycling hazard and the re-create is verbatim.
bool MicromapRestoreService::Resurrect(uint64_t micromapKey,
                                       uint64_t deviceKey,
                                       uint64_t physDevKey) {
  auto* state = m_Sts.GetState<MicromapState>(micromapKey);
  if (!state) {
    return false;
  }
  if (!state->Destroyed) {
    // Never enrolled for teardown: only micromaps the application itself destroyed may be
    // destroyed again.
    m_Sts.RestoreOne(state);
    return m_Sts.m_RestoredThisPass.count(micromapKey) != 0;
  }

  auto* bufState = m_Sts.GetState<BufferState>(state->BufferKey);
  if (!bufState) {
    return false;
  }
  ResurrectedMicromap resurrected{};
  resurrected.DeviceKey = deviceKey;
  bool created = false;
  if (!bufState->Destroyed) {
    // Storage buffer still live: bring it up and re-create the micromap verbatim, at its
    // original offset. Nothing needs relocating, because no micromap address is pinned.
    m_Sts.RestoreOne(bufState);
    created =
        m_Sts.m_RestoredThisPass.count(state->BufferKey) != 0 && m_Sts.EmitCreationCommand(state);
  } else {
    // Storage buffer destroyed too. It cannot come back through RestoreOne - it and its
    // allocation are chain-retained, which RestoreOne refuses as a dependency - so give the
    // micromap a dedicated relocated one.
    resurrected.BufferKey = m_Sts.AllocateSyntheticKey();
    resurrected.MemoryKey = m_Sts.AllocateSyntheticKey();
    created = EmitRelocatedCreate(deviceKey, physDevKey, *state, resurrected.BufferKey,
                                  resurrected.MemoryKey);
  }
  if (!created) {
    return false;
  }
  // The create was emitted directly rather than through RestoreOne, so register the handle
  // as restored by hand.
  m_Sts.m_RestoredThisPass.insert(micromapKey);
  // Kept alive for the whole of RestoreAccelerationStructureContents, since a retained build may
  // name any micromap of the chain.
  resurrected.MicromapKey = micromapKey;
  m_ResurrectedMicromaps.push_back(resurrected);
  return true;
}

// The micromap counterpart of StateTrackingService::RestoreBlasChain. Simpler in two ways: no
// update mode, so no source to repoint, and no per-op teardown, because every micromap this
// resurrects stays live until DestroyResurrected.
void MicromapRestoreService::RestoreChain() {
  const std::vector<MicromapChainOp>& chain = m_Sts.m_AnalyzerResults->GetMicromapChain();
  if (chain.empty()) {
    return;
  }
  GITS_ASSERT(m_Sts.m_GpuReadbackHelper);
  // Held past the end of this function, unlike the acceleration structure chain's mark: a
  // relocated storage buffer stays live in the stream until DestroyResurrected emits its destroy,
  // so its address may not be handed out again before then.
  m_ReservationMark = m_Sts.m_GpuReadbackHelper->MarkReservedAddresses();

  // Per-device queue/pool/physDev context, resolved lazily and cached.
  struct DevCtx {
    uint64_t PhysDev{};
    uint64_t Queue{};
    uint64_t Pool{};
    bool Ok{};
  };
  std::unordered_map<uint64_t, DevCtx> devCtxByDevice;
  auto resolveDev = [&](uint64_t deviceKey) -> const DevCtx* {
    auto it = devCtxByDevice.find(deviceKey);
    if (it != devCtxByDevice.end()) {
      return it->second.Ok ? &it->second : nullptr;
    }
    DevCtx ctx{};
    uint64_t queueKey = 0, poolKey = 0;
    if (m_Sts.FindQueueAndPool(deviceKey, queueKey, poolKey) &&
        m_Sts.m_RestoredThisPass.count(queueKey) && m_Sts.m_RestoredThisPass.count(poolKey)) {
      auto* devState = m_Sts.GetState<ObjectState>(deviceKey);
      if (devState && devState->ParentKey) {
        ctx = {devState->ParentKey, queueKey, poolKey, true};
      }
    }
    const DevCtx& stored = (devCtxByDevice[deviceKey] = ctx);
    return stored.Ok ? &stored : nullptr;
  };

  // Destinations the reduction kept, per command. A captured command writes every micromap
  // the application batched into it, of which the chain usually needs only some.
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> retainedDstByCmd;
  for (const MicromapChainOp& op : chain) {
    if (!op.IsCopy) {
      retainedDstByCmd[op.CommandKey].insert(op.DstMicromapKey);
    }
  }

  // Replay in the analyzer's execution (Id) order. A multi-info build is replayed once (dedup
  // by command key) since it rebuilds all its destinations. Copies replay verbatim, their
  // source micromap already produced by an earlier op in this order.
  std::unordered_set<uint64_t> replayedBuildCmds;

  for (const MicromapChainOp& op : chain) {
    auto rcIt = m_Sts.m_RetainedAsCommands.find(op.CommandKey);
    if (rcIt == m_Sts.m_RetainedAsCommands.end() || rcIt->second.CommandBytes.empty()) {
      // Usually a stale analysis file whose command keys no longer match this run.
      FatalSubcaptureError(
          "no captured command bytes for retained micromap chain op (command key=" +
          std::to_string(op.CommandKey) +
          ", destination micromap key=" + std::to_string(op.DstMicromapKey) + "); delete '" +
          AnalyzerResults::GetAnalysisFileName() +
          "' and re-run so the analysis pass regenerates it");
    }
    auto* dstState = m_Sts.GetState<MicromapState>(op.DstMicromapKey);
    if (!dstState) {
      FatalSubcaptureError("destination micromap key=" + std::to_string(op.DstMicromapKey) +
                           " of retained chain op (command key=" + std::to_string(op.CommandKey) +
                           ") is not tracked, so the op cannot be replayed");
    }
    const DevCtx* dev = resolveDev(dstState->ParentKey);
    if (!dev) {
      FatalSubcaptureError(
          "no restored queue and command pool with matching queue family indices on device key=" +
          std::to_string(dstState->ParentKey) + ", so retained micromap chain op (command key=" +
          std::to_string(op.CommandKey) + ") cannot be replayed");
    }

    // Give every micromap the emitted command names a live handle, re-creating the ones the
    // application already destroyed (build-then-compact destroys the uncompacted intermediate
    // right after the copy that reads it). For a build that is the whole retained destination
    // set, not just this op's: the command is emitted once below and writes all of them, so a
    // co-destination resurrected on its own later iteration would come up after the build that
    // needs it. Sorted so the keys Resurrect allocates do not depend on set iteration order.
    std::vector<uint64_t> required;
    if (op.IsCopy) {
      required = {op.SrcMicromapKey, op.DstMicromapKey};
    } else {
      const std::unordered_set<uint64_t>& keepDst = retainedDstByCmd[op.CommandKey];
      required.assign(keepDst.begin(), keepDst.end());
      std::sort(required.begin(), required.end());
    }
    for (uint64_t key : required) {
      if (!key || m_Sts.m_RestoredThisPass.count(key)) {
        continue;
      }
      if (!Resurrect(key, dstState->ParentKey, dev->PhysDev)) {
        FatalSubcaptureError(
            "micromap key=" + std::to_string(key) +
            " is required by retained chain op (command key=" + std::to_string(op.CommandKey) +
            ") but cannot be given a live handle, so the op cannot be replayed");
      }
    }

    // The op below regenerates the destination's backing-buffer content.
    MarkBackingContentRestored(op.DstMicromapKey);

    if (op.IsCopy) {
      EmitCopyReplay(dstState->ParentKey, dev->Queue, dev->Pool, rcIt->second.CommandBytes);
    } else if (replayedBuildCmds.insert(op.CommandKey).second) {
      // Inputs restricted to the destinations this replay keeps, as in RestoreBlasChain.
      const std::unordered_set<uint64_t>& keepDst = retainedDstByCmd[op.CommandKey];
      EmitRebuildBytes(
          dstState->ParentKey, dev->PhysDev, dev->Queue, dev->Pool, rcIt->second.CommandBytes,
          m_Sts.RetainedBuildInputsFor(op.CommandKey, keepDst), op.DstMicromapKey, keepDst);
    }
  }
}

// Emitted after RestoreAccelerationStructureContents, so every retained build that names one
// has already replayed. Reproduces the application's own destroy.
void MicromapRestoreService::DestroyResurrected() {
  size_t relocatedCount = 0;
  for (const ResurrectedMicromap& r : m_ResurrectedMicromaps) {
    vkDestroyMicromapEXTCommand destroyCmd;
    destroyCmd.m_device.Key = r.DeviceKey;
    destroyCmd.m_micromap.Key = r.MicromapKey;
    destroyCmd.m_Key = m_Sts.m_Recorder.CreateStateRestoreKey();
    m_Sts.m_Recorder.Record(vkDestroyMicromapEXTSerializer(destroyCmd));
    m_Sts.m_RestoredThisPass.erase(r.MicromapKey);
    // Only set when this restore created a dedicated relocated storage buffer for it.
    if (r.BufferKey || r.MemoryKey) {
      m_Sts.EmitCaptureReplayBufferDestroy(r.DeviceKey, r.BufferKey, r.MemoryKey);
      ++relocatedCount;
    }
  }
  m_ResurrectedMicromaps.clear();

  // Every relocated storage buffer is destroyed in the stream by now, so the player-side buffers
  // holding their addresses can go. ReleaseReservedAddressesSince truncates, so the assert guards
  // against silently freeing a reservation the acceleration structure restore leaked in between.
  if (m_ReservationMark != SIZE_MAX) {
    GITS_ASSERT(m_Sts.m_GpuReadbackHelper->MarkReservedAddresses() ==
                m_ReservationMark + relocatedCount);
    m_Sts.m_GpuReadbackHelper->ReleaseReservedAddressesSince(m_ReservationMark);
    m_ReservationMark = SIZE_MAX;
  }
}

} // namespace vulkan
} // namespace gits
