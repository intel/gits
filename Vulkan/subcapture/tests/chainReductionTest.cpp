// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for RaytracingOptimizationService: no GPU, no driver, no stream. The chain
// reduction decides what the state restore replays, and getting it wrong is silent - a dropped
// op leaves its destination holding uninitialized memory, which surfaces as a device loss
// frames later. Both graphs are covered, since they share one implementation.

#include "raytracingOptimizationService.h"

#include <cstdio>
#include <vector>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

// Arbitrary but distinct keys. Command keys and object keys share one space in GITS, so the
// test keeps them far apart for legibility only.
constexpr uint64_t kCb = 0x100;
constexpr uint64_t kCb2 = 0x101;
constexpr uint64_t kMicromapA = 0x1000;
constexpr uint64_t kMicromapB = 0x1001;
constexpr uint64_t kMicromapC = 0x1002;
constexpr uint64_t kMicromapD = 0x1003;
constexpr uint64_t kBlasA = 0x2000;
constexpr uint64_t kBlasB = 0x2001;

using gits::vulkan::RaytracingOptimizationService;

// Build then compact, the shape this reduction exists for: an application builds into a
// scratch micromap, queries the compacted size, copies into a right-sized one and destroys the
// intermediate. Both ops must survive, in that order, linked source to destination.
void TestMicromapCompactionChainRetained() {
  RaytracingOptimizationService svc;
  svc.RecordMicromapBuild(kCb, /*commandKey=*/1, kMicromapA);
  svc.RecordMicromapCopy(kCb, /*commandKey=*/2, kMicromapB, kMicromapA,
                         VK_COPY_MICROMAP_MODE_COMPACT_EXT);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapB}, {});

  const auto& chain = svc.GetOptimizedMicromapCommands();
  Check(chain.size() == 2, "compaction: both ops retained");
  if (chain.size() != 2) {
    return;
  }
  Check(chain[0].CommandKey == 1 && !chain[0].IsCopy && chain[0].DstMicromapKey == kMicromapA,
        "compaction: the root build replays first");
  Check(chain[0].SourceCommandKey == 0, "compaction: the root build has no source");
  Check(chain[1].CommandKey == 2 && chain[1].IsCopy && chain[1].DstMicromapKey == kMicromapB,
        "compaction: the copy replays second");
  Check(chain[1].SrcMicromapKey == kMicromapA && chain[1].SourceCommandKey == 1,
        "compaction: the copy is linked to the build that produced its source");
  Check(chain[1].CopyMode == VK_COPY_MICROMAP_MODE_COMPACT_EXT,
        "compaction: the copy mode round-trips through the untyped graph");
  Check(svc.GetMicromapOverwriteConflicts().empty(), "compaction: no overwrite conflict");
}

// A chain no consumer references costs restore time and can fail on its own, so it must be
// pruned even though its ops are perfectly well formed.
void TestUnusedMicromapChainPruned() {
  RaytracingOptimizationService svc;
  svc.RecordMicromapBuild(kCb, 1, kMicromapA);
  svc.RecordMicromapCopy(kCb, 2, kMicromapB, kMicromapA, VK_COPY_MICROMAP_MODE_COMPACT_EXT);
  svc.RecordMicromapBuild(kCb, 3, kMicromapC);
  svc.RecordMicromapCopy(kCb, 4, kMicromapD, kMicromapC, VK_COPY_MICROMAP_MODE_COMPACT_EXT);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapB}, {});

  const auto& chain = svc.GetOptimizedMicromapCommands();
  Check(chain.size() == 2, "pruning: only the used chain survives");
  for (const auto& op : chain) {
    Check(op.DstMicromapKey != kMicromapC && op.DstMicromapKey != kMicromapD,
          "pruning: no op of the unreferenced chain is retained");
  }
}

// A rebuild of the same micromap supersedes the earlier one, which is what makes "the last op
// wins" the whole description of a micromap nothing copies.
void TestMicromapRebuildSupersedes() {
  RaytracingOptimizationService svc;
  svc.RecordMicromapBuild(kCb, 1, kMicromapA);
  svc.RecordMicromapBuild(kCb, 2, kMicromapA);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapA}, {});

  const auto& chain = svc.GetOptimizedMicromapCommands();
  Check(chain.size() == 1, "supersede: only the last build is retained");
  if (!chain.empty()) {
    Check(chain[0].CommandKey == 2, "supersede: the retained build is the later one");
  }
}

// The gap this detection exists for: a retained acceleration structure build read one content
// of a micromap and a later op overwrote it. The restore can only reproduce the final content,
// so replaying that build would rebuild the structure over the wrong opacity data.
void TestMicromapOverwriteAfterConsumingBuildConflicts() {
  RaytracingOptimizationService svc;
  svc.RecordMicromapBuild(kCb, /*commandKey=*/1, kMicromapA);
  svc.RecordAsBuildMicromapReads(kCb, /*commandKey=*/10, {kMicromapA});
  svc.RecordMicromapBuild(kCb, /*commandKey=*/2, kMicromapA);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapA}, {10});

  const auto& conflicts = svc.GetMicromapOverwriteConflicts();
  Check(conflicts.size() == 1, "overwrite: the conflict is reported exactly once");
  if (conflicts.empty()) {
    return;
  }
  Check(conflicts[0].MicromapKey == kMicromapA, "overwrite: names the micromap");
  Check(conflicts[0].ConsumedCommandKey == 1, "overwrite: names the content the build read");
  Check(conflicts[0].AsBuildCommandKey == 10, "overwrite: names the consuming build");
  Check(conflicts[0].OverwritingCommandKey == 2, "overwrite: names the op that overwrote it");
}

// The same sequence without the overwrite, and with the overwrite but an unretained consumer:
// neither is a conflict, and reporting them would refuse streams that restore correctly.
void TestMicromapOverwriteFalsePositives() {
  {
    RaytracingOptimizationService svc;
    svc.RecordMicromapBuild(kCb, 1, kMicromapA);
    svc.RecordAsBuildMicromapReads(kCb, 10, {kMicromapA});
    svc.OnQueueSubmit(kCb);
    svc.Optimize({});
    svc.OptimizeMicromaps({kMicromapA}, {10});
    Check(svc.GetMicromapOverwriteConflicts().empty(),
          "overwrite: reading the final content is not a conflict");
  }
  {
    RaytracingOptimizationService svc;
    svc.RecordMicromapBuild(kCb, 1, kMicromapA);
    svc.RecordAsBuildMicromapReads(kCb, 10, {kMicromapA});
    svc.RecordMicromapBuild(kCb, 2, kMicromapA);
    svc.OnQueueSubmit(kCb);
    svc.Optimize({});
    // Command 10 is not in the retained set, so that build is never replayed.
    svc.OptimizeMicromaps({kMicromapA}, {});
    Check(svc.GetMicromapOverwriteConflicts().empty(),
          "overwrite: an unretained consuming build is not a conflict");
  }
  {
    // A compaction reads the micromap without overwriting it, so the source's own content is
    // still the one the restore reproduces.
    RaytracingOptimizationService svc;
    svc.RecordMicromapBuild(kCb, 1, kMicromapA);
    svc.RecordAsBuildMicromapReads(kCb, 10, {kMicromapA});
    svc.RecordMicromapCopy(kCb, 2, kMicromapB, kMicromapA, VK_COPY_MICROMAP_MODE_COMPACT_EXT);
    svc.OnQueueSubmit(kCb);
    svc.Optimize({});
    svc.OptimizeMicromaps({kMicromapA, kMicromapB}, {10});
    Check(svc.GetMicromapOverwriteConflicts().empty(),
          "overwrite: compacting out of a micromap does not overwrite it");
  }
}

// Ops are staged per command buffer and only enter the graph at submit, so execution order
// follows the submits - not the order the command buffers were recorded in.
void TestSubmitOrderDrivesTheChain() {
  RaytracingOptimizationService svc;
  // Recorded second-command-buffer-first, submitted the other way round.
  svc.RecordMicromapCopy(kCb2, 2, kMicromapB, kMicromapA, VK_COPY_MICROMAP_MODE_COMPACT_EXT);
  svc.RecordMicromapBuild(kCb, 1, kMicromapA);
  svc.OnQueueSubmit(kCb);
  svc.OnQueueSubmit(kCb2);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapB}, {});

  const auto& chain = svc.GetOptimizedMicromapCommands();
  Check(chain.size() == 2, "submit order: both ops retained");
  if (chain.size() == 2) {
    Check(chain[0].CommandKey == 1 && chain[1].CommandKey == 2,
          "submit order: the build submitted first replays first");
    Check(chain[1].SourceCommandKey == 1,
          "submit order: the copy resolves the source the earlier submit produced");
  }
}

// Regression guard for the shared graph: the acceleration structure reduction must behave
// exactly as it did when it owned the implementation. Updates collapse to
// {root build, last update}, copies never collapse.
void TestAsUpdateRunCollapses() {
  RaytracingOptimizationService svc;
  svc.RecordBuild(kCb, /*commandKey=*/1, kBlasA, /*srcAsKey=*/0, /*isUpdate=*/false);
  svc.RecordBuild(kCb, 2, kBlasA, kBlasA, /*isUpdate=*/true);
  svc.RecordBuild(kCb, 3, kBlasA, kBlasA, /*isUpdate=*/true);
  svc.RecordBuild(kCb, 4, kBlasA, kBlasA, /*isUpdate=*/true);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({kBlasA});

  const auto& chain = svc.GetOptimizedCommands();
  Check(chain.size() == 2, "AS updates: a run collapses to root build plus last update");
  if (chain.size() == 2) {
    Check(chain[0].CommandKey == 1, "AS updates: the root build survives");
    Check(chain[1].CommandKey == 4, "AS updates: the last update survives");
    Check(chain[1].SourceCommandKey == 1,
          "AS updates: the last update is repointed at the root build");
  }
}

void TestAsCompactionChainRetained() {
  RaytracingOptimizationService svc;
  svc.RecordBuild(kCb, 1, kBlasA, 0, false);
  svc.RecordCopy(kCb, 2, kBlasB, kBlasA, VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({kBlasB});

  const auto& chain = svc.GetOptimizedCommands();
  Check(chain.size() == 2, "AS compaction: both ops retained");
  if (chain.size() == 2) {
    Check(chain[1].IsCopy && chain[1].SrcAsKey == kBlasA && chain[1].SourceCommandKey == 1,
          "AS compaction: the copy is linked to its source's build");
    Check(chain[1].CopyMode == VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR,
          "AS compaction: the copy mode round-trips through the untyped graph");
  }
}

// A secondary command buffer's ops belong to the primary that executes it, so they must flush
// when the primary is submitted - the secondary is never submitted itself.
void TestSecondaryCommandBufferMerge() {
  RaytracingOptimizationService svc;
  svc.RecordMicromapBuild(kCb2, 1, kMicromapA);
  svc.MergeSecondary(kCb, kCb2);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({});
  svc.OptimizeMicromaps({kMicromapA}, {});

  Check(svc.GetOptimizedMicromapCommands().size() == 1,
        "secondary: a merged op flushes with the primary");
}

// The two graphs must not see each other's ops even though object keys share one space.
void TestGraphsAreIndependent() {
  RaytracingOptimizationService svc;
  svc.RecordBuild(kCb, 1, kBlasA, 0, false);
  svc.RecordMicromapBuild(kCb, 2, kMicromapA);
  svc.OnQueueSubmit(kCb);

  svc.Optimize({kBlasA});
  svc.OptimizeMicromaps({kMicromapA}, {});

  Check(svc.GetOptimizedCommands().size() == 1, "independence: the AS graph holds only its op");
  Check(svc.GetOptimizedMicromapCommands().size() == 1,
        "independence: the micromap graph holds only its op");
  if (!svc.GetOptimizedCommands().empty()) {
    Check(svc.GetOptimizedCommands()[0].CommandKey == 1, "independence: AS op identity");
  }
  if (!svc.GetOptimizedMicromapCommands().empty()) {
    Check(svc.GetOptimizedMicromapCommands()[0].CommandKey == 2,
          "independence: micromap op identity");
  }
}

} // namespace

int main() {
  TestMicromapCompactionChainRetained();
  TestUnusedMicromapChainPruned();
  TestMicromapRebuildSupersedes();
  TestMicromapOverwriteAfterConsumingBuildConflicts();
  TestMicromapOverwriteFalsePositives();
  TestSubmitOrderDrivesTheChain();
  TestAsUpdateRunCollapses();
  TestAsCompactionChainRetained();
  TestSecondaryCommandBufferMerge();
  TestGraphsAreIndependent();

  if (g_Failures != 0) {
    std::printf("chainReductionTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("chainReductionTest: all checks passed\n");
  return 0;
}
