// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "captureManager.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "mapTrackingService.h"
#include "openglDriversAuto.h"
#include "recursionGuard.h"

#include <cstring>
#include <mutex>
#include <optional>
#include <unordered_set>
#include <vector>

// Hand-declared instead of including <windows.h> - see recorderWrappersAuto.cpp.mako's
// identical GetCurrentThreadId precedent (that header comment explains why this backend
// avoids <windows.h> in any translation unit that also needs glTypes.h's own declarations).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace gits::opengl2 {

namespace {

// GL enum values used by this file, hand-inlined rather than including <GL/gl.h> - same
// convention as hdcVirtualizationService.cpp's PFD_*/<wingdi.h> constants, values confirmed
// against third_party/OpenGL-Registry/xml/gl.xml (see the plan doc's "Sources consulted").
// Only the buffer targets this backend's rendering-pipeline scope plausibly maps are covered -
// see docs/development/opengl2-buffer-map-tracking-plan.md's Decision 4.
constexpr GLenum kArrayBuffer = 0x8892;
constexpr GLenum kArrayBufferBinding = 0x8894;
constexpr GLenum kElementArrayBuffer = 0x8893;
constexpr GLenum kElementArrayBufferBinding = 0x8895;
constexpr GLenum kPixelPackBuffer = 0x88EB;
constexpr GLenum kPixelPackBufferBinding = 0x88ED;
constexpr GLenum kPixelUnpackBuffer = 0x88EC;
constexpr GLenum kPixelUnpackBufferBinding = 0x88EF;
constexpr GLenum kUniformBuffer = 0x8A11;
constexpr GLenum kUniformBufferBinding = 0x8A28;
constexpr GLenum kTransformFeedbackBuffer = 0x8C8E;
constexpr GLenum kTransformFeedbackBufferBinding = 0x8C8F;
constexpr GLenum kCopyReadBuffer = 0x8F36;
constexpr GLenum kCopyReadBufferBinding = 0x8F36; // Same value as the target itself - a
                                                  // real gl.xml quirk, not a typo.
constexpr GLenum kCopyWriteBuffer = 0x8F37;
constexpr GLenum kCopyWriteBufferBinding = 0x8F37; // Same quirk as GL_COPY_READ_BUFFER above.
constexpr GLenum kDrawIndirectBuffer = 0x8F3F;
constexpr GLenum kDrawIndirectBufferBinding = 0x8F43;
constexpr GLenum kAtomicCounterBuffer = 0x92C0;
constexpr GLenum kAtomicCounterBufferBinding = 0x92C1;
constexpr GLenum kDispatchIndirectBuffer = 0x90EE;
constexpr GLenum kDispatchIndirectBufferBinding = 0x90EF;
constexpr GLenum kShaderStorageBuffer = 0x90D2;
constexpr GLenum kShaderStorageBufferBinding = 0x90D3;
constexpr GLenum kQueryBuffer = 0x9192;
constexpr GLenum kQueryBufferBinding = 0x9193;

constexpr GLbitfield kMapReadBit = 0x0001;
constexpr GLbitfield kMapWriteBit = 0x0002;

// glUnmapBuffer's own GL_TRUE/GL_FALSE return value: GL_FALSE means the data store's contents
// became corrupted/undefined for implementation-specific reasons (e.g. a screen-mode change on
// some platforms) - the spec does not require
// this to reflect what the application itself actually wrote, so a real driver reading back
// through a temporary re-map still succeeds (no crash, no invalid memory), but the bytes read
// back are meaningless. CustomWrapper_glUnmapBuffer must not record those as a valid
// MappedDataMetaCommand snapshot.
constexpr GLboolean kGlTrue = 1;

// See Decision 4: the driver, not a hand-rolled binding tracker, is the source of truth for
// which buffer is currently bound to `target`. Returns 0 for a target this plan does not track
// (see Decision 1's scope) - the caller treats that identically to "recognized target, nothing
// bound".
GLenum BindingPnameForTarget(GLenum target) {
  switch (target) {
  case kArrayBuffer:
    return kArrayBufferBinding;
  case kElementArrayBuffer:
    return kElementArrayBufferBinding;
  case kPixelPackBuffer:
    return kPixelPackBufferBinding;
  case kPixelUnpackBuffer:
    return kPixelUnpackBufferBinding;
  case kUniformBuffer:
    return kUniformBufferBinding;
  case kTransformFeedbackBuffer:
    return kTransformFeedbackBufferBinding;
  case kCopyReadBuffer:
    return kCopyReadBufferBinding;
  case kCopyWriteBuffer:
    return kCopyWriteBufferBinding;
  case kDrawIndirectBuffer:
    return kDrawIndirectBufferBinding;
  case kAtomicCounterBuffer:
    return kAtomicCounterBufferBinding;
  case kDispatchIndirectBuffer:
    return kDispatchIndirectBufferBinding;
  case kShaderStorageBuffer:
    return kShaderStorageBufferBinding;
  case kQueryBuffer:
    return kQueryBufferBinding;
  default:
    return 0;
  }
}

// Warn-once state for the two "cannot resolve a buffer for this target" cases Decision 4's
// Consequences names (unrecognized target, recognized target with nothing bound) - low
// frequency path (map/unmap, not per-draw-call), so a plain mutex is fine, mirroring
// ObjectNameMapService's own warn-once convention.
std::mutex g_WarnedTargetsMutex;
std::unordered_set<GLenum> g_WarnedUnrecognizedTargets;
std::unordered_set<GLenum> g_WarnedNoBufferBound;

// Resolves the real, live GLuint currently bound to `target` via glGetIntegerv - see
// Decision 4. Returns 0 (with a single warning per unique target) if `target` is not one of
// the buffer targets this plan tracks, or if the query succeeds but nothing is bound there.
GLuint ResolveBoundBuffer(GLenum target) {
  const GLenum pname = BindingPnameForTarget(target);
  if (pname == 0) {
    std::lock_guard<std::mutex> lock(g_WarnedTargetsMutex);
    if (g_WarnedUnrecognizedTargets.insert(target).second) {
      LOG_WARNING << "CustomWrapper_glMapBufferRange/glUnmapBuffer: target " << target
                  << " is not a buffer target this plan resolves (see "
                     "opengl2-buffer-map-tracking-plan.md's Decision 4) - buffer-map "
                     "tracking skipped for this target";
    }
    return 0;
  }
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(pname, &value);
  const GLuint buffer = static_cast<GLuint>(value);
  if (buffer == 0) {
    std::lock_guard<std::mutex> lock(g_WarnedTargetsMutex);
    if (g_WarnedNoBufferBound.insert(target).second) {
      LOG_WARNING << "CustomWrapper_glMapBufferRange/glUnmapBuffer: target " << target
                  << " has no buffer bound - buffer-map tracking skipped for this target";
    }
  }
  return buffer;
}

// Builds and records the MappedDataMetaCommand for a just-captured snapshot (see Decision 6).
// `key` must already be reserved by the caller via CaptureManager::CreateCommandKey(), before
// the real glUnmapBuffer dispatch - see CustomWrapper_glUnmapBuffer's own comment on why (mirrors
// recorderWrappersAuto.cpp.mako's identical pre-dispatch reservation for every generated
// wrapper). `bytes.size()` is passed straight through to ArrayArgument<uint8_t>'s own templated,
// size_t-based constructor with no narrowing cast here - a mapped range can legitimately
// approach or exceed 4 GiB on this x64-only backend (see mappedDataMetaTypes.h's own comment on
// why MappedDataMetaCommand's coding path is 64-bit throughout); narrowing to uint32_t here,
// before ever reaching that already-64-bit-safe coding path, would silently reintroduce the
// exact truncation bug that path was fixed to avoid.
void RecordMappedDataMetaCommand(uint64_t key,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr length,
                                 const std::vector<uint8_t>& bytes) {
  MappedDataMetaCommand command(static_cast<uint32_t>(GetCurrentThreadId()));
  command.m_Buffer.Value = buffer;
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  command.m_Offset.Value = static_cast<uint64_t>(offset);
  command.m_Length.Value = static_cast<uint64_t>(length);
  command.m_Data = ArrayArgument<uint8_t>(bytes.data(), bytes.size());
  command.m_Key = key;
  CaptureManager::Get().RecordCommand(command, new MappedDataMetaSerializer(command));
}

void* APIENTRY CustomWrapper_glMapBufferRange(GLenum target,
                                              GLintptr offset,
                                              GLsizeiptr length,
                                              GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* result = realFunction(target, offset, length, access);
  if (record && result != nullptr) {
    const GLuint buffer = ResolveBoundBuffer(target);
    if (buffer != 0) {
      MapTrackingService::Get().StoreData(CurrentShareGroup(), buffer,
                                          MappedRange{result, offset, length, access});
    }
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapBuffer(GLenum target) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
  if (!record) {
    return realFunction(target);
  }
  const GLuint buffer = ResolveBoundBuffer(target);
  std::optional<MappedRange> range =
      buffer != 0 ? MapTrackingService::Get().TakeData(CurrentShareGroup(), buffer) : std::nullopt;
  // Nothing tracked (map wasn't via glMapBufferRange, buffer unresolved, ...), or a
  // genuinely read-only mapping - see Decision 5's third bullet. Skip straight to the real
  // unmap; no MappedDataMetaCommand, so no ordering key is reserved for this call at all.
  if (!range || (range->Access & kMapWriteBit) == 0) {
    return realFunction(target);
  }
  // Reserved here - before the real unmap dispatch below, for both the read-mapping and
  // write-only branches - rather than inside RecordMappedDataMetaCommand() after the fact, for
  // the same reason every generated wrapper reserves its own key before dispatch (see
  // recorderWrappersAuto.cpp.mako's header comment): a concurrent top-level call on another
  // thread must not be able to race this one's key assignment based on which driver call
  // happens to return first. Every path below that reaches this point (i.e. a
  // MappedDataMetaCommand is genuinely intended) must either record a command with this key
  // (RecordMappedDataMetaCommand) or explicitly release it via CaptureManager::SkipCommandKey -
  // never just drop it, or OrderingRecorder::CheckPendingSerializers() would stall forever
  // waiting for a key that will never arrive.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  std::vector<uint8_t> bytes(static_cast<size_t>(range->Length));
  if (range->Access & kMapReadBit) {
    // Safe to read directly through the still-live original pointer, right before the real
    // unmap invalidates it - see Decision 5's first bullet.
    std::memcpy(bytes.data(), range->Pointer, bytes.size());
    const GLboolean result = realFunction(target);
    if (result == kGlTrue) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE: the data store's contents are corrupted/undefined per spec (see kGlTrue's
      // own comment) - `bytes` was captured from the pointer that was live a moment ago, but
      // that snapshot no longer reflects anything meaningful now that the real unmap reports
      // failure. Recording it anyway would have replay inject bytes capture itself never
      // validly established.
      LOG_WARNING << "CustomWrapper_glUnmapBuffer: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      // `key` was already reserved above - it must still be released even though nothing will
      // ever be recorded under it, or OrderingRecorder would wait for it forever (see this
      // function's own comment above `key`'s declaration).
      CaptureManager::Get().SkipCommandKey(key);
    }
    return result;
  }
  // Write-only (or write-plus-invalidate): the original pointer is not guaranteed readable per
  // spec - see Decision 5's second bullet. Real unmap first (the one, app-visible unmap - its
  // result is what the app's own call returns), then a temporary read-only remap/memcpy/unmap
  // purely for capture, mirroring CgitsUnmapBuffer's own fallback.
  const GLboolean result = realFunction(target);
  auto realMapBufferRange = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* readPointer = realMapBufferRange(target, range->Offset, range->Length, kMapReadBit);
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    realFunction(target); // Second, purely internal unmap - its result is not the app's.
    if (result == kGlTrue) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE from the app's own (first) unmap above: the data store's contents are
      // corrupted/undefined per spec, even though this temporary re-map/memcpy itself
      // succeeded (reading back through valid, driver-owned memory whose *contents* are
      // simply meaningless now, not invalid memory - see kGlTrue's own comment). Recording it
      // anyway would have replay inject bytes capture never validly established.
      LOG_WARNING << "CustomWrapper_glUnmapBuffer: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
  } else {
    LOG_WARNING << "CustomWrapper_glUnmapBuffer: read-only re-map of buffer " << buffer
                << " failed - no MappedDataMetaCommand recorded for this unmap";
    // `key` was already reserved above - it must still be released even though nothing will
    // ever be recorded under it, or OrderingRecorder would wait for it forever (see this
    // function's own comment above `key`'s declaration).
    CaptureManager::Get().SkipCommandKey(key);
  }
  return result;
}

} // namespace

void* GetCustomRecorderWrapper(const char* name) {
  if (std::strcmp(name, "glMapBufferRange") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBufferRange);
  }
  if (std::strcmp(name, "glUnmapBuffer") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapBuffer);
  }
  return nullptr;
}

} // namespace gits::opengl2
