// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "captureManager.h"
#include "clientMemoryCapture.h"
#include "glEnumsAuto.h"
#include "log.h"
#include "mapTrackingService.h"
#include "mappedTextureService.h"
#include "openglDriversAuto.h"
#include "recursionGuard.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

namespace gits::opengl2 {

namespace {

bool CanQueryBufferBinding(GLenum target, int major, int minor) {
  const auto supports = [major, minor](int coreMajor, int coreMinor, const char* extension,
                                       const char* alternate = nullptr) {
    return major > coreMajor || (major == coreMajor && minor >= coreMinor) ||
           CurrentContextHasExtension(major, extension) ||
           (alternate != nullptr && CurrentContextHasExtension(major, alternate));
  };
  switch (target) {
  case GL_ARRAY_BUFFER:
  case GL_ELEMENT_ARRAY_BUFFER:
    return supports(1, 5, "GL_ARB_vertex_buffer_object");
  case GL_PIXEL_PACK_BUFFER:
  case GL_PIXEL_UNPACK_BUFFER:
    return supports(2, 1, "GL_ARB_pixel_buffer_object", "GL_EXT_pixel_buffer_object");
  case GL_UNIFORM_BUFFER:
    return supports(3, 1, "GL_ARB_uniform_buffer_object");
  case GL_TRANSFORM_FEEDBACK_BUFFER:
    return supports(3, 0, "GL_EXT_transform_feedback");
  case GL_COPY_READ_BUFFER:
  case GL_COPY_WRITE_BUFFER:
    return supports(3, 1, "GL_ARB_copy_buffer");
  case GL_DRAW_INDIRECT_BUFFER:
    return supports(4, 0, "GL_ARB_draw_indirect");
  case GL_ATOMIC_COUNTER_BUFFER:
    return supports(4, 2, "GL_ARB_shader_atomic_counters");
  case GL_DISPATCH_INDIRECT_BUFFER:
    return supports(4, 3, "GL_ARB_compute_shader");
  case GL_SHADER_STORAGE_BUFFER:
    return supports(4, 3, "GL_ARB_shader_storage_buffer_object");
  case GL_QUERY_BUFFER:
    return supports(4, 4, "GL_ARB_query_buffer_object");
  case GL_TEXTURE_BUFFER:
    return supports(3, 1, "GL_ARB_texture_buffer_object", "GL_EXT_texture_buffer_object");
  case GL_PARAMETER_BUFFER:
    return supports(4, 6, "GL_ARB_indirect_parameters");
  default:
    return false;
  }
}

void* APIENTRY CustomWrapper_glMapBufferRange(GLenum target,
                                              GLintptr offset,
                                              GLsizeiptr length,
                                              GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  std::vector<uint8_t> initialBytes;
  if (record && MapTrackingService::NeedsInitialShadowContents(access)) {
    const auto [major, minor] = CurrentContextGLVersion();
    const GLuint buffer = CanQueryBufferBinding(target, major, minor)
                              ? MapTrackingService::Get().ResolveBoundBuffer(target)
                              : 0;
    const bool getterAvailable = buffer != 0 && (major > 1 || (major == 1 && minor >= 5)) &&
                                 TryResolveRealFunction("glGetBufferParameteriv") &&
                                 (major < 3 || (major == 3 && minor < 2) ||
                                  TryResolveRealFunction("glGetBufferParameteri64v")) &&
                                 TryResolveRealFunction("glGetBufferSubData");
    initialBytes = MapTrackingService::Get().PrepareInitialBytes(
        offset, length, access, getterAvailable,
        /*parameter=*/
        [target](GLenum pname, GLint64& value) {
          if (pname == GL_BUFFER_SIZE) {
            return GetBoundBufferSize(target, value);
          }
          GLint mapped = 0;
          if (!GetBoundBufferParameter(target, pname, mapped)) {
            return false;
          }
          value = mapped;
          return true;
        },
        /*read=*/
        [target](GLintptr start, GLsizeiptr count, void* data) {
          g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire)(target, start, count,
                                                                             data);
        });
  }
  auto realFunction = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* result = realFunction(target, offset, length, access);
  if (record) {
    MapTrackingService::Get().NoteMap(true, access, result != nullptr);
  }
  if (record && result != nullptr) {
    return MapTrackingService::Get().OnMapBufferRange(target, offset, length, access, result,
                                                      initialBytes);
  }
  return result;
}

// GL_EXT_map_buffer_range is a GLES extension whose unmap uses glUnmapBufferOES, which this
// desktop map-tracking path does not support. Do not treat its EXT suffix as a DSA alias.
void* APIENTRY CustomWrapper_glMapBufferRangeEXT(GLenum target,
                                                 GLintptr offset,
                                                 GLsizeiptr length,
                                                 GLbitfield access) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    return g_DispatchTable.glMapBufferRangeEXT.load(std::memory_order_acquire)(target, offset,
                                                                               length, access);
  }
  GITS_ASSERT(false, "Target-EXT buffer range maps are not supported");
  return nullptr;
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
  if (record) {
    MapTrackingService::Get().NoteMap(false, access, result != nullptr);
  }
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
  if (record) {
    MapTrackingService::Get().NoteMap(false, access, result != nullptr);
  }
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
  if (record) {
    MapTrackingService::Get().NoteMap(false, access, result != nullptr);
  }
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapNamedBuffer(buffer, access, result, QuerySize);
  }
  return result;
}

enum class NamedBufferFamily {
  Core,
  EXT
};

template <NamedBufferFamily Family, auto MapRangeEntry, auto ParameterEntry, auto SubDataEntry>
void* APIENTRY CustomWrapper_MapNamedBufferRange(GLuint buffer,
                                                 GLintptr offset,
                                                 GLsizeiptr length,
                                                 GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  std::vector<uint8_t> initialBytes;
  if (record && MapTrackingService::NeedsInitialShadowContents(access)) {
    const int major = CurrentContextGLVersion().first;
    constexpr bool ext = Family == NamedBufferFamily::EXT;
    // EXT entry points require GL_EXT_direct_state_access; core DSA support does not imply it.
    const bool supported = ext ? CurrentContextHasExtension(major, "GL_EXT_direct_state_access")
                               : ContextSupportsDirectStateAccess();
    const char* getterName = ext ? "glGetNamedBufferSubDataEXT" : "glGetNamedBufferSubData";
    const char* parameterName =
        ext ? "glGetNamedBufferParameterivEXT" : "glGetNamedBufferParameteri64v";
    const bool getterAvailable =
        buffer != 0 && supported && TryResolveRealFunction("glIsBuffer") &&
        g_DispatchTable.glIsBuffer.load(std::memory_order_acquire)(buffer) == GL_TRUE &&
        TryResolveRealFunction(parameterName) && TryResolveRealFunction(getterName);
    initialBytes = MapTrackingService::Get().PrepareInitialBytes(
        offset, length, access, getterAvailable,
        /*parameter=*/
        [buffer](GLenum pname, GLint64& value) {
          using ParameterType = std::conditional_t<ext, GLint, GLint64>;
          ParameterType result = 0;
          (g_DispatchTable.*ParameterEntry).load(std::memory_order_acquire)(buffer, pname, &result);
          if constexpr (ext) {
            if (pname == GL_BUFFER_SIZE && result == std::numeric_limits<GLint>::max()) {
              return false;
            }
          }
          value = result;
          return true;
        },
        /*read=*/
        [buffer](GLintptr start, GLsizeiptr count, void* data) {
          (g_DispatchTable.*SubDataEntry)
              .load(std::memory_order_acquire)(buffer, start, count, data);
        });
  }
  void* result = (g_DispatchTable.*MapRangeEntry)
                     .load(std::memory_order_acquire)(buffer, offset, length, access);
  if (record) {
    MapTrackingService::Get().NoteMap(true, access, result != nullptr);
  }
  if (record && result != nullptr) {
    return MapTrackingService::Get().OnMapNamedBufferRange(buffer, offset, length, access, result,
                                                           initialBytes);
  }
  return result;
}

template <auto FlushEntry>
void APIENTRY CustomWrapper_FlushMappedBufferRange(GLenum target,
                                                   GLintptr offset,
                                                   GLsizeiptr length) {
  const RecursionGuard guard;
  auto realFlush = (g_DispatchTable.*FlushEntry).load(std::memory_order_acquire);
  if (guard.IsNested()) {
    realFlush(target, offset, length);
    return;
  }
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  MapTrackingService::Get().FlushMappedRange(MapTrackingService::Get().ResolveBoundBuffer(target),
                                             offset, length, key,
                                             [=] { realFlush(target, offset, length); });
}

// The matching GLES flush also needs the unsupported OES map/unmap lifecycle.
void APIENTRY CustomWrapper_glFlushMappedBufferRangeEXT(GLenum target,
                                                        GLintptr offset,
                                                        GLsizeiptr length) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    g_DispatchTable.glFlushMappedBufferRangeEXT.load(std::memory_order_acquire)(target, offset,
                                                                                length);
    return;
  }
  GITS_ASSERT(false, "Target-EXT mapped-buffer flushes are not supported");
}

template <auto FlushEntry>
void APIENTRY CustomWrapper_FlushMappedNamedBufferRange(GLuint buffer,
                                                        GLintptr offset,
                                                        GLsizeiptr length) {
  const RecursionGuard guard;
  auto realFlush = (g_DispatchTable.*FlushEntry).load(std::memory_order_acquire);
  if (guard.IsNested()) {
    realFlush(buffer, offset, length);
    return;
  }
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  MapTrackingService::Get().FlushMappedRange(buffer, offset, length, key,
                                             [=] { realFlush(buffer, offset, length); });
}

enum class TargetPointerFamily {
  Core,
  ARB,
  OES
};

template <TargetPointerFamily Family, auto GetEntry>
void APIENTRY CustomWrapper_GetBufferPointerv(GLenum target, GLenum pname, void** params) {
  const RecursionGuard guard;
  (g_DispatchTable.*GetEntry).load(std::memory_order_acquire)(target, pname, params);
  if (guard.IsNested() || pname != GL_BUFFER_MAP_POINTER || params == nullptr ||
      *params == nullptr) {
    return;
  }
  const auto [major, minor] = CurrentContextGLVersion();
  bool supported = false;
  const char* name = nullptr;
  if constexpr (Family == TargetPointerFamily::Core) {
    supported = major > 1 || (major == 1 && minor >= 5);
    name = "glGetBufferPointerv";
  } else if constexpr (Family == TargetPointerFamily::ARB) {
    supported = CurrentContextHasExtension(major, "GL_ARB_vertex_buffer_object");
    name = "glGetBufferPointervARB";
  } else {
    supported = CurrentContextHasExtension(major, "GL_OES_mapbuffer");
    name = "glGetBufferPointervOES";
  }
  if (!supported || !CanQueryBufferBinding(target, major, minor)) {
    return;
  }
  if (TryResolveRealFunction(name)) {
    *params = MapTrackingService::Get().TranslateShadowPointerQuery(
        MapTrackingService::Get().ResolveBoundBuffer(target), *params);
  }
}

template <NamedBufferFamily Family, auto GetEntry>
void APIENTRY CustomWrapper_GetNamedBufferPointerv(GLuint buffer, GLenum pname, void** params) {
  const RecursionGuard guard;
  (g_DispatchTable.*GetEntry).load(std::memory_order_acquire)(buffer, pname, params);
  if (guard.IsNested() || pname != GL_BUFFER_MAP_POINTER || params == nullptr ||
      *params == nullptr) {
    return;
  }
  const bool supported = Family == NamedBufferFamily::Core
                             ? ContextSupportsDirectStateAccess()
                             : CurrentContextHasExtension(CurrentContextGLVersion().first,
                                                          "GL_EXT_direct_state_access");
  const char* name = Family == NamedBufferFamily::Core ? "glGetNamedBufferPointerv"
                                                       : "glGetNamedBufferPointervEXT";
  if (supported && TryResolveRealFunction(name)) {
    *params = MapTrackingService::Get().TranslateShadowPointerQuery(buffer, *params);
  }
}

void APIENTRY CustomWrapper_glBufferParameteriAPPLE(GLenum target, GLenum pname, GLint param) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    g_DispatchTable.glBufferParameteriAPPLE.load(std::memory_order_acquire)(target, pname, param);
    return;
  }
  GITS_ASSERT(false, "APPLE flush-on-unmap buffer mappings are not supported");
}

void APIENTRY CustomWrapper_glFlushMappedBufferRangeAPPLE(GLenum target,
                                                          GLintptr offset,
                                                          GLsizeiptr length) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    g_DispatchTable.glFlushMappedBufferRangeAPPLE.load(std::memory_order_acquire)(target, offset,
                                                                                  length);
    return;
  }
  GITS_ASSERT(false, "APPLE mapped-buffer range flushes are not supported");
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

template <NamedBufferFamily Family, auto GetEntry>
bool QueryNamedStorageFlags(GLuint buffer, GLint& flags) {
  const char* name = Family == NamedBufferFamily::Core ? "glGetNamedBufferParameteriv"
                                                       : "glGetNamedBufferParameterivEXT";
  if (!TryResolveRealFunction(name)) {
    return false;
  }
  (g_DispatchTable.*GetEntry)
      .load(std::memory_order_acquire)(buffer, GL_BUFFER_STORAGE_FLAGS, &flags);
  return true;
}

template <const char* Name,
          NamedBufferFamily Family,
          auto UnmapEntry,
          auto MapRangeEntry,
          auto MapEntry,
          auto ResolveThunk,
          auto GetSubDataEntry,
          auto ResolveGetSubDataThunk,
          auto GetStorageFlagsEntry>
GLboolean APIENTRY CustomWrapper_UnmapNamedBuffer(GLuint buffer) {
  const RecursionGuard guard;
  auto unmap = (g_DispatchTable.*UnmapEntry).load(std::memory_order_acquire);
  if (guard.IsNested()) {
    return unmap(buffer);
  }
  return MapTrackingService::Get().UnmapNamedBufferAndCapture(
      Name, buffer, unmap, &ReadOnlyNamedBuffer<MapRangeEntry, MapEntry, ResolveThunk>,
      &ReadNamedBufferSubData<GetSubDataEntry, ResolveGetSubDataThunk>,
      &QueryNamedStorageFlags<Family, GetStorageFlagsEntry>);
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
    {"glMapBufferRangeEXT", reinterpret_cast<void*>(&CustomWrapper_glMapBufferRangeEXT)},
    {"glFlushMappedBufferRange",
     reinterpret_cast<void*>(
         &CustomWrapper_FlushMappedBufferRange<&OpenGLDispatchTable::glFlushMappedBufferRange>)},
    {"glFlushMappedBufferRangeEXT",
     reinterpret_cast<void*>(&CustomWrapper_glFlushMappedBufferRangeEXT)},
    {"glFlushMappedNamedBufferRange",
     reinterpret_cast<void*>(&CustomWrapper_FlushMappedNamedBufferRange<
                             &OpenGLDispatchTable::glFlushMappedNamedBufferRange>)},
    {"glFlushMappedNamedBufferRangeEXT",
     reinterpret_cast<void*>(&CustomWrapper_FlushMappedNamedBufferRange<
                             &OpenGLDispatchTable::glFlushMappedNamedBufferRangeEXT>)},
    {"glBufferParameteriAPPLE", reinterpret_cast<void*>(&CustomWrapper_glBufferParameteriAPPLE)},
    {"glFlushMappedBufferRangeAPPLE",
     reinterpret_cast<void*>(&CustomWrapper_glFlushMappedBufferRangeAPPLE)},
    {"glGetBufferPointerv",
     reinterpret_cast<void*>(
         &CustomWrapper_GetBufferPointerv<TargetPointerFamily::Core,
                                          &OpenGLDispatchTable::glGetBufferPointerv>)},
    {"glGetBufferPointervARB",
     reinterpret_cast<void*>(
         &CustomWrapper_GetBufferPointerv<TargetPointerFamily::ARB,
                                          &OpenGLDispatchTable::glGetBufferPointervARB>)},
    {"glGetBufferPointervOES",
     reinterpret_cast<void*>(
         &CustomWrapper_GetBufferPointerv<TargetPointerFamily::OES,
                                          &OpenGLDispatchTable::glGetBufferPointervOES>)},
    {"glGetNamedBufferPointerv",
     reinterpret_cast<void*>(
         &CustomWrapper_GetNamedBufferPointerv<NamedBufferFamily::Core,
                                               &OpenGLDispatchTable::glGetNamedBufferPointerv>)},
    {"glGetNamedBufferPointervEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_GetNamedBufferPointerv<NamedBufferFamily::EXT,
                                               &OpenGLDispatchTable::glGetNamedBufferPointervEXT>)},
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
         &CustomWrapper_MapNamedBufferRange<NamedBufferFamily::Core,
                                            &OpenGLDispatchTable::glMapNamedBufferRange,
                                            &OpenGLDispatchTable::glGetNamedBufferParameteri64v,
                                            &OpenGLDispatchTable::glGetNamedBufferSubData>)},
    {"glMapNamedBufferRangeEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_MapNamedBufferRange<NamedBufferFamily::EXT,
                                            &OpenGLDispatchTable::glMapNamedBufferRangeEXT,
                                            &OpenGLDispatchTable::glGetNamedBufferParameterivEXT,
                                            &OpenGLDispatchTable::glGetNamedBufferSubDataEXT>)},
    {"glUnmapNamedBuffer",
     reinterpret_cast<void*>(
         &CustomWrapper_UnmapNamedBuffer<UNMAP_NAMED_BUFFER,
                                         NamedBufferFamily::Core,
                                         &OpenGLDispatchTable::glUnmapNamedBuffer,
                                         &OpenGLDispatchTable::glMapNamedBufferRange,
                                         &OpenGLDispatchTable::glMapNamedBuffer,
                                         &ResolveAndCall_glMapNamedBufferRange,
                                         &OpenGLDispatchTable::glGetNamedBufferSubData,
                                         &ResolveAndCall_glGetNamedBufferSubData,
                                         &OpenGLDispatchTable::glGetNamedBufferParameteriv>)},
    {"glUnmapNamedBufferEXT",
     reinterpret_cast<void*>(
         &CustomWrapper_UnmapNamedBuffer<UNMAP_NAMED_BUFFER_EXT,
                                         NamedBufferFamily::EXT,
                                         &OpenGLDispatchTable::glUnmapNamedBufferEXT,
                                         &OpenGLDispatchTable::glMapNamedBufferRangeEXT,
                                         &OpenGLDispatchTable::glMapNamedBufferEXT,
                                         &ResolveAndCall_glMapNamedBufferRangeEXT,
                                         &OpenGLDispatchTable::glGetNamedBufferSubDataEXT,
                                         &ResolveAndCall_glGetNamedBufferSubDataEXT,
                                         &OpenGLDispatchTable::glGetNamedBufferParameterivEXT>)},
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
