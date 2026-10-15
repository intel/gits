// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"

#include "captureManager.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "openglDriversAuto.h"

#include <cstring>
#include <vector>

// Hand-declared instead of including <windows.h> - see recorderWrappersAuto.cpp.mako's
// identical GetCurrentThreadId precedent (that header comment explains why this backend
// avoids <windows.h> in any translation unit that also needs glTypes.h's own declarations).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace gits::opengl2 {

namespace {

// The pname reporting which buffer is bound to `target`. Returns 0 for a target this service
// does not track - see the plan doc's Decision 1 for that scope.
GLenum BindingPnameForTarget(GLenum target) {
  switch (target) {
  case GL_ARRAY_BUFFER:
    return GL_ARRAY_BUFFER_BINDING;
  case GL_ELEMENT_ARRAY_BUFFER:
    return GL_ELEMENT_ARRAY_BUFFER_BINDING;
  case GL_PIXEL_PACK_BUFFER:
    return GL_PIXEL_PACK_BUFFER_BINDING;
  case GL_PIXEL_UNPACK_BUFFER:
    return GL_PIXEL_UNPACK_BUFFER_BINDING;
  case GL_UNIFORM_BUFFER:
    return GL_UNIFORM_BUFFER_BINDING;
  case GL_TRANSFORM_FEEDBACK_BUFFER:
    return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
  case GL_COPY_READ_BUFFER:
    return GL_COPY_READ_BUFFER_BINDING;
  case GL_COPY_WRITE_BUFFER:
    return GL_COPY_WRITE_BUFFER_BINDING;
  case GL_DRAW_INDIRECT_BUFFER:
    return GL_DRAW_INDIRECT_BUFFER_BINDING;
  case GL_ATOMIC_COUNTER_BUFFER:
    return GL_ATOMIC_COUNTER_BUFFER_BINDING;
  case GL_DISPATCH_INDIRECT_BUFFER:
    return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
  case GL_SHADER_STORAGE_BUFFER:
    return GL_SHADER_STORAGE_BUFFER_BINDING;
  case GL_QUERY_BUFFER:
    return GL_QUERY_BUFFER_BINDING;
  default:
    return 0;
  }
}

// Builds and records the snapshot's MappedDataMetaCommand - see the plan doc's Decision 6.
// `bytes.size()` reaches ArrayArgument<uint8_t>'s own size_t-based constructor with no narrowing
// cast: a mapped range can legitimately approach or exceed 4 GiB on this x64-only backend, and
// narrowing here would reintroduce the exact truncation bug MappedDataMetaCommand's already
// 64-bit coding path was fixed to avoid (see mappedDataMetaTypes.h).
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

} // namespace

MapTrackingService& MapTrackingService::Get() {
  // Meyer's singleton - same rationale as ContextHandleMapService::Get()/
  // ObjectNameMapService::Get(): no MessageBus/OrderingRecorder ownership here, so none of
  // CaptureManager's static-destruction-order hazards apply.
  static MapTrackingService instance;
  return instance;
}

void MapTrackingService::OnMapBufferRange(
    GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mappedPointer) {
  const GLuint buffer = ResolveBoundBuffer(target);
  if (buffer == 0) {
    return;
  }
  if (access & GL_MAP_COHERENT_BIT) {
    ReportUnsupportedCoherentMapping(buffer);
  }
  // TODO: Phase 2 should hand the application a GITS-owned shadow for write-only explicit-flush
  // maps so each flushed range can be captured before unmap without reading a write-only
  // pointer.
  StoreMappedRange(CurrentShareGroup(), buffer, MappedRange{mappedPointer, offset, length, access});
}

GLboolean MapTrackingService::UnmapBufferAndCapture(GLenum target) {
  auto realUnmapBuffer = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
  const GLuint buffer = ResolveBoundBuffer(target);
  std::optional<MappedRange> range =
      buffer != 0 ? TakeMappedRange(CurrentShareGroup(), buffer) : std::nullopt;
  // Nothing tracked (the mapping did not come through glMapBufferRange, the buffer did not
  // resolve, ...), or a genuinely read-only mapping - see Decision 5's third bullet. There is
  // nothing to snapshot, so no ordering key is reserved for this call at all.
  if (!range || (range->Access & GL_MAP_WRITE_BIT) == 0) {
    return realUnmapBuffer(target);
  }
  // Reserved before the real unmap dispatch below, for both branches, for the same reason every
  // generated wrapper reserves its own key before dispatch (see recorderWrappersAuto.cpp.mako's
  // header comment): a concurrent top-level call on another thread must not be able to race this
  // one's key assignment based on which driver call happens to return first. Every path from
  // here must either record a command under this key or release it via
  // CaptureManager::SkipCommandKey - silently dropping it would stall
  // OrderingRecorder::CheckPendingSerializers() forever, waiting for a key that never arrives.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  std::vector<uint8_t> bytes(static_cast<size_t>(range->Length));
  if (range->Access & GL_MAP_READ_BIT) {
    // Safe to read directly through the still-live original pointer, right before the real
    // unmap invalidates it - see Decision 5's first bullet.
    std::memcpy(bytes.data(), range->Pointer, bytes.size());
    const GLboolean result = realUnmapBuffer(target);
    if (result == GL_TRUE) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE means the data store became corrupted and its contents are undefined for an
      // implementation-specific reason. `bytes` was copied while the pointer was still valid,
      // but it no longer represents defined buffer contents after the failed unmap. Since this
      // data is invalid, we should not record it.
      LOG_WARNING << "MapTrackingService: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
    return result;
  }
  // Write-only (or write-plus-invalidate): the original pointer is not guaranteed readable per
  // spec - see Decision 5's second bullet. Real unmap first (the one, app-visible unmap - its
  // result is what the app's own call returns), then a temporary read-only remap/memcpy/unmap
  // purely for capture, mirroring CgitsUnmapBuffer's own fallback.
  const GLboolean result = realUnmapBuffer(target);
  // TODO: Phase 1 storage tracking should avoid a knowingly invalid read remap for immutable
  // storage without GL_MAP_READ_BIT and use glGetBufferSubData as the correctness fallback.
  auto realMapBufferRange = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* readPointer = realMapBufferRange(target, range->Offset, range->Length, GL_MAP_READ_BIT);
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    realUnmapBuffer(target); // Second, purely internal unmap - its result is not the app's.
    if (result == GL_TRUE) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE means the data store became corrupted and its contents are undefined for an
      // implementation-specific reason. Do not read or record those undefined contents.
      LOG_WARNING << "MapTrackingService: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
  } else {
    LOG_WARNING << "MapTrackingService: read-only re-map of buffer " << buffer
                << " failed - no MappedDataMetaCommand recorded for this unmap";
    CaptureManager::Get().SkipCommandKey(key);
  }
  return result;
}

GLuint MapTrackingService::ResolveBoundBuffer(GLenum target) {
  const GLenum pname = BindingPnameForTarget(target);
  if (pname == 0) {
    std::lock_guard<std::mutex> lock(m_WarnedTargetsMutex);
    if (m_WarnedUnrecognizedTargets.insert(target).second) {
      LOG_WARNING << "MapTrackingService: target " << target
                  << " is not a buffer target this service resolves (see "
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
    std::lock_guard<std::mutex> lock(m_WarnedTargetsMutex);
    if (m_WarnedNoBufferBound.insert(target).second) {
      LOG_WARNING << "MapTrackingService: target " << target
                  << " has no buffer bound - buffer-map tracking skipped for this target";
    }
  }
  return buffer;
}

void MapTrackingService::ReportUnsupportedCoherentMapping(GLuint buffer) {
  std::call_once(m_WarnedCoherentMapping, [buffer] {
    LOG_ERROR << "MapTrackingService: application created a persistent coherent mapping for "
                 "buffer "
              << buffer
              << ". Capturing coherent mapped-memory updates "
                 "is not supported yet; the recorded stream is expected to replay incorrectly.";
  });
}

void MapTrackingService::StoreMappedRange(uint64_t shareGroup,
                                          GLuint buffer,
                                          const MappedRange& range) {
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  m_MappedBuffers[key] = range;
}

std::optional<MapTrackingService::MappedRange> MapTrackingService::TakeMappedRange(
    uint64_t shareGroup, GLuint buffer) {
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_MappedBuffersMutex);
  auto it = m_MappedBuffers.find(key);
  if (it == m_MappedBuffers.end()) {
    return std::nullopt;
  }
  MappedRange range = it->second;
  m_MappedBuffers.erase(it);
  return range;
}

} // namespace gits::opengl2
