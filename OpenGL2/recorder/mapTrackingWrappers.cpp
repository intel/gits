// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "mapTrackingService.h"
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
// Everything below extends the same mechanism to additional map/unmap entry points a real
// target application was found to call instead of (or alongside) glMapBufferRange/glUnmapBuffer
// - see docs/development/opengl2-buffer-map-tracking-plan.md's Decision 1 "Revisit when"
// clause, which calls exactly this out as an additive extension, not a rework. Each wrapper
// below is deliberately kept to "dispatch to the real driver, then hand the result to
// MapTrackingService" - the same shape as CustomWrapper_glMapBufferRange/glUnmapBuffer above -
// with no tracking/capture policy (buffer resolution, coherent-mapping detection, the
// read-before/after-unmap decision, MappedDataMetaCommand recording) living in this file. See
// mapTrackingService.h's own header comment for why that policy lives in the service instead of
// here.

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

// --- glMapNamedBufferEXT/glMapNamedBufferRangeEXT/glUnmapNamedBufferEXT (DSA - buffer given ---
// --- directly, no target resolution needed at all) ------------------------------------------

void* APIENTRY CustomWrapper_glMapNamedBufferEXT(GLuint buffer, GLenum access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapNamedBufferEXT.load(std::memory_order_acquire);
  void* result = realFunction(buffer, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapNamedBufferEXT(buffer, access, result);
  }
  return result;
}

void* APIENTRY CustomWrapper_glMapNamedBufferRangeEXT(GLuint buffer,
                                                      GLintptr offset,
                                                      GLsizeiptr length,
                                                      GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapNamedBufferRangeEXT.load(std::memory_order_acquire);
  void* result = realFunction(buffer, offset, length, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapNamedBufferRangeEXT(buffer, offset, length, access, result);
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapNamedBufferEXT(GLuint buffer) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    auto realFunction = g_DispatchTable.glUnmapNamedBufferEXT.load(std::memory_order_acquire);
    return realFunction(buffer);
  }
  return MapTrackingService::Get().UnmapNamedBufferEXTAndCapture(buffer);
}

} // namespace

void* GetCustomRecorderWrapper(const char* name) {
  if (std::strcmp(name, "glMapBufferRange") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBufferRange);
  }
  if (std::strcmp(name, "glUnmapBuffer") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapBuffer);
  }
  if (std::strcmp(name, "glMapBuffer") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBuffer);
  }
  if (std::strcmp(name, "glMapBufferARB") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBufferARB);
  }
  if (std::strcmp(name, "glUnmapBufferARB") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapBufferARB);
  }
  if (std::strcmp(name, "glMapNamedBufferEXT") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapNamedBufferEXT);
  }
  if (std::strcmp(name, "glMapNamedBufferRangeEXT") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapNamedBufferRangeEXT);
  }
  if (std::strcmp(name, "glUnmapNamedBufferEXT") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapNamedBufferEXT);
  }
  return nullptr;
}

} // namespace gits::opengl2
