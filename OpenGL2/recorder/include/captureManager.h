// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "captureLayerManager.h"
#include "command.h"
#include "log.h"
#include "orderingRecorder.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace gits::opengl2 {

// Owns the single OpenGL2 capture pipeline: the atomic ordering-key source, the
// backend-local OrderingRecorder instance (the common ordering service, see
// modern-opengl-backend-architecture.md "Establish one total capture order"), and the
// CaptureLayerManager with the registered Pre/Post layer lists.
class CaptureManager {
public:
  // Heap-allocated and intentionally never destroyed via a C++ static destructor,
  // matching Vulkan's CaptureManager (see Vulkan/recorder/captureManager.cpp): our
  // OrderingRecorder outlives us and publishes to MessageBus (a separate Meyer's
  // singleton) from its own destructor, and C++ does not guarantee construction/
  // destruction order across independent singletons in different translation units.
  // Real shutdown instead happens while the process is still fully alive, via the
  // shared ExitProcess/TerminateProcess hooks in recorder/recorderDLL.cpp publishing
  // TOPIC_PROGRAM_EXIT (OrderingRecorder::Close() is idempotent, so if this object's
  // destructor never runs at all, nothing is lost).
  static CaptureManager& Get();
  CaptureManager(const CaptureManager&) = delete;
  CaptureManager& operator=(const CaptureManager&) = delete;

  // Every key this hands out MUST reach RecordCommand(), RecordUnsupportedCommand(), or
  // SkipCommandKey() - see SkipCommandKey() for why dropping one stalls the stream forever.
  [[nodiscard]] uint64_t CreateCommandKey() {
    return m_CommandKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  // Atomically reserves TWO consecutive, strictly-increasing keys as one indivisible unit - a
  // single fetch_add(2), not two separate CreateCommandKey() calls - so no OTHER thread's own
  // CreateCommandKey()/CreateCommandKeyPair() call can ever observe (or be assigned) a key
  // value that falls between the pair returned here; the pair is exclusively owned by this
  // call's caller from the instant fetch_add returns. Returns {first, first + 1}.
  //
  // Exists specifically so a wrapper that must reserve its OWN command's ordering key AND a
  // second, synthetic command's key that has to sort strictly BEFORE it (e.g.
  // recorderWrappersAuto.cpp.mako's PRE_HOOK_KEY_PAIR_COMMANDS handling for
  // WindowGeometryTrackingLayer - see that table's own header comment) can do so at THE SAME
  // wrapper-entry point a single CreateCommandKey() call would otherwise occupy - preserving
  // this command's own relative dispatch-order position versus every OTHER, independently-
  // recording thread exactly as if only one key had been reserved here (see CreateCommandKey's
  // own header comment on RecordCommand for why that position matters), while *also*
  // guaranteeing the earlier of the two keys is available, up front, for the synthetic command
  // - with no possibility of a third party's key landing between them. Deliberately NOT the
  // same shape as UpdateCommandKey() used to be (see git history: SkipCommandKey() followed by
  // a separate, later CreateCommandKey() call) - that pattern reserves the "later" key only
  // AFTER the original single key was already fixed relative to every concurrent thread, so a
  // thread that raced in in-between could end up sandwiched between the two, silently
  // reordering this command relative to one it had originally out-raced. Reserving both keys
  // together, up front, before either is ever exposed to another thread, has no such window.
  std::pair<uint64_t, uint64_t> CreateCommandKeyPair() {
    const uint64_t first = m_CommandKey.fetch_add(2, std::memory_order_relaxed) + 1;
    return {first, first + 1};
  }

  // CreateCommandKeyPair() for `count` keys: reserves {first, ..., first + count - 1} atomically
  // and returns `first`.
  [[nodiscard]] uint64_t CreateCommandKeyRange(uint64_t count) {
    return m_CommandKey.fetch_add(count, std::memory_order_relaxed) + 1;
  }

  // CreateCommandKeyRange() for a command that can end a frame, whose ordering key is the last
  // key of the range. The ordering key stays pending until FrameEndPreLayersRan(), and is made
  // pending in the same step that reserves it, so a layer numbering frame ends in key order can
  // tell whether a frame end with a lower key has not reached it yet.
  [[nodiscard]] uint64_t CreateFrameEndKeyRange(uint64_t count) {
    const std::lock_guard<std::mutex> lock(m_FrameEndMutex);
    const uint64_t first = CreateCommandKeyRange(count);
    m_PendingFrameEnds.insert(first + count - 1);
    return first;
  }
  void FrameEndPreLayersRan(uint64_t key) {
    const std::lock_guard<std::mutex> lock(m_FrameEndMutex);
    m_PendingFrameEnds.erase(key);
  }
  // How many frame ends with a key lower than `key` have not run their Pre layers yet.
  size_t FrameEndsPendingBefore(uint64_t key) {
    const std::lock_guard<std::mutex> lock(m_FrameEndMutex);
    return static_cast<size_t>(
        std::distance(m_PendingFrameEnds.begin(), m_PendingFrameEnds.lower_bound(key)));
  }

  // Independent counter from CreateCommandKey() above - mirrors Vulkan's CaptureManager,
  // which also keeps command ordering keys and object handle keys in separate sequences.
  // Used by HandleMapService<HandleT> (see common/services/handleMapService.h) to mint a
  // GITSKey for a newly-created mapped handle (HGLRC, GLsync, ...).
  GITSKey CreateHandleKey() {
    return m_HandleKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  // Generic recording path for any typed Command (enabled commands, see
  // OpenGL2/common/layer_interface/commandsAuto.h): hands the command's already-built
  // serializer to OrderingRecorder under its ordering key. Takes ownership of
  // `serializer` (matches stream::OrderingRecorder::Record()).
  //
  // Callers MUST set `command.m_Key = CreateCommandKey()` themselves before constructing
  // `serializer` - by the time this function runs, `serializer` (a function argument) has
  // already captured/encoded `command`'s fields, including m_Key, so assigning it here
  // would be too late to affect the encoded bytes (only the ordering key passed to
  // OrderingRecorder::Record would be correct, not the payload). Mirrors Vulkan's wrapper
  // codegen, which assigns `command.m_Key` immediately after Pre layers run and well
  // before the Post-layer encoder constructs its Serializer.
  //
  // `CreateCommandKey()` itself should be called even earlier than that, in fact - at wrapper
  // entry, before the real driver dispatch, not merely before `serializer` - so concurrent
  // top-level calls on different threads get keys in dispatch order rather than driver-call-
  // completion order (see recorderWrappersAuto.cpp.mako's own header comment for the full
  // rationale and mapTrackingService.cpp's UnmapBufferAndCapture for a hand-written
  // example). This function only enforces the (necessary but not sufficient) "before
  // `serializer`" half of that ordering requirement.
  void RecordCommand(Command& command, stream::CommandSerializer* serializer);

  // Called once per top-level (non-recursive) call to a disabled command, for every
  // occurrence. See modern-opengl-backend-architecture.md, "Use a new stream namespace
  // and final schemas only": every disabled-command occurrence emits an
  // UnsupportedCommand, independent of the once-per-name warning in
  // recorder/unsupportedCommand.h.
  //
  // `key` must already be reserved by the caller via CreateCommandKey() - called at wrapper
  // entry, before the real driver dispatch, exactly like an enabled command's own
  // `command.m_Key` (see RecordCommand's own comment above) - rather than minted here, after
  // the fact: unlike an enabled command (whose Command object already exists pre-dispatch to
  // eager-copy IN-direction bytes, so assigning m_Key there costs nothing extra), a disabled
  // command has no such object, but still needs its key reserved at the same pre-dispatch
  // point for the same reason - otherwise concurrent top-level calls on different threads get
  // keys assigned in driver-call-COMPLETION order rather than dispatch order, and replay can
  // reorder them (see recorderWrappersAuto.cpp.mako's own
  // header comment, and Vulkan's wrappersAuto.cpp.mako, which reserves its key the same way).
  void RecordUnsupportedCommand(const char* name, uint64_t key, uint32_t threadId);

  // Releases a key reserved via CreateCommandKey() that will never be filled by RecordCommand/
  // RecordUnsupportedCommand - e.g. a wrapper that reserves a key before dispatch to preserve
  // ordering, then discovers post-dispatch that it has nothing to actually record (see
  // mapTrackingService.cpp's UnmapBufferAndCapture). Every reserved key MUST eventually
  // reach either RecordCommand/RecordUnsupportedCommand or this method - OrderingRecorder's own
  // CheckPendingSerializers() waits for keys strictly in sequence, so a key that is reserved and
  // then silently dropped (neither recorded nor skipped) would permanently stall every
  // higher-keyed command from ever reaching the stream.
  void SkipCommandKey(uint64_t key);

  // Thread-local handoff from a generated Wrapper_<cmd> (recorderWrappersAuto.cpp.mako's
  // PRE_HOOK_KEY_PAIR_COMMANDS handling) to the Pre layer that consumes the SPARE key of a
  // CreateCommandKeyPair() reservation - e.g. WindowGeometryTrackingLayer's hooked WGL
  // commands (see that layer's own header comment). thread_local, not a single
  // shared slot: the wrapper sets this immediately before entering the (whole-process-shared)
  // Pre layer loop, on the SAME thread that will consume it a few lines later, with no other
  // GITS code running on that thread in between - so there is no cross-thread interference to
  // guard against, and every call that sets this is matched by exactly one TakePendingPreHookKey()
  // call before the next one is ever set (enforced by the asserts below, not just documented).
  void SetPendingPreHookKey(uint64_t key) {
    GITS_ASSERT(s_PendingPreHookKey == 0);
    s_PendingPreHookKey = key;
  }

  // Consumes (reads AND clears) the key set by SetPendingPreHookKey() above - "clears" matters
  // just as much as "reads" here: it is what lets the next SetPendingPreHookKey() call's own
  // assert actually catch a real bug (two reservations on the same thread with no consumption
  // in between) instead of silently overwriting a still-pending key. Asserts the key was
  // actually set - a Pre() override that calls this MUST correspond 1:1 with a
  // PRE_HOOK_KEY_PAIR_COMMANDS entry that always calls SetPendingPreHookKey() first (see that
  // table's own header comment); 0 can otherwise never be a real key (CreateCommandKeyPair()'s
  // own m_CommandKey counter starts at 0 and only ever hands out fetch_add results plus 1, so
  // the smallest key it can ever produce is 1), so it doubles as a safe "never actually set"
  // sentinel.
  uint64_t TakePendingPreHookKey() {
    GITS_ASSERT(s_PendingPreHookKey != 0);
    const uint64_t key = s_PendingPreHookKey;
    s_PendingPreHookKey = 0;
    return key;
  }

  // Post-layer counterpart of SetPendingPreHookKey()/TakePendingPreHookKey(), for
  // generator_recorder.py's POST_HOOK_KEY_PAIR_COMMANDS: the wrapper keeps the FIRST key of the
  // pair as its own orderingKey and hands the second to the Post layer that may record a
  // metacommand replaying right after the hooked call. The layer records under the key only when
  // it has something to record, so the wrapper calls ReleasePendingPostHookKey() after its Post
  // loop to skip a key left unconsumed.
  void SetPendingPostHookKey(uint64_t key) {
    GITS_ASSERT(s_PendingPostHookKey == 0);
    s_PendingPostHookKey = key;
  }
  uint64_t TakePendingPostHookKey() {
    GITS_ASSERT(s_PendingPostHookKey != 0);
    const uint64_t key = s_PendingPostHookKey;
    s_PendingPostHookKey = 0;
    return key;
  }
  void ReleasePendingPostHookKey() {
    if (s_PendingPostHookKey != 0) {
      SkipCommandKey(s_PendingPostHookKey);
      s_PendingPostHookKey = 0;
    }
  }

  // Same handoff as SetPendingPreHookKey()/TakePendingPreHookKey(), for generator_recorder.py's
  // PRE_HOOK_EXTRA_KEY_COMMANDS: the swap's wrapper reserves this key ahead of its pre-hook
  // pair, for the mapped-texture update MappedTextureLayer records (or skips) before the swap.
  void SetPendingMappedTextureKey(uint64_t key) {
    GITS_ASSERT(s_PendingMappedTextureKey == 0);
    s_PendingMappedTextureKey = key;
  }
  uint64_t TakePendingMappedTextureKey() {
    GITS_ASSERT(s_PendingMappedTextureKey != 0);
    const uint64_t key = s_PendingMappedTextureKey;
    s_PendingMappedTextureKey = 0;
    return key;
  }
  // The same for wglSetPixelFormat's pixel format attributes, which PixelFormatCaptureLayer
  // records before the call.
  void SetPendingPixelFormatKey(uint64_t key) {
    GITS_ASSERT(s_PendingPixelFormatKey == 0);
    s_PendingPixelFormatKey = key;
  }
  uint64_t TakePendingPixelFormatKey() {
    GITS_ASSERT(s_PendingPixelFormatKey != 0);
    const uint64_t key = s_PendingPixelFormatKey;
    s_PendingPixelFormatKey = 0;
    return key;
  }
  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

private:
  CaptureManager();
  stream::OrderingRecorder& GetOrderingRecorder();

  std::once_flag m_RecorderInitFlag;
  std::unique_ptr<stream::OrderingRecorder> m_Recorder;
  std::atomic<uint64_t> m_CommandKey{0};
  std::atomic<GITSKey> m_HandleKey{0};
  std::mutex m_FrameEndMutex;
  std::set<uint64_t> m_PendingFrameEnds;
  CaptureLayerManager m_LayerManager;

  // See SetPendingPreHookKey()/TakePendingPreHookKey() above for the full contract. A plain
  // `static thread_local` data member (one instance per thread, not per CaptureManager
  // instance - there is only ever one CaptureManager instance anyway, see Get()) rather than a
  // non-static member, since C++ does not allow a non-static thread_local class member at all.
  static thread_local uint64_t s_PendingPreHookKey;
  static thread_local uint64_t s_PendingPostHookKey;
  static thread_local uint64_t s_PendingMappedTextureKey;
  static thread_local uint64_t s_PendingPixelFormatKey;
};

} // namespace gits::opengl2
