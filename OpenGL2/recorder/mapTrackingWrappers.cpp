// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "clientMemoryCapture.h"
#include "glEnumsAuto.h"
#include "mapTrackingService.h"
#include "mappedTextureService.h"
#include "openglDriversAuto.h"
#include "recursionGuard.h"

#include <cstring>

namespace gits::opengl2 {

namespace {

void* APIENTRY CustomWrapper_glMapBufferRange(GLenum target,
                                              GLintptr offset,
                                              GLsizeiptr length,
                                              GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* result = realFunction(target, offset, length, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapBufferRange(target, offset, length, access, result);
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapBuffer(GLenum target) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    auto realFunction = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
    return realFunction(target);
  }
  // The service makes the real call itself: it has to snapshot the mapped range either side of
  // that call, depending on the mapping's access flags.
  return MapTrackingService::Get().UnmapBufferAndCapture(target);
}

// --- Extensions beyond the original glMapBufferRange/glUnmapBuffer pair ---------------------
//
// Each variant calls its own real driver entry point; tracking and capture policy stays in
// MapTrackingService.

void* APIENTRY CustomWrapper_glMapBuffer(GLenum target, GLenum access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBuffer.load(std::memory_order_acquire);
  void* result = realFunction(target, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapBuffer(target, access, result);
  }
  return result;
}

// A separate function (not a second registration of CustomWrapper_glMapBuffer above) because
// each must call its own, specific real dispatch-table entry - glMapBuffer and glMapBufferARB
// are functionally identical but distinct driver exports, and a driver is not guaranteed to
// resolve both; calling the wrong one from a name-agnostic shared function would silently call
// whichever entry point happens to be resolved, not the one the app - and the real driver call
// this wrapper must forward to - actually asked for.
void* APIENTRY CustomWrapper_glMapBufferARB(GLenum target, GLenum access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBufferARB.load(std::memory_order_acquire);
  void* result = realFunction(target, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapBuffer(target, access, result);
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapBufferARB(GLenum target) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    auto realFunction = g_DispatchTable.glUnmapBufferARB.load(std::memory_order_acquire);
    return realFunction(target);
  }
  return MapTrackingService::Get().UnmapBufferARBAndCapture(target);
}

// --- glMapNamedBuffer/glMapNamedBufferRange/glUnmapNamedBuffer and their EXT variants (DSA - ---
// --- buffer given directly, no target resolution needed at all) -------------------------------
//
// Core and EXT entry points share one implementation, instantiated per dispatch-table entry so
// each still calls its own driver export.

// GL_BUFFER_SIZE through `GetEntry`, whose result type is `T` (the core family has a 64-bit
// getter, the EXT family only a GLint one). 0 if the getter is not resolved.
template <typename T, auto GetEntry>
GLint64 NamedBufferSize(GLuint buffer) {
  auto get = (g_DispatchTable.*GetEntry).load(std::memory_order_acquire);
  if (get == nullptr) {
    return 0;
  }
  T size = 0;
  get(buffer, GL_BUFFER_SIZE, &size);
  return size;
}

// The EXT family has no 64-bit getter and GL_BUFFER_SIZE is a GLsizeiptr, so the core (GL 4.5 or
// ARB_direct_state_access) glGetNamedBufferParameteri64v is preferred on a context that has it.
// The dispatch entry is process-wide, so being resolved says nothing about the current context;
// it is still resolved silently, since a driver may not export it for that context yet.
GLint64 ExtNamedBufferSize(GLuint buffer) {
  const bool hasCoreGetter =
      ContextSupportsDirectStateAccess() &&
      (g_DispatchTable.glGetNamedBufferParameteri64v.load(std::memory_order_acquire) !=
           &ResolveAndCall_glGetNamedBufferParameteri64v ||
       TryResolveRealFunction("glGetNamedBufferParameteri64v"));
  if (hasCoreGetter) {
    if (const GLint64 size =
            NamedBufferSize<GLint64, &OpenGLDispatchTable::glGetNamedBufferParameteri64v>(buffer);
        size != 0) {
      return size;
    }
  }
  return NamedBufferSize<GLint, &OpenGLDispatchTable::glGetNamedBufferParameterivEXT>(buffer);
}

template <auto MapEntry, MapTrackingService::NamedBufferSizeFn QuerySize>
void* APIENTRY CustomWrapper_MapNamedBuffer(GLuint buffer, GLenum access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  void* result = (g_DispatchTable.*MapEntry).load(std::memory_order_acquire)(buffer, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapNamedBuffer(buffer, access, result, QuerySize);
  }
  return result;
}

template <auto MapRangeEntry>
void* APIENTRY CustomWrapper_MapNamedBufferRange(GLuint buffer,
                                                 GLintptr offset,
                                                 GLsizeiptr length,
                                                 GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  void* result = (g_DispatchTable.*MapRangeEntry)
                     .load(std::memory_order_acquire)(buffer, offset, length, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapNamedBufferRange(buffer, offset, length, access, result);
  }
  return result;
}

template <auto MapRangeEntry, auto MapEntry, auto ResolveThunk>
void* ReadOnlyNamedBuffer(GLuint buffer, GLintptr offset, GLsizeiptr length) {
  auto mapRange = (g_DispatchTable.*MapRangeEntry).load(std::memory_order_acquire);
  void* ranged = mapRange(buffer, offset, length, GL_MAP_READ_BIT);
  if (ranged != nullptr ||
      (g_DispatchTable.*MapRangeEntry).load(std::memory_order_acquire) != ResolveThunk) {
    return ranged;
  }
  void* whole = (g_DispatchTable.*MapEntry).load(std::memory_order_acquire)(buffer, GL_READ_ONLY);
  return whole != nullptr ? static_cast<uint8_t*>(whole) + offset : nullptr;
}

template <auto GetEntry, auto ResolveThunk>
bool ReadNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length, void* data) {
  (g_DispatchTable.*GetEntry).load(std::memory_order_acquire)(buffer, offset, length, data);
  return (g_DispatchTable.*GetEntry).load(std::memory_order_acquire) != ResolveThunk;
}

template <const char* Name,
          auto UnmapEntry,
          auto MapRangeEntry,
          auto MapEntry,
          auto ResolveThunk,
          auto GetSubDataEntry,
          auto ResolveGetSubDataThunk>
GLboolean APIENTRY CustomWrapper_UnmapNamedBuffer(GLuint buffer) {
  const RecursionGuard guard;
  auto unmap = (g_DispatchTable.*UnmapEntry).load(std::memory_order_acquire);
  if (guard.IsNested()) {
    return unmap(buffer);
  }
  return MapTrackingService::Get().UnmapNamedBufferAndCapture(
      Name, buffer, unmap, &ReadOnlyNamedBuffer<MapRangeEntry, MapEntry, ResolveThunk>,
      &ReadNamedBufferSubData<GetSubDataEntry, ResolveGetSubDataThunk>);
}

// GL_INTEL_map_texture: the calls themselves are not recorded - MappedTextureService captures
// the mapped contents and the player maps, writes and unmaps the level itself.
void* APIENTRY CustomWrapper_glMapTexture2DINTEL(
    GLuint texture, GLint level, GLbitfield access, GLint* stride, GLenum* layout) {
  const RecursionGuard guard;
  auto realFunction = g_DispatchTable.glMapTexture2DINTEL.load(std::memory_order_acquire);
  // MappedTextureService reads the mapping, which a write-only mapping does not guarantee.
  const GLbitfield realAccess = guard.IsNested() ? access : access | GL_MAP_READ_BIT;
  void* result = realFunction(texture, level, realAccess, stride, layout);
  if (!guard.IsNested() && result != nullptr && stride != nullptr) {
    MappedTextureService::Get().OnMap(texture, level, access, result, *stride);
  }
  return result;
}

void APIENTRY CustomWrapper_glUnmapTexture2DINTEL(GLuint texture, GLint level) {
  const RecursionGuard guard;
  if (!guard.IsNested()) {
    MappedTextureService::Get().CaptureOnUnmap(texture, level);
  }
  g_DispatchTable.glUnmapTexture2DINTEL.load(std::memory_order_acquire)(texture, level);
}

void APIENTRY CustomWrapper_glSyncTextureINTEL(GLuint texture) {
  const RecursionGuard guard;
  if (!guard.IsNested()) {
    MappedTextureService::Get().CaptureOnSync(texture);
  }
  g_DispatchTable.glSyncTextureINTEL.load(std::memory_order_acquire)(texture);
}

constexpr char UNMAP_NAMED_BUFFER[] = "glUnmapNamedBuffer";
constexpr char UNMAP_NAMED_BUFFER_EXT[] = "glUnmapNamedBufferEXT";

struct CustomWrapper {
  const char* Name;
  void* Function;
};

const CustomWrapper CUSTOM_WRAPPERS[] = {
    {"glMapBufferRange", reinterpret_cast<void*>(&CustomWrapper_glMapBufferRange)},
    {"glUnmapBuffer", reinterpret_cast<void*>(&CustomWrapper_glUnmapBuffer)},
    {"glMapBuffer", reinterpret_cast<void*>(&CustomWrapper_glMapBuffer)},
    {"glMapBufferARB", reinterpret_cast<void*>(&CustomWrapper_glMapBufferARB)},
    {"glUnmapBufferARB", reinterpret_cast<void*>(&CustomWrapper_glUnmapBufferARB)},
    {"glMapNamedBuffer",
     reinterpret_cast<void*>(
         &CustomWrapper_MapNamedBuffer<
             &OpenGLDispatchTable::glMapNamedBuffer,
             &NamedBufferSize<GLint64, &OpenGLDispatchTable::glGetNamedBufferParameteri64v>>)},
    {"glMapNamedBufferEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_MapNamedBuffer<&OpenGLDispatchTable::glMapNamedBufferEXT,
                                       &ExtNamedBufferSize>)},
    {"glMapNamedBufferRange",
     reinterpret_cast<void*>(
         &CustomWrapper_MapNamedBufferRange<&OpenGLDispatchTable::glMapNamedBufferRange>)},
    {"glMapNamedBufferRangeEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_MapNamedBufferRange<&OpenGLDispatchTable::glMapNamedBufferRangeEXT>)},
    {"glUnmapNamedBuffer",
     reinterpret_cast<void*>(
         &CustomWrapper_UnmapNamedBuffer<UNMAP_NAMED_BUFFER,
                                         &OpenGLDispatchTable::glUnmapNamedBuffer,
                                         &OpenGLDispatchTable::glMapNamedBufferRange,
                                         &OpenGLDispatchTable::glMapNamedBuffer,
                                         &ResolveAndCall_glMapNamedBufferRange,
                                         &OpenGLDispatchTable::glGetNamedBufferSubData,
                                         &ResolveAndCall_glGetNamedBufferSubData>)},
    {"glUnmapNamedBufferEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_UnmapNamedBuffer<UNMAP_NAMED_BUFFER_EXT,
                                         &OpenGLDispatchTable::glUnmapNamedBufferEXT,
                                         &OpenGLDispatchTable::glMapNamedBufferRangeEXT,
                                         &OpenGLDispatchTable::glMapNamedBufferEXT,
                                         &ResolveAndCall_glMapNamedBufferRangeEXT,
                                         &OpenGLDispatchTable::glGetNamedBufferSubDataEXT,
                                         &ResolveAndCall_glGetNamedBufferSubDataEXT>)},
    {"glMapTexture2DINTEL", reinterpret_cast<void*>(&CustomWrapper_glMapTexture2DINTEL)},
    {"glUnmapTexture2DINTEL", reinterpret_cast<void*>(&CustomWrapper_glUnmapTexture2DINTEL)},
    {"glSyncTextureINTEL", reinterpret_cast<void*>(&CustomWrapper_glSyncTextureINTEL)},
};

} // namespace

void* GetCustomRecorderWrapper(const char* name) {
  for (const CustomWrapper& wrapper : CUSTOM_WRAPPERS) {
    if (std::strcmp(name, wrapper.Name) == 0) {
      return wrapper.Function;
    }
  }
  return nullptr;
}

} // namespace gits::opengl2
