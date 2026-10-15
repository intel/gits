// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"

#include "bufferShadowService.h"
#include "captureManager.h"
#include "clientMemoryCapture.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "openglDriversAuto.h"

#include <cstring>
#include <functional>
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
                                 uint64_t shareGroup,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr length,
                                 const std::vector<uint8_t>& bytes) {
  BufferShadowService::Get().OnMappedBytes(shareGroup, buffer, static_cast<uint64_t>(offset),
                                           bytes);
  MappedDataMetaCommand command(static_cast<uint32_t>(GetCurrentThreadId()));
  command.m_Buffer.Value = buffer;
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  command.m_Offset.Value = static_cast<uint64_t>(offset);
  command.m_Length.Value = static_cast<uint64_t>(length);
  command.m_Data = ArrayArgument<uint8_t>(bytes.data(), bytes.size());
  command.m_Key = key;
  CaptureManager::Get().RecordCommand(command, new MappedDataMetaSerializer(command));
}

// Read-only remap of [offset, offset + length) of the buffer bound to `target`, for capture only.
// If the range entry point is unavailable, map the whole buffer through glMapBuffer or its ARB
// variant. A supported range map that fails must instead use subdata readback; another map would
// also fail when immutable storage disallows reading. The matching unmap is the caller's.
void* MapRangeForCaptureRead(GLenum target, GLintptr offset, GLsizeiptr length) {
  auto mapRange = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  if (mapRange != nullptr) {
    void* pointer = mapRange(target, offset, length, GL_MAP_READ_BIT);
    if (pointer != nullptr || g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire) !=
                                  &ResolveAndCall_glMapBufferRange) {
      return pointer;
    }
  }
  auto map = g_DispatchTable.glMapBuffer.load(std::memory_order_acquire);
  void* whole = map != nullptr ? map(target, GL_READ_ONLY) : nullptr;
  if (whole == nullptr &&
      g_DispatchTable.glMapBuffer.load(std::memory_order_acquire) == &ResolveAndCall_glMapBuffer) {
    auto mapARB = g_DispatchTable.glMapBufferARB.load(std::memory_order_acquire);
    whole = mapARB != nullptr ? mapARB(target, GL_READ_ONLY) : nullptr;
  }
  return whole != nullptr ? static_cast<uint8_t*>(whole) + offset : nullptr;
}

// Void dispatch thunks return without writing data if their real getter cannot be resolved.
bool ReadBufferSubData(GLenum target, GLintptr offset, GLsizeiptr length, void* data) {
  g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire)(target, offset, length, data);
  return g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire) !=
         &ResolveAndCall_glGetBufferSubData;
}

bool ReadNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length, void* data) {
  g_DispatchTable.glGetNamedBufferSubData.load(std::memory_order_acquire)(buffer, offset, length,
                                                                          data);
  return g_DispatchTable.glGetNamedBufferSubData.load(std::memory_order_acquire) !=
         &ResolveAndCall_glGetNamedBufferSubData;
}

bool ReadNamedBufferSubDataEXT(GLuint buffer, GLintptr offset, GLsizeiptr length, void* data) {
  g_DispatchTable.glGetNamedBufferSubDataEXT.load(std::memory_order_acquire)(buffer, offset, length,
                                                                             data);
  return g_DispatchTable.glGetNamedBufferSubDataEXT.load(std::memory_order_acquire) !=
         &ResolveAndCall_glGetNamedBufferSubDataEXT;
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
  return SnapshotBufferAndUnmap(
      "glUnmapBuffer", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapBuffer, target] { return realUnmapBuffer(target); },
      /*realReadOnlyRemap=*/
      [target](GLintptr offset, GLsizeiptr length) {
        return MapRangeForCaptureRead(target, offset, length);
      },
      /*realInternalUnmap=*/[realUnmapBuffer, target] { return realUnmapBuffer(target); },
      /*realReadSubData=*/
      [target](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadBufferSubData(target, offset, length, data);
      });
}

// --- Extensions beyond the original glMapBufferRange/glUnmapBuffer pair ---------------------
//
// See this class's own header comment on each of the following for what real target-application
// gap each responds to. `SnapshotBufferAndUnmap` factors out the shared, delicate "read back and
// decide whether to keep the snapshot" logic (the same branching UnmapBufferAndCapture above
// hand-writes for glUnmapBuffer) so that logic is written and reviewed exactly once regardless
// of how many additional *unmap* entry points end up calling it.

GLbitfield MapTrackingService::TranslateWholeBufferAccessToBits(GLenum access) {
  switch (access) {
  case GL_READ_ONLY:
    return GL_MAP_READ_BIT;
  case GL_WRITE_ONLY:
    return GL_MAP_WRITE_BIT;
  case GL_READ_WRITE:
    return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
  default:
    return 0;
  }
}

void MapTrackingService::StoreWholeBufferMap(
    uint64_t shareGroup, GLuint buffer, GLenum access, void* mappedPointer, GLsizeiptr length) {
  StoreMappedRange(
      shareGroup, buffer,
      MappedRange{mappedPointer, /*offset=*/0, length, TranslateWholeBufferAccessToBits(access)});
}

void MapTrackingService::OnMapBuffer(GLenum target, GLenum access, void* mappedPointer) {
  if (mappedPointer == nullptr) {
    return;
  }
  const GLuint buffer = ResolveBoundBuffer(target);
  if (buffer == 0) {
    return;
  }
  // Neither glMapBuffer nor glMapBufferARB takes an explicit length (both map the entire
  // buffer), so it is not otherwise available at this call site - queried explicitly here, the
  // same "ask the driver, don't hand-roll tracking" precedent Decision 4 already established
  // for the bound-buffer lookup itself, applied to a second piece of state this function's own
  // parameter list simply does not carry.
  GLint64 size = 0;
  if (!GetBoundBufferSize(target, size) || size <= 0) {
    LOG_WARNING << "MapTrackingService: GL_BUFFER_SIZE of buffer " << buffer
                << " could not be queried - buffer-map tracking skipped for this map";
    return;
  }
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void MapTrackingService::OnMapNamedBuffer(GLuint buffer, GLenum access, void* mappedPointer) {
  if (mappedPointer == nullptr || buffer == 0) {
    return;
  }
  GLint64 size = 0;
  g_DispatchTable.glGetNamedBufferParameteri64v.load(std::memory_order_acquire)(
      buffer, GL_BUFFER_SIZE, &size);
  if (size <= 0) {
    LOG_WARNING << "MapTrackingService: GL_BUFFER_SIZE of buffer " << buffer
                << " could not be queried - buffer-map tracking skipped for this map";
    return;
  }
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void MapTrackingService::OnMapNamedBufferEXT(GLuint buffer, GLenum access, void* mappedPointer) {
  if (mappedPointer == nullptr || buffer == 0) {
    return;
  }
  // Like OnMapBuffer above, glMapNamedBufferEXT maps the entire buffer implicitly - its length
  // is queried the same way, but via the DSA-EXT getter (glGetNamedBufferParameterivEXT):
  // this backend has only been observed capturing the EXT-suffixed DSA family, not core
  // (GL 4.5) DSA, so the EXT-suffixed getter is used for consistency rather than assuming core
  // DSA support also exists. The EXT family has no 64-bit getter, so a GL 4.5 context's core
  // glGetNamedBufferParameteri64v is used when available, since GL_BUFFER_SIZE is a GLsizeiptr.
  GLint64 size = 0;
  const auto [major, minor] = CurrentContextGLVersion();
  if (major > 4 || (major == 4 && minor >= 5)) {
    auto realGetNamedBufferParameteri64v =
        g_DispatchTable.glGetNamedBufferParameteri64v.load(std::memory_order_acquire);
    realGetNamedBufferParameteri64v(buffer, GL_BUFFER_SIZE, &size);
  } else {
    GLint size32 = 0;
    auto realGetNamedBufferParameterivEXT =
        g_DispatchTable.glGetNamedBufferParameterivEXT.load(std::memory_order_acquire);
    realGetNamedBufferParameterivEXT(buffer, GL_BUFFER_SIZE, &size32);
    size = size32;
  }
  if (size <= 0) {
    LOG_WARNING << "MapTrackingService: GL_BUFFER_SIZE of buffer " << buffer
                << " could not be queried - buffer-map tracking skipped for this map";
    return;
  }
  StoreWholeBufferMap(CurrentShareGroup(), buffer, access, mappedPointer,
                      static_cast<GLsizeiptr>(size));
}

void MapTrackingService::OnMapNamedBufferRange(
    GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mappedPointer) {
  if (mappedPointer == nullptr || buffer == 0) {
    return;
  }
  // Explicit offset/length/access bitfield already match MappedRange's own shape exactly - no
  // translation or extra driver query needed here, unlike the whole-buffer EXT map above.
  if (access & GL_MAP_COHERENT_BIT) {
    ReportUnsupportedCoherentMapping(buffer);
  }
  StoreMappedRange(CurrentShareGroup(), buffer, MappedRange{mappedPointer, offset, length, access});
}

GLboolean MapTrackingService::SnapshotBufferAndUnmap(
    const char* callerName,
    GLuint buffer,
    uint64_t shareGroup,
    const std::function<GLboolean()>& realUnmap,
    const std::function<void*(GLintptr, GLsizeiptr)>& realReadOnlyRemap,
    const std::function<GLboolean()>& realInternalUnmap,
    const std::function<bool(GLintptr, GLsizeiptr, void*)>& realReadSubData) {
  std::optional<MappedRange> range =
      buffer != 0 ? TakeMappedRange(shareGroup, buffer) : std::nullopt;
  // Nothing tracked (map wasn't via one of this service's enabled map functions, buffer
  // unresolved, ...), or a genuinely read-only mapping - see Decision 5's third bullet. Skip
  // straight to the real unmap; no MappedDataMetaCommand, so no ordering key is reserved.
  if (!range || (range->Access & GL_MAP_WRITE_BIT) == 0) {
    return realUnmap();
  }
  // Reserve before the real unmap, then record or release on every path; otherwise another
  // thread can claim a later key and stall the ordering recorder on this one.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  std::vector<uint8_t> bytes(static_cast<size_t>(range->Length));
  if (range->Access & GL_MAP_READ_BIT) {
    // Safe to read directly through the still-live original pointer, right before the real
    // unmap invalidates it - see Decision 5's first bullet.
    std::memcpy(bytes.data(), range->Pointer, bytes.size());
    const GLboolean result = realUnmap();
    if (result == GL_TRUE) {
      RecordMappedDataMetaCommand(key, shareGroup, buffer, range->Offset, range->Length, bytes);
    } else {
      LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
    return result;
  }
  // Write-only (or write-plus-invalidate): the original pointer is not guaranteed readable per
  // spec - see Decision 5's second bullet. Real unmap first (the one, app-visible unmap - its
  // result is what the app's own call returns), then a temporary read-only remap/memcpy/unmap
  // purely for capture.
  const GLboolean result = realUnmap();
  if (result != GL_TRUE) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): real unmap of buffer " << buffer
                << " returned GL_FALSE (data store contents corrupted per spec) - no "
                   "MappedDataMetaCommand recorded for this unmap";
    CaptureManager::Get().SkipCommandKey(key);
    return result;
  }
  // TODO: Track immutable storage permissions to skip a read remap forbidden by its flags;
  // the failed probe may leave an application-visible GL error before subdata readback.
  void* readPointer = realReadOnlyRemap(range->Offset, range->Length);
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    if (realInternalUnmap() != GL_TRUE) {
      LOG_WARNING << "MapTrackingService (" << callerName << "): internal unmap of buffer "
                  << buffer << " returned GL_FALSE - no MappedDataMetaCommand recorded";
      CaptureManager::Get().SkipCommandKey(key);
      return result;
    }
  } else if (!realReadSubData(range->Offset, range->Length, bytes.data())) {
    LOG_WARNING << "MapTrackingService (" << callerName << "): no readback function for buffer "
                << buffer << " - no MappedDataMetaCommand recorded for this unmap";
    CaptureManager::Get().SkipCommandKey(key);
    return result;
  }
  RecordMappedDataMetaCommand(key, shareGroup, buffer, range->Offset, range->Length, bytes);
  return result;
}

GLboolean MapTrackingService::UnmapBufferARBAndCapture(GLenum target) {
  auto realUnmapBufferARB = g_DispatchTable.glUnmapBufferARB.load(std::memory_order_acquire);
  const GLuint buffer = ResolveBoundBuffer(target);
  return SnapshotBufferAndUnmap(
      "glUnmapBufferARB", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapBufferARB, target] { return realUnmapBufferARB(target); },
      /*realReadOnlyRemap=*/
      [target](GLintptr offset, GLsizeiptr length) {
        return MapRangeForCaptureRead(target, offset, length);
      },
      /*realInternalUnmap=*/[realUnmapBufferARB, target] { return realUnmapBufferARB(target); },
      /*realReadSubData=*/
      [target](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadBufferSubData(target, offset, length, data);
      });
}

GLboolean MapTrackingService::UnmapNamedBufferAndCapture(GLuint buffer) {
  auto realUnmapNamedBuffer = g_DispatchTable.glUnmapNamedBuffer.load(std::memory_order_acquire);
  return SnapshotBufferAndUnmap(
      "glUnmapNamedBuffer", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapNamedBuffer, buffer] { return realUnmapNamedBuffer(buffer); },
      /*realReadOnlyRemap=*/
      [buffer](GLintptr offset, GLsizeiptr length) {
        return g_DispatchTable.glMapNamedBufferRange.load(std::memory_order_acquire)(
            buffer, offset, length, GL_MAP_READ_BIT);
      },
      /*realInternalUnmap=*/[realUnmapNamedBuffer, buffer] { return realUnmapNamedBuffer(buffer); },
      /*realReadSubData=*/
      [buffer](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadNamedBufferSubData(buffer, offset, length, data);
      });
}

GLboolean MapTrackingService::UnmapNamedBufferEXTAndCapture(GLuint buffer) {
  auto realUnmapNamedBufferEXT =
      g_DispatchTable.glUnmapNamedBufferEXT.load(std::memory_order_acquire);
  return SnapshotBufferAndUnmap(
      "glUnmapNamedBufferEXT", buffer, CurrentShareGroup(),
      /*realUnmap=*/[realUnmapNamedBufferEXT, buffer] { return realUnmapNamedBufferEXT(buffer); },
      /*realReadOnlyRemap=*/
      [buffer](GLintptr offset, GLsizeiptr length) {
        // Use the EXT family for this EXT-addressed buffer. If its range entry point is
        // unavailable, read-map the whole buffer through its whole-buffer entry point.
        auto mapRange = g_DispatchTable.glMapNamedBufferRangeEXT.load(std::memory_order_acquire);
        if (mapRange != nullptr) {
          void* pointer = mapRange(buffer, offset, length, GL_MAP_READ_BIT);
          if (pointer != nullptr ||
              g_DispatchTable.glMapNamedBufferRangeEXT.load(std::memory_order_acquire) !=
                  &ResolveAndCall_glMapNamedBufferRangeEXT) {
            return pointer;
          }
        }
        auto map = g_DispatchTable.glMapNamedBufferEXT.load(std::memory_order_acquire);
        void* whole = map != nullptr ? map(buffer, GL_READ_ONLY) : nullptr;
        return whole != nullptr ? static_cast<void*>(static_cast<uint8_t*>(whole) + offset)
                                : nullptr;
      },
      /*realInternalUnmap=*/
      [realUnmapNamedBufferEXT, buffer] { return realUnmapNamedBufferEXT(buffer); },
      /*realReadSubData=*/
      [buffer](GLintptr offset, GLsizeiptr length, void* data) {
        return ReadNamedBufferSubDataEXT(buffer, offset, length, data);
      });
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
  if (range.Access & GL_MAP_WRITE_BIT) {
    BufferShadowService::Get().OnMapWrite(shareGroup, buffer, range.Access);
  }
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
