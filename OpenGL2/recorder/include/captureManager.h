// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "layer.h"
#include "orderingRecorder.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace gits::opengl2 {

// Owns the single OpenGL2 capture pipeline: the atomic ordering-key source, the
// backend-local OrderingRecorder instance (the common ordering service, see
// modern-opengl-backend-architecture.md "Establish one total capture order"), and the
// registered Pre/Post layer lists with the encoder always last in Post order (not yet
// populated; there are no enabled commands and therefore no encoder layer yet).
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

  uint64_t CreateCommandKey() {
    return m_CommandKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  // Independent counter from CreateCommandKey() above - mirrors Vulkan's CaptureManager,
  // which also keeps command ordering keys and object handle keys in separate sequences.
  // Used by ContextHandleMapService (see contextHandleMapService.h) to mint a GITSKey for
  // a newly-created HGLRC.
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
  // rationale and mapTrackingWrappers.cpp's CustomWrapper_glUnmapBuffer for a hand-written
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
  // mapTrackingWrappers.cpp's CustomWrapper_glUnmapBuffer). Every reserved key MUST eventually
  // reach either RecordCommand/RecordUnsupportedCommand or this method - OrderingRecorder's own
  // CheckPendingSerializers() waits for keys strictly in sequence, so a key that is reserved and
  // then silently dropped (neither recorded nor skipped) would permanently stall every
  // higher-keyed command from ever reaching the stream.
  void SkipCommandKey(uint64_t key);

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  CaptureManager() = default;
  stream::OrderingRecorder& GetOrderingRecorder();

  std::once_flag m_RecorderInitFlag;
  std::unique_ptr<stream::OrderingRecorder> m_Recorder;
  std::atomic<uint64_t> m_CommandKey{0};
  std::atomic<GITSKey> m_HandleKey{0};
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
};

} // namespace gits::opengl2
